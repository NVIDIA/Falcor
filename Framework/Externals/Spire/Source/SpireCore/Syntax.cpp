#include "Syntax.h"
#include "SyntaxVisitors.h"
#include <typeinfo>
#include <assert.h>

namespace Spire
{
    namespace Compiler
    {
        // BasicExpressionType

        bool BasicExpressionType::EqualsImpl(const ExpressionType * type) const
        {
            auto basicType = dynamic_cast<const BasicExpressionType*>(type);
            if (basicType == nullptr)
                return false;
            return basicType->BaseType == BaseType;
        }

        ExpressionType* BasicExpressionType::CreateCanonicalType()
        {
            // A basic type is already canonical, in our setup
            return this;
        }

        CoreLib::Basic::String BasicExpressionType::ToString() const
        {
            CoreLib::Basic::StringBuilder res;

            switch (BaseType)
            {
            case Compiler::BaseType::Int:
                res.Append("int");
                break;
            case Compiler::BaseType::UInt:
                res.Append("uint");
                break;
            case Compiler::BaseType::Bool:
                res.Append("bool");
                break;
            case Compiler::BaseType::Float:
                res.Append("float");
                break;
            case Compiler::BaseType::Void:
                res.Append("void");
                break;
            default:
                break;
            }
            return res.ProduceString();
        }

        RefPtr<SyntaxNode> ProgramSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitProgram(this);
        }

        RefPtr<SyntaxNode> FunctionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitFunction(this);
        }

        //

        RefPtr<SyntaxNode> ScopeDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitScopeDecl(this);
        }

        //

        RefPtr<SyntaxNode> BlockStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitBlockStatement(this);
        }

        RefPtr<SyntaxNode> BreakStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitBreakStatement(this);
        }

        RefPtr<SyntaxNode> ContinueStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitContinueStatement(this);
        }

        RefPtr<SyntaxNode> DoWhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitDoWhileStatement(this);
        }

        RefPtr<SyntaxNode> EmptyStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitEmptyStatement(this);
        }

        RefPtr<SyntaxNode> ForStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitForStatement(this);
        }

        RefPtr<SyntaxNode> IfStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitIfStatement(this);
        }

        RefPtr<SyntaxNode> ReturnStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitReturnStatement(this);
        }

        RefPtr<SyntaxNode> VarDeclrStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitVarDeclrStatement(this);
        }

        RefPtr<SyntaxNode> Variable::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitDeclrVariable(this);
        }

        RefPtr<SyntaxNode> WhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitWhileStatement(this);
        }

        RefPtr<SyntaxNode> ExpressionStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitExpressionStatement(this);
        }

        RefPtr<SyntaxNode> ConstantExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitConstantExpression(this);
        }

        RefPtr<SyntaxNode> IndexExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitIndexExpression(this);
        }
        RefPtr<SyntaxNode> MemberExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitMemberExpression(this);
        }

        // SwizzleExpr

        RefPtr<SyntaxNode> SwizzleExpr::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitSwizzleExpression(this);
        }

        // DerefExpr

        RefPtr<SyntaxNode> DerefExpr::Accept(SyntaxVisitor * /*visitor*/)
        {
            // throw "unimplemented";
            return this;
        }

        //

        RefPtr<SyntaxNode> InvokeExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitInvokeExpression(this);
        }

        RefPtr<SyntaxNode> TypeCastExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitTypeCastExpression(this);
        }

        RefPtr<SyntaxNode> VarExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitVarExpression(this);
        }

        // OverloadedExpr

        RefPtr<SyntaxNode> OverloadedExpr::Accept(SyntaxVisitor * /*visitor*/)
        {
//			throw "unimplemented";
            return this;
        }

        //

        RefPtr<SyntaxNode> ParameterSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitParameter(this);
        }

        // UsingFileDecl

        RefPtr<SyntaxNode> UsingFileDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitUsingFileDecl(this);
        }

        //

        RefPtr<SyntaxNode> StructField::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitStructField(this);
        }
        RefPtr<SyntaxNode> StructSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitStruct(this);
        }
        RefPtr<SyntaxNode> ClassSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitClass(this);
        }
        RefPtr<SyntaxNode> TypeDefDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitTypeDefDecl(this);
        }

        RefPtr<SyntaxNode> DiscardStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitDiscardStatement(this);
        }

        // BasicExpressionType

        BasicExpressionType* BasicExpressionType::GetScalarType() const
        {
            return const_cast<BasicExpressionType*>(this);
        }

        bool BasicExpressionType::IsIntegralImpl() const
        {
            return (BaseType == Compiler::BaseType::Int || BaseType == Compiler::BaseType::UInt || BaseType == Compiler::BaseType::Bool);
        }

        //

        bool ExpressionType::IsIntegral() const
        {
            return GetCanonicalType()->IsIntegralImpl();
        }

        bool ExpressionType::Equals(const ExpressionType * type) const
        {
            return GetCanonicalType()->EqualsImpl(type->GetCanonicalType());
        }

        bool ExpressionType::Equals(RefPtr<ExpressionType> type) const
        {
            return Equals(type.Ptr());
        }

        bool ExpressionType::EqualsVal(Val* val)
        {
            if (auto type = dynamic_cast<ExpressionType*>(val))
                return const_cast<ExpressionType*>(this)->Equals(type);
            return false;
        }

        NamedExpressionType* ExpressionType::AsNamedType() const
        {
            return dynamic_cast<NamedExpressionType*>(const_cast<ExpressionType*>(this));
        }

        RefPtr<Val> ExpressionType::SubstituteImpl(Substitutions* subst, int* ioDiff)
        {
            int diff = 0;
            auto canSubst = GetCanonicalType()->SubstituteImpl(subst, &diff);

            // If nothing changed, then don't drop any sugar that is applied
            if (!diff)
                return this;

            // If the canonical type changed, then we return a canonical type,
            // rather than try to re-construct any amount of sugar
            (*ioDiff)++;
            return canSubst;
        }


        ExpressionType* ExpressionType::GetCanonicalType() const
        {
            if (!this) return nullptr;
            ExpressionType* et = const_cast<ExpressionType*>(this);
            if (!et->canonicalType)
            {
                // TODO(tfoley): worry about thread safety here?
                et->canonicalType = et->CreateCanonicalType();
                assert(et->canonicalType);
            }
            return et->canonicalType;
        }

        BindableResourceType ExpressionType::GetBindableResourceType() const
        {
            if (auto textureType = As<TextureType>())
                return BindableResourceType::Texture;
            else if (auto samplerType = As<SamplerStateType>())
                return BindableResourceType::Sampler;
            else if(auto storageBufferType = As<StorageBufferType>())
            {
                return BindableResourceType::StorageBuffer;
            }
            else if(auto uniformBufferType = As<UniformBufferType>())
            {
                return BindableResourceType::Buffer;
            }

            return BindableResourceType::NonBindable;
        }

        bool ExpressionType::IsTextureOrSampler() const
        {
            return IsTexture() || IsSampler();
        }
        bool ExpressionType::IsStruct() const
        {
            auto declRefType = AsDeclRefType();
            if (!declRefType) return false;
            auto structDeclRef = declRefType->declRef.As<StructDeclRef>();
            if (!structDeclRef) return false;
            return true;
        }

        bool ExpressionType::IsClass() const
        {
            auto declRefType = AsDeclRefType();
            if (!declRefType) return false;
            auto classDeclRef = declRefType->declRef.As<ClassDeclRef>();
            if (!classDeclRef) return false;
            return true;
        }

#if 0
        RefPtr<ExpressionType> ExpressionType::Bool;
        RefPtr<ExpressionType> ExpressionType::UInt;
        RefPtr<ExpressionType> ExpressionType::Int;
        RefPtr<ExpressionType> ExpressionType::Float;
        RefPtr<ExpressionType> ExpressionType::Float2;
        RefPtr<ExpressionType> ExpressionType::Void;
#endif
        RefPtr<ExpressionType> ExpressionType::Error;
        RefPtr<ExpressionType> ExpressionType::Overloaded;

        Dictionary<int, RefPtr<ExpressionType>> ExpressionType::sBuiltinTypes;
        Dictionary<String, Decl*> ExpressionType::sMagicDecls;
        List<RefPtr<ExpressionType>> ExpressionType::sCanonicalTypes;

        void ExpressionType::Init()
        {
            Error = new ErrorType();
            Overloaded = new OverloadGroupType();
        }
        void ExpressionType::Finalize()
        {
            Error = nullptr;
            Overloaded = nullptr;
            // Note(tfoley): This seems to be just about the only way to clear out a List<T>
            sCanonicalTypes = List<RefPtr<ExpressionType>>();
			sBuiltinTypes = Dictionary<int, RefPtr<ExpressionType>>();
			sMagicDecls = Dictionary<String, Decl*>();
        }
        bool ArrayExpressionType::EqualsImpl(const ExpressionType * type) const
        {
            auto arrType = type->AsArrayType();
            if (!arrType)
                return false;
            return (ArrayLength == arrType->ArrayLength && BaseType->Equals(arrType->BaseType.Ptr()));
        }
        ExpressionType* ArrayExpressionType::CreateCanonicalType()
        {
            auto canonicalBaseType = BaseType->GetCanonicalType();
            auto canonicalArrayType = new ArrayExpressionType();
            sCanonicalTypes.Add(canonicalArrayType);
            canonicalArrayType->BaseType = canonicalBaseType;
            canonicalArrayType->ArrayLength = ArrayLength;
            return canonicalArrayType;
        }
        int ArrayExpressionType::GetHashCode() const
        {
            if (ArrayLength)
                return (BaseType->GetHashCode() * 16777619) ^ ArrayLength->GetHashCode();
            else
                return BaseType->GetHashCode();
        }
        CoreLib::Basic::String ArrayExpressionType::ToString() const
        {
            if (ArrayLength)
                return BaseType->ToString() + "[" + ArrayLength->ToString() + "]";
            else
                return BaseType->ToString() + "[]";
        }
        RefPtr<SyntaxNode> GenericAppExpr::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitGenericApp(this);
        }

        // DeclRefType

        String DeclRefType::ToString() const
        {
            return declRef.GetName();
        }

        int DeclRefType::GetHashCode() const
        {
            return (declRef.GetHashCode() * 16777619) ^ (int)(typeid(this).hash_code());
        }

        bool DeclRefType::EqualsImpl(const ExpressionType * type) const
        {
            if (auto declRefType = type->AsDeclRefType())
            {
                return declRef.Equals(declRefType->declRef);
            }
            return false;
        }

        ExpressionType* DeclRefType::CreateCanonicalType()
        {
            // A declaration reference is already canonical
            return this;
        }

        RefPtr<Val> DeclRefType::SubstituteImpl(Substitutions* subst, int* ioDiff)
        {
            if (!subst) return this;

            // the case we especially care about is when this type references a declaration
            // of a generic parameter, since that is what we might be substituting...
            if (auto genericTypeParamDecl = dynamic_cast<GenericTypeParamDecl*>(declRef.GetDecl()))
            {
                // search for a substitution that might apply to us
                for (auto s = subst; s; s = s->outer.Ptr())
                {
                    // the generic decl associated with the substitution list must be
                    // the generic decl that declared this parameter
                    auto genericDecl = s->genericDecl;
                    if (genericDecl != genericTypeParamDecl->ParentDecl)
                        continue;

                    int index = 0;
                    for (auto m : genericDecl->Members)
                    {
                        if (m.Ptr() == genericTypeParamDecl)
                        {
                            // We've found it, so return the corresponding specialization argument
                            (*ioDiff)++;
                            return s->args[index];
                        }
                        else if(auto typeParam = m.As<GenericTypeParamDecl>())
                        {
                            index++;
                        }
                        else if(auto valParam = m.As<GenericValueParamDecl>())
                        {
                            index++;
                        }
                        else
                        {
                        }
                    }

                }
            }


            int diff = 0;
            DeclRef substDeclRef = declRef.SubstituteImpl(subst, &diff);

            if (!diff)
                return this;

            // Re-construct the type in case we are using a specialized sub-class
            return DeclRefType::Create(substDeclRef);
        }

        static RefPtr<ExpressionType> ExtractGenericArgType(RefPtr<Val> val)
        {
            auto type = val.As<ExpressionType>();
            assert(type.Ptr());
            return type;
        }

        static RefPtr<IntVal> ExtractGenericArgInteger(RefPtr<Val> val)
        {
            auto intVal = val.As<IntVal>();
            assert(intVal.Ptr());
            return intVal;
        }

        // TODO: need to figure out how to unify this with the logic
        // in the generic case...
        DeclRefType* DeclRefType::Create(DeclRef declRef)
        {
            if (auto builtinMod = declRef.GetDecl()->FindModifier<BuiltinTypeModifier>())
            {
                auto type = new BasicExpressionType(builtinMod->tag);
                type->declRef = declRef;
                return type;
            }
            else if (auto magicMod = declRef.GetDecl()->FindModifier<MagicTypeModifier>())
            {
                Substitutions* subst = declRef.substitutions.Ptr();

                if (magicMod->name == "SamplerState")
                {
                    auto type = new SamplerStateType();
                    type->declRef = declRef;
                    type->flavor = SamplerStateType::Flavor(magicMod->tag);
                    return type;
                }
                else if (magicMod->name == "Vector")
                {
                    assert(subst && subst->args.Count() == 2);
                    auto vecType = new VectorExpressionType(
                        ExtractGenericArgType(subst->args[0]),
                        ExtractGenericArgInteger(subst->args[1]));
                    vecType->declRef = declRef;
                    return vecType;
                }
                else if (magicMod->name == "Matrix")
                {
                    assert(subst && subst->args.Count() == 3);
                    auto matType = new MatrixExpressionType(
                        ExtractGenericArgType(subst->args[0]),
                        ExtractGenericArgInteger(subst->args[1]),
                        ExtractGenericArgInteger(subst->args[2]));
                    matType->declRef = declRef;
                    return matType;
                }
                else if (magicMod->name == "Texture")
                {
                    assert(subst && subst->args.Count() >= 1);
                    auto textureType = new TextureType(
                        TextureType::Flavor(magicMod->tag),
                        ExtractGenericArgType(subst->args[0]));
                    textureType->declRef = declRef;
                    return textureType;
                }

                #define CASE(n,T)													\
                    else if(magicMod->name == #n) {									\
                        assert(subst && subst->args.Count() == 1);					\
                        auto type = new T();										\
                        type->elementType = ExtractGenericArgType(subst->args[0]);	\
                        type->declRef = declRef;									\
                        return type;												\
                    }

                CASE(ConstantBuffer, ConstantBufferType)
                CASE(TextureBuffer, TextureBufferType)

                CASE(PackedBuffer, PackedBufferType)
                CASE(Uniform, UniformBufferType)
                CASE(Patch, PatchType)

                CASE(HLSLBufferType, HLSLBufferType)
                CASE(HLSLStructuredBufferType, HLSLStructuredBufferType)
                CASE(HLSLRWBufferType, HLSLRWBufferType)
                CASE(HLSLRWStructuredBufferType, HLSLRWStructuredBufferType)
                CASE(HLSLAppendStructuredBufferType, HLSLAppendStructuredBufferType)
                CASE(HLSLConsumeStructuredBufferType, HLSLConsumeStructuredBufferType)
                CASE(HLSLInputPatchType, HLSLInputPatchType)
                CASE(HLSLOutputPatchType, HLSLOutputPatchType)

                CASE(HLSLPointStreamType, HLSLPointStreamType)
                CASE(HLSLLineStreamType, HLSLPointStreamType)
                CASE(HLSLTriangleStreamType, HLSLPointStreamType)

                #undef CASE

                // "magic" builtin types which have no generic parameters
                #define CASE(n,T)													\
                    else if(magicMod->name == #n) {									\
                        auto type = new T();										\
                        type->declRef = declRef;									\
                        return type;												\
                    }

                CASE(HLSLByteAddressBufferType, HLSLByteAddressBufferType)
                CASE(HLSLRWByteAddressBufferType, HLSLRWByteAddressBufferType)
                CASE(UntypedBufferResourceType, UntypedBufferResourceType)

                #undef CASE

                else
                {
                    throw "unimplemented";
                }
            }
            else
            {
                return new DeclRefType(declRef);
            }
        }

        // OverloadGroupType

        String OverloadGroupType::ToString() const
        {
            return "overload group";
        }

        bool OverloadGroupType::EqualsImpl(const ExpressionType * /*type*/) const
        {
            return false;
        }

        ExpressionType* OverloadGroupType::CreateCanonicalType()
        {
            return this;
        }

        int OverloadGroupType::GetHashCode() const
        {
            return (int)(int64_t)(void*)this;
        }

        // ErrorType

        String ErrorType::ToString() const
        {
            return "error";
        }

        bool ErrorType::EqualsImpl(const ExpressionType* type) const
        {
            if (auto errorType = type->As<ErrorType>())
                return true;
            return false;
        }

        ExpressionType* ErrorType::CreateCanonicalType()
        {
            return  this;
        }

        int ErrorType::GetHashCode() const
        {
            return (int)(int64_t)(void*)this;
        }


        // NamedExpressionType

        String NamedExpressionType::ToString() const
        {
            return declRef.GetName();
        }

        bool NamedExpressionType::EqualsImpl(const ExpressionType * /*type*/) const
        {
            assert(!"unreachable");
            return false;
        }

        ExpressionType* NamedExpressionType::CreateCanonicalType()
        {
            return declRef.GetType()->GetCanonicalType();
        }

        int NamedExpressionType::GetHashCode() const
        {
            assert(!"unreachable");
            return 0;
        }

        // FuncType

        String FuncType::ToString() const
        {
            // TODO: a better approach than this
            if (declRef)
                return declRef.GetName();
            else
                return "/* unknown FuncType */";
        }

        bool FuncType::EqualsImpl(const ExpressionType * type) const
        {
            if (auto funcType = type->As<FuncType>())
            {
                return declRef == funcType->declRef;
            }
            return false;
        }

        ExpressionType* FuncType::CreateCanonicalType()
        {
            return this;
        }

        int FuncType::GetHashCode() const
        {
            return declRef.GetHashCode();
        }

        // ImportOperatorGenericParamType

        String ImportOperatorGenericParamType::ToString() const
        {
            return GenericTypeVar;
        }

        bool ImportOperatorGenericParamType::EqualsImpl(const ExpressionType * type) const
        {
            if (auto genericType = type->As<ImportOperatorGenericParamType>())
            {
                // TODO(tfoley): This does not compare the shader closure,
                // because the original implementation in `BasicExpressionType`
                // didn't either. It isn't clear whether that would be right or wrong.
                return GenericTypeVar == genericType->GenericTypeVar;
            }
            return false;
        }

        ExpressionType* ImportOperatorGenericParamType::CreateCanonicalType()
        {
            return this;
        }

        // TypeExpressionType

        String TypeExpressionType::ToString() const
        {
            StringBuilder sb;
            sb << "typeof(" << type->ToString() << ")";
            return sb.ProduceString();
        }

        bool TypeExpressionType::EqualsImpl(const ExpressionType * t) const
        {
            if (auto typeType = t->AsTypeType())
            {
                return t->Equals(typeType->type);
            }
            return false;
        }

        ExpressionType* TypeExpressionType::CreateCanonicalType()
        {
            auto canType = new TypeExpressionType(type->GetCanonicalType());
            sCanonicalTypes.Add(canType);
            return canType;
        }

        int TypeExpressionType::GetHashCode() const
        {
            assert(!"unreachable");
            return 0;
        }

        // GenericDeclRefType

        String GenericDeclRefType::ToString() const
        {
            // TODO: what is appropriate here?
            return "<GenericDeclRef>";
        }

        bool GenericDeclRefType::EqualsImpl(const ExpressionType * type) const
        {
            if (auto genericDeclRefType = type->As<GenericDeclRefType>())
            {
                return declRef.Equals(genericDeclRefType->declRef);
            }
            return false;
        }

        int GenericDeclRefType::GetHashCode() const
        {
            return declRef.GetHashCode();
        }

        ExpressionType* GenericDeclRefType::CreateCanonicalType()
        {
            return this;
        }

        // ArithmeticExpressionType

        // VectorExpressionType

        String VectorExpressionType::ToString() const
        {
            StringBuilder sb;
            sb << "vector<" << elementType->ToString() << "," << elementCount->ToString() << ">";
            return sb.ProduceString();
        }

        BasicExpressionType* VectorExpressionType::GetScalarType() const
        {
            return elementType->AsBasicType();
        }

        bool VectorExpressionType::EqualsImpl(const ExpressionType * type) const
        {
            if (auto vecType = type->AsVectorType())
            {
                return elementType->Equals(vecType->elementType)
                    && elementCount->EqualsVal(vecType->elementCount.Ptr());
            }
            
            return false;
        }

        ExpressionType* VectorExpressionType::CreateCanonicalType()
        {
            auto canElementType = elementType->GetCanonicalType();
            auto canType = new VectorExpressionType(canElementType, elementCount);
            canType->declRef = declRef;
            sCanonicalTypes.Add(canType);
            return canType;
        }

        RefPtr<Val> VectorExpressionType::SubstituteImpl(Substitutions* subst, int* ioDiff)
        {
            int diff = 0;
            auto substDeclRef = declRef.SubstituteImpl(subst, &diff);
            auto substElementType = elementType->SubstituteImpl(subst, &diff).As<ExpressionType>();
            auto substElementCount = elementCount->SubstituteImpl(subst, &diff).As<IntVal>();

            if (!diff)
                return this;

            (*ioDiff)++;
            auto substType = new VectorExpressionType(substElementType, substElementCount);
            substType->declRef = substDeclRef;
            return substType;
        }


        // MatrixExpressionType

        String MatrixExpressionType::ToString() const
        {
            StringBuilder sb;
            sb << "matrix<" << elementType->ToString() << "," << rowCount->ToString() << "," << colCount->ToString() << ">";
            return sb.ProduceString();
        }

        BasicExpressionType* MatrixExpressionType::GetScalarType() const
        {
            return elementType->AsBasicType();
        }

        bool MatrixExpressionType::EqualsImpl(const ExpressionType * type) const
        {
            if (auto matType = type->AsMatrixType())
            {
                return elementType->Equals(matType->elementType)
                    && rowCount->EqualsVal(matType->rowCount.Ptr())
                    && colCount->EqualsVal(matType->colCount.Ptr());
            }
            
            return false;
        }

        ExpressionType* MatrixExpressionType::CreateCanonicalType()
        {
            auto canElementType = elementType->GetCanonicalType();
            auto canType = new MatrixExpressionType(canElementType, rowCount, colCount);
            canType->declRef = declRef;
            sCanonicalTypes.Add(canType);
            return canType;
        }


        RefPtr<Val> MatrixExpressionType::SubstituteImpl(Substitutions* subst, int* ioDiff)
        {
            int diff = 0;
            auto substDeclRef = declRef.SubstituteImpl(subst, &diff);
            auto substElementType = elementType->SubstituteImpl(subst, &diff).As<ExpressionType>();

            if (!diff)
                return this;

            (*ioDiff)++;
            auto substType = new MatrixExpressionType(substElementType, rowCount, colCount);
            substType->declRef = substDeclRef;
            return substType;
        }

        // TextureType

        RefPtr<Val> TextureType::SubstituteImpl(Substitutions* subst, int* ioDiff)
        {
            int diff = 0;
            auto substDeclRef = declRef.SubstituteImpl(subst, &diff);
            auto substElementType = elementType->SubstituteImpl(subst, &diff).As<ExpressionType>();

            if (!diff)
                return this;

            (*ioDiff)++;
            auto substType = new TextureType(flavor, substElementType);
            substType->declRef = substDeclRef;
            return substType;
        }

        //

        String GetOperatorFunctionName(Operator op)
        {
            switch (op)
            {
            case Operator::Add:
            case Operator::AddAssign:
                return "+";
            case Operator::Sub:
            case Operator::SubAssign:
                return "-";
            case Operator::Neg:
                return "-";
            case Operator::Not:
                return "!";
            case Operator::BitNot:
                return "~";
            case Operator::PreInc:
            case Operator::PostInc:
                return "++";
            case Operator::PreDec:
            case Operator::PostDec:
                return "--";
            case Operator::Mul:
            case Operator::MulAssign:
                return "*";
            case Operator::Div:
            case Operator::DivAssign:
                return "/";
            case Operator::Mod:
            case Operator::ModAssign:
                return "%";
            case Operator::Lsh:
            case Operator::LshAssign:
                return "<<";
            case Operator::Rsh:
            case Operator::RshAssign:
                return ">>";
            case Operator::Eql:
                return "==";
            case Operator::Neq:
                return "!=";
            case Operator::Greater:
                return ">";
            case Operator::Less:
                return "<";
            case Operator::Geq:
                return ">=";
            case Operator::Leq:
                return "<=";
            case Operator::BitAnd:
            case Operator::AndAssign:
                return "&";
            case Operator::BitXor:
            case Operator::XorAssign:
                return "^";
            case Operator::BitOr:
            case Operator::OrAssign:
                return "|";
            case Operator::And:
                return "&&";
            case Operator::Or:
                return "||";
            case Operator::Sequence:
                return ",";
            case Operator::Select:
                return "?:";
            default:
                return "";
            }
        }
        String OperatorToString(Operator op)
        {
            switch (op)
            {
            case Spire::Compiler::Operator::Neg:
                return "-";
            case Spire::Compiler::Operator::Not:
                return "!";
            case Spire::Compiler::Operator::PreInc:
                return "++";
            case Spire::Compiler::Operator::PreDec:
                return "--";
            case Spire::Compiler::Operator::PostInc:
                return "++";
            case Spire::Compiler::Operator::PostDec:
                return "--";
            case Spire::Compiler::Operator::Mul:
            case Spire::Compiler::Operator::MulAssign:
                return "*";
            case Spire::Compiler::Operator::Div:
            case Spire::Compiler::Operator::DivAssign:
                return "/";
            case Spire::Compiler::Operator::Mod:
            case Spire::Compiler::Operator::ModAssign:
                return "%";
            case Spire::Compiler::Operator::Add:
            case Spire::Compiler::Operator::AddAssign:
                return "+";
            case Spire::Compiler::Operator::Sub:
            case Spire::Compiler::Operator::SubAssign:
                return "-";
            case Spire::Compiler::Operator::Lsh:
            case Spire::Compiler::Operator::LshAssign:
                return "<<";
            case Spire::Compiler::Operator::Rsh:
            case Spire::Compiler::Operator::RshAssign:
                return ">>";
            case Spire::Compiler::Operator::Eql:
                return "==";
            case Spire::Compiler::Operator::Neq:
                return "!=";
            case Spire::Compiler::Operator::Greater:
                return ">";
            case Spire::Compiler::Operator::Less:
                return "<";
            case Spire::Compiler::Operator::Geq:
                return ">=";
            case Spire::Compiler::Operator::Leq:
                return "<=";
            case Spire::Compiler::Operator::BitAnd:
            case Spire::Compiler::Operator::AndAssign:
                return "&";
            case Spire::Compiler::Operator::BitXor:
            case Spire::Compiler::Operator::XorAssign:
                return "^";
            case Spire::Compiler::Operator::BitOr:
            case Spire::Compiler::Operator::OrAssign:
                return "|";
            case Spire::Compiler::Operator::And:
                return "&&";
            case Spire::Compiler::Operator::Or:
                return "||";
            case Spire::Compiler::Operator::Assign:
                return "=";
            default:
                return "ERROR";
            }
        }

        // TypeExp

        TypeExp TypeExp::Accept(SyntaxVisitor* visitor)
        {
            return visitor->VisitTypeExp(*this);
        }

        // BuiltinTypeModifier

        // MagicTypeModifier

        // GenericDecl

        RefPtr<SyntaxNode> GenericDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitGenericDecl(this);
        }

        // GenericTypeParamDecl

        RefPtr<SyntaxNode> GenericTypeParamDecl::Accept(SyntaxVisitor * /*visitor*/) {
            //throw "unimplemented";
            return this;
        }

        // GenericTypeConstraintDecl

        RefPtr<SyntaxNode> GenericTypeConstraintDecl::Accept(SyntaxVisitor * visitor)
        {
            return this;
        }

        // GenericValueParamDecl

        RefPtr<SyntaxNode> GenericValueParamDecl::Accept(SyntaxVisitor * /*visitor*/) {
            //throw "unimplemented";
            return this;
        }

        // GenericParamIntVal

        bool GenericParamIntVal::EqualsVal(Val* val)
        {
            if (auto genericParamVal = dynamic_cast<GenericParamIntVal*>(val))
            {
                return declRef.Equals(genericParamVal->declRef);
            }
            return false;
        }

        String GenericParamIntVal::ToString() const
        {
            return declRef.GetName();
        }

        int GenericParamIntVal::GetHashCode() const
        {
            return declRef.GetHashCode() ^ 0xFFFF;
        }

        RefPtr<Val> GenericParamIntVal::SubstituteImpl(Substitutions* subst, int* ioDiff)
        {
            // search for a substitution that might apply to us
            for (auto s = subst; s; s = s->outer.Ptr())
            {
                // the generic decl associated with the substitution list must be
                // the generic decl that declared this parameter
                auto genericDecl = s->genericDecl;
                if (genericDecl != declRef.GetDecl()->ParentDecl)
                    continue;

                int index = 0;
                for (auto m : genericDecl->Members)
                {
                    if (m.Ptr() == declRef.GetDecl())
                    {
                        // We've found it, so return the corresponding specialization argument
                        (*ioDiff)++;
                        return s->args[index];
                    }
                    else if(auto typeParam = m.As<GenericTypeParamDecl>())
                    {
                        index++;
                    }
                    else if(auto valParam = m.As<GenericValueParamDecl>())
                    {
                        index++;
                    }
                    else
                    {
                    }
                }
            }

            // Nothing found: don't substittue.
            return this;
        }

        // ExtensionDecl

        RefPtr<SyntaxNode> ExtensionDecl::Accept(SyntaxVisitor * visitor)
        {
            visitor->VisitExtensionDecl(this);
            return this;
        }

        // ConstructorDecl

        RefPtr<SyntaxNode> ConstructorDecl::Accept(SyntaxVisitor * visitor)
        {
            visitor->VisitConstructorDecl(this);
            return this;
        }

        // Substitutions

        RefPtr<Substitutions> Substitutions::SubstituteImpl(Substitutions* subst, int* /*ioDiff*/)
        {
            if (!this) return nullptr;

            int diff = 0;
            auto outerSubst = outer->SubstituteImpl(subst, &diff);

            List<RefPtr<Val>> substArgs;
            for (auto a : args)
            {
                substArgs.Add(a->SubstituteImpl(subst, &diff));
            }

            if (!diff) return this;

            auto substSubst = new Substitutions();
            substSubst->genericDecl = genericDecl;
            substSubst->args = substArgs;
            return substSubst;
        }

        bool Substitutions::Equals(Substitutions* subst)
        {
            // both must be NULL, or non-NULL
            if (!this || !subst)
                return !this && !subst;

            if (genericDecl != subst->genericDecl)
                return false;

            int argCount = args.Count();
            assert(args.Count() == subst->args.Count());
            for (int aa = 0; aa < argCount; ++aa)
            {
                if (!args[aa]->EqualsVal(subst->args[aa].Ptr()))
                    return false;
            }

            if (!outer->Equals(subst->outer.Ptr()))
                return false;

            return true;
        }


        // DeclRef

        RefPtr<ExpressionType> DeclRef::Substitute(RefPtr<ExpressionType> type) const
        {
            // No substitutions? Easy.
            if (!substitutions)
                return type;

            // Otherwise we need to recurse on the type structure
            // and apply substitutions where it makes sense

            return type->Substitute(substitutions.Ptr()).As<ExpressionType>();
        }

        DeclRef DeclRef::Substitute(DeclRef declRef) const
        {
            if(!substitutions)
                return declRef;

            int diff = 0;
            return declRef.SubstituteImpl(substitutions.Ptr(), &diff);
        }

        RefPtr<ExpressionSyntaxNode> DeclRef::Substitute(RefPtr<ExpressionSyntaxNode> expr) const
        {
            // No substitutions? Easy.
            if (!substitutions)
                return expr;

            assert(!"unimplemented");

            return expr;
        }


        DeclRef DeclRef::SubstituteImpl(Substitutions* subst, int* /*ioDiff*/)
        {
            if (!substitutions) return *this;

            int diff = 0;
            RefPtr<Substitutions> substSubst = substitutions->SubstituteImpl(subst, &diff);

            if (!diff)
                return *this;

            DeclRef substDeclRef;
            substDeclRef.decl = decl;
            substDeclRef.substitutions = substSubst;
            return substDeclRef;
        }


        // Check if this is an equivalent declaration reference to another
        bool DeclRef::Equals(DeclRef const& declRef) const
        {
            if (decl != declRef.decl)
                return false;

            if (!substitutions->Equals(declRef.substitutions.Ptr()))
                return false;

            return true;
        }

        // Convenience accessors for common properties of declarations
        String const& DeclRef::GetName() const
        {
            return decl->Name.Content;
        }

        DeclRef DeclRef::GetParent() const
        {
            auto parentDecl = decl->ParentDecl;
            if (auto parentGeneric = dynamic_cast<GenericDecl*>(parentDecl))
            {
                // We need to strip away one layer of specialization
                assert(substitutions);
                return DeclRef(parentGeneric, substitutions->outer);
            }
            else
            {
                // If the parent isn't a generic, then it must
                // use the same specializations as this declaration
                return DeclRef(parentDecl, substitutions);
            }

        }

        int DeclRef::GetHashCode() const
        {
            auto rs = PointerHash<1>::GetHashCode(decl.Ptr());
            if (substitutions)
            {
                rs *= 16777619;
                rs ^= substitutions->GetHashCode();
            }
            return rs;
        }

        // Val

        RefPtr<Val> Val::Substitute(Substitutions* subst)
        {
            if (!this) return nullptr;
            if (!subst) return this;
            int diff = 0;
            return SubstituteImpl(subst, &diff);
        }

        RefPtr<Val> Val::SubstituteImpl(Substitutions* /*subst*/, int* /*ioDiff*/)
        {
            // Default behavior is to not substitute at all
            return this;
        }

        // IntVal

        int GetIntVal(RefPtr<IntVal> val)
        {
            if (auto constantVal = val.As<ConstantIntVal>())
            {
                return constantVal->value;
            }
            assert(!"unexpected");
            return 0;
        }

        // ConstantIntVal

        bool ConstantIntVal::EqualsVal(Val* val)
        {
            if (auto intVal = dynamic_cast<ConstantIntVal*>(val))
                return value == intVal->value;
            return false;
        }

        String ConstantIntVal::ToString() const
        {
            return String(value);
        }

        int ConstantIntVal::GetHashCode() const
        {
            return value;
        }

        // SwitchStmt

        RefPtr<SyntaxNode> SwitchStmt::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitSwitchStmt(this);
        }

        RefPtr<SyntaxNode> CaseStmt::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitCaseStmt(this);
        }

        RefPtr<SyntaxNode> DefaultStmt::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitDefaultStmt(this);
        }

        // TraitDecl

        RefPtr<SyntaxNode> TraitDecl::Accept(SyntaxVisitor * visitor)
        {
            visitor->VisitTraitDecl(this);
            return this;
        }

        // TraitConformanceDecl

        RefPtr<SyntaxNode> TraitConformanceDecl::Accept(SyntaxVisitor * visitor)
        {
            visitor->VisitTraitConformanceDecl(this);
            return this;
        }

        // SharedTypeExpr

        RefPtr<SyntaxNode> SharedTypeExpr::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitSharedTypeExpr(this);
        }

        // OperatorExpressionSyntaxNode

        void OperatorExpressionSyntaxNode::SetOperator(ContainerDecl * scope, Spire::Compiler::Operator op)
        {
            this->Operator = op;
            auto opExpr = new VarExpressionSyntaxNode();
            opExpr->Variable = GetOperatorFunctionName(Operator);
            opExpr->scope = scope;
            opExpr->Position = this->Position;
            this->FunctionExpr = opExpr;
        }

        RefPtr<SyntaxNode> OperatorExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitOperatorExpression(this);
        }

        // DeclGroup

        RefPtr<SyntaxNode> DeclGroup::Accept(SyntaxVisitor * visitor)
        {
            visitor->VisitDeclGroup(this);
            return this;
        }

        //

        void RegisterBuiltinDecl(
            RefPtr<Decl>                decl,
            RefPtr<BuiltinTypeModifier> modifier)
        {
            auto type = DeclRefType::Create(DeclRef(decl.Ptr(), nullptr));
            ExpressionType::sBuiltinTypes[(int)modifier->tag] = type;
        }

        void RegisterMagicDecl(
            RefPtr<Decl>                decl,
            RefPtr<MagicTypeModifier>   modifier)
        {
            ExpressionType::sMagicDecls[decl->Name.Content] = decl.Ptr();
        }

        ExpressionType* ExpressionType::GetBool()
        {
            return sBuiltinTypes[(int)BaseType::Bool].GetValue().Ptr();
        }

        ExpressionType* ExpressionType::GetFloat()
        {
            return sBuiltinTypes[(int)BaseType::Float].GetValue().Ptr();
        }

        ExpressionType* ExpressionType::GetInt()
        {
            return sBuiltinTypes[(int)BaseType::Int].GetValue().Ptr();
        }

        ExpressionType* ExpressionType::GetUInt()
        {
            return sBuiltinTypes[(int)BaseType::UInt].GetValue().Ptr();
        }

        ExpressionType* ExpressionType::GetVoid()
        {
            return sBuiltinTypes[(int)BaseType::Void].GetValue().Ptr();
        }

        ExpressionType* ExpressionType::GetError()
        {
            return ExpressionType::Error.Ptr();
        }

        //

        RefPtr<SyntaxNode> UnparsedStmt::Accept(SyntaxVisitor * visitor)
        {
            return this;
        }

        //

        RefPtr<SyntaxNode> InitializerListExpr::Accept(SyntaxVisitor * visitor)
        {
            // TODO(tfoley): This case obviously needs to be implemented...
            return this;
        }


    }
}