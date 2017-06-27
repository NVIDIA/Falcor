#include "syntax-visitors.h"

#include "lookup.h"
#include "compiler.h"

#include <assert.h>

namespace Slang
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

    class SemanticsVisitor : public SyntaxVisitor
    {
//        ProgramSyntaxNode * program = nullptr;
        FunctionSyntaxNode * function = nullptr;

        CompileRequest* request = nullptr;
        TranslationUnitRequest* translationUnit = nullptr;

        // lexical outer statements
        List<StatementSyntaxNode*> outerStmts;

        // We need to track what has been `import`ed,
        // to avoid importing the same thing more than once
        //
        // TODO: a smarter approach might be to filter
        // out duplicate references during lookup.
        HashSet<ProgramSyntaxNode*> importedModules;

    public:
        SemanticsVisitor(
            DiagnosticSink * pErr,
            CompileRequest*         request,
            TranslationUnitRequest* translationUnit)
            : SyntaxVisitor(pErr)
            , request(request)
            , translationUnit(translationUnit)
        {
        }

        CompileRequest* getCompileRequest() { return request; }
        TranslationUnitRequest* getTranslationUnit() { return translationUnit; }

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
            if (auto typeType = typeRepr->Type->As<TypeType>())
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
            DeclRef<Decl>                     declRef,
            RefPtr<ExpressionSyntaxNode>    baseExpr,
            RefPtr<ExpressionSyntaxNode>    originalExpr)
        {
            if (baseExpr)
            {
                auto expr = new MemberExpressionSyntaxNode();
                expr->Position = originalExpr->Position;
                expr->BaseExpression = baseExpr;
                expr->name = declRef.GetName();
                expr->Type = GetTypeForDeclRef(declRef);
                expr->declRef = declRef;
                return expr;
            }
            else
            {
                auto expr = new VarExpressionSyntaxNode();
                expr->Position = originalExpr->Position;
                expr->name = declRef.GetName();
                expr->Type = GetTypeForDeclRef(declRef);
                expr->declRef = declRef;
                return expr;
            }
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
                    SLANG_UNREACHABLE("all cases handle");
                }
            }

            return ConstructDeclRefExpr(item.declRef, bb, originalExpr);
        }

        RefPtr<ExpressionSyntaxNode> createLookupResultExpr(
            LookupResult const&             lookupResult,
            RefPtr<ExpressionSyntaxNode>    baseExpr,
            RefPtr<ExpressionSyntaxNode>    originalExpr)
        {
            if (lookupResult.isOverloaded())
            {
                auto overloadedExpr = new OverloadedExpr();
                overloadedExpr->Position = originalExpr->Position;
                overloadedExpr->Type = ExpressionType::Overloaded;
                overloadedExpr->base = baseExpr;
                overloadedExpr->lookupResult2 = lookupResult;
                return overloadedExpr;
            }
            else
            {
                return ConstructLookupResultExpr(lookupResult.item, baseExpr, originalExpr);
            }
        }

        RefPtr<ExpressionSyntaxNode> ResolveOverloadedExpr(RefPtr<OverloadedExpr> overloadedExpr, LookupMask mask)
        {
            auto lookupResult = overloadedExpr->lookupResult2;
            assert(lookupResult.isValid() && lookupResult.isOverloaded());

            // Take the lookup result we had, and refine it based on what is expected in context.
            lookupResult = refineLookup(lookupResult, mask);

            if (!lookupResult.isValid())
            {
                // If we didn't find any symbols after filtering, then just
                // use the original and report errors that way
                return overloadedExpr;
            }

            if (lookupResult.isOverloaded())
            {
                // We had an ambiguity anyway, so report it.
                getSink()->diagnose(overloadedExpr, Diagnostics::ambiguousReference, lookupResult.items[0].declRef.GetName());

                for(auto item : lookupResult.items)
                {
                    String declString = getDeclSignatureString(item);
                    getSink()->diagnose(item.declRef, Diagnostics::overloadCandidate, declString);
                }

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

            if (auto typeType = expr->Type.type->As<TypeType>())
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
            if (auto typeType = typeRepr->Type->As<TypeType>())
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

            if (auto typeType = exp->Type->As<TypeType>())
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
            DeclRef<GenericDecl>								genericDeclRef,
            List<RefPtr<ExpressionSyntaxNode>> const&	args)
        {
            RefPtr<Substitutions> subst = new Substitutions();
            subst->genericDecl = genericDeclRef.getDecl();
            subst->outer = genericDeclRef.substitutions;

            for (auto argExpr : args)
            {
                subst->args.Add(ExtractGenericArgVal(argExpr));
            }

            DeclRef<Decl> innerDeclRef;
            innerDeclRef.decl = GetInner(genericDeclRef);
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
                for (RefPtr<Decl> member : genericDeclRef.getDecl()->Members)
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
            // No conversion at all
            kConversionCost_None = 0,

            // Conversions based on explicit sub-typing relationships are the cheapest
            //
            // TODO(tfoley): We will eventually need a discipline for ranking
            // when two up-casts are comparable.
            kConversionCost_CastToInterface = 50,

            // Conversion that is lossless and keeps the "kind" of the value the same
            kConversionCost_RankPromotion = 100,

            // Conversions that are lossless, but change "kind"
            kConversionCost_UnsignedToSignedPromotion = 200,

            // Conversion from signed->unsigned integer of same or greater size
            kConversionCost_SignedToUnsignedConversion = 300,

            // Cost of converting an integer to a floating-point type
            kConversionCost_IntegerToFloatConversion = 400,

            // Catch-all for conversions that should be discouraged
            // (i.e., that really shouldn't be made implicitly)
            //
            // TODO: make these conversions not be allowed implicitly in "Slang mode"
            kConversionCost_GeneralConversion = 900,

            // Additional conversion cost to add when promoting from a scalar to
            // a vector (this will be added to the cost, if any, of converting
            // the element type of the vector)
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
                CASE(UInt64, Unsigned, Int64);
                CASE(Float, Float, Int32);
                CASE(Void, Error, Error);

            #undef CASE

            default:
                break;
            }
            SLANG_UNREACHABLE("all cases handled");
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

            // Coercion from an initializer list is allowed for many types
            if( auto fromInitializerListExpr = fromExpr.As<InitializerListExpr>())
            {
                auto argCount = fromInitializerListExpr->args.Count();

                // In the case where we need to build a reuslt expression,
                // we will collect the new arguments here
                List<RefPtr<ExpressionSyntaxNode>> coercedArgs;

                if(auto toDeclRefType = toType->As<DeclRefType>())
                {
                    auto toTypeDeclRef = toDeclRefType->declRef;
                    if(auto toStructDeclRef = toTypeDeclRef.As<StructSyntaxNode>())
                    {
                        // Trying to initialize a `struct` type given an initializer list.
                        // We will go through the fields in order and try to match them
                        // up with initializer arguments.


                        int argIndex = 0;
                        for(auto fieldDeclRef : getMembersOfType<StructField>(toStructDeclRef))
                        {
                            if(argIndex >= argCount)
                            {
                                // We've consumed all the arguments, so we should stop
                                break;
                            }

                            auto arg = fromInitializerListExpr->args[argIndex++];

                            // 
                            RefPtr<ExpressionSyntaxNode> coercedArg;
                            ConversionCost argCost;

                            bool argResult = TryCoerceImpl(
                                GetType(fieldDeclRef),
                                outToExpr ? &coercedArg : nullptr,
                                arg->Type,
                                arg,
                                outCost ? &argCost : nullptr);

                            // No point in trying further if any argument fails
                            if(!argResult)
                                return false;

                            // TODO(tfoley): what to do with cost?
                            // This only matters if/when we allow an initializer list as an argument to
                            // an overloaded call.

                            if( outToExpr )
                            {
                                coercedArgs.Add(coercedArg);
                            }
                        }
                    }
                }
                else if(auto toArrayType = toType->As<ArrayExpressionType>())
                {
                    // TODO(tfoley): If we can compute the size of the array statically,
                    // then we want to check that there aren't too many initializers present

                    auto toElementType = toArrayType->BaseType;

                    for(auto& arg : fromInitializerListExpr->args)
                    {
                        RefPtr<ExpressionSyntaxNode> coercedArg;
                        ConversionCost argCost;

                        bool argResult = TryCoerceImpl(
                                toElementType,
                                outToExpr ? &coercedArg : nullptr,
                                arg->Type,
                                arg,
                                outCost ? &argCost : nullptr);

                        // No point in trying further if any argument fails
                        if(!argResult)
                            return false;

                        if( outToExpr )
                        {
                            coercedArgs.Add(coercedArg);
                        }
                    }
                }
                else
                {
                    // By default, we don't allow a type to be initialized using
                    // an initializer list.
                    return false;
                }

                // For now, coercion from an initializer list has no cost
                if(outCost)
                {
                    *outCost = kConversionCost_None;
                }

                // We were able to coerce all the arguments given, and so
                // we need to construct a suitable expression to remember the result
                if(outToExpr)
                {
                    auto toInitializerListExpr = new InitializerListExpr();
                    toInitializerListExpr->Position = fromInitializerListExpr->Position;
                    toInitializerListExpr->Type = toType;
                    toInitializerListExpr->args = coercedArgs;


                    *outToExpr = toInitializerListExpr;
                }

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

                    // We expect identical types to have been dealt with already.
                    assert(toInfo.kind != fromInfo.kind || toInfo.rank != fromInfo.rank);

                    if (outToExpr)
                        *outToExpr = CreateImplicitCastExpr(toType, fromExpr);


                    if (outCost)
                    {
                        // Conversions within the same kind are easist to handle
                        if (toInfo.kind == fromInfo.kind)
                        {
                            // If we are converting to a "larger" type, then
                            // we are doing a lossless promotion, and otherwise
                            // we are doing a demotion.
                            if( toInfo.rank > fromInfo.rank)
                                *outCost = kConversionCost_RankPromotion;
                            else
                                *outCost = kConversionCost_GeneralConversion;
                        }
                        // If we are converting from an unsigned integer type to
                        // a signed integer type that is guaranteed to be larger,
                        // then that is also a lossless promotion.
                        else if(toInfo.kind == kBaseTypeConversionKind_Signed
                            && fromInfo.kind == kBaseTypeConversionKind_Unsigned
                            && toInfo.rank > fromInfo.rank)
                        {
                            // TODO: probably need to weed out cases involving
                            // "pointer-sized" integers if these are treated
                            // as distinct from 32- and 64-bit types.
                            // E.g., there is no guarantee that conversion
                            // from 32-bit unsigned to pointer-sized signed
                            // is lossless, because pointers could be 32-bit,
                            // and the same applies for conversion from
                            // `uintptr` to `uint64`.
                            *outCost = kConversionCost_UnsignedToSignedPromotion;
                        }
                        // Conversion from signed to unsigned is always lossy,
                        // but it is preferred over conversions from unsigned
                        // to signed, for same-size types.
                        else if(toInfo.kind == kBaseTypeConversionKind_Unsigned
                            && fromInfo.kind == kBaseTypeConversionKind_Signed
                            && toInfo.rank >= fromInfo.rank)
                        {
                            *outCost = kConversionCost_SignedToUnsignedConversion;
                        }
                        // Conversion from an integer to a floating-point type
                        // is never considered a promotion (even when the value
                        // would fit in the available bits).
                        // If the destination type is at least 32 bits we consider
                        // this a reasonably good conversion, though.
                        else if (toInfo.kind == kBaseTypeConversionKind_Float
                            && toInfo.rank >= kBaseTypeConversionRank_Int32)
                        {
                            *outCost = kConversionCost_IntegerToFloatConversion;
                        }
                        // All other cases are considered as "general" conversions,
                        // where we don't consider any one conversion better than
                        // any others.
                        else
                        {
                            *outCost = kConversionCost_GeneralConversion;
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

            if (auto toDeclRefType = toType->As<DeclRefType>())
            {
                auto toTypeDeclRef = toDeclRefType->declRef;
                if (auto interfaceDeclRef = toTypeDeclRef.As<InterfaceDecl>())
                {
                    // Trying to convert to an interface type.
                    //
                    // We will allow this if the type conforms to the interface.
                    if (DoesTypeConformToInterface(fromType, interfaceDeclRef))
                    {
                        if (outToExpr)
                            *outToExpr = CreateImplicitCastExpr(toType, fromExpr);
                        if (outCost)
                            *outCost = kConversionCost_CastToInterface;
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
                if(getTranslationUnit()->compileFlags & SLANG_COMPILE_FLAG_NO_CHECKING )
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
                if(!(getTranslationUnit()->compileFlags & SLANG_COMPILE_FLAG_NO_CHECKING))
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

        virtual void visitInterfaceDecl(InterfaceDecl* /*decl*/) override
        {
            // TODO: do some actual checking of members here
        }

        virtual void visitInheritanceDecl(InheritanceDecl* inheritanceDecl) override
        {
            // check the type being inherited from
            auto base = inheritanceDecl->base;
            base = TranslateTypeNode(base);
            inheritanceDecl->base = base;

            // For now we only allow inheritance from interfaces, so
            // we will validate that the type expression names an interface

            if(auto declRefType = base.type->As<DeclRefType>())
            {
                if(auto interfaceDeclRef = declRefType->declRef.As<InterfaceDecl>())
                {
                    return;
                }
            }

            // If type expression didn't name an interface, we'll emit an error here
            // TODO: deal with the case of an error in the type expression (don't cascade)
            getSink()->diagnose( base.exp, Diagnostics::expectedAnInterfaceGot, base.type);
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
            Decl*               /*decl*/)
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

            // We need/want to visit any `import` declarations before
            // anything else, to make sure that scoping works.
            for(auto& importDecl : programNode->getMembersOfType<ImportDecl>())
            {
                EnsureDecl(importDecl);
            }

            //

            for (auto & s : programNode->GetTypeDefs())
                VisitTypeDefDecl(s.Ptr());
            for (auto & s : programNode->GetStructs())
            {
                VisitStruct(s.Ptr());
            }
			for (auto & s : programNode->GetClasses())
			{
				VisitClass(s.Ptr());
			}
            // HACK(tfoley): Visiting all generic declarations here,
            // because otherwise they won't get visited.
            for (auto & g : programNode->getMembersOfType<GenericDecl>())
            {
                VisitGenericDecl(g.Ptr());
            }

            for (auto & func : programNode->GetFunctions())
            {
                if (!func->IsChecked(DeclCheckState::Checked))
                {
                    VisitFunctionDeclaration(func.Ptr());
                }
            }
            for (auto & func : programNode->GetFunctions())
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
            buildMemberDictionary(parentDecl);

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
            auto condition = stmt->Predicate;
            condition = CheckTerm(condition);
            condition = Coerce(ExpressionType::GetBool(), condition);

            stmt->Predicate = condition;

#if 0
            if (stmt->Predicate != NULL)
                stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
            if (!stmt->Predicate->Type->Equals(ExpressionType::GetError())
                && (!stmt->Predicate->Type->Equals(ExpressionType::GetInt()) &&
                    !stmt->Predicate->Type->Equals(ExpressionType::GetBool())))
                getSink()->diagnose(stmt, Diagnostics::ifPredicateTypeError);
#endif

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

        void maybeInferArraySizeForVariable(Variable* varDecl)
        {
            // Not an array?
            auto arrayType = varDecl->Type->AsArrayType();
            if (!arrayType) return;

            // Explicit element count given?
            auto elementCount = arrayType->ArrayLength;
            if (elementCount) return;

            // No initializer?
            auto initExpr = varDecl->Expr;
            if(!initExpr) return;

            // Is the initializer an initializer list?
            if(auto initializerListExpr = initExpr.As<InitializerListExpr>())
            {
                auto argCount = initializerListExpr->args.Count();
                elementCount = new ConstantIntVal(argCount);
            }
            // Is the type of the initializer an array type?
            else if(auto arrayInitType = initExpr->Type->As<ArrayExpressionType>())
            {
                elementCount = arrayInitType->ArrayLength;
            }
            else
            {
                // Nothing to do: we couldn't infer a size
                return;
            }

            // Create a new array type based on the size we found,
            // and install it into our type.
            auto newArrayType = new ArrayExpressionType();
            newArrayType->BaseType = arrayType->BaseType;
            newArrayType->ArrayLength = elementCount;

            // Okay we are good to go!
            varDecl->Type.type = newArrayType;
        }

        void ValidateArraySizeForVariable(Variable* varDecl)
        {
            auto arrayType = varDecl->Type->AsArrayType();
            if (!arrayType) return;

            auto elementCount = arrayType->ArrayLength;
            if (!elementCount)
            {
                // Note(tfoley): For now we allow arrays of unspecified size
                // everywhere, because some source languages (e.g., GLSL)
                // allow them in specific cases.
#if 0
                getSink()->diagnose(varDecl, Diagnostics::invalidArraySize);
#endif
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
#if 0
            if (typeExp.type->GetBindableResourceType() != BindableResourceType::NonBindable)
            {
                // We don't want to allow bindable resource types as local variables (at least for now).
                auto parentDecl = varDecl->ParentDecl;
                if (auto parentScopeDecl = dynamic_cast<ScopeDecl*>(parentDecl))
                {
                    getSink()->diagnose(varDecl->Type, Diagnostics::invalidTypeForLocalVariable);
                }
            }
#endif
            varDecl->Type = typeExp;
            if (varDecl->Type.Equals(ExpressionType::GetVoid()))
                getSink()->diagnose(varDecl, Diagnostics::invalidTypeVoid);

            if(auto initExpr = varDecl->Expr)
            {
                initExpr = CheckTerm(initExpr);
                varDecl->Expr = initExpr;
            }

            // If this is an array variable, then we first want to give
            // it a chance to infer an array size from its initializer
            //
            // TODO(tfoley): May need to extend this to handle the
            // multi-dimensional case...
            maybeInferArraySizeForVariable(varDecl);
            //
            // Next we want to make sure that the declared (or inferred)
            // size for the array meets whatever language-specific
            // constraints we want to enforce (e.g., disallow empty
            // arrays in specific cases)
            ValidateArraySizeForVariable(varDecl);


            if(auto initExpr = varDecl->Expr)
            {
                // TODO(tfoley): should coercion of initializer lists be special-cased
                // here, or handled as a general case for coercion?

                initExpr = Coerce(varDecl->Type, initExpr);
                varDecl->Expr = initExpr;
            }

            varDecl->SetCheckState(DeclCheckState::Checked);

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
#if 0

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
#if 0
                    if (!(leftType->IsIntegral() && rightType->IsIntegral()))
                    {
                        // TODO(tfoley): This diagnostic shouldn't be handled here
//                            getSink()->diagnose(expr, Diagnostics::bitOperationNonIntegral);
                    }
#endif
                }

                // TODO(tfoley): Need to actual insert coercion here...
                if(CanCoerce(leftType, expr->Type))
                    expr->Type = leftType;
                else
                    expr->Type = ExpressionType::Error;
            };
#if 0
            if (expr->Operator == Operator::Assign)
            {
                expr->Type = rightType;
                checkAssign();
            }
            else
#endif
            {
                expr->FunctionExpr = CheckExpr(expr->FunctionExpr);
                CheckInvokeExprWithCheckedOperands(expr);
                if (expr->Operator > Operator::Assign)
                    checkAssign();
            }
            return expr;
#endif

            // Treat operator application just like a function call
            return VisitInvokeExpression(expr);
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
            auto intrinsicMod = funcDeclRef.getDecl()->FindModifier<IntrinsicOpModifier>();
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

                if (auto genericValParamRef = declRef.As<GenericValueParamDecl>())
                {
                    // TODO(tfoley): handle the case of non-`int` value parameters...
                    return new GenericParamIntVal(genericValParamRef);
                }

                // We may also need to check for references to variables that
                // are defined in a way that can be used as a constant expression:
                if(auto varRef = declRef.As<VarDeclBase>())
                {
                    auto varDecl = varRef.getDecl();

                    switch(sourceLanguage)
                    {
                    case SourceLanguage::Slang:
                    case SourceLanguage::HLSL:
                        // HLSL: `static const` is used to mark compile-time constant expressions
                        if(auto staticAttr = varDecl->FindModifier<HLSLStaticModifier>())
                        {
                            if(auto constAttr = varDecl->FindModifier<ConstModifier>())
                            {
                                // HLSL `static const` can be used as a constant expression
                                if(auto initExpr = getInitExpr(varRef))
                                {
                                    return TryConstantFoldExpr(initExpr.Ptr());
                                }
                            }
                        }
                        break;

                    case SourceLanguage::GLSL:
                        // GLSL: `const` indicates compile-time constant expression
                        //
                        // TODO(tfoley): The current logic here isn't robust against
                        // GLSL "specialization constants" - we will extract the
                        // initializer for a `const` variable and use it to extract
                        // a value, when we really should be using an opaque
                        // reference to the variable.
                        if(auto constAttr = varDecl->FindModifier<ConstModifier>())
                        {
                            // We need to handle a "specialization constant" (with a `constant_id` layout modifier)
                            // differently from an ordinary compile-time constant. The latter can/should be reduced
                            // to a value, while the former should be kept as a symbolic reference

                            if(auto constantIDModifier = varDecl->FindModifier<GLSLConstantIDLayoutModifier>())
                            {
                                // Retain the specialization constant as a symbolic reference
                                //
                                // TODO(tfoley): handle the case of non-`int` value parameters...
                                //
                                // TODO(tfoley): this is cloned from the case above that handles generic value parameters
                                return new GenericParamIntVal(varRef);
                            }
                            else if(auto initExpr = getInitExpr(varRef))
                            {
                                // This is an ordinary constant, and not a specialization constant, so we
                                // can try to fold its value right now.
                                return TryConstantFoldExpr(initExpr.Ptr());
                            }
                        }
                        break;
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

        // The way that we have designed out type system, pretyt much *every*
        // type is a reference to some declaration in the standard library.
        // That means that when we construct a new type on the fly, we need
        // to make sure that it is wired up to reference the appropriate
        // declaration, or else it won't compare as equal to other types
        // that *do* reference the declaration.
        //
        // This function is used to construct a `vector<T,N>` type
        // programmatically, so that it will work just like a type of
        // that form constructed by the user.
        RefPtr<VectorExpressionType> createVectorType(
            RefPtr<ExpressionType>  elementType,
            RefPtr<IntVal>          elementCount)
        {
            auto vectorGenericDecl = findMagicDecl("Vector").As<GenericDecl>();
            auto vectorTypeDecl = vectorGenericDecl->inner;
               
            auto substitutions = new Substitutions();
            substitutions->genericDecl = vectorGenericDecl.Ptr();
            substitutions->args.Add(elementType);
            substitutions->args.Add(elementCount);

            auto declRef = DeclRef<Decl>(vectorTypeDecl.Ptr(), substitutions);

            return DeclRefType::Create(declRef)->As<VectorExpressionType>();
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
            if (auto baseTypeType = baseType->As<TypeType>())
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
                subscriptExpr->Type = new TypeType(arrayType);
                return subscriptExpr;
            }
            else if (auto baseArrayType = baseType->As<ArrayExpressionType>())
            {
                return CheckSimpleSubscriptExpr(
                    subscriptExpr,
                    baseArrayType->BaseType);
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
                auto rowType = createVectorType(
                    matType->getElementType(),
                    matType->getColumnCount());

                return CheckSimpleSubscriptExpr(
                    subscriptExpr,
                    rowType);
            }

            // Default behavior is to look at all available `__subscript`
            // declarations on the type and try to call one of them.

            if (auto declRefType = baseType->AsDeclRefType())
            {
                if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDecl>())
                {
                    // Checking of the type must be complete before we can reference its members safely
                    EnsureDecl(aggTypeDeclRef.getDecl(), DeclCheckState::Checked);

                    // Note(tfoley): The name used for lookup here is a bit magical, since
                    // it must match what the parser installed in subscript declarations.
                    LookupResult lookupResult = LookUpLocal(this, "operator[]", aggTypeDeclRef);
                    if (!lookupResult.isValid())
                    {
                        goto fail;
                    }

                    RefPtr<ExpressionSyntaxNode> subscriptFuncExpr = createLookupResultExpr(
                        lookupResult, subscriptExpr->BaseExpression, subscriptExpr);

                    // Now that we know there is at least one subscript member,
                    // we will construct a reference to it and try to call it

                    RefPtr<InvokeExpressionSyntaxNode> subscriptCallExpr = new InvokeExpressionSyntaxNode();
                    subscriptCallExpr->Position = subscriptExpr->Position;
                    subscriptCallExpr->FunctionExpr = subscriptFuncExpr;

                    // TODO(tfoley): This path can support multiple arguments easily
                    subscriptCallExpr->Arguments.Add(subscriptExpr->IndexExpression);

                    return CheckInvokeExprWithCheckedOperands(subscriptCallExpr.Ptr());
                }
            }

        fail:
            {
                getSink()->diagnose(subscriptExpr, Diagnostics::subscriptNonArray, baseType);
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
                EnsureDecl(funcDeclRef.getDecl());

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
                if (auto aggTypeDeclRef = targetDeclRefType->declRef.As<AggTypeDecl>())
                {
                    auto aggTypeDecl = aggTypeDeclRef.getDecl();
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


        virtual void visitSubscriptDecl(SubscriptDecl* decl) override
        {
            if (decl->IsChecked(DeclCheckState::Checked)) return;
            decl->SetCheckState(DeclCheckState::CheckingHeader);

            for (auto& paramDecl : decl->GetParameters())
            {
                paramDecl->Type = CheckUsableType(paramDecl->Type);
            }

            decl->ReturnType = CheckUsableType(decl->ReturnType);

            decl->SetCheckState(DeclCheckState::CheckedHeader);

            decl->SetCheckState(DeclCheckState::Checked);
        }

        virtual void visitAccessorDecl(AccessorDecl* decl) override
        {
            // TODO: check the body!

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

            return createVectorType(
                joinElementType,
                vectorType->elementCount);
        }

        bool DoesTypeConformToInterface(
            RefPtr<ExpressionType>  type,
            DeclRef<InterfaceDecl>        interfaceDeclRef)
        {
            // for now look up a conformance member...
            if(auto declRefType = type->As<DeclRefType>())
            {
                if( auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDecl>() )
                {
                    for( auto inheritanceDeclRef : getMembersOfType<InheritanceDecl>(aggTypeDeclRef))
                    {
                        EnsureDecl(inheritanceDeclRef.getDecl());

                        auto inheritedDeclRefType = getBaseType(inheritanceDeclRef)->As<DeclRefType>();
                        if (!inheritedDeclRefType)
                            continue;

                        if(interfaceDeclRef.Equals(inheritedDeclRefType->declRef))
                            return true;
                    }
                }
            }

            // default is failure
            return false;
        }

        RefPtr<ExpressionType> TryJoinTypeWithInterface(
            RefPtr<ExpressionType>  type,
            DeclRef<InterfaceDecl>        interfaceDeclRef)
        {
            // The most basic test here should be: does the type declare conformance to the trait.
            if(DoesTypeConformToInterface(type, interfaceDeclRef))
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

                    return createVectorType(
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
                if( auto leftInterfaceRef = leftDeclRefType->declRef.As<InterfaceDecl>() )
                {
                    // 
                    return TryJoinTypeWithInterface(right, leftInterfaceRef);
                }
            }
            if(auto rightDeclRefType = right->As<DeclRefType>())
            {
                if( auto rightInterfaceRef = rightDeclRefType->declRef.As<InterfaceDecl>() )
                {
                    // 
                    return TryJoinTypeWithInterface(left, rightInterfaceRef);
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
            DeclRef<GenericDecl>          genericDeclRef)
        {
            // For now the "solver" is going to be ridiculously simplistic.

            // The generic itself will have some constraints, so we need to try and solve those too
            for( auto constraintDeclRef : getMembersOfType<GenericTypeConstraintDecl>(genericDeclRef) )
            {
                if(!TryUnifyTypes(*system, GetSub(constraintDeclRef), GetSup(constraintDeclRef)))
                    return nullptr;
            }

            // We will loop over the generic parameters, and for
            // each we will try to find a way to satisfy all
            // the constraints for that parameter
            List<RefPtr<Val>> args;
            for (auto m : getMembers(genericDeclRef))
            {
                if (auto typeParam = m.As<GenericTypeParamDecl>())
                {
                    RefPtr<ExpressionType> type = nullptr;
                    for (auto& c : system->constraints)
                    {
                        if (c.decl != typeParam.getDecl())
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
                else if (auto valParam = m.As<GenericValueParamDecl>())
                {
                    // TODO(tfoley): maybe support more than integers some day?
                    // TODO(tfoley): figure out how this needs to interact with
                    // compile-time integers that aren't just constants...
                    RefPtr<IntVal> val = nullptr;
                    for (auto& c : system->constraints)
                    {
                        if (c.decl != valParam.getDecl())
                            continue;

                        auto cVal = c.val.As<IntVal>();
                        assert(cVal.Ptr());

                        if (!val)
                        {
                            val = cVal;
                        }
                        else
                        {
                            if(!val->EqualsVal(cVal.Ptr()))
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
            solvedSubst->genericDecl = genericDeclRef.getDecl();
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
                        if (c.decl != typeVar->declRef.getDecl())
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
                        if (c.decl != valueVar->declRef.getDecl())
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
                FixityChecked,
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
        ParamCounts CountParameters(FilteredMemberRefList<ParameterSyntaxNode> params)
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
                if (!param.getDecl()->Expr)
                {
                    counts.required++;
                }
            }
            return counts;
        }

        // count the number of parameters required/allowed for a generic
        ParamCounts CountParameters(DeclRef<GenericDecl> genericRef)
        {
            ParamCounts counts = { 0, 0 };
            for (auto m : genericRef.getDecl()->Members)
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
                paramCounts = CountParameters(GetParameters(candidate.item.declRef.As<CallableDecl>()));
                break;

            case OverloadCandidate::Flavor::Generic:
                paramCounts = CountParameters(candidate.item.declRef.As<GenericDecl>());
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
                    getSink()->diagnose(context.appExpr, Diagnostics::notEnoughArguments, argCount, paramCounts.required);
                }
                else
                {
                    assert(argCount > paramCounts.allowed);
                    getSink()->diagnose(context.appExpr, Diagnostics::tooManyArguments, argCount, paramCounts.allowed);
                }
            }

            return false;
        }

        bool TryCheckOverloadCandidateFixity(
            OverloadResolveContext&		context,
            OverloadCandidate const&	candidate)
        {
            auto expr = context.appExpr;

            auto decl = candidate.item.declRef.decl;

            if(auto prefixExpr = expr.As<PrefixExpr>())
            {
                if(decl->HasModifier<PrefixModifier>())
                    return true;

                if (context.mode != OverloadResolveContext::Mode::JustTrying)
                {
                    getSink()->diagnose(context.appExpr, Diagnostics::expectedPrefixOperator);
                    getSink()->diagnose(decl, Diagnostics::seeDefinitionOf, decl->getName());
                }

                return false;
            }
            else if(auto postfixExpr = expr.As<PostfixExpr>())
            {
                if(decl->HasModifier<PostfixModifier>())
                    return true;

                if (context.mode != OverloadResolveContext::Mode::JustTrying)
                {
                    getSink()->diagnose(context.appExpr, Diagnostics::expectedPostfixOperator);
                    getSink()->diagnose(decl, Diagnostics::seeDefinitionOf, decl->getName());
                }

                return false;
            }
            else
            {
                return true;
            }

            return false;
        }

        bool TryCheckGenericOverloadCandidateTypes(
            OverloadResolveContext&	context,
            OverloadCandidate&		candidate)
        {
            auto& args = context.appExpr->Arguments;

            auto genericDeclRef = candidate.item.declRef.As<GenericDecl>();

            int aa = 0;
            for (auto memberRef : getMembers(genericDeclRef))
            {
                if (auto typeParamRef = memberRef.As<GenericTypeParamDecl>())
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
                else if (auto valParamRef = memberRef.As<GenericValueParamDecl>())
                {
                    auto arg = args[aa++];

                    if (context.mode == OverloadResolveContext::Mode::JustTrying)
                    {
                        ConversionCost cost = kConversionCost_None;
                        if (!CanCoerce(GetType(valParamRef), arg->Type, &cost))
                        {
                            return false;
                        }
                        candidate.conversionCostSum += cost;
                    }
                    else
                    {
                        arg = Coerce(GetType(valParamRef), arg);
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

            List<DeclRef<ParameterSyntaxNode>> params;
            switch (candidate.flavor)
            {
            case OverloadCandidate::Flavor::Func:
                params = GetParameters(candidate.item.declRef.As<CallableDecl>()).ToArray();
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
                    if (!CanCoerce(GetType(param), arg->Type, &cost))
                    {
                        return false;
                    }
                    candidate.conversionCostSum += cost;
                }
                else
                {
                    arg = Coerce(GetType(param), arg);
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
            if (!TryCheckOverloadCandidateFixity(context, candidate))
                return;

            candidate.status = OverloadCandidate::Status::FixityChecked;
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
            auto baseGenericRef = baseDeclRefExpr->declRef.As<GenericDecl>();
            if (!baseGenericRef)
            {
                assert(!"unexpected");
                return CreateErrorExpr(appExpr.Ptr());
            }

            RefPtr<Substitutions> subst = new Substitutions();
            subst->genericDecl = baseGenericRef.getDecl();
            subst->outer = baseGenericRef.substitutions;

            for (auto arg : appExpr->Arguments)
            {
                subst->args.Add(ExtractGenericArgVal(arg));
            }

            DeclRef<Decl> innerDeclRef(GetInner(baseGenericRef), subst);

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

            if (!TryCheckOverloadCandidateFixity(context, candidate))
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

                    // A call may yield an l-value, and we should take a look at the candidate to be sure
                    if(auto subscriptDeclRef = candidate.item.declRef.As<SubscriptDecl>())
                    {
                        for(auto setter : subscriptDeclRef.getDecl()->getMembersOfType<SetterDecl>())
                        {
                            context.appExpr->Type.IsLeftValue = true;
                        }
                    }

                    // TODO: there may be other cases that confer l-value-ness

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

            // If both candidates are applicable, then we need to compare
            // the costs of their type conversion sequences
            if(left->status == OverloadCandidate::Status::Appicable)
            {
                if (left->conversionCostSum != right->conversionCostSum)
                    return left->conversionCostSum - right->conversionCostSum;
            }

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
            DeclRef<CallableDecl>             funcDeclRef,
            OverloadResolveContext&		context)
        {
            EnsureDecl(funcDeclRef.getDecl());

            OverloadCandidate candidate;
            candidate.flavor = OverloadCandidate::Flavor::Func;
            candidate.item = item;
            candidate.resultType = GetResultType(funcDeclRef);

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
            DeclRef<ConstructorDecl>		ctorDeclRef,
            OverloadResolveContext&	context)
        {
            EnsureDecl(ctorDeclRef.getDecl());

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
                        TryUnifyIntParam(constraints, fstParam->declRef, sndInt);
                    if (sndParam)
                        TryUnifyIntParam(constraints, sndParam->declRef, fstInt);

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

        bool TryUnifyIntParam(
            ConstraintSystem&       constraints,
            DeclRef<VarDeclBase> const&   varRef,
            RefPtr<IntVal>          val)
        {
            if(auto genericValueParamRef = varRef.As<GenericValueParamDecl>())
            {
                return TryUnifyIntParam(constraints, genericValueParamRef.getDecl(), val);
            }
            else
            {
                return false;
            }
        }

        bool TryUnifyTypesByStructuralMatch(
            ConstraintSystem&       constraints,
            RefPtr<ExpressionType>  fst,
            RefPtr<ExpressionType>  snd)
        {
            if (auto fstDeclRefType = fst->As<DeclRefType>())
            {
                auto fstDeclRef = fstDeclRefType->declRef;

                if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(fstDeclRef.getDecl()))
                    return TryUnifyTypeParam(constraints, typeParamDecl, snd);

                if (auto sndDeclRefType = snd->As<DeclRefType>())
                {
                    auto sndDeclRef = sndDeclRefType->declRef;

                    if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(sndDeclRef.getDecl()))
                        return TryUnifyTypeParam(constraints, typeParamDecl, fst);

                    // can't be unified if they refer to differnt declarations.
                    if (fstDeclRef.getDecl() != sndDeclRef.getDecl()) return false;

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

            return false;
        }

        bool TryUnifyTypes(
            ConstraintSystem&       constraints,
            RefPtr<ExpressionType>  fst,
            RefPtr<ExpressionType>  snd)
        {
            if (fst->Equals(snd)) return true;

            // An error type can unify with anything, just so we avoid cascading errors.

            if (auto fstErrorType = fst->As<ErrorType>())
                return true;

            if (auto sndErrorType = snd->As<ErrorType>())
                return true;

            // A generic parameter type can unify with anything.
            // TODO: there actually needs to be some kind of "occurs check" sort
            // of thing here...

            if (auto fstDeclRefType = fst->As<DeclRefType>())
            {
                auto fstDeclRef = fstDeclRefType->declRef;

                if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(fstDeclRef.getDecl()))
                    return TryUnifyTypeParam(constraints, typeParamDecl, snd);
            }

            if (auto sndDeclRefType = snd->As<DeclRefType>())
            {
                auto sndDeclRef = sndDeclRefType->declRef;

                if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(sndDeclRef.getDecl()))
                    return TryUnifyTypeParam(constraints, typeParamDecl, fst);
            }

            // If we can unify the types structurally, then we are golden
            if(TryUnifyTypesByStructuralMatch(constraints, fst, snd))
                return true;

            // Now we need to consider cases where coercion might
            // need to be applied. For now we can try to do this
            // in a completely ad hoc fashion, but eventually we'd
            // want to do it more formally.

            if(auto fstVectorType = fst->As<VectorExpressionType>())
            {
                if(auto sndScalarType = snd->As<BasicExpressionType>())
                {
                    return TryUnifyTypes(
                        constraints,
                        fstVectorType->elementType,
                        sndScalarType);
                }
            }

            if(auto fstScalarType = fst->As<BasicExpressionType>())
            {
                if(auto sndVectorType = snd->As<VectorExpressionType>())
                {
                    return TryUnifyTypes(
                        constraints,
                        fstScalarType,
                        sndVectorType->elementType);
                }
            }

            // TODO: the same thing for vectors...

            return false;
        }

        // Is the candidate extension declaration actually applicable to the given type
        DeclRef<ExtensionDecl> ApplyExtensionToType(
            ExtensionDecl*			extDecl,
            RefPtr<ExpressionType>	type)
        {
            if (auto extGenericDecl = GetOuterGeneric(extDecl))
            {
                ConstraintSystem constraints;

                if (!TryUnifyTypes(constraints, extDecl->targetType, type))
                    return DeclRef<Decl>().As<ExtensionDecl>();

                auto constraintSubst = TrySolveConstraintSystem(&constraints, DeclRef<Decl>(extGenericDecl, nullptr).As<GenericDecl>());
                if (!constraintSubst)
                {
                    return DeclRef<Decl>().As<ExtensionDecl>();
                }

                // Consruct a reference to the extension with our constraint variables
                // set as they were found by solving the constraint system.
                DeclRef<ExtensionDecl> extDeclRef = DeclRef<Decl>(extDecl, constraintSubst).As<ExtensionDecl>();

                // We expect/require that the result of unification is such that
                // the target types are now equal
                assert(GetTargetType(extDeclRef)->Equals(type));

                return extDeclRef;
            }
            else
            {
                // The easy case is when the extension isn't generic:
                // either it applies to the type or not.
                if (!type->Equals(extDecl->targetType))
                    return DeclRef<Decl>().As<ExtensionDecl>();
                return DeclRef<Decl>(extDecl, nullptr).As<ExtensionDecl>();
            }
        }

        bool TryUnifyArgAndParamTypes(
            ConstraintSystem&				system,
            RefPtr<ExpressionSyntaxNode>	argExpr,
            DeclRef<ParameterSyntaxNode>					paramDeclRef)
        {
            // TODO(tfoley): potentially need a bit more
            // nuance in case where argument might be
            // an overload group...
            return TryUnifyTypes(system, argExpr->Type, GetType(paramDeclRef));
        }

        // Take a generic declaration and try to specialize its parameters
        // so that the resulting inner declaration can be applicable in
        // a particular context...
        DeclRef<Decl> SpecializeGenericForOverload(
            DeclRef<GenericDecl>			genericDeclRef,
            OverloadResolveContext&	context)
        {
            ConstraintSystem constraints;

            // Construct a reference to the inner declaration that has any generic
            // parameter substitutions in place already, but *not* any substutions
            // for the generic declaration we are currently trying to infer.
            auto innerDecl = GetInner(genericDeclRef);
            DeclRef<Decl> unspecializedInnerRef = DeclRef<Decl>(innerDecl, genericDeclRef.substitutions);

            // Check what type of declaration we are dealing with, and then try
            // to match it up with the arguments accordingly...
            if (auto funcDeclRef = unspecializedInnerRef.As<CallableDecl>())
            {
                auto& args = context.appExpr->Arguments;
                auto params = GetParameters(funcDeclRef).ToArray();

                int argCount = args.Count();
                int paramCount = params.Count();

                // Bail out on mismatch.
                // TODO(tfoley): need more nuance here
                if (argCount != paramCount)
                {
                    return DeclRef<Decl>(nullptr, nullptr);
                }

                for (int aa = 0; aa < argCount; ++aa)
                {
#if 0
                    if (!TryUnifyArgAndParamTypes(constraints, args[aa], params[aa]))
                        return DeclRef<Decl>(nullptr, nullptr);
#else
                    // The question here is whether failure to "unify" an argument
                    // and parameter should lead to immediate failure.
                    //
                    // The case that is interesting is if we want to unify, say:
                    // `vector<float,N>` and `vector<int,3>`
                    //
                    // It is clear that we should solve with `N = 3`, and then
                    // a later step may find that the resulting types aren't
                    // actually a match.
                    //
                    // A more refined approach to "unification" could of course
                    // see that `int` can convert to `float` and use that fact.
                    // (and indeed we already use something like this to unify
                    // `float` and `vector<T,3>`)
                    //
                    // So the question is then whether a mismatch during the
                    // unification step should be taken as an immediate failure...

                    TryUnifyArgAndParamTypes(constraints, args[aa], params[aa]);
#endif
                }
            }
            else
            {
                // TODO(tfoley): any other cases needed here?
                return DeclRef<Decl>(nullptr, nullptr);
            }

            auto constraintSubst = TrySolveConstraintSystem(&constraints, genericDeclRef);
            if (!constraintSubst)
            {
                // constraint solving failed
                return DeclRef<Decl>(nullptr, nullptr);
            }

            // We can now construct a reference to the inner declaration using
            // the solution to our constraints.
            return DeclRef<Decl>(innerDecl, constraintSubst);
        }

        void AddAggTypeOverloadCandidates(
            LookupResultItem		typeItem,
            RefPtr<ExpressionType>	type,
            DeclRef<AggTypeDecl>			aggTypeDeclRef,
            OverloadResolveContext&	context)
        {
            for (auto ctorDeclRef : getMembersOfType<ConstructorDecl>(aggTypeDeclRef))
            {
                // now work through this candidate...
                AddCtorOverloadCandidate(typeItem, type, ctorDeclRef, context);
            }

            // Now walk through any extensions we can find for this types
            for (auto ext = GetCandidateExtensions(aggTypeDeclRef); ext; ext = ext->nextCandidateExtension)
            {
                auto extDeclRef = ApplyExtensionToType(ext, type);
                if (!extDeclRef)
                    continue;

                for (auto ctorDeclRef : getMembersOfType<ConstructorDecl>(extDeclRef))
                {
                    // TODO(tfoley): `typeItem` here should really reference the extension...

                    // now work through this candidate...
                    AddCtorOverloadCandidate(typeItem, type, ctorDeclRef, context);
                }

                // Also check for generic constructors
                for (auto genericDeclRef : getMembersOfType<GenericDecl>(extDeclRef))
                {
                    if (auto ctorDecl = genericDeclRef.getDecl()->inner.As<ConstructorDecl>())
                    {
                        DeclRef<Decl> innerRef = SpecializeGenericForOverload(genericDeclRef, context);
                        if (!innerRef)
                            continue;

                        DeclRef<ConstructorDecl> innerCtorRef = innerRef.As<ConstructorDecl>();

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
                if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDecl>())
                {
                    AddAggTypeOverloadCandidates(LookupResultItem(aggTypeDeclRef), type, aggTypeDeclRef, context);
                }
            }
        }

        void AddDeclRefOverloadCandidates(
            LookupResultItem		item,
            OverloadResolveContext&	context)
        {
            auto declRef = item.declRef;

            if (auto funcDeclRef = item.declRef.As<CallableDecl>())
            {
                AddFuncOverloadCandidate(item, funcDeclRef, context);
            }
            else if (auto aggTypeDeclRef = item.declRef.As<AggTypeDecl>())
            {
                auto type = DeclRefType::Create(aggTypeDeclRef);
                AddAggTypeOverloadCandidates(item, type, aggTypeDeclRef, context);
            }
            else if (auto genericDeclRef = item.declRef.As<GenericDecl>())
            {
                // Try to infer generic arguments, based on the context
                DeclRef<Decl> innerRef = SpecializeGenericForOverload(genericDeclRef, context);

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
            else if( auto typeDefDeclRef = item.declRef.As<TypeDefDecl>() )
            {
                AddTypeOverloadCandidates(GetType(typeDefDeclRef), context);
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
            else if (auto typeType = funcExprType->As<TypeType>())
            {
                // If none of the above cases matched, but we are
                // looking at a type, then I suppose we have
                // a constructor call on our hands.
                //
                // TODO(tfoley): are there any meaningful types left
                // that aren't declaration references?
                AddTypeOverloadCandidates(typeType->type, context);
                return;
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

        void formatDeclPath(StringBuilder& sb, DeclRef<Decl> declRef)
        {
            // Find the parent declaration
            auto parentDeclRef = declRef.GetParent();

            // If the immediate parent is a generic, then we probably
            // want the declaration above that...
            auto parentGenericDeclRef = parentDeclRef.As<GenericDecl>();
            if(parentGenericDeclRef)
            {
                parentDeclRef = parentGenericDeclRef.GetParent();
            }

            // Depending on what the parent is, we may want to format things specially
            if(auto aggTypeDeclRef = parentDeclRef.As<AggTypeDecl>())
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
                assert(declRef.substitutions->genericDecl == parentGenericDeclRef.getDecl());

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

        void formatDeclParams(StringBuilder& sb, DeclRef<Decl> declRef)
        {
            if (auto funcDeclRef = declRef.As<CallableDecl>())
            {

                // This is something callable, so we need to also print parameter types for overloading
                sb << "(";

                bool first = true;
                for (auto paramDeclRef : GetParameters(funcDeclRef))
                {
                    if (!first) sb << ", ";

                    formatType(sb, GetType(paramDeclRef));

                    first = false;

                }

                sb << ")";
            }
            else if(auto genericDeclRef = declRef.As<GenericDecl>())
            {
                sb << "<";
                bool first = true;
                for (auto paramDeclRef : getMembers(genericDeclRef))
                {
                    if(auto genericTypeParam = paramDeclRef.As<GenericTypeParamDecl>())
                    {
                        if (!first) sb << ", ";
                        first = false;

                        sb << genericTypeParam.GetName();
                    }
                    else if(auto genericValParam = paramDeclRef.As<GenericValueParamDecl>())
                    {
                        if (!first) sb << ", ";
                        first = false;

                        formatType(sb, GetType(genericValParam));
                        sb << " ";
                        sb << genericValParam.GetName();
                    }
                    else
                    {}
                }
                sb << ">";

                formatDeclParams(sb, DeclRef<Decl>(GetInner(genericDeclRef), genericDeclRef.substitutions));
            }
            else
            {
            }
        }

        void formatDeclSignature(StringBuilder& sb, DeclRef<Decl> declRef)
        {
            formatDeclPath(sb, declRef);
            formatDeclParams(sb, declRef);
        }

        String getDeclSignatureString(DeclRef<Decl> declRef)
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
                    funcName = baseVar->name;
                else if(auto baseMemberRef = funcExpr.As<MemberExpressionSyntaxNode>())
                    funcName = baseMemberRef->name;

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

                    declString = declString + "[" + String(candidate.conversionCostSum) + "]";

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
            if (auto genericDeclRef = baseItem.declRef.As<GenericDecl>())
            {
                EnsureDecl(genericDeclRef.getDecl());

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

                if (auto genericDeclRef = declRef.As<GenericDecl>())
                {
                    int argCount = typeNode->Args.Count();
                    int argIndex = 0;
                    for (RefPtr<Decl> member : genericDeclRef.getDecl()->Members)
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
                    if (auto func = funcType->declRef.getDecl())
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


        virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode *expr) override
        {
            // If we've already resolved this expression, don't try again.
            if (expr->declRef)
                return expr;

            expr->Type = ExpressionType::Error;

            auto lookupResult = LookUp(this, expr->name, expr->scope);
            if (lookupResult.isValid())
            {
                return createLookupResultExpr(
                    lookupResult,
                    nullptr,
                    expr);
            }

            getSink()->diagnose(expr, Diagnostics::undefinedIdentifier2, expr->name);

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
        QualType GetTypeForDeclRef(DeclRef<Decl> declRef)
        {
            return getTypeForDeclRef(
                this,
                getSink(),
                declRef,
                &typeResult);
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

            for (int i = 0; i < memberRefExpr->name.Length(); i++)
            {
                auto ch = memberRefExpr->name[i];
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
                swizExpr->Type = createVectorType(
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
            else if(auto typeType = baseType->As<TypeType>())
            {
                // We are looking up a member inside a type.
                // We want to be careful here because we should only find members
                // that are implicitly or explicitly `static`.
                //
                // TODO: this duplicates a *lot* of logic with the case below.
                // We need to fix that.
                auto type = typeType->type;
                if(auto declRefType = type->AsDeclRefType())
                {
                    if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDecl>())
                    {
                        // Checking of the type must be complete before we can reference its members safely
                        EnsureDecl(aggTypeDeclRef.getDecl(), DeclCheckState::Checked);

                        LookupResult lookupResult = LookUpLocal(this, expr->name, aggTypeDeclRef);
                        if (!lookupResult.isValid())
                        {
                            goto fail;
                        }

                        // TODO: need to filter for declarations that are valid to refer
                        // to in this context...

                        return createLookupResultExpr(
                            lookupResult,
                            expr->BaseExpression,
                            expr);
                    }
                }
            }
            else if (auto declRefType = baseType->AsDeclRefType())
            {
                if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDecl>())
                {
                    // Checking of the type must be complete before we can reference its members safely
                    EnsureDecl(aggTypeDeclRef.getDecl(), DeclCheckState::Checked);

                    LookupResult lookupResult = LookUpLocal(this, expr->name, aggTypeDeclRef);
                    if (!lookupResult.isValid())
                    {
                        goto fail;
                    }

                    return createLookupResultExpr(
                        lookupResult,
                        expr->BaseExpression,
                        expr);
                }

                // catch-all
            fail:
                getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->name, baseType);
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
                getSink()->diagnose(expr, Diagnostics::typeHasNoPublicMemberOfName, baseType, expr->name);
            }
            return expr;
        }
        SemanticsVisitor & operator = (const SemanticsVisitor &) = delete;


        //

        virtual RefPtr<ExpressionSyntaxNode> visitInitializerListExpr(InitializerListExpr* expr) override
        {
            // When faced with an initializer list, we first just check the sub-expressions blindly.
            // Actually making them conform to a desired type will wait for when we know the desired
            // type based on context.

            for( auto& arg : expr->args )
            {
                arg = CheckTerm(arg);
            }

            expr->Type = ExpressionType::getInitializerListType();

            return expr;
        }

        void importModuleIntoScope(Scope* scope, ProgramSyntaxNode* moduleDecl)
        {
            // If we've imported this one already, then
            // skip the step where we modify the current scope.
            if (importedModules.Contains(moduleDecl))
            {
                return;
            }
            importedModules.Add(moduleDecl);


            // Create a new sub-scope to wire the module
            // into our lookup chain.
            auto subScope = new Scope();
            subScope->containerDecl = moduleDecl;

            subScope->nextSibling = scope->nextSibling;
            scope->nextSibling = subScope;

            // Also import any modules from nested `import` declarations
            // with the `__exported` modifier
            for (auto importDecl : moduleDecl->getMembersOfType<ImportDecl>())
            {
                if (!importDecl->HasModifier<ExportedModifier>())
                    continue;

                importModuleIntoScope(scope, importDecl->importedModuleDecl.Ptr());
            }
        }

        virtual void visitImportDecl(ImportDecl* decl) override
        {
            if(decl->IsChecked(DeclCheckState::Checked))
                return;

            // We need to look for a module with the specified name
            // (whether it has already been loaded, or needs to
            // be loaded), and then put its declarations into
            // the current scope.

            auto name = decl->nameToken.Content;
            auto scope = decl->scope;

            // Try to load a module matching the name
            auto importedModuleDecl = findOrImportModule(request, name, decl->nameToken.Position);

            // If we didn't find a matching module, then bail out
            if (!importedModuleDecl)
                return;

            // Record the module that was imported, so that we can use
            // it later during code generation.
            decl->importedModuleDecl = importedModuleDecl;

            importModuleIntoScope(scope.Ptr(), importedModuleDecl.Ptr());

            decl->SetCheckState(DeclCheckState::Checked);
        }
    };

    SyntaxVisitor* CreateSemanticsVisitor(
        DiagnosticSink*         err,
        CompileRequest*         request,
        TranslationUnitRequest* translationUnit)
    {
        return new SemanticsVisitor(err, request, translationUnit);
    }

    //

    // Get the type to use when referencing a declaration
    QualType getTypeForDeclRef(
        SemanticsVisitor*       sema,
        DiagnosticSink*         sink,
        DeclRef<Decl>                 declRef,
        RefPtr<ExpressionType>* outTypeResult)
    {
        if( sema )
        {
            sema->EnsureDecl(declRef.getDecl());
        }

        // We need to insert an appropriate type for the expression, based on
        // what we found.
        if (auto varDeclRef = declRef.As<VarDeclBase>())
        {
            QualType qualType;
            qualType.type = GetType(varDeclRef);
            qualType.IsLeftValue = true; // TODO(tfoley): allow explicit `const` or `let` variables
            return qualType;
        }
        else if (auto typeAliasDeclRef = declRef.As<TypeDefDecl>())
        {
            auto type = new NamedExpressionType(typeAliasDeclRef);
            *outTypeResult = type;
            return new TypeType(type);
        }
        else if (auto aggTypeDeclRef = declRef.As<AggTypeDecl>())
        {
            auto type = DeclRefType::Create(aggTypeDeclRef);
            *outTypeResult = type;
            return new TypeType(type);
        }
        else if (auto simpleTypeDeclRef = declRef.As<SimpleTypeDecl>())
        {
            auto type = DeclRefType::Create(simpleTypeDeclRef);
            *outTypeResult = type;
            return new TypeType(type);
        }
        else if (auto genericDeclRef = declRef.As<GenericDecl>())
        {
            auto type = new GenericDeclRefType(genericDeclRef);
            *outTypeResult = type;
            return new TypeType(type);
        }
        else if (auto funcDeclRef = declRef.As<CallableDecl>())
        {
            auto type = new FuncType();
            type->declRef = funcDeclRef;
            return type;
        }

        if( sink )
        {
            sink->diagnose(declRef, Diagnostics::unimplemented, "cannot form reference to this kind of declaration");
        }
        return ExpressionType::Error;
    }

    QualType getTypeForDeclRef(
        DeclRef<Decl>                 declRef)
    {
        RefPtr<ExpressionType> typeResult;
        return getTypeForDeclRef(nullptr, nullptr, declRef, &typeResult);
    }

    DeclRef<ExtensionDecl> ApplyExtensionToType(
        SemanticsVisitor*       semantics,
        ExtensionDecl*          extDecl,
        RefPtr<ExpressionType>  type)
    {
        if(!semantics)
            return DeclRef<ExtensionDecl>();

        return semantics->ApplyExtensionToType(extDecl, type);
    }

}
