#include "SyntaxVisitors.h"

#include "ShaderCompiler.h"

#include <assert.h>

namespace Spire
{
    namespace Compiler
    {
        bool IsNumeric(BaseType t)
        {
            return t == BaseType::Int || t == BaseType::Float || t == BaseType::UInt;
        }

        String TranslateHLSLTypeNames(String name)
        {
            if (name == "float2" || name == "half2")
                return "vec2";
            else if (name == "float3" || name == "half3")
                return "vec3";
            else if (name == "float4" || name == "half4")
                return "vec4";
            else if (name == "half")
                return "float";
            else if (name == "int2")
                return "ivec2";
            else if (name == "int3")
                return "ivec3";
            else if (name == "int4")
                return "ivec4";
            else if (name == "uint2")
                return "uvec2";
            else if (name == "uint3")
                return "uvec3";
            else if (name == "uint4")
                return "uvec4";
            else if (name == "float3x3" || name == "half3x3")
                return "mat3";
            else if (name == "float4x4" || name == "half4x4")
                return "mat4";
            else
                return name;
        }

        void BuildMemberDictionary(ContainerDecl* decl)
        {
            // Don't rebuild if already built
            if (decl->memberDictionaryIsValid)
                return;

            decl->memberDictionary.Clear();
            decl->transparentMembers.Clear();

            for (auto m : decl->Members)
            {
                auto name = m->Name.Content;

                // Add any transparent members to a separate list for lookup
                if (m->HasModifier<TransparentModifier>())
                {
                    TransparentMemberInfo info;
                    info.decl = m.Ptr();
                    decl->transparentMembers.Add(info);
                }

                // Ignore members with an empty name
                if (name.Length() == 0)
                    continue;

                m->nextInContainerWithSameName = nullptr;

                Decl* next = nullptr;
                if (decl->memberDictionary.TryGetValue(name, next))
                    m->nextInContainerWithSameName = next;

                decl->memberDictionary[name] = m.Ptr();

            }
            decl->memberDictionaryIsValid = true;
        }


        class SemanticsVisitor : public SyntaxVisitor
        {
            ProgramSyntaxNode * program = nullptr;
            FunctionSyntaxNode * function = nullptr;
            CompileOptions const* options = nullptr;

            // lexical outer statements
            List<StatementSyntaxNode*> outerStmts;
        public:
            SemanticsVisitor(
                DiagnosticSink * pErr,
                CompileOptions const& options)
                : SyntaxVisitor(pErr)
                , options(&options)
            {
            }

            CompileOptions const& getOptions() { return *options; }

        public:
            // Translate Types
            RefPtr<ExpressionType> typeResult;
            RefPtr<ExpressionSyntaxNode> TranslateTypeNodeImpl(const RefPtr<ExpressionSyntaxNode> & node)
            {
                if (!node) return nullptr;
                auto expr = node->Accept(this).As<ExpressionSyntaxNode>();
                expr = ExpectATypeRepr(expr);
                return expr;
            }
            RefPtr<ExpressionType> ExtractTypeFromTypeRepr(const RefPtr<ExpressionSyntaxNode>& typeRepr)
            {
                if (!typeRepr) return nullptr;
                if (auto typeType = typeRepr->Type->As<TypeExpressionType>())
                {
                    return typeType->type;
                }
                return ExpressionType::Error;
            }
            RefPtr<ExpressionType> TranslateTypeNode(const RefPtr<ExpressionSyntaxNode> & node)
            {
                if (!node) return nullptr;
                auto typeRepr = TranslateTypeNodeImpl(node);
                return ExtractTypeFromTypeRepr(typeRepr);
            }
            TypeExp TranslateTypeNode(TypeExp const& typeExp)
            {
                // HACK(tfoley): It seems that in some cases we end up re-checking
                // syntax that we've already checked. We need to root-cause that
                // issue, but for now a quick fix in this case is to early
                // exist if we've already got a type associated here:
                if (typeExp.type)
                {
                    return typeExp;
                }


                auto typeRepr = TranslateTypeNodeImpl(typeExp.exp);

                TypeExp result;
                result.exp = typeRepr;
                result.type = ExtractTypeFromTypeRepr(typeRepr);
                return result;
            }

            RefPtr<ExpressionSyntaxNode> ConstructDeclRefExpr(
                DeclRef							declRef,
                RefPtr<ExpressionSyntaxNode>	baseExpr,
                RefPtr<ExpressionSyntaxNode>	originalExpr)
            {
                if (baseExpr)
                {
                    auto expr = new MemberExpressionSyntaxNode();
                    expr->Position = originalExpr->Position;
                    expr->BaseExpression = baseExpr;
                    expr->MemberName = declRef.GetName();
                    expr->Type = GetTypeForDeclRef(declRef);
                    expr->declRef = declRef;
                    return expr;
                }
                else
                {
                    auto expr = new VarExpressionSyntaxNode();
                    expr->Position = originalExpr->Position;
                    expr->Variable = declRef.GetName();
                    expr->Type = GetTypeForDeclRef(declRef);
                    expr->declRef = declRef;
                    return expr;
                }
            }

            LookupResult RefineLookup(LookupResult const& inResult, LookupMask mask)
            {
                if (!inResult.isValid()) return inResult;
                if (!inResult.isOverloaded()) return inResult;

                LookupResult result;
                for (auto item : inResult.items)
                {
                    if (!DeclPassesLookupMask(item.declRef.GetDecl(), mask))
                        continue;

                    AddToLookupResult(result, item);
                }
                return result;
            }

            RefPtr<ExpressionSyntaxNode> ConstructDerefExpr(
                RefPtr<ExpressionSyntaxNode> base,
                RefPtr<ExpressionSyntaxNode> originalExpr)
            {
                auto ptrLikeType = base->Type->As<PointerLikeType>();
                assert(ptrLikeType);

                auto derefExpr = new DerefExpr();
                derefExpr->Position = originalExpr->Position;
                derefExpr->base = base;
                derefExpr->Type = ptrLikeType->elementType;

                // TODO(tfoley): handle l-value status here

                return derefExpr;
            }

            RefPtr<ExpressionSyntaxNode> ConstructLookupResultExpr(
                LookupResultItem const&			item,
                RefPtr<ExpressionSyntaxNode>	baseExpr,
                RefPtr<ExpressionSyntaxNode>	originalExpr)
            {
                // If we collected any breadcrumbs, then these represent
                // additional segments of the lookup path that we need
                // to expand here.
                auto bb = baseExpr;
                for (auto breadcrumb = item.breadcrumbs; breadcrumb; breadcrumb = breadcrumb->next)
                {
                    switch (breadcrumb->kind)
                    {
                    case LookupResultItem::Breadcrumb::Kind::Member:
                        bb = ConstructDeclRefExpr(breadcrumb->declRef, bb, originalExpr);
                        break;
                    case LookupResultItem::Breadcrumb::Kind::Deref:
                        bb = ConstructDerefExpr(bb, originalExpr);
                        break;
                    default:
                        SPIRE_UNREACHABLE("all cases handle");
                    }
                }

                return ConstructDeclRefExpr(item.declRef, bb, originalExpr);
            }

            RefPtr<ExpressionSyntaxNode> ResolveOverloadedExpr(RefPtr<OverloadedExpr> overloadedExpr, LookupMask mask)
            {
                auto lookupResult = overloadedExpr->lookupResult2;
                assert(lookupResult.isValid() && lookupResult.isOverloaded());

                // Take the lookup result we had, and refine it based on what is expected in context.
                lookupResult = RefineLookup(lookupResult, mask);

                if (!lookupResult.isValid())
                {
                    // If we didn't find any symbols after filtering, then just
                    // use the original and report errors that way
                    return overloadedExpr;
                }

                if (lookupResult.isOverloaded())
                {
                    // We had an ambiguity anyway, so report it.
                    getSink()->diagnose(overloadedExpr, Diagnostics::unimplemented, "ambiguous reference");

                    // TODO(tfoley): should we construct a new ErrorExpr here?
                    overloadedExpr->Type = ExpressionType::Error;
                    return overloadedExpr;
                }

                // otherwise, we had a single decl and it was valid, hooray!
                return ConstructLookupResultExpr(lookupResult.item, overloadedExpr->base, overloadedExpr);
            }

            RefPtr<ExpressionSyntaxNode> ExpectATypeRepr(RefPtr<ExpressionSyntaxNode> expr)
            {
                if (auto overloadedExpr = expr.As<OverloadedExpr>())
                {
                    expr = ResolveOverloadedExpr(overloadedExpr, LookupMask::Type);
                }

                if (auto typeType = expr->Type.type->As<TypeExpressionType>())
                {
                    return expr;
                }
                else if (expr->Type.type->Equals(ExpressionType::Error))
                {
                    return expr;
                }

                getSink()->diagnose(expr, Diagnostics::unimplemented, "expected a type");
                // TODO: construct some kind of `ErrorExpr`?
                return expr;
            }

            RefPtr<ExpressionType> ExpectAType(RefPtr<ExpressionSyntaxNode> expr)
            {
                auto typeRepr = ExpectATypeRepr(expr);
                if (auto typeType = typeRepr->Type->As<TypeExpressionType>())
                {
                    return typeType->type;
                }
                return ExpressionType::Error;
            }

            RefPtr<ExpressionType> ExtractGenericArgType(RefPtr<ExpressionSyntaxNode> exp)
            {
                return ExpectAType(exp);
            }

            RefPtr<IntVal> ExtractGenericArgInteger(RefPtr<ExpressionSyntaxNode> exp)
            {
                return CheckIntegerConstantExpression(exp.Ptr());
            }

            RefPtr<Val> ExtractGenericArgVal(RefPtr<ExpressionSyntaxNode> exp)
            {
                if (auto overloadedExpr = exp.As<OverloadedExpr>())
                {
                    // assume that if it is overloaded, we want a type
                    exp = ResolveOverloadedExpr(overloadedExpr, LookupMask::Type);
                }

                if (auto typeType = exp->Type->As<TypeExpressionType>())
                {
                    return typeType->type;
                }
                else if (exp->Type->Equals(ExpressionType::Error))
                {
                    return exp->Type.type;
                }
                else
                {
                    return ExtractGenericArgInteger(exp);
                }
            }

            // Construct a type reprsenting the instantiation of
            // the given generic declaration for the given arguments.
            // The arguments should already be checked against
            // the declaration.
            RefPtr<ExpressionType> InstantiateGenericType(
                GenericDeclRef								genericDeclRef,
                List<RefPtr<ExpressionSyntaxNode>> const&	args)
            {
                RefPtr<Substitutions> subst = new Substitutions();
                subst->genericDecl = genericDeclRef.GetDecl();
                subst->outer = genericDeclRef.substitutions;

                for (auto argExpr : args)
                {
                    subst->args.Add(ExtractGenericArgVal(argExpr));
                }

                DeclRef innerDeclRef;
                innerDeclRef.decl = genericDeclRef.GetInner();
                innerDeclRef.substitutions = subst;

                return DeclRefType::Create(innerDeclRef);
            }

            // Make sure a declaration has been checked, so we can refer to it.
            // Note that this may lead to us recursively invoking checking,
            // so this may not be the best way to handle things.
            void EnsureDecl(RefPtr<Decl> decl, DeclCheckState state = DeclCheckState::CheckedHeader)
            {
                if (decl->IsChecked(state)) return;
                if (decl->checkState == DeclCheckState::CheckingHeader)
                {
                    // We tried to reference the same declaration while checking it!
                    throw "circularity";
                }

                if (DeclCheckState::CheckingHeader > decl->checkState)
                {
                    decl->SetCheckState(DeclCheckState::CheckingHeader);
                }

                // TODO: not all of the `Visit` cases are ready to
                // handle this being called on-the-fly
                decl->Accept(this);

                decl->SetCheckState(DeclCheckState::Checked);
            }

            void EnusreAllDeclsRec(RefPtr<Decl> decl)
            {
                EnsureDecl(decl, DeclCheckState::Checked);
                if (auto containerDecl = decl.As<ContainerDecl>())
                {
                    for (auto m : containerDecl->Members)
                    {
                        EnusreAllDeclsRec(m);
                    }
                }
            }

            // A "proper" type is one that can be used as the type of an expression.
            // Put simply, it can be a concrete type like `int`, or a generic
            // type that is applied to arguments, like `Texture2D<float4>`.
            // The type `void` is also a proper type, since we can have expressions
            // that return a `void` result (e.g., many function calls).
            //
            // A "non-proper" type is any type that can't actually have values.
            // A simple example of this in C++ is `std::vector` - you can't have
            // a value of this type.
            //
            // Part of what this function does is give errors if somebody tries
            // to use a non-proper type as the type of a variable (or anything
            // else that needs a proper type).
            //
            // The other thing it handles is the fact that HLSL lets you use
            // the name of a non-proper type, and then have the compiler fill
            // in the default values for its type arguments (e.g., a variable
            // given type `Texture2D` will actually have type `Texture2D<float4>`).
            bool CoerceToProperTypeImpl(TypeExp const& typeExp, RefPtr<ExpressionType>* outProperType)
            {
                ExpressionType* type = typeExp.type.Ptr();
                if (auto genericDeclRefType = type->As<GenericDeclRefType>())
                {
                    // We are using a reference to a generic declaration as a concrete
                    // type. This means we should substitute in any default parameter values
                    // if they are available.
                    //
                    // TODO(tfoley): A more expressive type system would substitute in
                    // "fresh" variables and then solve for their values...
                    //

                    auto genericDeclRef = genericDeclRefType->GetDeclRef();
                    EnsureDecl(genericDeclRef.decl);
                    List<RefPtr<ExpressionSyntaxNode>> args;
                    for (RefPtr<Decl> member : genericDeclRef.GetDecl()->Members)
                    {
                        if (auto typeParam = member.As<GenericTypeParamDecl>())
                        {
                            if (!typeParam->initType.exp)
                            {
                                if (outProperType)
                                {
                                    getSink()->diagnose(typeExp.exp.Ptr(), Diagnostics::unimplemented, "can't fill in default for generic type parameter");
                                    *outProperType = ExpressionType::Error;
                                }
                                return false;
                            }

                            // TODO: this is one place where syntax should get cloned!
                            if(outProperType)
                                args.Add(typeParam->initType.exp);
                        }
                        else if (auto valParam = member.As<GenericValueParamDecl>())
                        {
                            if (!valParam->Expr)
                            {
                                if (outProperType)
                                {
                                    getSink()->diagnose(typeExp.exp.Ptr(), Diagnostics::unimplemented, "can't fill in default for generic type parameter");
                                    *outProperType = ExpressionType::Error;
                                }
                                return false;
                            }

                            // TODO: this is one place where syntax should get cloned!
                            if(outProperType)
                                args.Add(valParam->Expr);
                        }
                        else
                        {
                            // ignore non-parameter members
                        }
                    }

                    if (outProperType)
                    {
                        *outProperType = InstantiateGenericType(genericDeclRef, args);
                    }
                    return true;
                }
                else
                {
                    // default case: we expect this to already be a proper type
                    if (outProperType)
                    {
                        *outProperType = type;
                    }
                    return true;
                }
            }



            TypeExp CoerceToProperType(TypeExp const& typeExp)
            {
                TypeExp result = typeExp;
                CoerceToProperTypeImpl(typeExp, &result.type);
                return result;
            }

            bool CanCoerceToProperType(TypeExp const& typeExp)
            {
                return CoerceToProperTypeImpl(typeExp, nullptr);
            }

            // Check a type, and coerce it to be proper
            TypeExp CheckProperType(TypeExp typeExp)
            {
                return CoerceToProperType(TranslateTypeNode(typeExp));
            }

            // For our purposes, a "usable" type is one that can be
            // used to declare a function parameter, variable, etc.
            // These turn out to be all the proper types except
            // `void`.
            //
            // TODO(tfoley): consider just allowing `void` as a
            // simple example of a "unit" type, and get rid of
            // this check.
            TypeExp CoerceToUsableType(TypeExp const& typeExp)
            {
                TypeExp result = CoerceToProperType(typeExp);
                ExpressionType* type = result.type.Ptr();
                if (auto basicType = type->As<BasicExpressionType>())
                {
                    // TODO: `void` shouldn't be a basic type, to make this easier to avoid
                    if (basicType->BaseType == BaseType::Void)
                    {
                        // TODO(tfoley): pick the right diagnostic message
                        getSink()->diagnose(result.exp.Ptr(), Diagnostics::invalidTypeVoid);
                        result.type = ExpressionType::Error;
                        return result;
                    }
                }
                return result;
            }

            // Check a type, and coerce it to be usable
            TypeExp CheckUsableType(TypeExp typeExp)
            {
                return CoerceToUsableType(TranslateTypeNode(typeExp));
            }

            RefPtr<ExpressionSyntaxNode> CheckTerm(RefPtr<ExpressionSyntaxNode> term)
            {
                if (!term) return nullptr;
                return term->Accept(this).As<ExpressionSyntaxNode>();
            }

            RefPtr<ExpressionSyntaxNode> CreateErrorExpr(ExpressionSyntaxNode* expr)
            {
                expr->Type = ExpressionType::Error;
                return expr;
            }

            bool IsErrorExpr(RefPtr<ExpressionSyntaxNode> expr)
            {
                // TODO: we may want other cases here...

                if (expr->Type->Equals(ExpressionType::Error))
                    return true;

                return false;
            }

            // Capture the "base" expression in case this is a member reference
            RefPtr<ExpressionSyntaxNode> GetBaseExpr(RefPtr<ExpressionSyntaxNode> expr)
            {
                if (auto memberExpr = expr.As<MemberExpressionSyntaxNode>())
                {
                    return memberExpr->BaseExpression;
                }
                else if(auto overloadedExpr = expr.As<OverloadedExpr>())
                {
                    return overloadedExpr->base;
                }
                return nullptr;
            }

        public:

            typedef unsigned int ConversionCost;
            enum : ConversionCost
            {
                kConversionCost_None = 0,
                kConversionCost_Promotion = 10,
                kConversionCost_Conversion = 20,

                kConversionCost_ScalarToVector = 1,
            };

            enum BaseTypeConversionKind : uint8_t
            {
                kBaseTypeConversionKind_Signed,
                kBaseTypeConversionKind_Unsigned,
                kBaseTypeConversionKind_Float,
                kBaseTypeConversionKind_Error,
            };

            enum BaseTypeConversionRank : uint8_t
            {
                kBaseTypeConversionRank_Bool,
                kBaseTypeConversionRank_Int8,
                kBaseTypeConversionRank_Int16,
                kBaseTypeConversionRank_Int32,
                kBaseTypeConversionRank_IntPtr,
                kBaseTypeConversionRank_Int64,
                kBaseTypeConversionRank_Error,
            };

            struct BaseTypeConversionInfo
            {
                BaseTypeConversionKind	kind;
                BaseTypeConversionRank	rank;
            };
            static BaseTypeConversionInfo GetBaseTypeConversionInfo(BaseType baseType)
            {
                switch (baseType)
                {
                #define CASE(TAG, KIND, RANK) \
                    case BaseType::TAG: { BaseTypeConversionInfo info = {kBaseTypeConversionKind_##KIND, kBaseTypeConversionRank_##RANK}; return info; } break

                    CASE(Bool, Unsigned, Bool);
                    CASE(Int, Signed, Int32);
                    CASE(UInt, Unsigned, Int32);
                    CASE(Float, Float, Int32);
                    CASE(Void, Error, Error);

                #undef CASE

                default:
                    break;
                }
                SPIRE_UNREACHABLE("all cases handled");
            }

            bool ValuesAreEqual(
                RefPtr<IntVal> left,
                RefPtr<IntVal> right)
            {
                if(left == right) return true;

                if(auto leftConst = left.As<ConstantIntVal>())
                {
                    if(auto rightConst = right.As<ConstantIntVal>())
                    {
                        return leftConst->value == rightConst->value;
                    }
                }

                if(auto leftVar = left.As<GenericParamIntVal>())
                {
                    if(auto rightVar = right.As<GenericParamIntVal>())
                    {
                        return leftVar->declRef.Equals(rightVar->declRef);
                    }
                }

                return false;
            }

            // Central engine for implementing implicit coercion logic
            bool TryCoerceImpl(
                RefPtr<ExpressionType>			toType,		// the target type for conversion
                RefPtr<ExpressionSyntaxNode>*	outToExpr,	// (optional) a place to stuff the target expression
                RefPtr<ExpressionType>			fromType,	// the source type for the conversion
                RefPtr<ExpressionSyntaxNode>	fromExpr,	// the source expression
                ConversionCost*					outCost)	// (optional) a place to stuff the conversion cost
            {
                // Easy case: the types are equal
                if (toType->Equals(fromType))
                {
                    if (outToExpr)
                        *outToExpr = fromExpr;
                    if (outCost)
                        *outCost = kConversionCost_None;
                    return true;
                }

                // If either type is an error, then let things pass.
                if (toType->As<ErrorType>() || fromType->As<ErrorType>())
                {
                    if (outToExpr)
                        *outToExpr = CreateImplicitCastExpr(toType, fromExpr);
                    if (outCost)
                        *outCost = kConversionCost_None;
                    return true;
                }

                //

                if (auto toBasicType = toType->AsBasicType())
                {
                    if (auto fromBasicType = fromType->AsBasicType())
                    {
                        // Conversions between base types are always allowed,
                        // and the only question is what the cost will be.

                        auto toInfo = GetBaseTypeConversionInfo(toBasicType->BaseType);
                        auto fromInfo = GetBaseTypeConversionInfo(fromBasicType->BaseType);

                        if (outToExpr)
                            *outToExpr = CreateImplicitCastExpr(toType, fromExpr);

                        if (outCost)
                        {
                            if (toInfo.kind == fromInfo.kind && toInfo.rank > fromInfo.rank)
                            {
                                *outCost = kConversionCost_Promotion;
                            }
                            else
                            {
                                *outCost = kConversionCost_Conversion;
                            }
                        }

                        return true;
                    }
                }

                if (auto toVectorType = toType->AsVectorType())
                {
                    if (auto fromVectorType = fromType->AsVectorType())
                    {
                        // Conversion between vector types.

                        // If element counts don't match, then bail:
                        if (!ValuesAreEqual(toVectorType->elementCount, fromVectorType->elementCount))
                            return false;

                        // Otherwise, if we can convert the element types, we are golden
                        ConversionCost elementCost;
                        if (CanCoerce(toVectorType->elementType, fromVectorType->elementType, &elementCost))
                        {
                            if (outToExpr)
                                *outToExpr = CreateImplicitCastExpr(toType, fromExpr);
                            if (outCost)
                                *outCost = elementCost;
                            return true;
                        }
                    }
                    else if (auto fromScalarType = fromType->AsBasicType())
                    {
                        // Conversion from scalar to vector.
                        // Should allow as long as we can coerce the scalar to our element type.
                        ConversionCost elementCost;
                        if (CanCoerce(toVectorType->elementType, fromScalarType, &elementCost))
                        {
                            if (outToExpr)
                                *outToExpr = CreateImplicitCastExpr(toType, fromExpr);
                            if (outCost)
                                *outCost = elementCost + kConversionCost_ScalarToVector;
                            return true;
                        }
                    }
                }

                // TODO: more cases!

                return false;
            }

            // Check whether a type coercion is possible
            bool CanCoerce(
                RefPtr<ExpressionType>			toType,			// the target type for conversion
                RefPtr<ExpressionType>			fromType,		// the source type for the conversion
                ConversionCost*					outCost = 0)	// (optional) a place to stuff the conversion cost
            {
                return TryCoerceImpl(
                    toType,
                    nullptr,
                    fromType,
                    nullptr,
                    outCost);
            }

            RefPtr<ExpressionSyntaxNode> CreateImplicitCastExpr(
                RefPtr<ExpressionType>			toType,
                RefPtr<ExpressionSyntaxNode>	fromExpr)
            {
                auto castExpr = new TypeCastExpressionSyntaxNode();
                castExpr->Position = fromExpr->Position;
                castExpr->TargetType.type = toType;
                castExpr->Type = toType;
                castExpr->Expression = fromExpr;
                return castExpr;
            }


            // Perform type coercion, and emit errors if it isn't possible
            RefPtr<ExpressionSyntaxNode> Coerce(
                RefPtr<ExpressionType>			toType,
                RefPtr<ExpressionSyntaxNode>	fromExpr)
            {
                // If semantic checking is being suppressed, then we might see
                // expressions without a type, and we need to ignore them.
                if( !fromExpr->Type.type )
                {
                    if(getOptions().flags & SPIRE_COMPILE_FLAG_NO_CHECKING )
                        return fromExpr;
                }

                RefPtr<ExpressionSyntaxNode> expr;
                if (!TryCoerceImpl(
                    toType,
                    &expr,
                    fromExpr->Type.Ptr(),
                    fromExpr.Ptr(),
                    nullptr))
                {
                    if(!(getOptions().flags & SPIRE_COMPILE_FLAG_NO_CHECKING))
                    {
                        getSink()->diagnose(fromExpr->Position, Diagnostics::typeMismatch, toType, fromExpr->Type);
                    }

                    // Note(tfoley): We don't call `CreateErrorExpr` here, because that would
                    // clobber the type on `fromExpr`, and an invariant here is that coercion
                    // really shouldn't *change* the expression that is passed in, but should
                    // introduce new AST nodes to coerce its value to a different type...
                    return CreateImplicitCastExpr(ExpressionType::Error, fromExpr);
                }
                return expr;
            }




            bool MatchType_ValueReceiver(ExpressionType * receiverType, ExpressionType * valueType)
            {
                if (receiverType->Equals(valueType))
                    return true;
                if (receiverType->IsIntegral() && valueType->Equals(ExpressionType::GetInt()))
                    return true;
                if (receiverType->Equals(ExpressionType::GetFloat()) && valueType->IsIntegral())
                    return true;
                if (receiverType->IsVectorType() && valueType->IsVectorType())
                {
                    auto recieverVecType = receiverType->AsVectorType();
                    auto valueVecType = valueType->AsVectorType();
                    if (GetVectorBaseType(recieverVecType) == BaseType::Float &&
                        GetVectorSize(recieverVecType) == GetVectorSize(valueVecType))
                        return true;
                    if (GetVectorBaseType(recieverVecType) == BaseType::UInt &&
                        GetVectorBaseType(valueVecType) == BaseType::Int &&
                        GetVectorSize(recieverVecType) == GetVectorSize(valueVecType))
                        return true;
                }
                return false;
            }

            void CheckVarDeclCommon(RefPtr<VarDeclBase> varDecl)
            {
                // Check the type, if one was given
                TypeExp type = CheckUsableType(varDecl->Type);

                // TODO: Additional validation rules on types should go here,
                // but we need to deal with the fact that some cases might be
                // allowed in one context (e.g., an unsized array parameter)
                // but not in othters (e.g., an unsized array field in a struct).

                // Check the initializers, if one was given
                RefPtr<ExpressionSyntaxNode> initExpr = CheckTerm(varDecl->Expr);

                // If a type was given, ...
                if (type.Ptr())
                {
                    // then coerce any initializer to the type
                    if (initExpr)
                    {
                        initExpr = Coerce(type, initExpr);
                    }
                }
                else
                {
                    // TODO: infer a type from the initializers
                    if (!initExpr)
                    {
                        getSink()->diagnose(varDecl, Diagnostics::unimplemented, "variable declaration with no type must have initializer");
                    }
                    else
                    {
                        getSink()->diagnose(varDecl, Diagnostics::unimplemented, "type inference for variable declaration");
                    }
                }

                varDecl->Type = type;
                varDecl->Expr = initExpr;
            }

            void CheckGenericConstraintDecl(GenericTypeConstraintDecl* decl)
            {
                // TODO: are there any other validations we can do at this point?
                //
                // There probably needs to be a kind of "occurs check" to make
                // sure that the constraint actually applies to at least one
                // of the parameters of the generic.

                decl->sub = TranslateTypeNode(decl->sub);
                decl->sup = TranslateTypeNode(decl->sup);
            }

            virtual RefPtr<GenericDecl> VisitGenericDecl(GenericDecl* genericDecl) override
            {
                // check the parameters
                for (auto m : genericDecl->Members)
                {
                    if (auto typeParam = m.As<GenericTypeParamDecl>())
                    {
                        typeParam->initType = CheckProperType(typeParam->initType);
                    }
                    else if (auto valParam = m.As<GenericValueParamDecl>())
                    {
                        // TODO: some real checking here...
                        CheckVarDeclCommon(valParam);
                    }
                    else if(auto constraint = m.As<GenericTypeConstraintDecl>())
                    {
                        CheckGenericConstraintDecl(constraint.Ptr());
                    }
                }

                // check the nested declaration
                // TODO: this needs to be done in an appropriate environment...
                genericDecl->inner->Accept(this);
                return genericDecl;
            }

            virtual void VisitTraitConformanceDecl(TraitConformanceDecl* conformanceDecl) override
            {
                // check the type being conformed to
                auto base = conformanceDecl->base;
                base = TranslateTypeNode(base);
                conformanceDecl->base = base;

                if(auto declRefType = base.type->As<DeclRefType>())
                {
                    if(auto traitDeclRef = declRefType->declRef.As<TraitDeclRef>())
                    {
                        conformanceDecl->traitDeclRef = traitDeclRef;
                        return;
                    }
                }

                // We expected a trait here
                getSink()->diagnose( conformanceDecl, Diagnostics::expectedATraitGot, base.type);
            }

            RefPtr<ConstantIntVal> checkConstantIntVal(
                RefPtr<ExpressionSyntaxNode>    expr)
            {
                // First type-check the expression as normal
                expr = CheckExpr(expr);

                auto intVal = CheckIntegerConstantExpression(expr.Ptr());
                if(!intVal)
                    return nullptr;

                auto constIntVal = intVal.As<ConstantIntVal>();
                if(!constIntVal)
                {
                    getSink()->diagnose(expr->Position, Diagnostics::expectedIntegerConstantNotLiteral);
                    return nullptr;
                }
                return constIntVal;
            }

            RefPtr<Modifier> checkModifier(
                RefPtr<Modifier>    m,
                Decl*               decl)
            {
                if(auto hlslUncheckedAttribute = m.As<HLSLUncheckedAttribute>())
                {
                    // We have an HLSL `[name(arg,...)]` attribute, and we'd like
                    // to check that it is provides all the expected arguments
                    //
                    // For now we will do this in a completely ad hoc fashion,
                    // but it would be nice to have some generic routine to
                    // do the needed type checking/coercion.
                    if(hlslUncheckedAttribute->nameToken.Content == "numthreads")
                    {
                        if(hlslUncheckedAttribute->args.Count() != 3)
                            return m;

                        auto xVal = checkConstantIntVal(hlslUncheckedAttribute->args[0]);
                        auto yVal = checkConstantIntVal(hlslUncheckedAttribute->args[1]);
                        auto zVal = checkConstantIntVal(hlslUncheckedAttribute->args[2]);

                        if(!xVal) return m;
                        if(!yVal) return m;
                        if(!zVal) return m;

                        auto hlslNumThreadsAttribute = new HLSLNumThreadsAttribute();

                        hlslNumThreadsAttribute->Position   = hlslUncheckedAttribute->Position;
                        hlslNumThreadsAttribute->nameToken  = hlslUncheckedAttribute->nameToken;
                        hlslNumThreadsAttribute->args       = hlslUncheckedAttribute->args;
                        hlslNumThreadsAttribute->x          = xVal->value;
                        hlslNumThreadsAttribute->y          = yVal->value;
                        hlslNumThreadsAttribute->z          = zVal->value;

                        return hlslNumThreadsAttribute;
                    }
                }

                // Default behavior is to leave things as they are,
                // and assume that modifiers are mostly already checked.
                //
                // TODO: This would be a good place to validate that
                // a modifier is actually valid for the thing it is
                // being applied to, and potentially to check that
                // it isn't in conflict with any other modifiers
                // on the same declaration.

                return m;
            }


            void checkModifiers(Decl* decl)
            {
                // TODO(tfoley): need to make sure this only
                // performs semantic checks on a `SharedModifier` once...

                // The process of checking a modifier may produce a new modifier in its place,
                // so we will build up a new linked list of modifiers that will replace
                // the old list.
                RefPtr<Modifier> resultModifiers;
                RefPtr<Modifier>* resultModifierLink = &resultModifiers;

                RefPtr<Modifier> modifier = decl->modifiers.first;
                while(modifier)
                {
                    // Because we are rewriting the list in place, we need to extract
                    // the next modifier here (not at the end of the loop).
                    auto next = modifier->next;

                    // We also go ahead and clobber the `next` field on the modifier
                    // itself, so that the default behavior of `checkModifier()` can
                    // be to return a single unlinked modifier.
                    modifier->next = nullptr;

                    auto checkedModifier = checkModifier(modifier, decl);
                    if(checkedModifier)
                    {
                        // If checking gave us a modifier to add, then we
                        // had better add it.

                        // Just in case `checkModifier` ever returns multiple
                        // modifiers, lets advance to the end of the list we
                        // are building.
                        while(*resultModifierLink)
                            resultModifierLink = &(*resultModifierLink)->next;

                        // attach the new modifier at the end of the list,
                        // and now set the "link" to point to its `next` field
                        *resultModifierLink = checkedModifier;
                        resultModifierLink = &checkedModifier->next;
                    }

                    // Move along to the next modifier
                    modifier = next;
                }

                // Whether we actually re-wrote anything or note, lets
                // install the new list of modifiers on the declaration
                decl->modifiers.first = resultModifiers;
            }

            virtual RefPtr<ProgramSyntaxNode> VisitProgram(ProgramSyntaxNode * programNode) override
            {
                // Try to register all the builtin decls
                for (auto decl : programNode->Members)
                {
                    auto inner = decl;
                    if (auto genericDecl = decl.As<GenericDecl>())
                    {
                        inner = genericDecl->inner;
                    }

                    if (auto builtinMod = inner->FindModifier<BuiltinTypeModifier>())
                    {
                        RegisterBuiltinDecl(decl, builtinMod);
                    }
                    if (auto magicMod = inner->FindModifier<MagicTypeModifier>())
                    {
                        RegisterMagicDecl(decl, magicMod);
                    }
                }

                //

                HashSet<String> funcNames;
                this->program = programNode;
                this->function = nullptr;

                for (auto & s : program->GetTypeDefs())
                    VisitTypeDefDecl(s.Ptr());
                for (auto & s : program->GetStructs())
                {
                    VisitStruct(s.Ptr());
                }
				for (auto & s : program->GetClasses())
				{
					VisitClass(s.Ptr());
				}
                // HACK(tfoley): Visiting all generic declarations here,
                // because otherwise they won't get visited.
                for (auto & g : program->GetMembersOfType<GenericDecl>())
                {
                    VisitGenericDecl(g.Ptr());
                }

                for (auto & func : program->GetFunctions())
                {
                    if (!func->IsChecked(DeclCheckState::Checked))
                    {
                        VisitFunctionDeclaration(func.Ptr());
                    }
                }
                for (auto & func : program->GetFunctions())
                {
                    EnsureDecl(func);
                }
        
                if (sink->GetErrorCount() != 0)
                    return programNode;
               
                // Force everything to be fully checked, just in case
                // Note that we don't just call this on the program,
                // because we'd end up recursing into this very code path...
                for (auto d : programNode->Members)
                {
                    EnusreAllDeclsRec(d);
                }

                // Do any semantic checking required on modifiers?
                for (auto d : programNode->Members)
                {
                    checkModifiers(d.Ptr());
                }

                return programNode;
            }

			virtual RefPtr<ClassSyntaxNode> VisitClass(ClassSyntaxNode * classNode) override
			{
				if (classNode->IsChecked(DeclCheckState::Checked))
					return classNode;
				classNode->SetCheckState(DeclCheckState::Checked);

				for (auto field : classNode->GetFields())
				{
					field->Type = CheckUsableType(field->Type);
					field->SetCheckState(DeclCheckState::Checked);
				}
				return classNode;
			}

            virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * structNode) override
            {
                if (structNode->IsChecked(DeclCheckState::Checked))
                    return structNode;
                structNode->SetCheckState(DeclCheckState::Checked);

                for (auto field : structNode->GetFields())
                {
                    field->Type = CheckUsableType(field->Type);
                    field->SetCheckState(DeclCheckState::Checked);
                }
                return structNode;
            }

            virtual RefPtr<TypeDefDecl> VisitTypeDefDecl(TypeDefDecl* decl) override
            {
                if (decl->IsChecked(DeclCheckState::Checked)) return decl;

                decl->SetCheckState(DeclCheckState::CheckingHeader);
                decl->Type = CheckProperType(decl->Type);
                decl->SetCheckState(DeclCheckState::Checked);
                return decl;
            }

            virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode *functionNode) override
            {
                if (functionNode->IsChecked(DeclCheckState::Checked))
                    return functionNode;

                VisitFunctionDeclaration(functionNode);
                functionNode->SetCheckState(DeclCheckState::Checked);

                if (!functionNode->IsExtern())
                {
                    this->function = functionNode;
                    if (functionNode->Body)
                    {
                        functionNode->Body->Accept(this);
                    }
                    this->function = nullptr;
                }
                return functionNode;
            }

            // Check if two functions have the same signature for the purposes
            // of overload resolution.
            bool DoFunctionSignaturesMatch(
                FunctionSyntaxNode* fst,
                FunctionSyntaxNode* snd)
            {
                // TODO(tfoley): This function won't do anything sensible for generics,
                // so we need to figure out a plan for that...

                // TODO(tfoley): This copies the parameter array, which is bad for performance.
                auto fstParams = fst->GetParameters().ToArray();
                auto sndParams = snd->GetParameters().ToArray();

                // If the functions have different numbers of parameters, then
                // their signatures trivially don't match.
                auto fstParamCount = fstParams.Count();
                auto sndParamCount = sndParams.Count();
                if (fstParamCount != sndParamCount)
                    return false;

                for (int ii = 0; ii < fstParamCount; ++ii)
                {
                    auto fstParam = fstParams[ii];
                    auto sndParam = sndParams[ii];

                    // If a given parameter type doesn't match, then signatures don't match
                    if (!fstParam->Type.Equals(sndParam->Type))
                        return false;

                    // If one parameter is `out` and the other isn't, then they don't match
                    //
                    // Note(tfoley): we don't consider `out` and `inout` as distinct here,
                    // because there is no way for overload resolution to pick between them.
                    if (fstParam->HasModifier<OutModifier>() != sndParam->HasModifier<OutModifier>())
                        return false;
                }

                // Note(tfoley): return type doesn't enter into it, because we can't take
                // calling context into account during overload resolution.

                return true;
            }

            void ValidateFunctionRedeclaration(FunctionSyntaxNode* funcDecl)
            {
                auto parentDecl = funcDecl->ParentDecl;
                assert(parentDecl);
                if (!parentDecl) return;

                // Look at previously-declared functions with the same name,
                // in the same container
                BuildMemberDictionary(parentDecl);

                for (auto prevDecl = funcDecl->nextInContainerWithSameName; prevDecl; prevDecl = prevDecl->nextInContainerWithSameName)
                {
                    // Look through generics to the declaration underneath
                    auto prevGenericDecl = dynamic_cast<GenericDecl*>(prevDecl);
                    if (prevGenericDecl)
                        prevDecl = prevGenericDecl->inner.Ptr();

                    // We only care about previously-declared functions
                    // Note(tfoley): although we should really error out if the
                    // name is already in use for something else, like a variable...
                    auto prevFuncDecl = dynamic_cast<FunctionSyntaxNode*>(prevDecl);
                    if (!prevFuncDecl)
                        continue;

                    // If the parameter signatures don't match, then don't worry
                    if (!DoFunctionSignaturesMatch(funcDecl, prevFuncDecl))
                        continue;

                    // If we get this far, then we've got two declarations in the same
                    // scope, with the same name and signature.
                    //
                    // They might just be redeclarations, which we would want to allow.

                    // First, check if the return types match.
                    // TODO(tfolye): this code won't work for generics
                    if (!funcDecl->ReturnType.Equals(prevFuncDecl->ReturnType))
                    {
                        // Bad dedeclaration
                        getSink()->diagnose(funcDecl, Diagnostics::unimplemented, "redeclaration has a different return type");

                        // Don't bother emitting other errors at this point
                        break;
                    }

                    // TODO(tfoley): track the fact that there is redeclaration going on,
                    // so that we can detect it and react accordingly during overload resolution
                    // (e.g., by only considering one declaration as the canonical one...)

                    // If both have a body, then there is trouble
                    if (funcDecl->Body && prevFuncDecl->Body)
                    {
                        // Redefinition
                        getSink()->diagnose(funcDecl, Diagnostics::unimplemented, "function redefinition");

                        // Don't bother emitting other errors
                        break;
                    }

                    // TODO(tfoley): If both specific default argument expressions
                    // for the same value, then that is an error too...
                }
            }

            void VisitFunctionDeclaration(FunctionSyntaxNode *functionNode)
            {
                if (functionNode->IsChecked(DeclCheckState::CheckedHeader)) return;
                functionNode->SetCheckState(DeclCheckState::CheckingHeader);

                this->function = functionNode;
                auto returnType = CheckProperType(functionNode->ReturnType);
                functionNode->ReturnType = returnType;
                HashSet<String> paraNames;
                for (auto & para : functionNode->GetParameters())
                {
                    if (paraNames.Contains(para->Name.Content))
                        getSink()->diagnose(para, Diagnostics::parameterAlreadyDefined, para->Name);
                    else
                        paraNames.Add(para->Name.Content);
                    para->Type = CheckUsableType(para->Type);
                    if (para->Type.Equals(ExpressionType::GetVoid()))
                        getSink()->diagnose(para, Diagnostics::parameterCannotBeVoid);
                }
                this->function = NULL;
                functionNode->SetCheckState(DeclCheckState::CheckedHeader);

                // One last bit of validation: check if we are redeclaring an existing function
                ValidateFunctionRedeclaration(functionNode);
            }

            virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode *stmt) override
            {
                for (auto & node : stmt->Statements)
                {
                    node->Accept(this);
                }
                return stmt;
            }

            template<typename T>
            T* FindOuterStmt()
            {
                int outerStmtCount = outerStmts.Count();
                for (int ii = outerStmtCount - 1; ii >= 0; --ii)
                {
                    auto outerStmt = outerStmts[ii];
                    auto found = dynamic_cast<T*>(outerStmt);
                    if (found)
                        return found;
                }
                return nullptr;
            }

            virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode *stmt) override
            {
                auto outer = FindOuterStmt<BreakableStmt>();
                if (!outer)
                {
                    getSink()->diagnose(stmt, Diagnostics::breakOutsideLoop);
                }
                stmt->parentStmt = outer;
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode *stmt) override
            {
                auto outer = FindOuterStmt<LoopStmt>();
                if (!outer)
                {
                    getSink()->diagnose(stmt, Diagnostics::continueOutsideLoop);
                }
                stmt->parentStmt = outer;
                return stmt;
            }

            void PushOuterStmt(StatementSyntaxNode* stmt)
            {
                outerStmts.Add(stmt);
            }

            void PopOuterStmt(StatementSyntaxNode* /*stmt*/)
            {
                outerStmts.RemoveAt(outerStmts.Count() - 1);
            }

            virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode *stmt) override
            {
                PushOuterStmt(stmt);
                if (stmt->Predicate != NULL)
                    stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
                if (!stmt->Predicate->Type->Equals(ExpressionType::GetError()) &&
                    !stmt->Predicate->Type->Equals(ExpressionType::GetInt()) &&
                    !stmt->Predicate->Type->Equals(ExpressionType::GetBool()))
                {
                    getSink()->diagnose(stmt, Diagnostics::whilePredicateTypeError);
                }
                stmt->Statement->Accept(this);

                PopOuterStmt(stmt);
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode *stmt) override
            {
                PushOuterStmt(stmt);
                if (stmt->InitialStatement)
                {
                    stmt->InitialStatement = stmt->InitialStatement->Accept(this).As<StatementSyntaxNode>();
                }
                if (stmt->PredicateExpression)
                {
                    stmt->PredicateExpression = stmt->PredicateExpression->Accept(this).As<ExpressionSyntaxNode>();
                    if (!stmt->PredicateExpression->Type->Equals(ExpressionType::GetBool()) &&
                        !stmt->PredicateExpression->Type->Equals(ExpressionType::GetInt()) &&
                        !stmt->PredicateExpression->Type->Equals(ExpressionType::GetUInt()))
                    {
                        getSink()->diagnose(stmt->PredicateExpression.Ptr(), Diagnostics::forPredicateTypeError);
                    }
                }
                if (stmt->SideEffectExpression)
                {
                    stmt->SideEffectExpression = stmt->SideEffectExpression->Accept(this).As<ExpressionSyntaxNode>();
                }
                stmt->Statement->Accept(this);

                PopOuterStmt(stmt);
                return stmt;
            }
            virtual RefPtr<SwitchStmt> VisitSwitchStmt(SwitchStmt* stmt) override
            {
                PushOuterStmt(stmt);
                // TODO(tfoley): need to coerce condition to an integral type...
                stmt->condition = CheckExpr(stmt->condition);
                stmt->body->Accept(this);
                PopOuterStmt(stmt);
                return stmt;
            }
            virtual RefPtr<CaseStmt> VisitCaseStmt(CaseStmt* stmt) override
            {
                auto expr = CheckExpr(stmt->expr);
                auto switchStmt = FindOuterStmt<SwitchStmt>();

                if (!switchStmt)
                {
                    getSink()->diagnose(stmt, Diagnostics::caseOutsideSwitch);
                }
                else
                {
                    // TODO: need to do some basic matching to ensure the type
                    // for the `case` is consistent with the type for the `switch`...
                }

                stmt->expr = expr;
                stmt->parentStmt = switchStmt;

                return stmt;
            }
            virtual RefPtr<DefaultStmt> VisitDefaultStmt(DefaultStmt* stmt) override
            {
                auto switchStmt = FindOuterStmt<SwitchStmt>();
                if (!switchStmt)
                {
                    getSink()->diagnose(stmt, Diagnostics::defaultOutsideSwitch);
                }
                stmt->parentStmt = switchStmt;
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode *stmt) override
            {
                if (stmt->Predicate != NULL)
                    stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
                if (!stmt->Predicate->Type->Equals(ExpressionType::GetError())
                    && (!stmt->Predicate->Type->Equals(ExpressionType::GetInt()) &&
                        !stmt->Predicate->Type->Equals(ExpressionType::GetBool())))
                    getSink()->diagnose(stmt, Diagnostics::ifPredicateTypeError);

                if (stmt->PositiveStatement != NULL)
                    stmt->PositiveStatement->Accept(this);

                if (stmt->NegativeStatement != NULL)
                    stmt->NegativeStatement->Accept(this);
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode *stmt) override
            {
                if (!stmt->Expression)
                {
                    if (function && !function->ReturnType.Equals(ExpressionType::GetVoid()))
                        getSink()->diagnose(stmt, Diagnostics::returnNeedsExpression);
                }
                else
                {
                    stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
                    if (!stmt->Expression->Type->Equals(ExpressionType::Error.Ptr()))
                    {
                        if (function)
                        {
                            stmt->Expression = Coerce(function->ReturnType, stmt->Expression);
                        }
                        else
                        {
                            // TODO(tfoley): this case currently gets triggered for member functions,
                            // which aren't being checked consistently (because of the whole symbol
                            // table idea getting in the way).

//							getSink()->diagnose(stmt, Diagnostics::unimplemented, "case for return stmt");
                        }
                    }
                }
                return stmt;
            }

            int GetMinBound(RefPtr<IntVal> val)
            {
                if (auto constantVal = val.As<ConstantIntVal>())
                    return constantVal->value;

                // TODO(tfoley): Need to track intervals so that this isn't just a lie...
                return 1;
            }

            void ValidateArraySizeForVariable(Variable* varDecl)
            {
                auto arrayType = varDecl->Type->AsArrayType();
                if (!arrayType) return;

                auto elementCount = arrayType->ArrayLength;
                if (!elementCount)
                {
                    getSink()->diagnose(varDecl, Diagnostics::invalidArraySize);
                    return;
                }

                // TODO(tfoley): How to handle the case where bound isn't known?
                if (GetMinBound(elementCount) <= 0)
                {
                    getSink()->diagnose(varDecl, Diagnostics::invalidArraySize);
                    return;
                }
            }

            virtual RefPtr<Variable> VisitDeclrVariable(Variable* varDecl)
            {
                TypeExp typeExp = CheckUsableType(varDecl->Type);
                if (typeExp.type->GetBindableResourceType() != BindableResourceType::NonBindable)
                {
                    // We don't want to allow bindable resource types as local variables (at least for now).
                    auto parentDecl = varDecl->ParentDecl;
                    if (auto parentScopeDecl = dynamic_cast<ScopeDecl*>(parentDecl))
                    {
                        getSink()->diagnose(varDecl->Type, Diagnostics::invalidTypeForLocalVariable);
                    }
                }
                varDecl->Type = typeExp;
                if (varDecl->Type.Equals(ExpressionType::GetVoid()))
                    getSink()->diagnose(varDecl, Diagnostics::invalidTypeVoid);

                // If this is an array variable, then make sure it is an okay array type...
                ValidateArraySizeForVariable(varDecl);

                if (varDecl->Expr != NULL)
                {
                    varDecl->Expr = varDecl->Expr->Accept(this).As<ExpressionSyntaxNode>();
                    varDecl->Expr = Coerce(varDecl->Type, varDecl->Expr);
                }
                return varDecl;
            }

            virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode *stmt) override
            {
                PushOuterStmt(stmt);
                stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
                if (!stmt->Predicate->Type->Equals(ExpressionType::GetError()) &&
                    !stmt->Predicate->Type->Equals(ExpressionType::GetInt()) &&
                    !stmt->Predicate->Type->Equals(ExpressionType::GetBool()))
                    getSink()->diagnose(stmt, Diagnostics::whilePredicateTypeError2);

                stmt->Statement->Accept(this);
                PopOuterStmt(stmt);
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode *stmt) override
            {
                stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitOperatorExpression(OperatorExpressionSyntaxNode *expr) override
            {
                for (int i = 0; i < expr->Arguments.Count(); i++)
                    expr->Arguments[i] = expr->Arguments[i]->Accept(this).As<ExpressionSyntaxNode>();
                auto & leftType = expr->Arguments[0]->Type;
                QualType rightType;
                if (expr->Arguments.Count() == 2)
                    rightType = expr->Arguments[1]->Type;
                RefPtr<ExpressionType> matchedType;
                auto checkAssign = [&]()
                {
                    if (!leftType.IsLeftValue &&
                        !leftType->Equals(ExpressionType::Error.Ptr()))
                        getSink()->diagnose(expr->Arguments[0].Ptr(), Diagnostics::assignNonLValue);
                    if (expr->Operator == Operator::AndAssign ||
                        expr->Operator == Operator::OrAssign ||
                        expr->Operator == Operator::XorAssign ||
                        expr->Operator == Operator::LshAssign ||
                        expr->Operator == Operator::RshAssign)
                    {
                        if (!(leftType->IsIntegral() && rightType->IsIntegral()))
                        {
                            getSink()->diagnose(expr, Diagnostics::bitOperationNonIntegral);
                        }
                    }

                    // TODO(tfoley): Need to actual insert coercion here...
                    if(CanCoerce(leftType, expr->Type))
                        expr->Type = leftType;
                    else
                        expr->Type = ExpressionType::Error;
                };
                if (expr->Operator == Operator::Assign)
                {
                    expr->Type = rightType;
                    checkAssign();
                }
                else
                {
                    expr->FunctionExpr = CheckExpr(expr->FunctionExpr);
                    CheckInvokeExprWithCheckedOperands(expr);
                    if (expr->Operator > Operator::Assign)
                        checkAssign();
                }
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode *expr) override
            {
                switch (expr->ConstType)
                {
                case ConstantExpressionSyntaxNode::ConstantType::Int:
                    expr->Type = ExpressionType::GetInt();
                    break;
                case ConstantExpressionSyntaxNode::ConstantType::Bool:
                    expr->Type = ExpressionType::GetBool();
                    break;
                case ConstantExpressionSyntaxNode::ConstantType::Float:
                    expr->Type = ExpressionType::GetFloat();
                    break;
                default:
                    expr->Type = ExpressionType::Error;
                    throw "Invalid constant type.";
                    break;
                }
                return expr;
            }

            IntVal* GetIntVal(ConstantExpressionSyntaxNode* expr)
            {
                // TODO(tfoley): don't keep allocating here!
                return new ConstantIntVal(expr->IntValue);
            }

            RefPtr<IntVal> TryConstantFoldExpr(
                InvokeExpressionSyntaxNode* invokeExpr)
            {
                // We need all the operands to the expression

                // Check if the callee is an operation that is amenable to constant-folding.
                //
                // For right now we will look for calls to intrinsic functions, and then inspect
                // their names (this is bad and slow).
                auto funcDeclRefExpr = invokeExpr->FunctionExpr.As<DeclRefExpr>();
                if (!funcDeclRefExpr) return nullptr;

                auto funcDeclRef = funcDeclRefExpr->declRef;
                auto intrinsicMod = funcDeclRef.GetDecl()->FindModifier<IntrinsicModifier>();
                if (!intrinsicMod) return nullptr;

                // Let's not constant-fold operations with more than a certain number of arguments, for simplicity
                static const int kMaxArgs = 8;
                if (invokeExpr->Arguments.Count() > kMaxArgs)
                    return nullptr;

                // Before checking the operation name, let's look at the arguments
                RefPtr<IntVal> argVals[kMaxArgs];
                int constArgVals[kMaxArgs];
                int argCount = 0;
                bool allConst = true;
                for (auto argExpr : invokeExpr->Arguments)
                {
                    auto argVal = TryCheckIntegerConstantExpression(argExpr.Ptr());
                    if (!argVal)
                        return nullptr;

                    argVals[argCount] = argVal;

                    if (auto constArgVal = argVal.As<ConstantIntVal>())
                    {
                        constArgVals[argCount] = constArgVal->value;
                    }
                    else
                    {
                        allConst = false;
                    }
                    argCount++;
                }

                if (!allConst)
                {
                    // TODO(tfoley): We probably want to support a very limited number of operations
                    // on "constants" that aren't actually known, to be able to handle a generic
                    // that takes an integer `N` but then constructs a vector of size `N+1`.
                    //
                    // The hard part there is implementing the rules for value unification in the
                    // presence of more complicated `IntVal` subclasses, like `SumIntVal`. You'd
                    // need inference to be smart enough to know that `2 + N` and `N + 2` are the
                    // same value, as are `N + M + 1 + 1` and `M + 2 + N`.
                    //
                    // For now we can just bail in this case.
                    return nullptr;
                }

                // At this point, all the operands had simple integer values, so we are golden.
                int resultValue = 0;
                auto opName = funcDeclRef.GetName();

                // handle binary operators
                if (opName == "-")
                {
                    if (argCount == 1)
                    {
                        resultValue = -constArgVals[0];
                    }
                    else if (argCount == 2)
                    {
                        resultValue = constArgVals[0] - constArgVals[1];
                    }
                }

                // simple binary operators
#define CASE(OP)                                                        \
                else if(opName == #OP) do {                             \
                    if(argCount != 2) return nullptr;                   \
                    resultValue = constArgVals[0] OP constArgVals[1];   \
                } while(0)

                CASE(+); // TODO: this can also be unary...
                CASE(*);
#undef CASE

                // binary operators with chance of divide-by-zero
                // TODO: issue a suitable error in that case
#define CASE(OP)                                                        \
                else if(opName == #OP) do {                             \
                    if(argCount != 2) return nullptr;                   \
                    if(!constArgVals[1]) return nullptr;                \
                    resultValue = constArgVals[0] OP constArgVals[1];   \
                } while(0)

                CASE(/);
                CASE(%);
#undef CASE

                // TODO(tfoley): more cases
                else
                {
                    return nullptr;
                }

                RefPtr<IntVal> result = new ConstantIntVal(resultValue);
                return result;
            }

            RefPtr<IntVal> TryConstantFoldExpr(
                ExpressionSyntaxNode* expr)
            {
                // TODO(tfoley): more serious constant folding here
                if (auto constExp = dynamic_cast<ConstantExpressionSyntaxNode*>(expr))
                {
                    return GetIntVal(constExp);
                }

                // it is possible that we are referring to a generic value param
                if (auto declRefExpr = dynamic_cast<DeclRefExpr*>(expr))
                {
                    auto declRef = declRefExpr->declRef;

                    if (auto genericValParamRef = declRef.As<GenericValueParamDeclRef>())
                    {
                        // TODO(tfoley): handle the case of non-`int` value parameters...
                        return new GenericParamIntVal(genericValParamRef);
                    }

                    // We may also need to check for references to variables that
                    // are defined in a way that can be used as a constant expression:
                    if(auto varRef = declRef.As<VarDeclBaseRef>())
                    {
                        auto varDecl = varRef.GetDecl();
                        if(auto staticAttr = varDecl->FindModifier<HLSLStaticModifier>())
                        {
                            if(auto constAttr = varDecl->FindModifier<ConstModifier>())
                            {
                                // HLSL `static const` can be used as a constant expression
                                if(auto initExpr = varRef.getInitExpr())
                                {
                                    return TryConstantFoldExpr(initExpr.Ptr());
                                }
                            }
                        }
                    }
                }

                if (auto invokeExpr = dynamic_cast<InvokeExpressionSyntaxNode*>(expr))
                {
                    auto val = TryConstantFoldExpr(invokeExpr);
                    if (val)
                        return val;
                }
                else if(auto castExpr = dynamic_cast<TypeCastExpressionSyntaxNode*>(expr))
                {
                    auto val = TryConstantFoldExpr(castExpr->Expression.Ptr());
                    if(val)
                        return val;
                }

                return nullptr;
            }

            // Try to check an integer constant expression, either returning the value,
            // or NULL if the expression isn't recognized as a constant.
            RefPtr<IntVal> TryCheckIntegerConstantExpression(ExpressionSyntaxNode* exp)
            {
                if (!exp->Type.type->Equals(ExpressionType::GetInt()))
                {
                    return nullptr;
                }



                // Otherwise, we need to consider operations that we might be able to constant-fold...
                return TryConstantFoldExpr(exp);
            }

            // Enforce that an expression resolves to an integer constant, and get its value
            RefPtr<IntVal> CheckIntegerConstantExpression(ExpressionSyntaxNode* inExpr)
            {
                // First coerce the expression to the expected type
                auto expr = Coerce(ExpressionType::GetInt(),inExpr);
                auto result = TryCheckIntegerConstantExpression(expr.Ptr());
                if (!result)
                {
                    getSink()->diagnose(expr, Diagnostics::expectedIntegerConstantNotConstant);
                }
                return result;
            }



            RefPtr<ExpressionSyntaxNode> CheckSimpleSubscriptExpr(
                RefPtr<IndexExpressionSyntaxNode>   subscriptExpr,
                RefPtr<ExpressionType>              elementType)
            {
                auto baseExpr = subscriptExpr->BaseExpression;
                auto indexExpr = subscriptExpr->IndexExpression;

                if (!indexExpr->Type->Equals(ExpressionType::GetInt()) &&
                    !indexExpr->Type->Equals(ExpressionType::GetUInt()))
                {
                    getSink()->diagnose(indexExpr, Diagnostics::subscriptIndexNonInteger);
                    return CreateErrorExpr(subscriptExpr.Ptr());
                }

                subscriptExpr->Type = elementType;

                // TODO(tfoley): need to be more careful about this stuff
                subscriptExpr->Type.IsLeftValue = baseExpr->Type.IsLeftValue;

                return subscriptExpr;
            }

            virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode* subscriptExpr) override
            {
                auto baseExpr = subscriptExpr->BaseExpression;
                baseExpr = CheckExpr(baseExpr);

                RefPtr<ExpressionSyntaxNode> indexExpr = subscriptExpr->IndexExpression;
                if (indexExpr)
                {
                    indexExpr = CheckExpr(indexExpr);
                }

                subscriptExpr->BaseExpression = baseExpr;
                subscriptExpr->IndexExpression = indexExpr;

                // If anything went wrong in the base expression,
                // then just move along...
                if (IsErrorExpr(baseExpr))
                    return CreateErrorExpr(subscriptExpr);

                // Otherwise, we need to look at the type of the base expression,
                // to figure out how subscripting should work.
                auto baseType = baseExpr->Type.Ptr();
                if (auto baseTypeType = baseType->As<TypeExpressionType>())
                {
                    // We are trying to "index" into a type, so we have an expression like `float[2]`
                    // which should be interpreted as resolving to an array type.

                    RefPtr<IntVal> elementCount = nullptr;
                    if (indexExpr)
                    {
                        elementCount = CheckIntegerConstantExpression(indexExpr.Ptr());
                    }

                    auto elementType = CoerceToUsableType(TypeExp(baseExpr, baseTypeType->type));
                    auto arrayType = new ArrayExpressionType();
                    arrayType->BaseType = elementType;
                    arrayType->ArrayLength = elementCount;

                    typeResult = arrayType;
                    subscriptExpr->Type = new TypeExpressionType(arrayType);
                    return subscriptExpr;
                }
                else if (auto baseArrayType = baseType->As<ArrayExpressionType>())
                {
                    return CheckSimpleSubscriptExpr(
                        subscriptExpr,
                        baseArrayType->BaseType);
                }
                else if (auto baseArrayLikeType = baseType->As<ArrayLikeType>())
                {
                    return CheckSimpleSubscriptExpr(
                        subscriptExpr,
                        baseArrayLikeType->elementType);
                }
                else if (auto vecType = baseType->As<VectorExpressionType>())
                {
                    return CheckSimpleSubscriptExpr(
                        subscriptExpr,
                        vecType->elementType);
                }
                else if (auto matType = baseType->As<MatrixExpressionType>())
                {
                    // TODO(tfoley): We shouldn't go and recompute
                    // row types over and over like this... :(
                    auto rowType = new VectorExpressionType(
                        matType->elementType,
                        matType->colCount);

                    return CheckSimpleSubscriptExpr(
                        subscriptExpr,
                        rowType);
                }
                else
                {
                    getSink()->diagnose(subscriptExpr, Diagnostics::subscriptNonArray);
                    return CreateErrorExpr(subscriptExpr);
                }
            }
            bool MatchArguments(FunctionSyntaxNode * functionNode, List <RefPtr<ExpressionSyntaxNode>> &args)
            {
                if (functionNode->GetParameters().Count() != args.Count())
                    return false;
                int i = 0;
                for (auto param : functionNode->GetParameters())
                {
                    if (!param->Type.Equals(args[i]->Type.Ptr()))
                        return false;
                    i++;
                }
                return true;
            }

            template<typename GetParamFunc, typename PFuncT>
            PFuncT FindFunctionOverload(const List<PFuncT> & funcs, const GetParamFunc & getParam, const List<RefPtr<ExpressionType>> & arguments)
            {
                int bestMatchConversions = 1 << 30;
                PFuncT func = nullptr;
                for (auto & f : funcs)
                {
                    auto params = getParam(f);
                    if (params.Count() == arguments.Count())
                    {
                        int conversions = 0;
                        bool match = true;
                        int i = 0;
                        for (auto param : params)
                        {
                            auto argType = arguments[i];
                            auto paramType = param->Type;
                            if (argType->Equals(paramType.Ptr()))
                            {
                            }
                            else if (MatchType_ValueReceiver(paramType.Ptr(), argType.Ptr()))
                            {
                                conversions++;
                            }
                            else
                            {
                                match = false;
                                break;
                            }
                            i++;
                        }
                        if (match && conversions < bestMatchConversions)
                        {
                            func = f;
                            bestMatchConversions = conversions;
                        }
                    }
                }
                return func;
            }

            // Coerce an expression to a specific  type that it is expected to have in context
            RefPtr<ExpressionSyntaxNode> CoerceExprToType(
                RefPtr<ExpressionSyntaxNode>	expr,
                RefPtr<ExpressionType>			type)
            {
                // TODO(tfoley): clean this up so there is only one version...
                return Coerce(type, expr);
            }

            // Resolve a call to a function, represented here
            // by a symbol with a `FuncType` type.
            RefPtr<ExpressionSyntaxNode> ResolveFunctionApp(
                RefPtr<FuncType>			funcType,
                InvokeExpressionSyntaxNode*	/*appExpr*/)
            {
                // TODO(tfoley): Actual checking logic needs to go here...
#if 0
                auto& args = appExpr->Arguments;
                List<RefPtr<ParameterSyntaxNode>> params;
                RefPtr<ExpressionType> resultType;
                if (auto funcDeclRef = funcType->declRef)
                {
                    EnsureDecl(funcDeclRef.GetDecl());

                    params = funcDeclRef->GetParameters().ToArray();
                    resultType = funcDecl->ReturnType;
                }
                else if (auto funcSym = funcType->Func)
                {
                    auto funcDecl = funcSym->SyntaxNode;
                    EnsureDecl(funcDecl);

                    params = funcDecl->GetParameters().ToArray();
                    resultType = funcDecl->ReturnType;
                }
                else if (auto componentFuncSym = funcType->Component)
                {
                    auto componentFuncDecl = componentFuncSym->Implementations.First()->SyntaxNode;
                    params = componentFuncDecl->GetParameters().ToArray();
                    resultType = componentFuncDecl->Type;
                }

                auto argCount = args.Count();
                auto paramCount = params.Count();
                if (argCount != paramCount)
                {
                    getSink()->diagnose(appExpr, Diagnostics::unimplemented, "wrong number of arguments for call");
                    appExpr->Type = ExpressionType::Error;
                    return appExpr;
                }

                for (int ii = 0; ii < argCount; ++ii)
                {
                    auto arg = args[ii];
                    auto param = params[ii];

                    arg = CoerceExprToType(arg, param->Type);

                    args[ii] = arg;
                }

                assert(resultType);
                appExpr->Type = resultType;
                return appExpr;
#else
                throw "unimplemented";
#endif
            }

            // Resolve a constructor call, formed by apply a type to arguments
            RefPtr<ExpressionSyntaxNode> ResolveConstructorApp(
                RefPtr<ExpressionType>		type,
                InvokeExpressionSyntaxNode*	appExpr)
            {
                // TODO(tfoley): Actual checking logic needs to go here...

                appExpr->Type = type;
                return appExpr;
            }


            //

            virtual void VisitExtensionDecl(ExtensionDecl* decl) override
            {
                if (decl->IsChecked(DeclCheckState::Checked)) return;

                decl->SetCheckState(DeclCheckState::CheckingHeader);
                decl->targetType = CheckProperType(decl->targetType);

                // TODO: need to check that the target type names a declaration...

                if (auto targetDeclRefType = decl->targetType->As<DeclRefType>())
                {
                    // Attach our extension to that type as a candidate...
                    if (auto aggTypeDeclRef = targetDeclRefType->declRef.As<AggTypeDeclRef>())
                    {
                        auto aggTypeDecl = aggTypeDeclRef.GetDecl();
                        decl->nextCandidateExtension = aggTypeDecl->candidateExtensions;
                        aggTypeDecl->candidateExtensions = decl;
                    }
                    else
                    {
                        getSink()->diagnose(decl->targetType.exp, Diagnostics::unimplemented, "expected a nominal type here");
                    }
                }
                else if (decl->targetType->Equals(ExpressionType::Error))
                {
                    // there was an error, so ignore
                }
                else
                {
                    getSink()->diagnose(decl->targetType.exp, Diagnostics::unimplemented, "expected a nominal type here");
                }

                decl->SetCheckState(DeclCheckState::CheckedHeader);

                // now check the members of the extension
                for (auto m : decl->Members)
                {
                    EnsureDecl(m);
                }

                decl->SetCheckState(DeclCheckState::Checked);
            }

            virtual void VisitConstructorDecl(ConstructorDecl* decl) override
            {
                if (decl->IsChecked(DeclCheckState::Checked)) return;
                decl->SetCheckState(DeclCheckState::CheckingHeader);

                for (auto& paramDecl : decl->GetParameters())
                {
                    paramDecl->Type = CheckUsableType(paramDecl->Type);
                }
                decl->SetCheckState(DeclCheckState::CheckedHeader);

                // TODO(tfoley): check body
                decl->SetCheckState(DeclCheckState::Checked);
            }

            //

            struct Constraint
            {
                Decl*		decl; // the declaration of the thing being constraints
                RefPtr<Val>	val; // the value to which we are constraining it
                bool satisfied = false; // Has this constraint been met?
            };

            // A collection of constraints that will need to be satisified (solved)
            // in order for checking to suceed.
            struct ConstraintSystem
            {
                List<Constraint> constraints;
            };

            RefPtr<ExpressionType> TryJoinVectorAndScalarType(
                RefPtr<VectorExpressionType> vectorType,
                RefPtr<BasicExpressionType>  scalarType)
            {
                // Join( vector<T,N>, S ) -> vetor<Join(T,S), N>
                //
                // That is, the join of a vector and a scalar type is
                // a vector type with a joined element type.
                auto joinElementType = TryJoinTypes(
                    vectorType->elementType,
                    scalarType);
                if(!joinElementType)
                    return nullptr;

                return new VectorExpressionType(
                    joinElementType,
                    vectorType->elementCount);
            }

            bool DoesTypeConformToTrait(
                RefPtr<ExpressionType>  type,
                TraitDeclRef            traitDeclRef)
            {
                // for now look up a conformance member...
                if(auto declRefType = type->As<DeclRefType>())
                {
                    if( auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDeclRef>() )
                    {
                        for( auto conformanceRef : aggTypeDeclRef.GetMembersOfType<TraitConformanceDeclRef>())
                        {
                            EnsureDecl(conformanceRef.GetDecl());

                            if(traitDeclRef.Equals(conformanceRef.GetTraitDeclRef()))
                                return true;
                        }
                    }
                }

                // default is failure
                return false;
            }

            RefPtr<ExpressionType> TryJoinTypeWithTrait(
                RefPtr<ExpressionType>  type,
                TraitDeclRef            traitDeclRef)
            {
                // The most basic test here should be: does the type declare conformance to the trait.
                if(DoesTypeConformToTrait(type, traitDeclRef))
                    return type;

                // There is a more nuanced case if `type` is a builtin type, and we need to make it
                // conform to a trait that some but not all builtin types support (the main problem
                // here is when an operation wants an integer type, but one of our operands is a `float`.
                // The HLSL rules will allow that, with implicit conversion, but our default join rules
                // will end up picking `float` and we don't want that...).

                // For now we don't handle the hard case and just bail
                return nullptr;
            }

            // Try to compute the "join" between two types
            RefPtr<ExpressionType> TryJoinTypes(
                RefPtr<ExpressionType>  left,
                RefPtr<ExpressionType>  right)
            {
                // Easy case: they are the same type!
                if (left->Equals(right))
                    return left;

                // We can join two basic types by picking the "better" of the two
                if (auto leftBasic = left->As<BasicExpressionType>())
                {
                    if (auto rightBasic = right->As<BasicExpressionType>())
                    {
                        auto leftFlavor = leftBasic->BaseType;
                        auto rightFlavor = rightBasic->BaseType;

                        // TODO(tfoley): Need a special-case rule here that if
                        // either operand is of type `half`, then we promote
                        // to at least `float`

                        // Return the one that had higher rank...
                        if (leftFlavor > rightFlavor)
                            return left;
                        else
                        {
                            assert(rightFlavor > leftFlavor);
                            return right;
                        }
                    }

                    // We can also join a vector and a scalar
                    if(auto rightVector = right->As<VectorExpressionType>())
                    {
                        return TryJoinVectorAndScalarType(rightVector, leftBasic);
                    }
                }

                // We can join two vector types by joining their element types
                // (and also their sizes...)
                if( auto leftVector = left->As<VectorExpressionType>())
                {
                    if(auto rightVector = right->As<VectorExpressionType>())
                    {
                        // Check if the vector sizes match
                        if(!leftVector->elementCount->EqualsVal(rightVector->elementCount.Ptr()))
                            return nullptr;

                        // Try to join the element types
                        auto joinElementType = TryJoinTypes(
                            leftVector->elementType,
                            rightVector->elementType);
                        if(!joinElementType)
                            return nullptr;

                        return new VectorExpressionType(
                            joinElementType,
                            leftVector->elementCount);
                    }

                    // We can also join a vector and a scalar
                    if(auto rightBasic = right->As<BasicExpressionType>())
                    {
                        return TryJoinVectorAndScalarType(leftVector, rightBasic);
                    }
                }

                // HACK: trying to work trait types in here...
                if(auto leftDeclRefType = left->As<DeclRefType>())
                {
                    if( auto leftTraitRef = leftDeclRefType->declRef.As<TraitDeclRef>() )
                    {
                        // 
                        return TryJoinTypeWithTrait(right, leftTraitRef);
                    }
                }
                if(auto rightDeclRefType = right->As<DeclRefType>())
                {
                    if( auto rightTraitRef = rightDeclRefType->declRef.As<TraitDeclRef>() )
                    {
                        // 
                        return TryJoinTypeWithTrait(left, rightTraitRef);
                    }
                }

                // TODO: all the cases for vectors apply to matrices too!

                // Default case is that we just fail.
                return nullptr;
            }

            // Try to solve a system of generic constraints.
            // The `system` argument provides the constraints.
            // The `varSubst` argument provides the list of constraint
            // variables that were created for the system.
            //
            // Returns a new substitution representing the values that
            // we solved for along the way.
            RefPtr<Substitutions> TrySolveConstraintSystem(
                ConstraintSystem*		system,
                GenericDeclRef          genericDeclRef)
            {
                // For now the "solver" is going to be ridiculously simplistic.

                // The generic itself will have some constraints, so we need to try and solve those too
                for( auto constraintDeclRef : genericDeclRef.GetMembersOfType<GenericTypeConstraintDeclRef>() )
                {
                    if(!TryUnifyTypes(*system, constraintDeclRef.GetSub(), constraintDeclRef.GetSup()))
                        return nullptr;
                }

                // We will loop over the generic parameters, and for
                // each we will try to find a way to satisfy all
                // the constraints for that parameter
                List<RefPtr<Val>> args;
                for (auto m : genericDeclRef.GetMembers())
                {
                    if (auto typeParam = m.As<GenericTypeParamDeclRef>())
                    {
                        RefPtr<ExpressionType> type = nullptr;
                        for (auto& c : system->constraints)
                        {
                            if (c.decl != typeParam.GetDecl())
                                continue;

                            auto cType = c.val.As<ExpressionType>();
                            assert(cType.Ptr());

                            if (!type)
                            {
                                type = cType;
                            }
                            else
                            {
                                auto joinType = TryJoinTypes(type, cType);
                                if (!joinType)
                                {
                                    // failure!
                                    return nullptr;
                                }
                                type = joinType;
                            }

                            c.satisfied = true;
                        }

                        if (!type)
                        {
                            // failure!
                            return nullptr;
                        }
                        args.Add(type);
                    }
                    else if (auto valParam = m.As<GenericValueParamDeclRef>())
                    {
                        // TODO(tfoley): maybe support more than integers some day?
                        // TODO(tfoley): figure out how this needs to interact with
                        // compile-time integers that aren't just constants...
                        RefPtr<ConstantIntVal> val = nullptr;
                        for (auto& c : system->constraints)
                        {
                            if (c.decl != valParam.GetDecl())
                                continue;

                            auto cVal = c.val.As<ConstantIntVal>();
                            assert(cVal.Ptr());

                            if (!val)
                            {
                                val = cVal;
                            }
                            else
                            {
                                if (val->value != cVal->value)
                                {
                                    // failure!
                                    return nullptr;
                                }
                            }

                            c.satisfied = true;
                        }

                        if (!val)
                        {
                            // failure!
                            return nullptr;
                        }
                        args.Add(val);
                    }
                    else
                    {
                        // ignore anything that isn't a generic parameter
                    }
                }

                // Make sure we haven't constructed any spurious constraints
                // that we aren't able to satisfy:
                for (auto c : system->constraints)
                {
                    if (!c.satisfied)
                    {
                        return nullptr;
                    }
                }

                // Consruct a reference to the extension with our constraint variables
                // as the 
                RefPtr<Substitutions> solvedSubst = new Substitutions();
                solvedSubst->genericDecl = genericDeclRef.GetDecl();
                solvedSubst->outer = genericDeclRef.substitutions;
                solvedSubst->args = args;

                return solvedSubst;


#if 0
                List<RefPtr<Val>> solvedArgs;
                for (auto varArg : varSubst->args)
                {
                    if (auto typeVar = dynamic_cast<ConstraintVarType*>(varArg.Ptr()))
                    {
                        RefPtr<ExpressionType> type = nullptr;
                        for (auto& c : system->constraints)
                        {
                            if (c.decl != typeVar->declRef.GetDecl())
                                continue;

                            auto cType = c.val.As<ExpressionType>();
                            assert(cType.Ptr());

                            if (!type)
                            {
                                type = cType;
                            }
                            else
                            {
                                if (!type->Equals(cType))
                                {
                                    // failure!
                                    return nullptr;
                                }
                            }

                            c.satisfied = true;
                        }

                        if (!type)
                        {
                            // failure!
                            return nullptr;
                        }
                        solvedArgs.Add(type);
                    }
                    else if (auto valueVar = dynamic_cast<ConstraintVarInt*>(varArg.Ptr()))
                    {
                        // TODO(tfoley): maybe support more than integers some day?
                        RefPtr<IntVal> val = nullptr;
                        for (auto& c : system->constraints)
                        {
                            if (c.decl != valueVar->declRef.GetDecl())
                                continue;

                            auto cVal = c.val.As<IntVal>();
                            assert(cVal.Ptr());

                            if (!val)
                            {
                                val = cVal;
                            }
                            else
                            {
                                if (val->value != cVal->value)
                                {
                                    // failure!
                                    return nullptr;
                                }
                            }

                            c.satisfied = true;
                        }

                        if (!val)
                        {
                            // failure!
                            return nullptr;
                        }
                        solvedArgs.Add(val);
                    }
                    else
                    {
                        // ignore anything that isn't a generic parameter
                    }
                }

                // Make sure we haven't constructed any spurious constraints
                // that we aren't able to satisfy:
                for (auto c : system->constraints)
                {
                    if (!c.satisfied)
                    {
                        return nullptr;
                    }
                }

                RefPtr<Substitutions> newSubst = new Substitutions();
                newSubst->genericDecl = varSubst->genericDecl;
                newSubst->outer = varSubst->outer;
                newSubst->args = solvedArgs;
                return newSubst;

#endif
            }

            //

            struct OverloadCandidate
            {
                enum class Flavor
                {
                    Func,
                    Generic,
                    UnspecializedGeneric,
                };
                Flavor flavor;

                enum class Status
                {
                    GenericArgumentInferenceFailed,
                    Unchecked,
                    ArityChecked,
                    TypeChecked,
                    Appicable,
                };
                Status status = Status::Unchecked;

                // Reference to the declaration being applied
                LookupResultItem item;

                // The type of the result expression if this candidate is selected
                RefPtr<ExpressionType>	resultType;

                // A system for tracking constraints introduced on generic parameters
                ConstraintSystem constraintSystem;

                // How much conversion cost should be considered for this overload,
                // when ranking candidates.
                ConversionCost conversionCostSum = kConversionCost_None;
            };



            // State related to overload resolution for a call
            // to an overloaded symbol
            struct OverloadResolveContext
            {
                enum class Mode
                {
                    // We are just checking if a candidate works or not
                    JustTrying,

                    // We want to actually update the AST for a chosen candidate
                    ForReal,
                };

                RefPtr<AppExprBase> appExpr;
                RefPtr<ExpressionSyntaxNode> baseExpr;

                // Are we still trying out candidates, or are we
                // checking the chosen one for real?
                Mode mode = Mode::JustTrying;

                // We store one candidate directly, so that we don't
                // need to do dynamic allocation on the list every time
                OverloadCandidate bestCandidateStorage;
                OverloadCandidate*	bestCandidate = nullptr;

                // Full list of all candidates being considered, in the ambiguous case
                List<OverloadCandidate> bestCandidates;
            };

            struct ParamCounts
            {
                int required;
                int allowed;
            };

            // count the number of parameters required/allowed for a callable
            ParamCounts CountParameters(FilteredMemberRefList<ParamDeclRef> params)
            {
                ParamCounts counts = { 0, 0 };
                for (auto param : params)
                {
                    counts.allowed++;

                    // No initializer means no default value
                    //
                    // TODO(tfoley): The logic here is currently broken in two ways:
                    //
                    // 1. We are assuming that once one parameter has a default, then all do.
                    //    This can/should be validated earlier, so that we can assume it here.
                    //
                    // 2. We are not handling the possibility of multiple declarations for
                    //    a single function, where we'd need to merge default parameters across
                    //    all the declarations.
                    if (!param.GetDecl()->Expr)
                    {
                        counts.required++;
                    }
                }
                return counts;
            }

            // count the number of parameters required/allowed for a generic
            ParamCounts CountParameters(GenericDeclRef genericRef)
            {
                ParamCounts counts = { 0, 0 };
                for (auto m : genericRef.GetDecl()->Members)
                {
                    if (auto typeParam = m.As<GenericTypeParamDecl>())
                    {
                        counts.allowed++;
                        if (!typeParam->initType.Ptr())
                        {
                            counts.required++;
                        }
                    }
                    else if (auto valParam = m.As<GenericValueParamDecl>())
                    {
                        counts.allowed++;
                        if (!valParam->Expr)
                        {
                            counts.required++;
                        }
                    }
                }
                return counts;
            }

            bool TryCheckOverloadCandidateArity(
                OverloadResolveContext&		context,
                OverloadCandidate const&	candidate)
            {
                int argCount = context.appExpr->Arguments.Count();
                ParamCounts paramCounts = { 0, 0 };
                switch (candidate.flavor)
                {
                case OverloadCandidate::Flavor::Func:
                    paramCounts = CountParameters(candidate.item.declRef.As<FuncDeclBaseRef>().GetParameters());
                    break;

                case OverloadCandidate::Flavor::Generic:
                    paramCounts = CountParameters(candidate.item.declRef.As<GenericDeclRef>());
                    break;

                default:
                    assert(!"unexpected");
                    break;
                }

                if (argCount >= paramCounts.required && argCount <= paramCounts.allowed)
                    return true;

                // Emit an error message if we are checking this call for real
                if (context.mode != OverloadResolveContext::Mode::JustTrying)
                {
                    if (argCount < paramCounts.required)
                    {
                        getSink()->diagnose(context.appExpr, Diagnostics::unimplemented, "not enough arguments for call");
                    }
                    else
                    {
                        assert(argCount > paramCounts.allowed);
                        getSink()->diagnose(context.appExpr, Diagnostics::unimplemented, "too many arguments for call");
                    }
                }

                return false;
            }

            bool TryCheckGenericOverloadCandidateTypes(
                OverloadResolveContext&	context,
                OverloadCandidate&		candidate)
            {
                auto& args = context.appExpr->Arguments;

                auto genericDeclRef = candidate.item.declRef.As<GenericDeclRef>();

                int aa = 0;
                for (auto memberRef : genericDeclRef.GetMembers())
                {
                    if (auto typeParamRef = memberRef.As<GenericTypeParamDeclRef>())
                    {
                        auto arg = args[aa++];

                        if (context.mode == OverloadResolveContext::Mode::JustTrying)
                        {
                            if (!CanCoerceToProperType(TypeExp(arg)))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            TypeExp typeExp = CoerceToProperType(TypeExp(arg));
                        }
                    }
                    else if (auto valParamRef = memberRef.As<GenericValueParamDeclRef>())
                    {
                        auto arg = args[aa++];

                        if (context.mode == OverloadResolveContext::Mode::JustTrying)
                        {
                            ConversionCost cost = kConversionCost_None;
                            if (!CanCoerce(valParamRef.GetType(), arg->Type, &cost))
                            {
                                return false;
                            }
                            candidate.conversionCostSum += cost;
                        }
                        else
                        {
                            arg = Coerce(valParamRef.GetType(), arg);
                            auto val = ExtractGenericArgInteger(arg);
                        }
                    }
                    else
                    {
                        continue;
                    }
                }

                return true;
            }

            bool TryCheckOverloadCandidateTypes(
                OverloadResolveContext&	context,
                OverloadCandidate&		candidate)
            {
                auto& args = context.appExpr->Arguments;
                int argCount = args.Count();

                List<ParamDeclRef> params;
                switch (candidate.flavor)
                {
                case OverloadCandidate::Flavor::Func:
                    params = candidate.item.declRef.As<FuncDeclBaseRef>().GetParameters().ToArray();
                    break;

                case OverloadCandidate::Flavor::Generic:
                    return TryCheckGenericOverloadCandidateTypes(context, candidate);

                default:
                    assert(!"unexpected");
                    break;
                }

                // Note(tfoley): We might have fewer arguments than parameters in the
                // case where one or more parameters had defaults.
                assert(argCount <= params.Count());

                for (int ii = 0; ii < argCount; ++ii)
                {
                    auto& arg = args[ii];
                    auto param = params[ii];

                    if (context.mode == OverloadResolveContext::Mode::JustTrying)
                    {
                        ConversionCost cost = kConversionCost_None;
                        if (!CanCoerce(param.GetType(), arg->Type, &cost))
                        {
                            return false;
                        }
                        candidate.conversionCostSum += cost;
                    }
                    else
                    {
                        arg = Coerce(param.GetType(), arg);
                    }
                }
                return true;
            }

            bool TryCheckOverloadCandidateDirections(
                OverloadResolveContext&		/*context*/,
                OverloadCandidate const&	/*candidate*/)
            {
                // TODO(tfoley): check `in` and `out` markers, as needed.
                return true;
            }

            // Try to check an overload candidate, but bail out
            // if any step fails
            void TryCheckOverloadCandidate(
                OverloadResolveContext&		context,
                OverloadCandidate&			candidate)
            {
                if (!TryCheckOverloadCandidateArity(context, candidate))
                    return;

                candidate.status = OverloadCandidate::Status::ArityChecked;
                if (!TryCheckOverloadCandidateTypes(context, candidate))
                    return;

                candidate.status = OverloadCandidate::Status::TypeChecked;
                if (!TryCheckOverloadCandidateDirections(context, candidate))
                    return;

                candidate.status = OverloadCandidate::Status::Appicable;
            }

            // Create the representation of a given generic applied to some arguments
            RefPtr<ExpressionSyntaxNode> CreateGenericDeclRef(
                RefPtr<ExpressionSyntaxNode>	baseExpr,
                RefPtr<AppExprBase>				appExpr)
            {
                auto baseDeclRefExpr = baseExpr.As<DeclRefExpr>();
                if (!baseDeclRefExpr)
                {
                    assert(!"unexpected");
                    return CreateErrorExpr(appExpr.Ptr());
                }
                auto baseGenericRef = baseDeclRefExpr->declRef.As<GenericDeclRef>();
                if (!baseGenericRef)
                {
                    assert(!"unexpected");
                    return CreateErrorExpr(appExpr.Ptr());
                }

                RefPtr<Substitutions> subst = new Substitutions();
                subst->genericDecl = baseGenericRef.GetDecl();
                subst->outer = baseGenericRef.substitutions;

                for (auto arg : appExpr->Arguments)
                {
                    subst->args.Add(ExtractGenericArgVal(arg));
                }

                DeclRef innerDeclRef(baseGenericRef.GetInner(), subst);

                return ConstructDeclRefExpr(
                    innerDeclRef,
                    nullptr,
                    appExpr);
            }

            // Take an overload candidate that previously got through
            // `TryCheckOverloadCandidate` above, and try to finish
            // up the work and turn it into a real expression.
            //
            // If the candidate isn't actually applicable, this is
            // where we'd start reporting the issue(s).
            RefPtr<ExpressionSyntaxNode> CompleteOverloadCandidate(
                OverloadResolveContext&		context,
                OverloadCandidate&			candidate)
            {
                // special case for generic argument inference failure
                if (candidate.status == OverloadCandidate::Status::GenericArgumentInferenceFailed)
                {
                    String callString = GetCallSignatureString(context.appExpr);
                    getSink()->diagnose(
                        context.appExpr,
                        Diagnostics::genericArgumentInferenceFailed,
                        callString);

                    String declString = getDeclSignatureString(candidate.item);
                    getSink()->diagnose(candidate.item.declRef, Diagnostics::genericSignatureTried, declString);
                    goto error;
                }

                context.mode = OverloadResolveContext::Mode::ForReal;
                context.appExpr->Type = ExpressionType::Error;

                if (!TryCheckOverloadCandidateArity(context, candidate))
                    goto error;

                if (!TryCheckOverloadCandidateTypes(context, candidate))
                    goto error;

                if (!TryCheckOverloadCandidateDirections(context, candidate))
                    goto error;

                {
                    auto baseExpr = ConstructLookupResultExpr(
                        candidate.item, context.baseExpr, context.appExpr->FunctionExpr);

                    switch(candidate.flavor)
                    {
                    case OverloadCandidate::Flavor::Func:
                        context.appExpr->FunctionExpr = baseExpr;
                        context.appExpr->Type = candidate.resultType;
                        return context.appExpr;
                        break;

                    case OverloadCandidate::Flavor::Generic:
                        return CreateGenericDeclRef(baseExpr, context.appExpr);
                        break;

                    default:
                        assert(!"unexpected");
                        break;
                    }
                }


            error:
                return CreateErrorExpr(context.appExpr.Ptr());
            }

            // Implement a comparison operation between overload candidates,
            // so that the better candidate compares as less-than the other
            int CompareOverloadCandidates(
                OverloadCandidate*	left,
                OverloadCandidate*	right)
            {
                // If one candidate got further along in validation, pick it
                if (left->status != right->status)
                    return int(right->status) - int(left->status);

                // If one candidate is 
                if (left->conversionCostSum != right->conversionCostSum)
                    return left->conversionCostSum - right->conversionCostSum;


                return 0;
            }

            void AddOverloadCandidateInner(
                OverloadResolveContext& context,
                OverloadCandidate&		candidate)
            {
                // Filter our existing candidates, to remove any that are worse than our new one

                bool keepThisCandidate = true; // should this candidate be kept?

                if (context.bestCandidates.Count() != 0)
                {
                    // We have multiple candidates right now, so filter them.
                    bool anyFiltered = false;
                    // Note that we are querying the list length on every iteration,
                    // because we might remove things.
                    for (int cc = 0; cc < context.bestCandidates.Count(); ++cc)
                    {
                        int cmp = CompareOverloadCandidates(&candidate, &context.bestCandidates[cc]);
                        if (cmp < 0)
                        {
                            // our new candidate is better!

                            // remove it from the list (by swapping in a later one)
                            context.bestCandidates.FastRemoveAt(cc);
                            // and then reduce our index so that we re-visit the same index
                            --cc;

                            anyFiltered = true;
                        }
                        else if(cmp > 0)
                        {
                            // our candidate is worse!
                            keepThisCandidate = false;
                        }
                    }
                    // It should not be possible that we removed some existing candidate *and*
                    // chose not to keep this candidate (otherwise the better-ness relation
                    // isn't transitive). Therefore we confirm that we either chose to keep
                    // this candidate (in which case filtering is okay), or we didn't filter
                    // anything.
                    assert(keepThisCandidate || !anyFiltered);
                }
                else if(context.bestCandidate)
                {
                    // There's only one candidate so far
                    int cmp = CompareOverloadCandidates(&candidate, context.bestCandidate);
                    if(cmp < 0)
                    {
                        // our new candidate is better!
                        context.bestCandidate = nullptr;
                    }
                    else if (cmp > 0)
                    {
                        // our candidate is worse!
                        keepThisCandidate = false;
                    }
                }

                // If our candidate isn't good enough, then drop it
                if (!keepThisCandidate)
                    return;

                // Otherwise we want to keep the candidate
                if (context.bestCandidates.Count() > 0)
                {
                    // There were already multiple candidates, and we are adding one more
                    context.bestCandidates.Add(candidate);
                }
                else if (context.bestCandidate)
                {
                    // There was a unique best candidate, but now we are ambiguous
                    context.bestCandidates.Add(*context.bestCandidate);
                    context.bestCandidates.Add(candidate);
                    context.bestCandidate = nullptr;
                }
                else
                {
                    // This is the only candidate worthe keeping track of right now
                    context.bestCandidateStorage = candidate;
                    context.bestCandidate = &context.bestCandidateStorage;
                }
            }

            void AddOverloadCandidate(
                OverloadResolveContext& context,
                OverloadCandidate&		candidate)
            {
                // Try the candidate out, to see if it is applicable at all.
                TryCheckOverloadCandidate(context, candidate);

                // Now (potentially) add it to the set of candidate overloads to consider.
                AddOverloadCandidateInner(context, candidate);
            }

            void AddFuncOverloadCandidate(
                LookupResultItem			item,
                FuncDeclBaseRef				funcDeclRef,
                OverloadResolveContext&		context)
            {
                EnsureDecl(funcDeclRef.GetDecl());

                OverloadCandidate candidate;
                candidate.flavor = OverloadCandidate::Flavor::Func;
                candidate.item = item;
                candidate.resultType = funcDeclRef.GetResultType();

                AddOverloadCandidate(context, candidate);
            }

            void AddFuncOverloadCandidate(
                RefPtr<FuncType>		/*funcType*/,
                OverloadResolveContext&	/*context*/)
            {
#if 0
                if (funcType->decl)
                {
                    AddFuncOverloadCandidate(funcType->decl, context);
                }
                else if (funcType->Func)
                {
                    AddFuncOverloadCandidate(funcType->Func->SyntaxNode, context);
                }
                else if (funcType->Component)
                {
                    AddComponentFuncOverloadCandidate(funcType->Component, context);
                }
#else
                throw "unimplemented";
#endif
            }

            void AddCtorOverloadCandidate(
                LookupResultItem		typeItem,
                RefPtr<ExpressionType>	type,
                ConstructorDeclRef		ctorDeclRef,
                OverloadResolveContext&	context)
            {
                EnsureDecl(ctorDeclRef.GetDecl());

                // `typeItem` refers to the type being constructed (the thing
                // that was applied as a function) so we need to construct
                // a `LookupResultItem` that refers to the constructor instead

                LookupResultItem ctorItem;
                ctorItem.declRef = ctorDeclRef;
                ctorItem.breadcrumbs = new LookupResultItem::Breadcrumb(LookupResultItem::Breadcrumb::Kind::Member, typeItem.declRef, typeItem.breadcrumbs);

                OverloadCandidate candidate;
                candidate.flavor = OverloadCandidate::Flavor::Func;
                candidate.item = ctorItem;
                candidate.resultType = type;

                AddOverloadCandidate(context, candidate);
            }

            // If the given declaration has generic parameters, then
            // return the corresponding `GenericDecl` that holds the
            // parameters, etc.
            GenericDecl* GetOuterGeneric(Decl* decl)
            {
                auto parentDecl = decl->ParentDecl;
                if (!parentDecl) return nullptr;
                auto parentGeneric = dynamic_cast<GenericDecl*>(parentDecl);
                return parentGeneric;
            }

            // Try to find a unification for two values
            bool TryUnifyVals(
                ConstraintSystem&	constraints,
                RefPtr<Val>			fst,
                RefPtr<Val>			snd)
            {
                // if both values are types, then unify types
                if (auto fstType = fst.As<ExpressionType>())
                {
                    if (auto sndType = snd.As<ExpressionType>())
                    {
                        return TryUnifyTypes(constraints, fstType, sndType);
                    }
                }

                // if both values are constant integers, then compare them
                if (auto fstIntVal = fst.As<ConstantIntVal>())
                {
                    if (auto sndIntVal = snd.As<ConstantIntVal>())
                    {
                        return fstIntVal->value == sndIntVal->value;
                    }
                }

                // Check if both are integer values in general
                if (auto fstInt = fst.As<IntVal>())
                {
                    if (auto sndInt = snd.As<IntVal>())
                    {
                        auto fstParam = fstInt.As<GenericParamIntVal>();
                        auto sndParam = sndInt.As<GenericParamIntVal>();

                        if (fstParam)
                            TryUnifyIntParam(constraints, fstParam->declRef.GetDecl(), sndInt);
                        if (sndParam)
                            TryUnifyIntParam(constraints, sndParam->declRef.GetDecl(), fstInt);

                        if (fstParam || sndParam)
                            return true;
                    }
                }

                throw "unimplemented";

                // default: fail
                return false;
            }

            bool TryUnifySubstitutions(
                ConstraintSystem&		constraints,
                RefPtr<Substitutions>	fst,
                RefPtr<Substitutions>	snd)
            {
                // They must both be NULL or non-NULL
                if (!fst || !snd)
                    return fst == snd;

                // They must be specializing the same generic
                if (fst->genericDecl != snd->genericDecl)
                    return false;

                // Their arguments must unify
                assert(fst->args.Count() == snd->args.Count());
                int argCount = fst->args.Count();
                for (int aa = 0; aa < argCount; ++aa)
                {
                    if (!TryUnifyVals(constraints, fst->args[aa], snd->args[aa]))
                        return false;
                }

                // Their "base" specializations must unify
                if (!TryUnifySubstitutions(constraints, fst->outer, snd->outer))
                    return false;

                return true;
            }

            bool TryUnifyTypeParam(
                ConstraintSystem&				constraints,
                RefPtr<GenericTypeParamDecl>	typeParamDecl,
                RefPtr<ExpressionType>			type)
            {
                // We want to constrain the given type parameter
                // to equal the given type.
                Constraint constraint;
                constraint.decl = typeParamDecl.Ptr();
                constraint.val = type;

                constraints.constraints.Add(constraint);

                return true;
            }

            bool TryUnifyIntParam(
                ConstraintSystem&               constraints,
                RefPtr<GenericValueParamDecl>	paramDecl,
                RefPtr<IntVal>                  val)
            {
                // We want to constrain the given parameter to equal the given value.
                Constraint constraint;
                constraint.decl = paramDecl.Ptr();
                constraint.val = val;

                constraints.constraints.Add(constraint);

                return true;
            }

            bool TryUnifyTypes(
                ConstraintSystem&	constraints,
                RefPtr<ExpressionType> fst,
                RefPtr<ExpressionType> snd)
            {
                if (fst->Equals(snd)) return true;

                if (auto fstErrorType = fst->As<ErrorType>())
                    return true;

                if (auto sndErrorType = snd->As<ErrorType>())
                    return true;

                if (auto fstDeclRefType = fst->As<DeclRefType>())
                {
                    auto fstDeclRef = fstDeclRefType->declRef;

                    if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(fstDeclRef.GetDecl()))
                        return TryUnifyTypeParam(constraints, typeParamDecl, snd);

                    if (auto sndDeclRefType = snd->As<DeclRefType>())
                    {
                        auto sndDeclRef = sndDeclRefType->declRef;

                        if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(sndDeclRef.GetDecl()))
                            return TryUnifyTypeParam(constraints, typeParamDecl, fst);

                        // can't be unified if they refer to differnt declarations.
                        if (fstDeclRef.GetDecl() != sndDeclRef.GetDecl()) return false;

                        // next we need to unify the substitutions applied
                        // to each decalration reference.
                        if (!TryUnifySubstitutions(
                            constraints,
                            fstDeclRef.substitutions,
                            sndDeclRef.substitutions))
                        {
                            return false;
                        }

                        return true;
                    }
                }

                if (auto sndDeclRefType = snd->As<DeclRefType>())
                {
                    auto sndDeclRef = sndDeclRefType->declRef;

                    if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(sndDeclRef.GetDecl()))
                        return TryUnifyTypeParam(constraints, typeParamDecl, fst);
                }

                throw "unimplemented";
                return false;
            }

            // Is the candidate extension declaration actually applicable to the given type
            ExtensionDeclRef ApplyExtensionToType(
                ExtensionDecl*			extDecl,
                RefPtr<ExpressionType>	type)
            {
                if (auto extGenericDecl = GetOuterGeneric(extDecl))
                {
                    ConstraintSystem constraints;

                    if (!TryUnifyTypes(constraints, extDecl->targetType, type))
                        return DeclRef().As<ExtensionDeclRef>();

                    auto constraintSubst = TrySolveConstraintSystem(&constraints, DeclRef(extGenericDecl, nullptr).As<GenericDeclRef>());
                    if (!constraintSubst)
                    {
                        return DeclRef().As<ExtensionDeclRef>();
                    }

                    // Consruct a reference to the extension with our constraint variables
                    // set as they were found by solving the constraint system.
                    ExtensionDeclRef extDeclRef = DeclRef(extDecl, constraintSubst).As<ExtensionDeclRef>();

                    // We expect/require that the result of unification is such that
                    // the target types are now equal
                    assert(extDeclRef.GetTargetType()->Equals(type));

                    return extDeclRef;
                }
                else
                {
                    // The easy case is when the extension isn't generic:
                    // either it applies to the type or not.
                    if (!type->Equals(extDecl->targetType))
                        return DeclRef().As<ExtensionDeclRef>();
                    return DeclRef(extDecl, nullptr).As<ExtensionDeclRef>();
                }
            }

            bool TryUnifyArgAndParamTypes(
                ConstraintSystem&				system,
                RefPtr<ExpressionSyntaxNode>	argExpr,
                ParamDeclRef					paramDeclRef)
            {
                // TODO(tfoley): potentially need a bit more
                // nuance in case where argument might be
                // an overload group...
                return TryUnifyTypes(system, argExpr->Type, paramDeclRef.GetType());
            }

            // Take a generic declaration and try to specialize its parameters
            // so that the resulting inner declaration can be applicable in
            // a particular context...
            DeclRef SpecializeGenericForOverload(
                GenericDeclRef			genericDeclRef,
                OverloadResolveContext&	context)
            {
                ConstraintSystem constraints;

                // Construct a reference to the inner declaration that has any generic
                // parameter substitutions in place already, but *not* any substutions
                // for the generic declaration we are currently trying to infer.
                auto innerDecl = genericDeclRef.GetInner();
                DeclRef unspecializedInnerRef = DeclRef(innerDecl, genericDeclRef.substitutions);

                // Check what type of declaration we are dealing with, and then try
                // to match it up with the arguments accordingly...
                if (auto funcDeclRef = unspecializedInnerRef.As<FuncDeclBaseRef>())
                {
                    auto& args = context.appExpr->Arguments;
                    auto params = funcDeclRef.GetParameters().ToArray();

                    int argCount = args.Count();
                    int paramCount = params.Count();

                    // Bail out on mismatch.
                    // TODO(tfoley): need more nuance here
                    if (argCount != paramCount)
                    {
                        return DeclRef(nullptr, nullptr);
                    }

                    for (int aa = 0; aa < argCount; ++aa)
                    {
                        if (!TryUnifyArgAndParamTypes(constraints, args[aa], params[aa]))
                            return DeclRef(nullptr, nullptr);
                    }
                }
                else
                {
                    // TODO(tfoley): any other cases needed here?
                    return DeclRef(nullptr, nullptr);
                }

                auto constraintSubst = TrySolveConstraintSystem(&constraints, genericDeclRef);
                if (!constraintSubst)
                {
                    // constraint solving failed
                    return DeclRef(nullptr, nullptr);
                }

                // We can now construct a reference to the inner declaration using
                // the solution to our constraints.
                return DeclRef(innerDecl, constraintSubst);
            }

            void AddAggTypeOverloadCandidates(
                LookupResultItem		typeItem,
                RefPtr<ExpressionType>	type,
                AggTypeDeclRef			aggTypeDeclRef,
                OverloadResolveContext&	context)
            {
                for (auto ctorDeclRef : aggTypeDeclRef.GetMembersOfType<ConstructorDeclRef>())
                {
                    // now work through this candidate...
                    AddCtorOverloadCandidate(typeItem, type, ctorDeclRef, context);
                }

                // Now walk through any extensions we can find for this types
                for (auto ext = aggTypeDeclRef.GetCandidateExtensions(); ext; ext = ext->nextCandidateExtension)
                {
                    auto extDeclRef = ApplyExtensionToType(ext, type);
                    if (!extDeclRef)
                        continue;

                    for (auto ctorDeclRef : extDeclRef.GetMembersOfType<ConstructorDeclRef>())
                    {
                        // TODO(tfoley): `typeItem` here should really reference the extension...

                        // now work through this candidate...
                        AddCtorOverloadCandidate(typeItem, type, ctorDeclRef, context);
                    }

                    // Also check for generic constructors
                    for (auto genericDeclRef : extDeclRef.GetMembersOfType<GenericDeclRef>())
                    {
                        if (auto ctorDecl = genericDeclRef.GetDecl()->inner.As<ConstructorDecl>())
                        {
                            DeclRef innerRef = SpecializeGenericForOverload(genericDeclRef, context);
                            if (!innerRef)
                                continue;

                            ConstructorDeclRef innerCtorRef = innerRef.As<ConstructorDeclRef>();

                            AddCtorOverloadCandidate(typeItem, type, innerCtorRef, context);

                            // TODO(tfoley): need a way to do the solving step for the constraint system
                        }
                    }
                }
            }

            void AddTypeOverloadCandidates(
                RefPtr<ExpressionType>	type,
                OverloadResolveContext&	context)
            {
                if (auto declRefType = type->As<DeclRefType>())
                {
                    if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDeclRef>())
                    {
                        AddAggTypeOverloadCandidates(LookupResultItem(aggTypeDeclRef), type, aggTypeDeclRef, context);
                    }
                }
            }

            void AddDeclRefOverloadCandidates(
                LookupResultItem		item,
                OverloadResolveContext&	context)
            {
                if (auto funcDeclRef = item.declRef.As<FuncDeclBaseRef>())
                {
                    AddFuncOverloadCandidate(item, funcDeclRef, context);
                }
                else if (auto aggTypeDeclRef = item.declRef.As<AggTypeDeclRef>())
                {
                    auto type = DeclRefType::Create(aggTypeDeclRef);
                    AddAggTypeOverloadCandidates(item, type, aggTypeDeclRef, context);
                }
                else if (auto genericDeclRef = item.declRef.As<GenericDeclRef>())
                {
                    // Try to infer generic arguments, based on the context
                    DeclRef innerRef = SpecializeGenericForOverload(genericDeclRef, context);

                    if (innerRef)
                    {
                        // If inference works, then we've now got a
                        // specialized declaration reference we can apply.

                        LookupResultItem innerItem;
                        innerItem.breadcrumbs = item.breadcrumbs;
                        innerItem.declRef = innerRef;

                        AddDeclRefOverloadCandidates(innerItem, context);
                    }
                    else
                    {
                        // If inference failed, then we need to create
                        // a candidate that can be used to reflect that fact
                        // (so we can report a good error)
                        OverloadCandidate candidate;
                        candidate.item = item;
                        candidate.flavor = OverloadCandidate::Flavor::UnspecializedGeneric;
                        candidate.status = OverloadCandidate::Status::GenericArgumentInferenceFailed;

                        AddOverloadCandidateInner(context, candidate);
                    }
                }
                else
                {
                    // TODO(tfoley): any other cases needed here?
                }
            }

            void AddOverloadCandidates(
                RefPtr<ExpressionSyntaxNode>	funcExpr,
                OverloadResolveContext&			context)
            {
                auto funcExprType = funcExpr->Type;

                if (auto typeType = funcExprType->As<TypeExpressionType>())
                {
                    // The expression named a type, so we have a constructor call
                    // on our hands.
                    AddTypeOverloadCandidates(typeType->type, context);
                }

                // Note(tfoley): we aren't using `else` here, so that
                // we can catch generic declarations in this case:
                if (auto funcDeclRefExpr = funcExpr.As<DeclRefExpr>())
                {
                    // The expression referenced a function declaration
                    AddDeclRefOverloadCandidates(LookupResultItem(funcDeclRefExpr->declRef), context);
                }
                else if (auto funcType = funcExprType->As<FuncType>())
                {
                    // TODO(tfoley): deprecate this path...
                    AddFuncOverloadCandidate(funcType, context);
                }
                else if (auto overloadedExpr = funcExpr.As<OverloadedExpr>())
                {
                    auto lookupResult = overloadedExpr->lookupResult2;
                    assert(lookupResult.isOverloaded());
                    for(auto item : lookupResult.items)
                    {
                        AddDeclRefOverloadCandidates(item, context);
                    }
                }
            }

            void formatType(StringBuilder& sb, RefPtr<ExpressionType> type)
            {
                sb << type->ToString();
            }

            void formatVal(StringBuilder& sb, RefPtr<Val> val)
            {
                sb << val->ToString();
            }

            void formatDeclPath(StringBuilder& sb, DeclRef declRef)
            {
                // Find the parent declaration
                auto parentDeclRef = declRef.GetParent();

                // If the immediate parent is a generic, then we probably
                // want the declaration above that...
                auto parentGenericDeclRef = parentDeclRef.As<GenericDeclRef>();
                if(parentGenericDeclRef)
                {
                    parentDeclRef = parentGenericDeclRef.GetParent();
                }

                // Depending on what the parent is, we may want to format things specially
                if(auto aggTypeDeclRef = parentDeclRef.As<AggTypeDeclRef>())
                {
                    formatDeclPath(sb, aggTypeDeclRef);
                    sb << ".";
                }

                sb << declRef.GetName();

                // If the parent declaration is a generic, then we need to print out its
                // signature
                if( parentGenericDeclRef )
                {
                    assert(declRef.substitutions);
                    assert(declRef.substitutions->genericDecl == parentGenericDeclRef.GetDecl());

                    sb << "<";
                    bool first = true;
                    for(auto arg : declRef.substitutions->args)
                    {
                        if(!first) sb << ", ";
                        formatVal(sb, arg);
                        first = false;
                    }
                    sb << ">";
                }
            }

            void formatDeclParams(StringBuilder& sb, DeclRef declRef)
            {
                if (auto funcDeclRef = declRef.As<FuncDeclBaseRef>())
                {

                    // This is something callable, so we need to also print parameter types for overloading
                    sb << "(";

                    bool first = true;
                    for (auto paramDeclRef : funcDeclRef.GetParameters())
                    {
                        if (!first) sb << ", ";

                        formatType(sb, paramDeclRef.GetType());

                        first = false;

                    }

                    sb << ")";
                }
                else if(auto genericDeclRef = declRef.As<GenericDeclRef>())
                {
                    sb << "<";
                    bool first = true;
                    for (auto paramDeclRef : genericDeclRef.GetMembers())
                    {
                        if(auto genericTypeParam = paramDeclRef.As<GenericTypeParamDeclRef>())
                        {
                            if (!first) sb << ", ";
                            first = false;

                            sb << genericTypeParam.GetName();
                        }
                        else if(auto genericValParam = paramDeclRef.As<GenericValueParamDeclRef>())
                        {
                            if (!first) sb << ", ";
                            first = false;

                            formatType(sb, genericValParam.GetType());
                            sb << " ";
                            sb << genericValParam.GetName();
                        }
                        else
                        {}
                    }
                    sb << ">";

                    formatDeclParams(sb, DeclRef(genericDeclRef.GetInner(), genericDeclRef.substitutions));
                }
                else
                {
                }
            }

            void formatDeclSignature(StringBuilder& sb, DeclRef declRef)
            {
                formatDeclPath(sb, declRef);
                formatDeclParams(sb, declRef);
            }

            String getDeclSignatureString(DeclRef declRef)
            {
                StringBuilder sb;
                formatDeclSignature(sb, declRef);
                return sb.ProduceString();
            }

            String getDeclSignatureString(LookupResultItem item)
            {
                return getDeclSignatureString(item.declRef);
            }

            String GetCallSignatureString(RefPtr<AppExprBase> expr)
            {
                 StringBuilder argsListBuilder;
                argsListBuilder << "(";
                bool first = true;
                for (auto a : expr->Arguments)
                {
                    if (!first) argsListBuilder << ", ";
                    argsListBuilder << a->Type->ToString();
                    first = false;
                }
                argsListBuilder << ")";
                return argsListBuilder.ProduceString();
            }


            RefPtr<ExpressionSyntaxNode> ResolveInvoke(InvokeExpressionSyntaxNode * expr)
            {
                // Look at the base expression for the call, and figure out how to invoke it.
                auto funcExpr = expr->FunctionExpr;
                auto funcExprType = funcExpr->Type;

                // If we are trying to apply an erroroneous expression, then just bail out now.
                if(IsErrorExpr(funcExpr))
                {
                    return CreateErrorExpr(expr);
                }

                OverloadResolveContext context;
                context.appExpr = expr;
                if (auto funcMemberExpr = funcExpr.As<MemberExpressionSyntaxNode>())
                {
                    context.baseExpr = funcMemberExpr->BaseExpression;
                }
                else if (auto funcOverloadExpr = funcExpr.As<OverloadedExpr>())
                {
                    context.baseExpr = funcOverloadExpr->base;
                }
                AddOverloadCandidates(funcExpr, context);

                if (context.bestCandidates.Count() > 0)
                {
                    // Things were ambiguous.

                    // It might be that things were only ambiguous because
                    // one of the argument expressions had an error, and
                    // so a bunch of candidates could match at that position.
                    //
                    // If any argument was an error, we skip out on printing
                    // another message, to avoid cascading errors.
                    for (auto arg : expr->Arguments)
                    {
                        if (IsErrorExpr(arg))
                        {
                            return CreateErrorExpr(expr);
                        }
                    }

                    String funcName;
                    if (auto baseVar = funcExpr.As<VarExpressionSyntaxNode>())
                        funcName = baseVar->Variable;
                    else if(auto baseMemberRef = funcExpr.As<MemberExpressionSyntaxNode>())
                        funcName = baseMemberRef->MemberName;

                    String argsList = GetCallSignatureString(expr);

                    if (context.bestCandidates[0].status != OverloadCandidate::Status::Appicable)
                    {
                        // There were multple equally-good candidates, but none actually usable.
                        // We will construct a diagnostic message to help out.
                        if (funcName.Length() != 0)
                        {
                            getSink()->diagnose(expr, Diagnostics::noApplicableOverloadForNameWithArgs, funcName, argsList);
                        }
                        else
                        {
                            getSink()->diagnose(expr, Diagnostics::noApplicableWithArgs, argsList);
                        }
                    }
                    else
                    {
                        // There were multiple applicable candidates, so we need to report them.

                        if (funcName.Length() != 0)
                        {
                            getSink()->diagnose(expr, Diagnostics::ambiguousOverloadForNameWithArgs, funcName, argsList);
                        }
                        else
                        {
                            getSink()->diagnose(expr, Diagnostics::ambiguousOverloadWithArgs, argsList);
                        }
                    }

                    int candidateCount = context.bestCandidates.Count();
                    int maxCandidatesToPrint = 10; // don't show too many candidates at once...
                    int candidateIndex = 0;
                    for (auto candidate : context.bestCandidates)
                    {
                        String declString = getDeclSignatureString(candidate.item);
                        getSink()->diagnose(candidate.item.declRef, Diagnostics::overloadCandidate, declString);

                        candidateIndex++;
                        if (candidateIndex == maxCandidatesToPrint)
                            break;
                    }
                    if (candidateIndex != candidateCount)
                    {
                        getSink()->diagnose(expr, Diagnostics::moreOverloadCandidates, candidateCount - candidateIndex);
                    }

                    return CreateErrorExpr(expr);
                }
                else if (context.bestCandidate)
                {
                    // There was one best candidate, even if it might not have been
                    // applicable in the end.
                    // We will report errors for this one candidate, then, to give
                    // the user the most help we can.
                    return CompleteOverloadCandidate(context, *context.bestCandidate);
                }
                else
                {
                    // Nothing at all was found that we could even consider invoking
                    getSink()->diagnose(expr->FunctionExpr, Diagnostics::expectedFunction);
                    expr->Type = ExpressionType::Error;
                    return expr;
                }
            }

            void AddGenericOverloadCandidate(
                LookupResultItem		baseItem,
                OverloadResolveContext&	context)
            {
                if (auto genericDeclRef = baseItem.declRef.As<GenericDeclRef>())
                {
                    EnsureDecl(genericDeclRef.GetDecl());

                    OverloadCandidate candidate;
                    candidate.flavor = OverloadCandidate::Flavor::Generic;
                    candidate.item = baseItem;
                    candidate.resultType = nullptr;

                    AddOverloadCandidate(context, candidate);
                }
            }

            void AddGenericOverloadCandidates(
                RefPtr<ExpressionSyntaxNode>	baseExpr,
                OverloadResolveContext&			context)
            {
                if(auto baseDeclRefExpr = baseExpr.As<DeclRefExpr>())
                {
                    auto declRef = baseDeclRefExpr->declRef;
                    AddGenericOverloadCandidate(LookupResultItem(declRef), context);
                }
                else if (auto overloadedExpr = baseExpr.As<OverloadedExpr>())
                {
                    // We are referring to a bunch of declarations, each of which might be generic
                    LookupResult result;
                    for (auto item : overloadedExpr->lookupResult2.items)
                    {
                        AddGenericOverloadCandidate(item, context);
                    }
                }
                else
                {
                    // any other cases?
                }
            }

            RefPtr<ExpressionSyntaxNode> VisitGenericApp(GenericAppExpr * genericAppExpr) override
            {
                // We are applying a generic to arguments, but there might be multiple generic
                // declarations with the same name, so this becomes a specialized case of
                // overload resolution.


                // Start by checking the base expression and arguments.
                auto& baseExpr = genericAppExpr->FunctionExpr;
                baseExpr = CheckTerm(baseExpr);
                auto& args = genericAppExpr->Arguments;
                for (auto& arg : args)
                {
                    arg = CheckTerm(arg);
                }

                // If there was an error in the base expression,  or in any of
                // the arguments, then just bail.
                if (IsErrorExpr(baseExpr))
                {
                    return CreateErrorExpr(genericAppExpr);
                }
                for (auto argExpr : args)
                {
                    if (IsErrorExpr(argExpr))
                    {
                        return CreateErrorExpr(genericAppExpr);
                    }
                }

                // Otherwise, let's start looking at how to find an overload...

                OverloadResolveContext context;
                context.appExpr = genericAppExpr;
                context.baseExpr = GetBaseExpr(baseExpr);

                AddGenericOverloadCandidates(baseExpr, context);

                if (context.bestCandidates.Count() > 0)
                {
                    // Things were ambiguous.
                    if (context.bestCandidates[0].status != OverloadCandidate::Status::Appicable)
                    {
                        // There were multple equally-good candidates, but none actually usable.
                        // We will construct a diagnostic message to help out.

                        // TODO(tfoley): print a reasonable message here...

                        getSink()->diagnose(genericAppExpr, Diagnostics::unimplemented, "no applicable generic");

                        return CreateErrorExpr(genericAppExpr);
                    }
                    else
                    {
                        // There were multiple viable candidates, but that isn't an error: we just need
                        // to complete all of them and create an overloaded expression as a result.

                        LookupResult result;
                        for (auto candidate : context.bestCandidates)
                        {
                            auto candidateExpr = CompleteOverloadCandidate(context, candidate);
                        }

                        throw "what now?";
//                        auto overloadedExpr = new OverloadedExpr();
//                        return overloadedExpr;
                    }
                }
                else if (context.bestCandidate)
                {
                    // There was one best candidate, even if it might not have been
                    // applicable in the end.
                    // We will report errors for this one candidate, then, to give
                    // the user the most help we can.
                    return CompleteOverloadCandidate(context, *context.bestCandidate);
                }
                else
                {
                    // Nothing at all was found that we could even consider invoking
                    getSink()->diagnose(genericAppExpr, Diagnostics::unimplemented, "expected a generic");
                    return CreateErrorExpr(genericAppExpr);
                }


#if TIMREMOVED

                if (IsErrorExpr(base))
                {
                    return CreateErrorExpr(typeNode);
                }
                else if(auto baseDeclRefExpr = base.As<DeclRefExpr>())
                {
                    auto declRef = baseDeclRefExpr->declRef;

                    if (auto genericDeclRef = declRef.As<GenericDeclRef>())
                    {
                        int argCount = typeNode->Args.Count();
                        int argIndex = 0;
                        for (RefPtr<Decl> member : genericDeclRef.GetDecl()->Members)
                        {
                            if (auto typeParam = member.As<GenericTypeParamDecl>())
                            {
                                if (argIndex == argCount)
                                {
                                    // Too few arguments!

                                }

                                // TODO: checking!
                            }
                            else if (auto valParam = member.As<GenericValueParamDecl>())
                            {
                                // TODO: checking
                            }
                            else
                            {

                            }
                        }
                        if (argIndex != argCount)
                        {
                            // Too many arguments!
                        }

                        // Now instantiate the declaration given those arguments
                        auto type = InstantiateGenericType(genericDeclRef, args);
                        typeResult = type;
                        typeNode->Type = new TypeExpressionType(type);
                        return typeNode;
                    }
                }
                else if (auto overloadedExpr = base.As<OverloadedExpr>())
                {
                    // We are referring to a bunch of declarations, each of which might be generic
                    LookupResult result;
                    for (auto item : overloadedExpr->lookupResult2.items)
                    {
                        auto applied = TryApplyGeneric(item, typeNode);
                        if (!applied)
                            continue;

                        AddToLookupResult(result, appliedItem);
                    }
                }

                // TODO: correct diagnostic here!
                getSink()->diagnose(typeNode, Diagnostics::expectedAGeneric, base->Type);
                return CreateErrorExpr(typeNode);
#endif
            }

            RefPtr<ExpressionSyntaxNode> VisitSharedTypeExpr(SharedTypeExpr* expr) override
            {
                if (!expr->Type.Ptr())
                {
                    expr->base = CheckProperType(expr->base);
                    expr->Type = expr->base.exp->Type;
                }
                return expr;
            }




            RefPtr<ExpressionSyntaxNode> CheckExpr(RefPtr<ExpressionSyntaxNode> expr)
            {
                return expr->Accept(this).As<ExpressionSyntaxNode>();
            }

            RefPtr<ExpressionSyntaxNode> CheckInvokeExprWithCheckedOperands(InvokeExpressionSyntaxNode *expr)
            {

                auto rs = ResolveInvoke(expr);
                if (auto invoke = dynamic_cast<InvokeExpressionSyntaxNode*>(rs.Ptr()))
                {
                    // if this is still an invoke expression, test arguments passed to inout/out parameter are LValues
                    if(auto funcType = invoke->FunctionExpr->Type->As<FuncType>())
                    {
                        List<RefPtr<ParameterSyntaxNode>> paramsStorage;
                        List<RefPtr<ParameterSyntaxNode>> * params = nullptr;
                        if (auto func = funcType->declRef.GetDecl())
                        {
                            paramsStorage = func->GetParameters().ToArray();
                            params = &paramsStorage;
                        }
                        if (params)
                        {
                            for (int i = 0; i < (*params).Count(); i++)
                            {
                                if ((*params)[i]->HasModifier<OutModifier>())
                                {
                                    if (i < expr->Arguments.Count() && expr->Arguments[i]->Type->AsBasicType() &&
                                        !expr->Arguments[i]->Type.IsLeftValue)
                                    {
                                        getSink()->diagnose(expr->Arguments[i], Diagnostics::argumentExpectedLValue, (*params)[i]->Name);
                                    }
                                }
                            }
                        }
                    }
                }
                return rs;
            }

            virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode *expr) override
            {
                // check the base expression first
                expr->FunctionExpr = CheckExpr(expr->FunctionExpr);

                // Next check the argument expressions
                for (auto & arg : expr->Arguments)
                {
                    arg = CheckExpr(arg);
                }

                return CheckInvokeExprWithCheckedOperands(expr);
            }


            bool DeclPassesLookupMask(Decl* decl, LookupMask mask)
            {
                // type declarations
                if(auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(decl))
                {
                    return int(mask) & int(LookupMask::Type);
                }
                else if(auto simpleTypeDecl = dynamic_cast<SimpleTypeDecl*>(decl))
                {
                    return int(mask) & int(LookupMask::Type);
                }
                // function declarations
                else if(auto funcDecl = dynamic_cast<FunctionDeclBase*>(decl))
                {
                    return (int(mask) & int(LookupMask::Function)) != 0;
                }

                // default behavior is to assume a value declaration
                // (no overloading allowed)

                return (int(mask) & int(LookupMask::Value)) != 0;
            }

            void AddToLookupResult(
                LookupResult&		result,
                LookupResultItem	item)
            {
                if (!result.isValid())
                {
                    // If we hadn't found a hit before, we have one now
                    result.item = item;
                }
                else if (!result.isOverloaded())
                {
                    // We are about to make this overloaded
                    result.items.Add(result.item);
                    result.items.Add(item);
                }
                else
                {
                    // The result was already overloaded, so we pile on
                    result.items.Add(item);
                }
            }

            // Helper for constructing breadcrumb trails during lookup, without
            // any heap allocation
            struct BreadcrumbInfo
            {
                LookupResultItem::Breadcrumb::Kind kind;
                DeclRef declRef;
                BreadcrumbInfo* prev = nullptr;
            };

            LookupResultItem CreateLookupResultItem(
                DeclRef declRef,
                BreadcrumbInfo* breadcrumbInfos)
            {
                LookupResultItem item;
                item.declRef = declRef;

                // breadcrumbs were constructed "backwards" on the stack, so we
                // reverse them here by building a linked list the other way
                RefPtr<LookupResultItem::Breadcrumb> breadcrumbs;
                for (auto bb = breadcrumbInfos; bb; bb = bb->prev)
                {
                    breadcrumbs = new LookupResultItem::Breadcrumb(
                        bb->kind,
                        bb->declRef,
                        breadcrumbs);
                }
                item.breadcrumbs = breadcrumbs;
                return item;
            }

            // Look for members of the given name in the given container for declarations
            void DoLocalLookupImpl(
                String const&		name,
                ContainerDeclRef	containerDeclRef,
                LookupResult&		result,
                BreadcrumbInfo*		inBreadcrumbs)
            {
                ContainerDecl* containerDecl = containerDeclRef.GetDecl();

                // Ensure that the lookup dictionary in the container is up to date
                if (!containerDecl->memberDictionaryIsValid)
                {
                    BuildMemberDictionary(containerDecl);
                }

                // Look up the declarations with the chosen name in the container.
                Decl* firstDecl = nullptr;
                containerDecl->memberDictionary.TryGetValue(name, firstDecl);

                // Now iterate over those declarations (if any) and see if
                // we find any that meet our filtering criteria.
                // For example, we might be filtering so that we only consider
                // type declarations.
                for (auto m = firstDecl; m; m = m->nextInContainerWithSameName)
                {
                    if (!DeclPassesLookupMask(m, result.mask))
                        continue;

                    // The declaration passed the test, so add it!
                    AddToLookupResult(result, CreateLookupResultItem(DeclRef(m, containerDeclRef.substitutions), inBreadcrumbs));
                }


                // TODO(tfoley): should we look up in the transparent decls
                // if we already has a hit in the current container?

                for(auto transparentInfo : containerDecl->transparentMembers)
                {
                    // The reference to the transparent member should use whatever
                    // substitutions we used in referring to its outer container
                    DeclRef transparentMemberDeclRef(transparentInfo.decl, containerDeclRef.substitutions);

                    // We need to leave a breadcrumb so that we know that the result
                    // of lookup involves a member lookup step here

                    BreadcrumbInfo memberRefBreadcrumb;
                    memberRefBreadcrumb.kind = LookupResultItem::Breadcrumb::Kind::Member;
                    memberRefBreadcrumb.declRef = transparentMemberDeclRef;
                    memberRefBreadcrumb.prev = inBreadcrumbs;

                    DoMemberLookupImpl(name, transparentMemberDeclRef, result, &memberRefBreadcrumb);
                }

                // TODO(tfoley): need to consider lookup via extension here?
            }

            void DoMemberLookupImpl(
                String const&			name,
                RefPtr<ExpressionType>	baseType,
                LookupResult&			ioResult,
                BreadcrumbInfo*			breadcrumbs)
            {
                // If the type was pointer-like, then dereference it
                // automatically here.
                if (auto pointerLikeType = baseType->As<PointerLikeType>())
                {
                    // Need to leave a breadcrumb to indicate that we
                    // did an implicit dereference here
                    BreadcrumbInfo derefBreacrumb;
                    derefBreacrumb.kind = LookupResultItem::Breadcrumb::Kind::Deref;
                    derefBreacrumb.prev = breadcrumbs;

                    // Recursively perform lookup on the result of deref
                    return DoMemberLookupImpl(name, pointerLikeType->elementType, ioResult, &derefBreacrumb);
                }

                // Default case: no dereference needed

                if (auto baseDeclRefType = baseType->As<DeclRefType>())
                {
                    if (auto baseAggTypeDeclRef = baseDeclRefType->declRef.As<AggTypeDeclRef>())
                    {
                        DoLocalLookupImpl(name, baseAggTypeDeclRef, ioResult, breadcrumbs);
                    }
                }

                // TODO(tfoley): any other cases to handle here?
            }

            void DoMemberLookupImpl(
                String const&	name,
                DeclRef			baseDeclRef,
                LookupResult&	ioResult,
                BreadcrumbInfo*	breadcrumbs)
            {
                auto baseType = GetTypeForDeclRef(baseDeclRef);
                return DoMemberLookupImpl(name, baseType, ioResult, breadcrumbs);
            }

            void DoLookupImpl(String const& name, LookupResult& result)
            {
                ContainerDecl* scope = result.scope;
                ContainerDecl* endScope = result.endScope;
                for (;scope != endScope; scope = scope->ParentDecl)
                {
                    ContainerDeclRef scopeRef = DeclRef(scope, nullptr).As<ContainerDeclRef>();
                    DoLocalLookupImpl(name, scopeRef, result, nullptr);

                    if (result.isValid())
                    {
                        // If we've found a result in this scope, then there
                        // is no reason to look further up (for now).
                        return;
                    }

#if 0


                    auto memberCount = scope->Members.Count();

                    for (;index < memberCount; index++)
                    {
                        auto member = scope->Members[index].Ptr();
                        if (member->Name.Content != name)
                            continue;

                        // TODO: filter based on our mask
                        if (!DeclPassesLookupMask(member, result.mask))
                            continue;

                        // if we had previously found a result
                        if (result.isValid())
                        {
                            // we now have a potentially overloaded result,
                            // and can return with this knowledge
                            result.flags |= LookupResult::Flags::Overloaded;
                            return result;
                        }
                        else
                        {
                            // this is the first result!
                            result.decl = member;
                            result.scope = scope;
                            result.index = index;

                            // TODO: need to establish a mask for subsequent
                            // lookup...
                        }
                    }

                    // we reached the end of a scope, so if we found
                    // anything inside that scope, we should return now
                    // rather than continue to search parent scopes
                    if (result.isValid())
                    {
                        return result;
                    }

                    // Otherwise, we proceed to the next scope up.
                    scope = scope->ParentDecl;
                    index = 0;
#endif
                }

                // If we run out of scopes, then we are done.
            }

            LookupResult DoLookup(String const& name, LookupResult inResult)
            {
                LookupResult result;
                result.mask = inResult.mask;
                result.endScope = inResult.endScope;
                DoLookupImpl(name, result);
                return result;
            }

            LookupResult LookUp(String const& name, ContainerDecl* scope)
            {
                LookupResult result;
                result.scope = scope;
                DoLookupImpl(name, result);
                return result;
            }

            // perform lookup within the context of a particular container declaration,
            // and do *not* look further up the chain
            LookupResult LookUpLocal(String const& name, ContainerDecl* scope)
            {
                LookupResult result;
                result.scope = scope;
                result.endScope = scope->ParentDecl;
                return DoLookup(name, result);

            }

            LookupResult LookUpLocal(String const& name, ContainerDeclRef containerDeclRef)
            {
                LookupResult result;
                DoLocalLookupImpl(name, containerDeclRef, result, nullptr);
                return result;
            }

            virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode *expr) override
            {
                // If we've already resolved this expression, don't try again.
                if (expr->declRef)
                    return expr;

                expr->Type = ExpressionType::Error;

                auto lookupResult = LookUp(expr->Variable, expr->scope);
                if (lookupResult.isValid())
                {
                    // Found at least one declaration, but did we find many?
                    if (lookupResult.isOverloaded())
                    {
                        auto overloadedExpr = new OverloadedExpr();
                        overloadedExpr->Position = expr->Position;
                        overloadedExpr->Type = ExpressionType::Overloaded;
                        overloadedExpr->lookupResult2 = lookupResult;
                        return overloadedExpr;
                    }
                    else
                    {
                        // Only a single decl, that's good
                        return ConstructLookupResultExpr(lookupResult.item, nullptr, expr);
#if 0
                        auto declRef = lookupResult.declRef;
                        expr->declRef = declRef;
                        expr->Type = GetTypeForDeclRef(declRef);
                        return expr;
#endif
                    }
                }

                getSink()->diagnose(expr, Diagnostics::undefinedIdentifier2, expr->Variable);

                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
            {
                expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
                auto targetType = CheckProperType(expr->TargetType);
                expr->TargetType = targetType;

                // The way to perform casting depends on the types involved
                if (expr->Expression->Type->Equals(ExpressionType::Error.Ptr()))
                {
                    // If the expression being casted has an error type, then just silently succeed
                    expr->Type = targetType;
                    return expr;
                }
                else if (auto targetArithType = targetType->AsArithmeticType())
                {
                    if (auto exprArithType = expr->Expression->Type->AsArithmeticType())
                    {
                        // Both source and destination types are arithmetic, so we might
                        // have a valid cast
                        auto targetScalarType = targetArithType->GetScalarType();
                        auto exprScalarType = exprArithType->GetScalarType();

                        if (!IsNumeric(exprScalarType->BaseType)) goto fail;
                        if (!IsNumeric(targetScalarType->BaseType)) goto fail;

                        // TODO(tfoley): this checking is incomplete here, and could
                        // lead to downstream compilation failures
                        expr->Type = targetType;
                        return expr;
                    }
                }

            fail:
                // Default: in no other case succeds, then the cast failed and we emit a diagnostic.
                getSink()->diagnose(expr, Diagnostics::invalidTypeCast, expr->Expression->Type, targetType->ToString());
                expr->Type = ExpressionType::Error;
                return expr;
            }
#if TIMREMOVED
            virtual RefPtr<ExpressionSyntaxNode> VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
            {
                auto selectorExpr = expr->SelectorExpr;
                selectorExpr = CheckExpr(selectorExpr);
                selectorExpr = Coerce(ExpressionType::GetBool(), selectorExpr);
                expr->SelectorExpr = selectorExpr;

                // TODO(tfoley): We need a general purpose "join" on types for inferring
                // generic argument types for builtins/intrinsics, so this should really
                // be using the exact same logic...
                //
                expr->Expr0 = expr->Expr0->Accept(this).As<ExpressionSyntaxNode>();
                expr->Expr1 = expr->Expr1->Accept(this).As<ExpressionSyntaxNode>();
                if (!expr->Expr0->Type->Equals(expr->Expr1->Type.Ptr()))
                {
                    getSink()->diagnose(expr, Diagnostics::selectValuesTypeMismatch);
                }
                expr->Type = expr->Expr0->Type;
                return expr;
            }
#endif

            // Get the type to use when referencing a declaration
            QualType GetTypeForDeclRef(DeclRef declRef)
            {
                // We need to insert an appropriate type for the expression, based on
                // what we found.
                if (auto varDeclRef = declRef.As<VarDeclBaseRef>())
                {
                    QualType qualType;
                    qualType.type = varDeclRef.GetType();
                    qualType.IsLeftValue = true; // TODO(tfoley): allow explicit `const` or `let` variables
                    return qualType;
                }
                else if (auto typeAliasDeclRef = declRef.As<TypeDefDeclRef>())
                {
                    EnsureDecl(typeAliasDeclRef.GetDecl());
                    auto type = new NamedExpressionType(typeAliasDeclRef);
                    typeResult = type;
                    return new TypeExpressionType(type);
                }
                else if (auto aggTypeDeclRef = declRef.As<AggTypeDeclRef>())
                {
                    auto type = DeclRefType::Create(aggTypeDeclRef);
                    typeResult = type;
                    return new TypeExpressionType(type);
                }
                else if (auto simpleTypeDeclRef = declRef.As<SimpleTypeDeclRef>())
                {
                    auto type = DeclRefType::Create(simpleTypeDeclRef);
                    typeResult = type;
                    return new TypeExpressionType(type);
                }
                else if (auto genericDeclRef = declRef.As<GenericDeclRef>())
                {
                    auto type = new GenericDeclRefType(genericDeclRef);
                    typeResult = type;
                    return new TypeExpressionType(type);
                }
                else if (auto funcDeclRef = declRef.As<FuncDeclBaseRef>())
                {
                    auto type = new FuncType();
                    type->declRef = funcDeclRef;
                    return type;
                }

                getSink()->diagnose(declRef, Diagnostics::unimplemented, "cannot form reference to this kind of declaration");
                return ExpressionType::Error;
            }

            RefPtr<ExpressionSyntaxNode> MaybeDereference(RefPtr<ExpressionSyntaxNode> inExpr)
            {
                RefPtr<ExpressionSyntaxNode> expr = inExpr;
                for (;;)
                {
                    auto& type = expr->Type;
                    if (auto pointerLikeType = type->As<PointerLikeType>())
                    {
                        type = pointerLikeType->elementType;

                        auto derefExpr = new DerefExpr();
                        derefExpr->base = expr;
                        derefExpr->Type = pointerLikeType->elementType;

                        // TODO(tfoley): deal with l-value-ness here

                        expr = derefExpr;
                        continue;
                    }

                    // Default case: just use the expression as-is
                    return expr;
                }
            }

            RefPtr<ExpressionSyntaxNode> CheckSwizzleExpr(
                MemberExpressionSyntaxNode*	memberRefExpr,
                RefPtr<ExpressionType>		baseElementType,
                int							baseElementCount)
            {
                RefPtr<SwizzleExpr> swizExpr = new SwizzleExpr();
                swizExpr->Position = memberRefExpr->Position;
                swizExpr->base = memberRefExpr->BaseExpression;

                int limitElement = baseElementCount;

                int elementIndices[4];
                int elementCount = 0;

                bool elementUsed[4] = { false, false, false, false };
                bool anyDuplicates = false;
                bool anyError = false;

                for (int i = 0; i < memberRefExpr->MemberName.Length(); i++)
                {
                    auto ch = memberRefExpr->MemberName[i];
                    int elementIndex = -1;
                    switch (ch)
                    {
                    case 'x': case 'r': elementIndex = 0; break;
                    case 'y': case 'g': elementIndex = 1; break;
                    case 'z': case 'b': elementIndex = 2; break;
                    case 'w': case 'a': elementIndex = 3; break;
                    default:
                        // An invalid character in the swizzle is an error
                        getSink()->diagnose(swizExpr, Diagnostics::unimplemented, "invalid component name for swizzle");
                        anyError = true;
                        continue;
                    }

                    // TODO(tfoley): GLSL requires that all component names
                    // come from the same "family"...

                    // Make sure the index is in range for the source type
                    if (elementIndex >= limitElement)
                    {
                        getSink()->diagnose(swizExpr, Diagnostics::unimplemented, "swizzle component out of range for type");
                        anyError = true;
                        continue;
                    }

                    // Check if we've seen this index before
                    for (int ee = 0; ee < elementCount; ee++)
                    {
                        if (elementIndices[ee] == elementIndex)
                            anyDuplicates = true;
                    }

                    // add to our list...
                    elementIndices[elementCount++] = elementIndex;
                }

                for (int ee = 0; ee < elementCount; ++ee)
                {
                    swizExpr->elementIndices[ee] = elementIndices[ee];
                }
                swizExpr->elementCount = elementCount;

                if (anyError)
                {
                    swizExpr->Type = ExpressionType::Error;
                }
                else if (elementCount == 1)
                {
                    // single-component swizzle produces a scalar
                    //
                    // Note(tfoley): the official HLSL rules seem to be that it produces
                    // a one-component vector, which is then implicitly convertible to
                    // a scalar, but that seems like it just adds complexity.
                    swizExpr->Type = baseElementType;
                }
                else
                {
                    // TODO(tfoley): would be nice to "re-sugar" type
                    // here if the input type had a sugared name...
                    swizExpr->Type = new VectorExpressionType(
                        baseElementType,
                        new ConstantIntVal(elementCount));
                }

                // A swizzle can be used as an l-value as long as there
                // were no duplicates in the list of components
                swizExpr->Type.IsLeftValue = !anyDuplicates;

                return swizExpr;
            }

            RefPtr<ExpressionSyntaxNode> CheckSwizzleExpr(
                MemberExpressionSyntaxNode*	memberRefExpr,
                RefPtr<ExpressionType>		baseElementType,
                RefPtr<IntVal>				baseElementCount)
            {
                if (auto constantElementCount = baseElementCount.As<ConstantIntVal>())
                {
                    return CheckSwizzleExpr(memberRefExpr, baseElementType, constantElementCount->value);
                }
                else
                {
                    getSink()->diagnose(memberRefExpr, Diagnostics::unimplemented, "swizzle on vector of unknown size");
                    return CreateErrorExpr(memberRefExpr);
                }
            }


            virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
            {
                expr->BaseExpression = CheckExpr(expr->BaseExpression);

                expr->BaseExpression = MaybeDereference(expr->BaseExpression);

                auto & baseType = expr->BaseExpression->Type;

                // Note: Checking for vector types before declaration-reference types,
                // because vectors are also declaration reference types...
                if (auto baseVecType = baseType->AsVectorType())
                {
                    return CheckSwizzleExpr(
                        expr,
                        baseVecType->elementType,
                        baseVecType->elementCount);
                }
                else if(auto baseScalarType = baseType->AsBasicType())
                {
                    // Treat scalar like a 1-element vector when swizzling
                    return CheckSwizzleExpr(
                        expr,
                        baseScalarType,
                        1);
                }
                else if (auto declRefType = baseType->AsDeclRefType())
                {
                    if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDeclRef>())
                    {
                        // Checking of the type must be complete before we can reference its members safely
                        EnsureDecl(aggTypeDeclRef.GetDecl(), DeclCheckState::Checked);


                        LookupResult lookupResult = LookUpLocal(expr->MemberName, aggTypeDeclRef);
                        if (!lookupResult.isValid())
                        {
                            goto fail;
                        }

                        if (lookupResult.isOverloaded())
                        {
                            auto overloadedExpr = new OverloadedExpr();
                            overloadedExpr->Position = expr->Position;
                            overloadedExpr->Type = ExpressionType::Overloaded;
                            overloadedExpr->base = expr->BaseExpression;
                            overloadedExpr->lookupResult2 = lookupResult;
                            return overloadedExpr;
                        }

                        // default case: we have found something
                        return ConstructLookupResultExpr(lookupResult.item, expr->BaseExpression, expr);
#if 0
                        DeclRef memberDeclRef(lookupResult.decl, aggTypeDeclRef.substitutions);
                        return ConstructDeclRefExpr(memberDeclRef, expr->BaseExpression, expr);
#endif

#if 0


                        // TODO(tfoley): It is unfortunate that the lookup strategy
                        // here isn't unified with the ordinary `Scope` case.
                        // In particular, if we add support for "transparent" declarations,
                        // etc. here then we would need to add them in ordinary lookup
                        // as well.

                        Decl* memberDecl = nullptr; // The first declaration we found, if any
                        Decl* secondDecl = nullptr; // Another declaration with the same name, if any
                        for (auto m : aggTypeDeclRef.GetMembers())
                        {
                            if (m.GetName() != expr->MemberName)
                                continue;

                            if (!memberDecl)
                            {
                                memberDecl = m.GetDecl();
                            }
                            else
                            {
                                secondDecl = m.GetDecl();
                                break;
                            }
                        }

                        // If we didn't find any member, then we signal an error
                        if (!memberDecl)
                        {
                            expr->Type = ExpressionType::Error;
                            getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->MemberName, baseType);
                            return expr;
                        }

                        // If we found only a single member, then we are fine
                        if (!secondDecl)
                        {
                            // TODO: need to
                            DeclRef memberDeclRef(memberDecl, aggTypeDeclRef.substitutions);

                            expr->declRef = memberDeclRef;
                            expr->Type = GetTypeForDeclRef(memberDeclRef);

                            // When referencing a member variable, the result is an l-value
                            // if and only if the base expression was.
                            if (auto memberVarDecl = dynamic_cast<VarDeclBase*>(memberDecl))
                            {
                                expr->Type.IsLeftValue = expr->BaseExpression->Type.IsLeftValue;
                            }
                            return expr;
                        }

                        // We found multiple members with the same name, and need
                        // to resolve the embiguity at some point...
                        expr->Type = ExpressionType::Error;
                        getSink()->diagnose(expr, Diagnostics::unimplemented, "ambiguous member reference");
                        return expr;

#endif

#if 0

                        StructField* field = structDecl->FindField(expr->MemberName);
                        if (!field)
                        {
                            expr->Type = ExpressionType::Error;
                            getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->MemberName, baseType);
                        }
                        else
                            expr->Type = field->Type;

                        // A reference to a struct member is an l-value if the reference to the struct
                        // value was also an l-value.
                        expr->Type.IsLeftValue = expr->BaseExpression->Type.IsLeftValue;
                        return expr;
#endif
                    }

                    // catch-all
                fail:
                    getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->MemberName, baseType);
                    expr->Type = ExpressionType::Error;
                    return expr;
                }
                // All remaining cases assume we have a `BasicType`
                else if (!baseType->AsBasicType())
                    expr->Type = ExpressionType::Error;
                else
                    expr->Type = ExpressionType::Error;
                if (!baseType->Equals(ExpressionType::Error.Ptr()) &&
                    expr->Type->Equals(ExpressionType::Error.Ptr()))
                {
                    getSink()->diagnose(expr, Diagnostics::typeHasNoPublicMemberOfName, baseType, expr->MemberName);
                }
                return expr;
            }
            SemanticsVisitor & operator = (const SemanticsVisitor &) = delete;
        };

        SyntaxVisitor * CreateSemanticsVisitor(
            DiagnosticSink * err,
            CompileOptions const& options)
        {
            return new SemanticsVisitor(err, options);
        }

    }
}