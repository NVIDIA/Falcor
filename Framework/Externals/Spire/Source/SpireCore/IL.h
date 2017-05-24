#ifndef RASTER_RENDERER_IL_H
#define RASTER_RENDERER_IL_H

#include "../CoreLib/Basic.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
    namespace Compiler
    {
        using CoreLib::Text::CodePosition;

        using namespace CoreLib::Basic;
        enum class ILBaseType
        {
            Void = 0,
            Bool,
            Int,
            UInt,
            UInt64,
            Float,
        };
        int SizeofBaseType(ILBaseType type);
        int RoundToAlignment(int offset, int alignment);
        extern int NamingCounter;

        enum class BindableResourceType
        {
            NonBindable, Texture, Sampler, Buffer, StorageBuffer
        };
        int GetMaxResourceBindings(BindableResourceType type);

        class ILBasicType;
        class ILVectorType;
        class ILMatrixType;
        class ILTextureType;
        class ILPointerLikeType;
        class ILArrayLikeType;

        class ILType : public RefObject
        {
        public:
            bool IsBool();
            bool IsInt();
            bool IsUInt();
            bool IsIntegral();
            bool IsFloat();
            bool IsVoid();
            bool IsScalar()
            {
                return IsInt() || IsUInt() || IsFloat() || IsBool();
            }
            bool IsBoolVector(); 
            bool IsIntVector();
            bool IsUIntVector();
            bool IsFloatVector();
            bool IsFloatMatrix();
            bool IsVector()
            {
                return IsIntVector() || IsUIntVector() || IsFloatVector() || IsBoolVector();
            }
            bool IsTexture();
            bool IsSamplerState();
            bool IsNonShadowTexture();

            ILBasicType* AsBasicType();
            ILVectorType* AsVectorType();
            ILMatrixType* AsMatrixType();
            ILTextureType* AsTextureType();
            ILPointerLikeType* AsPointerLikeType();
            ILArrayLikeType* AsArrayLikeType();

            virtual BindableResourceType GetBindableResourceType() = 0;
            virtual ILType * Clone() = 0;
            virtual String ToString() = 0;
            virtual bool Equals(ILType* type) = 0;
            virtual void Serialize(StringBuilder & sb) = 0;
            static RefPtr<ILType> Deserialize(CoreLib::Text::TokenReader & reader);
        };

        class ILObjectDefinition
        {
        public:
            RefPtr<ILType> Type;
            String Name;
            EnumerableDictionary<String, CoreLib::Text::Token> Attributes;
            CodePosition Position;
            int Binding = -1;
        };

        class ILRecordType : public ILType
        {
        public:
            String TypeName;
            EnumerableDictionary<String, ILObjectDefinition> Members;
            virtual ILType * Clone() override;
            virtual String ToString() override;
            virtual bool Equals(ILType* type) override;
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "record " << TypeName;
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::NonBindable;
            }
        };

        const char * ILBaseTypeToString(ILBaseType type);

        class ILBasicType : public ILType
        {
        public:
            ILBaseType Type;
            ILBasicType()
            {
                Type = ILBaseType::Int;
            }
            ILBasicType(ILBaseType t)
            {
                Type = t;
            }
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILBasicType*>(type);
                if (!btype)
                    return false;
                return Type == btype->Type;
            }

            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::NonBindable;
            }

            virtual ILType * Clone() override
            {
                auto rs = new ILBasicType();
                rs->Type = Type;
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "basic " << ToString();
            }
            virtual String ToString() override
            {
                return ILBaseTypeToString(Type);
            }
        };

        class ILVectorType : public ILType
        {
        public:
            ILBaseType BaseType = ILBaseType::Float;
            int Size = 4;
            ILVectorType() = default;
            ILVectorType(ILBaseType baseType, int size)
            {
                BaseType = baseType;
                Size = size;
            }
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILVectorType*>(type);
                if (!btype)
                    return false;
                return BaseType == btype->BaseType;
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILVectorType();
                rs->BaseType = BaseType;
                rs->Size = Size;
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "vector<";
                sb << ILBaseTypeToString(BaseType);
                sb << ", " << Size << ">";
            }
            virtual String ToString() override
            {
                return ILBaseTypeToString(BaseType) + String(Size);
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::NonBindable;
            }
        };

        class ILMatrixType : public ILType
        {
        public:
            ILBaseType BaseType = ILBaseType::Float;
            int Size[2] = { 4, 4 };
            ILMatrixType() = default;
            ILMatrixType(ILBaseType baseType, int size0, int size1)
            {
                BaseType = baseType;
                Size[0] = size0;
                Size[1] = size1;
            }
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILVectorType*>(type);
                if (!btype)
                    return false;
                return BaseType == btype->BaseType;
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILMatrixType();
                rs->BaseType = BaseType;
                rs->Size[0] = Size[0];
                rs->Size[1] = Size[1];
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "matrix<";
                sb << ILBaseTypeToString(BaseType);
                sb << ", " << Size[0] << ", " << Size[1] << ">";
            }
            virtual String ToString() override
            {
                return (StringBuilder() << ILBaseTypeToString(BaseType) << Size[0] << "x" << Size[1]).ProduceString();
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::NonBindable;
            }
        };

        enum class ILTextureShape : uint8_t
        {
            Texture1D = 1, Texture2D = 2, Texture3D = 3, TextureCube = 4
        };

        const char * ILTextureShapeToString(ILTextureShape shape);

        union ILTextureFlavor
        {
            struct TextureFlavorBits
            {
                ILTextureShape Shape : 3;
                bool IsMultisample : 1;
                bool IsArray : 1;
                bool IsShadow : 1;
            } Fields;
            uint32_t Bits;
        };

        class ILTextureType : public ILType
        {
        public:
            ILTextureFlavor Flavor;
            RefPtr<ILType> BaseType;
            ILTextureType()
            {
                Flavor.Fields.IsArray = false;
                Flavor.Fields.IsMultisample = false;
                Flavor.Fields.IsShadow = false;
                Flavor.Fields.Shape = ILTextureShape::Texture2D;
            }
			ILTextureType(RefPtr<ILType> baseType, ILTextureShape shape, bool isMultisample, bool isArray, bool isShadow)
			{
				Flavor.Fields.Shape = shape;
				Flavor.Fields.IsMultisample = isMultisample;
				Flavor.Fields.IsArray = isArray;
				Flavor.Fields.IsShadow = isShadow;
				BaseType = baseType;
			}
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILTextureType*>(type);
                if (!btype)
                    return false;
                return Flavor.Bits == btype->Flavor.Bits && BaseType && BaseType->Equals(btype->BaseType.Ptr());
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILTextureType();
                rs->BaseType = BaseType->Clone();
                rs->Flavor = Flavor;
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "texture<";
                BaseType->Serialize(sb);
                sb << ", ";
                sb << (unsigned int)Flavor.Bits << ">";
            }
            virtual String ToString() override
            {
                StringBuilder sb;
                sb << ILTextureShapeToString(Flavor.Fields.Shape);
                if (Flavor.Fields.IsMultisample)
                    sb << "MS";
                if (Flavor.Fields.IsArray)
                    sb << "Array";
                if (Flavor.Fields.IsShadow)
                    sb << "Shadow";
                if (!BaseType->IsFloatVector() || BaseType->AsVectorType()->Size != 4)
                    sb << "<" << BaseType->ToString() << ">";
                return sb.ProduceString();
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::Texture;
            }
        };

        enum ILArrayLikeTypeName
        {
            Buffer, RWBuffer, StructuredBuffer, RWStructuredBuffer
        };

        enum ILPointerLikeTypeName
        {
            ConstantBuffer
        };

        class ILArrayLikeType : public ILType
        {
        public:
            RefPtr<ILType> BaseType;
            ILArrayLikeTypeName Name;
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILArrayLikeType*>(type);
                if (!btype)
                    return false;
                return BaseType->Equals(btype->BaseType.Ptr());
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILArrayLikeType();
                rs->BaseType = BaseType->Clone();
                rs->Name = Name;
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "array_like(";
                BaseType->Serialize(sb);
                sb << ", " << (int)Name << ")";
            }
            virtual String ToString() override
            {
                StringBuilder sb;
                switch (Name)
                {
                case ILArrayLikeTypeName::Buffer:
                    sb << "Buffer";
                    break;
                case ILArrayLikeTypeName::RWBuffer:
                    sb << "RWBuffer";
                    break;
                case ILArrayLikeTypeName::RWStructuredBuffer:
                    sb << "RWStructuredBuffer";
                    break;
                case ILArrayLikeTypeName::StructuredBuffer:
                    sb << "StructuredBuffer";
                    break;
                default:
                    sb << "unknown";
                    break;
                }
                sb << "<" << BaseType->ToString() << ">";
                return sb.ProduceString();
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::StorageBuffer;
            }
        };

        class ILPointerLikeType : public ILType
        {
        public:
            RefPtr<ILType> BaseType;
            ILPointerLikeTypeName Name;
            ILPointerLikeType() = default;
            ILPointerLikeType(ILPointerLikeTypeName name, RefPtr<ILType> baseType)
            {
                Name = name;
                BaseType = baseType;
            }
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILPointerLikeType*>(type);
                if (!btype)
                    return false;
                return BaseType->Equals(btype->BaseType.Ptr());
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILPointerLikeType();
                rs->BaseType = BaseType->Clone();
                rs->Name = Name;
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "ptr_like(";
                BaseType->Serialize(sb);
                sb << ", " << (int)Name << ")";
            }
            virtual String ToString() override
            {
                StringBuilder sb;
                switch (Name)
                {
                case ILPointerLikeTypeName::ConstantBuffer:
                    sb << "ConstantBuffer";
                    break;
                default:
                    sb << "unknown";
                    break;
                }
                sb << "<" << BaseType->ToString() << ">";
                return sb.ProduceString();
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::Buffer;
            }
        };

        class ILArrayType : public ILType
        {
        public:
            RefPtr<ILType> BaseType;
            int ArrayLength;
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILArrayType*>(type);
                if (!btype)
                    return false;
                return BaseType->Equals(btype->BaseType.Ptr());
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILArrayType();
                rs->BaseType = BaseType->Clone();
                rs->ArrayLength = ArrayLength;
                return rs;
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "array(";
                BaseType->Serialize(sb);
                sb << ", " << ArrayLength << ")";
            }
            virtual String ToString() override
            {
                if (ArrayLength > 0)
                    return BaseType->ToString() + "[" + String(ArrayLength) + "]";
                else
                    return BaseType->ToString() + "[]";
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::NonBindable;
            }
        };

        class ILGenericType : public ILType
        {
        public:
            RefPtr<ILType> BaseType;
            String GenericTypeName;
            virtual bool Equals(ILType* type) override
            {
                auto btype = dynamic_cast<ILArrayType*>(type);
                if (!btype)
                    return false;
                return BaseType->Equals(btype->BaseType.Ptr());;
            }
            virtual ILType * Clone() override
            {
                auto rs = new ILGenericType();
                rs->BaseType = BaseType->Clone();
                rs->GenericTypeName = GenericTypeName;
                return rs;
            }
            virtual String ToString() override
            {
                return GenericTypeName + "<" + BaseType->ToString() + ">";
            }
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "generic " << GenericTypeName << "(";
                BaseType->Serialize(sb);
                sb << ")";
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                if (GenericTypeName == "StructuredBuffer" || GenericTypeName == "RWStructuredBuffer")
                    return BindableResourceType::StorageBuffer;
                else if (GenericTypeName == "Buffer" || GenericTypeName == "RWBuffer" || GenericTypeName == "ByteAddressBuffer" ||
                    GenericTypeName == "RWByteAddressBuffer")
                    return BindableResourceType::Buffer;
                return BindableResourceType::NonBindable;
            }
        };

        class ILStructType : public ILType
        {
        public:
            String TypeName;
            bool IsIntrinsic = false;
            class ILStructField
            {
            public:
                RefPtr<ILType> Type;
                String FieldName;
            };
            List<ILStructField> Members;
            virtual ILType * Clone() override;
            virtual String ToString() override;
            virtual bool Equals(ILType * type) override;
            virtual void Serialize(StringBuilder & sb) override
            {
                sb << "struct " << TypeName << "(";
                for (auto & member : Members)
                {
                    sb << member.FieldName << ":";
                    member.Type->Serialize(sb);
                    sb << "; ";
                }
                sb << ")";
            }
            virtual BindableResourceType GetBindableResourceType() override
            {
                return BindableResourceType::NonBindable;
            }
        };

		class ILSamplerStateType : public ILType
		{
		public:
			bool IsComparison = false;
			virtual ILType * Clone() override;
			virtual String ToString() override;
			virtual bool Equals(ILType * type) override;
			virtual void Serialize(StringBuilder & sb) override
			{
				sb << "SamplerState(" << (IsComparison?"1":"0") << ")";
			}
			virtual BindableResourceType GetBindableResourceType() override
			{
				return BindableResourceType::Sampler;
			}
            ILSamplerStateType() = default;
            ILSamplerStateType(bool isComparison)
                : IsComparison(isComparison)
            {}
		};
        class ILOperand;

        class UserReferenceSet
        {
        private:
            EnumerableDictionary<ILOperand*, int> userRefCounts;
            int count;
        public:
            UserReferenceSet()
            {
                count = 0;
            }
            int Count()
            {
                return count;
            }
            int GetUseCount(ILOperand * op)
            {
                int rs = -1;
                userRefCounts.TryGetValue(op, rs);
                return rs;
            }
            void Add(ILOperand * user)
            {
                this->count++;
                int ncount = 0;
                if (userRefCounts.TryGetValue(user, ncount))
                {
                    ncount++;
                    userRefCounts[user] = ncount;
                }
                else
                {
                    userRefCounts.Add(user, 1);
                }
            }
            void Remove(ILOperand * user)
            {
                int ncount = 0;
                if (userRefCounts.TryGetValue(user, ncount))
                {
                    this->count--;
                    ncount--;
                    if (ncount)
                        userRefCounts[user] = ncount;
                    else
                        userRefCounts.Remove(user);
                }
            }
            void RemoveAll(ILOperand * user)
            {
                int ncount = 0;
                if (userRefCounts.TryGetValue(user, ncount))
                {
                    this->count -= ncount;
                    userRefCounts.Remove(user);
                }
            }
            class UserIterator
            {
            private:
                EnumerableDictionary<ILOperand*, int>::Iterator iter;
            public:
                ILOperand * operator *()
                {
                    return iter.Current->Value.Key;
                }
                ILOperand ** operator ->()
                {
                    return &iter.Current->Value.Key;
                }
                UserIterator & operator ++()
                {
                    iter++;
                    return *this;
                }
                UserIterator operator ++(int)
                {
                    UserIterator rs = *this;
                    operator++();
                    return rs;
                }
                bool operator != (const UserIterator & _that)
                {
                    return iter != _that.iter;
                }
                bool operator == (const UserIterator & _that)
                {
                    return iter == _that.iter;
                }
                UserIterator(const EnumerableDictionary<ILOperand*, int>::Iterator & iter)
                {
                    this->iter = iter;
                }
                UserIterator()
                {
                }
            };
            UserIterator begin()
            {
                return UserIterator(userRefCounts.begin());
            }
            UserIterator end()
            {
                return UserIterator(userRefCounts.end());
            }
        };

        class ILOperand : public Object
        {
        public:
            String Name;
            RefPtr<ILType> Type;
            UserReferenceSet Users;
            String Attribute;
            void * Tag;
            CodePosition Position;
            union VMFields
            {
                void * VMData;
                struct Fields
                {
                    int VMDataWords[2];
                } Fields;
            } VMFields;
            Procedure<ILOperand*> OnDelete;
            ILOperand()
            {
                Tag = nullptr;
            }
            ILOperand(const ILOperand & op)
            {
                Tag = op.Tag;
                Name = op.Name;
                Attribute = op.Attribute;
                if (op.Type)
                    Type = op.Type->Clone();
                //Users = op.Users;
            }
            virtual ~ILOperand()
            {
                OnDelete(this);
            }
            virtual String ToString()
            {
                return "<operand>";
            }
            virtual bool IsUndefined()
            {
                return false;
            }
        };

        class ILUndefinedOperand : public ILOperand
        {
        public:
            ILUndefinedOperand()
            {
                Name = "<undef>";
            }
            virtual String ToString() override
            {
                return "<undef>";
            }
            virtual bool IsUndefined() override
            {
                return true;
            }
        };

        class UseReference
        {
        private:
            ILOperand * user;
            ILOperand * reference;
        public:
            UseReference()
                : user(0), reference(0)
            {}
            UseReference(const UseReference &)
            {
                user = 0;
                reference = 0;
            }
            UseReference(ILOperand * user)
                : user(user), reference(0)
            {}
            UseReference(ILOperand * user, ILOperand * ref)
            {
                this->user = user;
                this->reference = ref;
            }
            ~UseReference()
            {
                if (reference)
                    reference->Users.Remove(user);
            }
            void SetUser(ILOperand * _user)
            {
                this->user = _user;
            }
            void operator = (const UseReference & ref)
            {
                if (reference)
                    reference->Users.Remove(user);
                reference = ref.Ptr();
                if (ref.Ptr())
                {
                    if (!user)
                        throw InvalidOperationException("user not initialized.");
                    ref.Ptr()->Users.Add(user);
                }
            }
            void operator = (ILOperand * newRef)
            {
                if (reference)
                    reference->Users.Remove(user);
                reference = newRef;
                if (newRef)
                {
                    if (!user)
                        throw InvalidOperationException("user not initialized.");
                    newRef->Users.Add(user);
                }
            }
            bool operator != (const UseReference & _that)
            {
                return reference != _that.reference || user != _that.user;
            }
            bool operator == (const UseReference & _that)
            {
                return reference == _that.reference && user == _that.user;
            }
            ILOperand * Ptr() const
            {
                return reference;
            }
            ILOperand * operator->()
            {
                return reference;
            }
            ILOperand & operator*()
            {
                return *reference;
            }
            explicit operator bool()
            {
                return (reference != 0);
            }
            String ToString()
            {
                if (reference)
                    return reference->Name;
                else
                    return "<null>";
            }
        };

        class OperandIterator
        {
        private:
            UseReference * use;
        public:
            OperandIterator()
            {
                use = 0;
            }
            OperandIterator(UseReference * use)
                : use(use)
            {}
            ILOperand & operator *()
            {
                return use->operator*();
            }
            ILOperand * operator ->()
            {
                return use->operator->();
            }
            void Set(ILOperand * user, ILOperand * op)
            {
                (*use).SetUser(user);
                (*use) = op;
            }
            void Set(ILOperand * op)
            {
                (*use) = op; 
            }
            OperandIterator & operator ++()
            {
                use++;
                return *this;
            }
            OperandIterator operator ++(int)
            {
                OperandIterator rs = *this;
                operator++();
                return rs;
            }
            bool operator != (const OperandIterator & _that)
            {
                return use != _that.use;
            }
            bool operator == (const OperandIterator & _that)
            {
                return use == _that.use;
            }
            bool operator == (const ILOperand * op)
            {
                return use->Ptr() == op;
            }
            bool operator != (const ILOperand * op)
            {
                return use->Ptr() != op;
            }
        };

        class ILConstOperand : public ILOperand
        {
        public:
            union
            {
                int IntValues[16];
                float FloatValues[16];
            };
            virtual String ToString() override
            {
                if (Type->IsFloat())
                    return String(FloatValues[0]) + "f";
                else if (Type->IsInt())
                    return String(IntValues[0]);
                else if (auto baseType = dynamic_cast<ILVectorType*>(Type.Ptr()))
                {
                    StringBuilder sb(256);
                    sb << baseType->ToString() << "(";
                    for (int i = 0; i < baseType->Size; i++)
                    {
                        if (baseType->BaseType == ILBaseType::Float)
                        {
                            sb << FloatValues[i];
                            sb << "f";
                        }
                        else if (baseType->BaseType == ILBaseType::UInt)
                        {
                            sb << IntValues[i];
                            sb << "u";
                        }
                        else if (baseType->BaseType == ILBaseType::Bool)
                        {
                            sb << (IntValues[i] == 0 ? "false" : "true");
                        }
                        if (i != baseType->Size - 1)
                            sb << ", ";
                    }
                    sb << ")";
                    return sb.ProduceString();
                }
                else
                    throw InvalidOperationException("Illegal constant.");
            }
        };

        class InstructionVisitor;

        class CFGNode;

        class ILInstruction : public ILOperand
        {
        private:
            ILInstruction *next, *prev;
        public:
            CFGNode * Parent;
            ILInstruction()
            {
                next = 0;
                prev = 0;
                Parent = 0;
            }
            ILInstruction(const ILInstruction & instr)
                : ILOperand(instr)
            {
                next = 0;
                prev = 0;
                Parent = 0;
            }
            ~ILInstruction()
            {
                
            }
            virtual ILInstruction * Clone()
            {
                return new ILInstruction(*this);
            }

            virtual String GetOperatorString()
            {
                return "<instruction>";
            }
            virtual bool HasSideEffect()
            {
                return false;
            }
            virtual bool IsDeterministic()
            {
                return true;
            }
            virtual void Accept(InstructionVisitor *)
            {
            }
            void InsertBefore(ILInstruction * instr)
            {
                instr->Parent = Parent;
                instr->prev = prev;
                instr->next = this;
                prev = instr;
                auto *npp = instr->prev;
                if (npp)
                    npp->next = instr;
            }
            void InsertAfter(ILInstruction * instr)
            {
                instr->Parent = Parent;
                instr->prev = this;
                instr->next = this->next;
                next = instr;
                auto *npp = instr->next;
                if (npp)
                    npp->prev = instr;
            }
            ILInstruction * GetNext()
            {
                return next;
            }
            ILInstruction * GetPrevious()
            {
                return prev;
            }
            void Remove()
            {
                if (prev)
                    prev->next = next;
                if (next)
                    next->prev = prev;
            }
            void Erase()
            {
                Remove();
                if (Users.Count())
                {
                    throw InvalidOperationException("All uses must be removed before removing this instruction");
                }
                delete this;
            }
            virtual OperandIterator begin()
            {
                return OperandIterator();
            }
            virtual OperandIterator end()
            {
                return OperandIterator();
            }
            virtual int GetSubBlockCount()
            {
                return 0;
            }
            virtual CFGNode * GetSubBlock(int)
            {
                return nullptr;
            }
            template<typename T>
            T * As()
            {
                return dynamic_cast<T*>(this);
            }
            template<typename T>
            bool Is()
            {
                return dynamic_cast<T*>(this) != 0;
            }
        };

        template <typename T, typename TOperand>
        bool Is(TOperand * op)
        {
            auto ptr = dynamic_cast<T*>(op);
            if (ptr)
                return true;
            else
                return false;
        }

        class SwitchInstruction : public ILInstruction
        {
        public:
            List<UseReference> Candidates;
            virtual OperandIterator begin() override
            {
                return Candidates.begin();
            }
            virtual OperandIterator end() override
            {
                return Candidates.end();
            }
            virtual String ToString() override
            {
                StringBuilder sb(256);
                sb << Name;
                sb << " = switch ";
                for (auto & op : Candidates)
                {
                    sb << op.ToString();
                    if (op != Candidates.Last())
                        sb << ", ";
                }
                return sb.ProduceString();
            }
            virtual String GetOperatorString() override
            {
                return "switch";
            }
            virtual bool HasSideEffect() override
            {
                return false;
            }
            SwitchInstruction(int argSize)
            {
                Candidates.SetSize(argSize);
                for (auto & use : Candidates)
                    use.SetUser(this);
            }
            SwitchInstruction(const SwitchInstruction & other)
                : ILInstruction(other)
            {
                Candidates.SetSize(other.Candidates.Count());
                for (int i = 0; i < other.Candidates.Count(); i++)
                {
                    Candidates[i].SetUser(this);
                    Candidates[i] = other.Candidates[i].Ptr();
                }

            }
            virtual SwitchInstruction * Clone() override
            {
                return new SwitchInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class LeaInstruction : public ILInstruction
        {};

        class AllocVarInstruction : public LeaInstruction
        {
        public:
            AllocVarInstruction(ILType * type)
            {
                this->Type = type;
            }
            AllocVarInstruction(RefPtr<ILType> & type)
            {
                auto ptrType = type->Clone();
                if (!type)
                    throw ArgumentException("type cannot be null.");
                this->Type = ptrType;
            }
            AllocVarInstruction(const AllocVarInstruction & other)
                :LeaInstruction(other)
            {
            }
            virtual bool IsDeterministic() override
            {
                return false;
            }
            virtual String ToString() override
            {
                return Name + " = VAR " + Type->ToString();
            }
            virtual String GetOperatorString() override
            {
                return "avar";
            }
            virtual AllocVarInstruction * Clone() override
            {
                return new AllocVarInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };


		// TODO(tfoley): Only used by IL at this point
		enum class ParameterQualifier
		{
			In, Out, InOut, Uniform
		};

        class FetchArgInstruction : public LeaInstruction
        {
        public:
            int ArgId;
			ParameterQualifier Qualifier;
            FetchArgInstruction(RefPtr<ILType> type)
            {
                this->Type = type;
                ArgId = 0;
            }
            virtual String ToString() override
            {
                return Name + " = ARG " + Type->ToString();
            }
            virtual String GetOperatorString() override
            {
                return "arg " + String(ArgId);
            }
            virtual bool IsDeterministic() override
            {
                return false;
            }
            virtual FetchArgInstruction * Clone() override
            {
                return new FetchArgInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class CFGNode;

        class AllInstructionsIterator
        {
        private:
            struct StackItem
            {
                ILInstruction* instr;
                int subBlockPtr;
            };
            List<StackItem> stack;
            ILInstruction * curInstr = nullptr;
            int subBlockPtr = 0;
        public:
            AllInstructionsIterator(ILInstruction * instr)
            {
                curInstr = instr;
            }
            AllInstructionsIterator & operator ++();
            
            AllInstructionsIterator operator ++(int)
            {
                AllInstructionsIterator rs = *this;
                operator++();
                return rs;
            }
            bool operator != (const AllInstructionsIterator & _that)
            {
                return curInstr != _that.curInstr || subBlockPtr != _that.subBlockPtr;
            }
            bool operator == (const AllInstructionsIterator & _that)
            {
                return curInstr == _that.curInstr && subBlockPtr == _that.subBlockPtr;
            }
            ILOperand & operator *()
            {
                return *curInstr;
            }
            ILOperand * operator ->()
            {
                return curInstr;
            }
        };

        class AllInstructionsCollection
        {
        private:
            CFGNode * node;
        public:
            AllInstructionsCollection(CFGNode * _node)
                : node(_node)
            {}
            AllInstructionsIterator begin();
            AllInstructionsIterator end();
        };
        
        class CFGNode : public Object
        {
        private:
            ILInstruction *headInstr, *tailInstr;
        public:
            class Iterator
            {
            public:
                ILInstruction * Current, *Next;
                void SetCurrent(ILInstruction * cur)
                {
                    Current = cur;
                    if (Current)
                        Next = Current->GetNext();
                    else
                        Next = 0;
                }
                Iterator(ILInstruction * cur)
                {
                    SetCurrent(cur);
                }
                ILInstruction & operator *() const
                {
                    return *Current;
                }
                Iterator& operator ++()
                {
                    SetCurrent(Next);
                    return *this;
                }
                Iterator operator ++(int)
                {
                    Iterator rs = *this;
                    SetCurrent(Next);
                    return rs;
                }
                bool operator != (const Iterator & iter) const
                {
                    return Current != iter.Current;
                }
                bool operator == (const Iterator & iter) const
                {
                    return Current == iter.Current;
                }
            };

            String ToString() 
			{
				NameAllInstructions();
                StringBuilder sb;
                bool first = true;
                auto pintr = begin();
                while (pintr != end())
				{
                    if (!first)
                        sb << EndLine;
                    first = false;
                    sb << pintr.Current->ToString();
                    pintr++;
                }
                return sb.ToString();
            }

            Iterator begin() const
            {
                return Iterator(headInstr->GetNext());
            }

            Iterator end() const
            {
                return Iterator(tailInstr);
            }

            AllInstructionsCollection GetAllInstructions()
            {
                return AllInstructionsCollection(this);
            }
            
            ILInstruction * GetFirstNonPhiInstruction();
            bool HasPhiInstruction();

            ILInstruction * GetLastInstruction()
            {
                return (tailInstr->GetPrevious());
            }

            String Name;

            CFGNode()
            {
                headInstr = new ILInstruction();
                tailInstr = new ILInstruction();
                headInstr->Parent = this;
                headInstr->InsertAfter(tailInstr);
            }
            ~CFGNode()
            {
                ILInstruction * instr = headInstr;
                while (instr)
                {
                    for (auto user : instr->Users)
                    {
                        auto userInstr = dynamic_cast<ILInstruction*>(user);
                        if (userInstr)
                        {
                            for (auto iter = userInstr->begin(); iter != userInstr->end(); ++iter)
                            if (iter == instr)
                                iter.Set(0);
                        }
                    }
                
                    auto next = instr->GetNext();
                    delete instr;
                    instr = next;
                }
            }
            void InsertHead(ILInstruction * instr)
            {
                headInstr->InsertAfter(instr);
            }
            void InsertTail(ILInstruction * instr)
            {
                tailInstr->InsertBefore(instr);
            }
            void NameAllInstructions();
            void DebugPrint();
        };

        template<typename T>
        struct ConstKey
        {
            Array<T, 16> Value;
            int Size;
            ConstKey()
            {
                Value.SetSize(Value.GetCapacity());
            }
            ConstKey(T value, int size)
            {
                if (size == 0)
                    size = 1;
                Value.SetSize(Value.GetCapacity());
                for (int i = 0; i < size; i++)
                    Value[i] = value;
                Size = size;
            }
            static ConstKey<T> FromValues(T value, T value1)
            {
                ConstKey<T> result;
                result.Value.SetSize(result.Value.GetCapacity());
                result.Size = 2;
                result.Value[0] = value;
                result.Value[1] = value1;
                return result;
            }
            static ConstKey<T> FromValues(T value, T value1, T value2)
            {
                ConstKey<T> result;
                result.Value.SetSize(result.Value.GetCapacity());
                result.Size = 3;
                result.Value[0] = value;
                result.Value[1] = value1;
                result.Value[2] = value2;
                return result;
            }
            static ConstKey<T> FromValues(T value, T value1, T value2, T value3)
            {
                ConstKey<T> result;
                result.Value.SetSize(result.Value.GetCapacity());
                result.Size = 4;
                result.Value[0] = value;
                result.Value[1] = value1;
                result.Value[2] = value2;
                result.Value[3] = value3;
                return result;
            }
            int GetHashCode()
            {
                int result = Size;
                for (int i = 0; i < Size; i++)
                    result ^= ((*(int*)&Value) << 5);
                return result;
            }
            bool operator == (const ConstKey<T> & other)
            {
                if (Size != other.Size)
                    return false;
                for (int i = 0; i < Size; i++)
                    if (Value[i] != other.Value[i])
                        return false;
                return true;
            }
        };


        class PhiInstruction : public ILInstruction
        {
        public:
            List<UseReference> Operands; // Use as fixed array, no insert or resize
        public:
            PhiInstruction(int opCount)
            {
                Operands.SetSize(opCount);
                for (int i = 0; i < opCount; i++)
                    Operands[i].SetUser(this);
            }
            PhiInstruction(const PhiInstruction & other)
                : ILInstruction(other)
            {
                Operands.SetSize(other.Operands.Count());
                for (int i = 0; i < Operands.Count(); i++)
                {
                    Operands[i].SetUser(this);
                    Operands[i] = other.Operands[i].Ptr();
                }
            }
            virtual String GetOperatorString() override
            {
                return "phi";
            }
            virtual OperandIterator begin() override
            {
                return Operands.begin();
            }
            virtual OperandIterator end() override
            {
                return Operands.end();
            }
            virtual String ToString() override
            {
                StringBuilder sb;
                sb << Name << " = phi ";
                for (auto & op : Operands)
                {
                    if (op)
                    {
                        sb << op.ToString();
                    }
                    else
                        sb << "<?>";
                    sb << ", ";
                }
                return sb.ProduceString();
            }
            virtual PhiInstruction * Clone() override
            {
                return new PhiInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class UnaryInstruction : public ILInstruction
        {
        public:
            UseReference Operand;
            UnaryInstruction()
                : Operand(this)
            {}
            UnaryInstruction(const UnaryInstruction & other)
                : ILInstruction(other), Operand(this)
            {
                Operand = other.Operand.Ptr();
            }
            virtual OperandIterator begin() override
            {
                return &Operand;
            }
            virtual OperandIterator end() override
            {
                return &Operand + 1;
            }
        };

        class BinaryInstruction : public ILInstruction
        {
        public:
            Array<UseReference, 2> Operands;
            BinaryInstruction()
            {
                Operands.SetSize(2);
                Operands[0].SetUser(this);
                Operands[1].SetUser(this);
            }
            BinaryInstruction(const BinaryInstruction & other)
                : ILInstruction(other)
            {
                Operands.SetSize(2);
                Operands[0].SetUser(this);
                Operands[1].SetUser(this);
                Operands[0] = other.Operands[0].Ptr();
                Operands[1] = other.Operands[1].Ptr();
            }
            virtual OperandIterator begin() override
            {
                return Operands.begin();
            }
            virtual OperandIterator end() override
            {
                return Operands.end();
            }
        };

        class SelectInstruction : public ILInstruction
        {
        public:
            UseReference Operands[3];
            SelectInstruction()
            {
                Operands[0].SetUser(this);
                Operands[1].SetUser(this);
                Operands[2].SetUser(this);
            }
            SelectInstruction(const SelectInstruction & other)
                : ILInstruction(other)
            {
                Operands[0].SetUser(this);
                Operands[1].SetUser(this);
                Operands[2].SetUser(this);
                Operands[0] = other.Operands[0].Ptr();
                Operands[1] = other.Operands[1].Ptr();
                Operands[2] = other.Operands[2].Ptr();
            }
            SelectInstruction(ILOperand * mask, ILOperand * val0, ILOperand * val1)
            {
                Operands[0].SetUser(this);
                Operands[1].SetUser(this);
                Operands[2].SetUser(this);
                Operands[0] = mask;
                Operands[1] = val0;
                Operands[2] = val1;
                Type = val0->Type->Clone();
            }
            virtual OperandIterator begin() override
            {
                return Operands;
            }
            virtual OperandIterator end() override
            {
                return Operands + 3;
            }

            virtual String ToString() override
            {
                return Name + " = select " + Operands[0].ToString() + ": " + Operands[1].ToString() + ", " + Operands[2].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "select";
            }
            virtual SelectInstruction * Clone() override
            {
                return new SelectInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class CallInstruction : public ILInstruction
        {
        public:
            String Function;
            List<UseReference> Arguments;
            bool SideEffect = true;
            virtual OperandIterator begin() override
            {
                return Arguments.begin();
            }
            virtual OperandIterator end() override
            {
                return Arguments.end();
            }
            virtual String ToString() override
            {
                StringBuilder sb(256);
                sb << Name;
                sb << " = call " << Function << "(";
                for (auto & op : Arguments)
                {
                    sb << op.ToString();
                    if (op != Arguments.Last())
                        sb << ", ";
                }
                sb << ") : " << this->Type->ToString();
                return sb.ProduceString();
            }
            virtual String GetOperatorString() override
            {
                return "call " + Function;
            }
            virtual bool HasSideEffect() override
            {
                return SideEffect;
            }
            CallInstruction(int argSize)
            {
                Arguments.SetSize(argSize);
                for (auto & use : Arguments)
                    use.SetUser(this);
            }
            CallInstruction(const CallInstruction & other)
                : ILInstruction(other)
            {
                Function = other.Function;
                SideEffect = other.SideEffect;
                Arguments.SetSize(other.Arguments.Count());
                for (int i = 0; i < other.Arguments.Count(); i++)
                {
                    Arguments[i].SetUser(this);
                    Arguments[i] = other.Arguments[i].Ptr();
                }

            }
            virtual CallInstruction * Clone() override
            {
                return new CallInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class NotInstruction : public UnaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return  Name + " = not " + Operand.ToString();
            }
            virtual String GetOperatorString() override
            {
                return "not";
            }
            virtual NotInstruction * Clone() override
            {
                return new NotInstruction(*this);
            }
            NotInstruction() = default;
            NotInstruction(const NotInstruction & other) = default;

            NotInstruction(ILOperand * op)
            {
                Operand = op;
                Type = op->Type->Clone();
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class NegInstruction : public UnaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return  Name + " = neg " + Operand.ToString();
            }
            virtual String GetOperatorString() override
            {
                return "neg";
            }
            virtual NegInstruction * Clone() override
            {
                return new NegInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };


        class SwizzleInstruction : public UnaryInstruction
        {
        public:
            String SwizzleString;
            virtual String ToString() override
            {
                return  Name + " = " + Operand.ToString() + "." + SwizzleString;
            }
            virtual String GetOperatorString() override
            {
                return "swizzle";
            }
            virtual SwizzleInstruction * Clone() override
            {
                return new SwizzleInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class BitNotInstruction : public UnaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return  Name + " = bnot " + Operand.ToString();
            }
            virtual String GetOperatorString() override
            {
                return "bnot";
            }
            virtual BitNotInstruction * Clone() override
            {
                return new BitNotInstruction(*this);
            }
            BitNotInstruction() = default;
            BitNotInstruction(const BitNotInstruction & instr) = default;

            BitNotInstruction(ILOperand * op)
            {
                Operand = op;
                Type = op->Type->Clone();
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class AddInstruction : public BinaryInstruction
        {
        public:
            AddInstruction() = default;
            AddInstruction(const AddInstruction & instr) = default;
            AddInstruction(ILOperand * v0, ILOperand * v1)
            {
                Operands[0] = v0;
                Operands[1] = v1;
                Type = v0->Type->Clone();
            }
            virtual String ToString() override
            {
                return Name + " = add " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "add";
            }
            virtual AddInstruction * Clone() override
            {
                return new AddInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class MemberAccessInstruction : public BinaryInstruction
        {
        public:
			MemberAccessInstruction() = default;
			MemberAccessInstruction(const MemberAccessInstruction &) = default;
			MemberAccessInstruction(ILOperand * v0, ILOperand * v1)
            {
                Operands[0] = v0;
                Operands[1] = v1;
                if (auto arrType = dynamic_cast<ILArrayType *>(v0->Type.Ptr()))
                {
                    Type = arrType->BaseType->Clone();
                }
                else if (auto genType = dynamic_cast<ILGenericType*>(v0->Type.Ptr()))
                {
                    Type = genType->BaseType->Clone();
                }
                else if (auto baseType = dynamic_cast<ILVectorType *>(v0->Type.Ptr()))
                {
                    Type = new ILBasicType(baseType->BaseType);
                }
                else if (auto matrixType = dynamic_cast<ILMatrixType*>(v0->Type.Ptr()))
                {
                    Type = new ILVectorType(matrixType->BaseType, matrixType->Size[1]);
                }
                else if (auto structType = dynamic_cast<ILStructType*>(v0->Type.Ptr()))
                {
                    auto cv1 = dynamic_cast<ILConstOperand*>(v1);
                    if (!cv1)
                        throw InvalidProgramException("member field access offset is not constant.");
                    if (cv1->IntValues[0] < 0 || cv1->IntValues[0] >= structType->Members.Count())
                        throw InvalidProgramException("member field access offset out of bounds.");
                    Type = structType->Members[cv1->IntValues[0]].Type;
                }
				else if (auto arrLikeType = dynamic_cast<ILArrayLikeType*>(v0->Type.Ptr()))
				{
					Type = arrLikeType->BaseType;
				}
                else
                    throw InvalidOperationException("Unsupported aggregate type.");
            }
            virtual String ToString() override
            {
                return Name + " = member " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "member";
            }
            virtual MemberAccessInstruction * Clone() override
            {
                return new MemberAccessInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class SubInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = sub " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "sub";
            }
            virtual SubInstruction * Clone() override
            {
                return new SubInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class MulInstruction : public BinaryInstruction
        {
        public:
            MulInstruction(){}
            MulInstruction(ILOperand * v0, ILOperand * v1)
            {
                Operands[0] = v0;
                Operands[1] = v1;
                Type = v0->Type->Clone();
            }
            MulInstruction(const MulInstruction &) = default;

            virtual String ToString() override
            {
                return Name + " = mul " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "mu";
            }
            virtual MulInstruction * Clone() override
            {
                return new MulInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class DivInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = div " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "div";
            }
            virtual DivInstruction * Clone() override
            {
                return new DivInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class ModInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = mod " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "mod";
            }
            virtual ModInstruction * Clone() override
            {
                return new ModInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class AndInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = and " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "and";
            }
            virtual AndInstruction * Clone() override
            {
                return new AndInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class OrInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = or " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "or";
            }
            virtual OrInstruction * Clone() override
            {
                return new OrInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class BitAndInstruction : public BinaryInstruction
        {
        public:
            BitAndInstruction(){}
            BitAndInstruction(ILOperand * v0, ILOperand * v1)
            {
                Operands[0] = v0;
                Operands[1] = v1;
                Type = v0->Type->Clone();
            }
            BitAndInstruction(const BitAndInstruction &) = default;
            virtual String ToString() override
            {
                return Name + " = band " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "band";
            }
            virtual BitAndInstruction * Clone() override
            {
                return new BitAndInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class BitOrInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = bor " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "bor";
            }
            virtual BitOrInstruction * Clone() override
            {
                return new BitOrInstruction(*this);
            }
            BitOrInstruction(){}
            BitOrInstruction(const BitOrInstruction &) = default;
            BitOrInstruction(ILOperand * v0, ILOperand * v1)
            {
                Operands[0] = v0;
                Operands[1] = v1;
                Type = v0->Type->Clone();
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class BitXorInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = bxor " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "bxor";
            }
            virtual BitXorInstruction * Clone() override
            {
                return new BitXorInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class ShlInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = shl " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "sh";
            }
            virtual ShlInstruction * Clone() override
            {
                return new ShlInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class ShrInstruction : public BinaryInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = shr " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "shr";
            }
            virtual ShrInstruction * Clone() override
            {
                return new ShrInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class CompareInstruction : public BinaryInstruction
        {};
        class CmpgtInstruction : public CompareInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = gt " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "gt";
            }
            virtual CmpgtInstruction * Clone() override
            {
                return new CmpgtInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class CmpgeInstruction : public CompareInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = ge " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "ge";
            }
            virtual CmpgeInstruction * Clone() override
            {
                return new CmpgeInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class CmpltInstruction : public CompareInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = lt " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "lt";
            }
            virtual CmpltInstruction * Clone() override
            {
                return new CmpltInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class CmpleInstruction : public CompareInstruction
        {
        public:
            CmpleInstruction() = default;
            CmpleInstruction(const CmpleInstruction &) = default;
            CmpleInstruction(ILOperand * v0, ILOperand * v1)
            {
                Operands[0] = v0;
                Operands[1] = v1;
                Type = v0->Type->Clone();
            }

            virtual String ToString() override
            {
                return Name + " = le " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "le";
            }
            virtual CmpleInstruction * Clone() override
            {
                return new CmpleInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class CmpeqlInstruction : public CompareInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = eql " + Operands[0].ToString()
                    + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "eq";
            }
            virtual CmpeqlInstruction * Clone() override
            {
                return new CmpeqlInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        class CmpneqInstruction : public CompareInstruction
        {
        public:
            virtual String ToString() override
            {
                return Name + " = neq " + Operands[0].ToString() + ", " + Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "neq";
            }
            virtual CmpneqInstruction * Clone() override
            {
                return new CmpneqInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        
        class CopyInstruction : public UnaryInstruction
        {
        public:
            CopyInstruction(){}
            CopyInstruction(const CopyInstruction &) = default;

            CopyInstruction(ILOperand * dest)
            {
                Operand = dest;
                Type = dest->Type->Clone();
            }
        public:
            virtual String ToString() override
            {
                return Name + " = " + Operand.ToString();
            }
            virtual String GetOperatorString() override
            {
                return "copy";
            }
            virtual CopyInstruction * Clone() override
            {
                return new CopyInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };
        // load(src)
        class LoadInstruction : public UnaryInstruction
        {
        public:
            bool Deterministic;
            LoadInstruction()
            {
                Deterministic = false;
            }
            LoadInstruction(const LoadInstruction & other)
                : UnaryInstruction(other)
            {
                Deterministic = other.Deterministic;
            }
            LoadInstruction(ILOperand * dest);
        public:
            virtual String ToString() override
            {
                return Name + " = load " + Operand.ToString();
            }
            virtual bool IsDeterministic() override
            {
                return Deterministic;
            }
            virtual String GetOperatorString() override
            {
                return "ld";
            }
            virtual LoadInstruction * Clone() override
            {
                auto rs = new LoadInstruction(*this);
                if (!rs->Type)
                    printf("shit");
                return rs;
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class DiscardInstruction : public ILInstruction
        {
        public:
            virtual bool IsDeterministic() override
            {
                return true;
            }
            virtual bool HasSideEffect() override
            {
                return true;
            }
            virtual String ToString() override
            {
                return  "discard";
            }
            virtual String GetOperatorString() override
            {
                return "discard";
            }
            virtual DiscardInstruction * Clone() override
            {
                return new DiscardInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        // store(dest, value)
        class StoreInstruction : public BinaryInstruction
        {
        public:
            StoreInstruction(){}
            StoreInstruction(const StoreInstruction &) = default;

            StoreInstruction(ILOperand * dest, ILOperand * value)
            {
                Operands.SetSize(2);
                Operands[0] = dest;
                Operands[1] = value;
            }
        public:
            virtual String ToString() override
            {
                return "store " + Operands[0].ToString() + ", " +
                    Operands[1].ToString();
            }
            virtual String GetOperatorString() override
            {
                return "st";
            }
            virtual bool HasSideEffect() override
            {
                return true;
            }
            virtual StoreInstruction * Clone() override
            {
                return new StoreInstruction(*this);
            }
            virtual void Accept(InstructionVisitor * visitor) override;
        };

        class InstructionVisitor : public Object
        {
        public:
            virtual void VisitAddInstruction(AddInstruction *){}
            virtual void VisitSubInstruction(SubInstruction *){}
            virtual void VisitDivInstruction(DivInstruction *){}
            virtual void VisitMulInstruction(MulInstruction *){}
            virtual void VisitModInstruction(ModInstruction *){}
            virtual void VisitNegInstruction(NegInstruction *){}
            virtual void VisitAndInstruction(AndInstruction *){}
            virtual void VisitOrInstruction(OrInstruction *){}
            virtual void VisitBitAndInstruction(BitAndInstruction *){}
            virtual void VisitBitOrInstruction(BitOrInstruction *){}
            virtual void VisitBitXorInstruction(BitXorInstruction *){}
            virtual void VisitShlInstruction(ShlInstruction *){}
            virtual void VisitShrInstruction(ShrInstruction *){}
            virtual void VisitBitNotInstruction(BitNotInstruction *){}
            virtual void VisitNotInstruction(NotInstruction *){}
            virtual void VisitCmpeqlInstruction(CmpeqlInstruction *){}
            virtual void VisitCmpneqInstruction(CmpneqInstruction *){}
            virtual void VisitCmpltInstruction(CmpltInstruction *){}
            virtual void VisitCmpleInstruction(CmpleInstruction *){}
            virtual void VisitCmpgtInstruction(CmpgtInstruction *){}
            virtual void VisitCmpgeInstruction(CmpgeInstruction *){}
            virtual void VisitLoadInstruction(LoadInstruction *){}
            virtual void VisitStoreInstruction(StoreInstruction *){}
            virtual void VisitCopyInstruction(CopyInstruction *){}
            virtual void VisitAllocVarInstruction(AllocVarInstruction *){}
            virtual void VisitFetchArgInstruction(FetchArgInstruction *){}
            virtual void VisitMemberAccessInstruction(MemberAccessInstruction *){}
            virtual void VisitSelectInstruction(SelectInstruction *){}
            virtual void VisitCallInstruction(CallInstruction *){}
            virtual void VisitSwitchInstruction(SwitchInstruction *){}
            virtual void VisitDiscardInstruction(DiscardInstruction *) {}
            virtual void VisitPhiInstruction(PhiInstruction *){}
            virtual void VisitSwizzleInstruction(SwizzleInstruction*) {}
        };

        class ForInstruction : public ILInstruction
        {
        public:
            RefPtr<CFGNode> InitialCode, ConditionCode, SideEffectCode, BodyCode;
            virtual int GetSubBlockCount() override
            {
                int count = 0;
                if (InitialCode)
                    count++;
                if (ConditionCode)
                    count++;
                if (SideEffectCode)
                    count++;
                if (BodyCode)
                    count++;
                return count;
            }
            virtual CFGNode * GetSubBlock(int i) override
            {
                int id = 0;
                if (InitialCode)
                {
                    if (id == i) return InitialCode.Ptr();
                    id++;
                }
                if (ConditionCode)
                {
                    if (id == i) return ConditionCode.Ptr();
                    id++;
                }
                if (SideEffectCode)
                {
                    if (id == i) return SideEffectCode.Ptr();
                    id++;
                }
                if (BodyCode)
                {
                    if (id == i) return BodyCode.Ptr();
                }
                return nullptr;
            }

            virtual String ToString() override
            {
                StringBuilder sb;
                sb << "for (" << InitialCode->ToString() << "; " << ConditionCode->ToString() << "; ";
                sb << SideEffectCode->ToString() << ")" << EndLine;
                sb << "{" << EndLine;
                sb << BodyCode->ToString() << EndLine;
                sb << "}" << EndLine;
                return sb.ProduceString();
            }
        };
        class IfInstruction : public UnaryInstruction
        {
        public:
            RefPtr<CFGNode> TrueCode, FalseCode;
            virtual int GetSubBlockCount() override
            {
                if (FalseCode)
                    return 2;
                else
                    return 1;
            }
            virtual CFGNode * GetSubBlock(int i) override
            {
                if (i == 0)
                    return TrueCode.Ptr();
                else if (i == 1)
                    return FalseCode.Ptr();
                return nullptr;
            }

            virtual String ToString() override
            {
                StringBuilder sb;
                sb << "if (" << Operand->ToString() << ")" << EndLine;
                sb << "{" << EndLine;
                sb << TrueCode->ToString() << EndLine;
                sb << "}" << EndLine;
                if (FalseCode)
                {
                    sb << "else" << EndLine;
                    sb << "{" << EndLine;
                    sb << FalseCode->ToString() << EndLine;
                    sb << "}" << EndLine;
                }
                return sb.ProduceString();
            }
        };
        class WhileInstruction : public ILInstruction
        {
        public:
            RefPtr<CFGNode> ConditionCode, BodyCode;
            virtual int GetSubBlockCount() override
            {
                return 2;
            }
            virtual CFGNode * GetSubBlock(int i) override
            {
                if (i == 0)
                    return ConditionCode.Ptr();
                else if (i == 1)
                    return BodyCode.Ptr();
                return nullptr;
            }

            virtual String ToString() override
            {
                StringBuilder sb;
                sb << "while (" << ConditionCode->ToString() << ")" << EndLine;
                sb << "{" << EndLine;
                sb << BodyCode->ToString();
                sb << "}" << EndLine;
                return sb.ProduceString();
            }
        };
        class DoInstruction : public ILInstruction
        {
        public:
            RefPtr<CFGNode> ConditionCode, BodyCode;
            virtual int GetSubBlockCount() override
            {
                return 2;
            }
            virtual CFGNode * GetSubBlock(int i) override
            {
                if (i == 1)
                    return ConditionCode.Ptr();
                else if (i == 0)
                    return BodyCode.Ptr();
                return nullptr;
            }

            virtual String ToString() override
            {
                StringBuilder sb;
                sb << "{" << EndLine;
                sb << BodyCode->ToString();
                sb << "}" << EndLine;
                sb << "while (" << ConditionCode->ToString() << ")" << EndLine;
                return sb.ProduceString();
            }
        };
        class ReturnInstruction : public UnaryInstruction
        {
        public:
            ReturnInstruction(ILOperand * op)
                :UnaryInstruction()
            {
                Operand = op;
            }

            virtual String ToString() override
            {
                return "return " + Operand->ToString() + ";";
            }
        };
        class BreakInstruction : public ILInstruction
        {};
        class ContinueInstruction : public ILInstruction
        {};

        class KeyHoleNode
        {
        public:
            String NodeType;
            int CaptureId = -1;
            List<RefPtr<KeyHoleNode>> Children;
            bool Match(List<ILOperand*> & matchResult, ILOperand * instr);
            static RefPtr<KeyHoleNode> Parse(String format);
        };

        class ConstantPoolImpl;

        class ConstantPool
        {
        private:
            ConstantPoolImpl * impl;
        public:
            ILConstOperand * CreateConstant(ILConstOperand * c);
            ILConstOperand * CreateConstantIntVec(int val0, int val1);
            ILConstOperand * CreateConstantIntVec(int val0, int val1, int val2);
            ILConstOperand * CreateConstantIntVec(int val0, int val1, int val3, int val4);
            ILConstOperand * CreateConstant(int val, int vectorSize = 0);
            ILConstOperand * CreateConstant(float val, int vectorSize = 0);
            ILConstOperand * CreateConstant(float val, float val1);
            ILConstOperand * CreateConstant(float val, float val1, float val2);
            ILConstOperand * CreateConstant(float val, float val1, float val2, float val3);
            ILConstOperand * CreateConstant(bool b);
            ILOperand * CreateDefaultValue(ILType * type);
            ILUndefinedOperand * GetUndefinedOperand();
            ConstantPool();
            ~ConstantPool();
        };

        class ILModuleParameterSet;

        class ILModuleParameterInstance : public ILOperand
        {
        public:
            ILModuleParameterSet * Module = nullptr;
            int BufferOffset = -1;
            List<int> BindingPoints; // for legacy API, usually one item. Samplers may have multiple binding points in OpenGL.
            virtual String ToString()
            {
                return "moduleParam<" + Name + ">";
            }
        };

        class ILModuleParameterSet : public RefObject
        {
        public:
            int BufferSize = 0;
            String BindingName;
            int DescriptorSetId = -1;
            int UniformBufferLegacyBindingPoint = -1;
            EnumerableDictionary<String, RefPtr<ILModuleParameterInstance>> Parameters;
        };
		
		enum class ILStageName
        {
            General, Compute, Vertex, Fragment, Hull, Domain, Geometry
        };

        enum class ILStageVersion
        {
            SM_4_0, SM_4_0_DX9_0, SM_4_0_DX9_1, SM_4_0_DX9_3, SM_4_1, SM_5_0,
        };

        class ILProfile
        {
        public:
            ILStageName Stage = ILStageName::General;
            ILStageVersion Version = ILStageVersion::SM_5_0;
        };

		class ILFunction : public RefObject
        {
        public:
            EnumerableDictionary<String, FetchArgInstruction*> Parameters;
            RefPtr<ILType> ReturnType;
            RefPtr<CFGNode> Code;
            bool IsEntryPoint = false;
            ILProfile Profile;
            String Name;
        };

        class ILGlobalVariable : public AllocVarInstruction
        {
        public:
            bool IsConst = false;
            RefPtr<CFGNode> Code;
            ILGlobalVariable(ILType * type)
                : AllocVarInstruction(type)
            {
            }
            ILGlobalVariable(CoreLib::RefPtr<ILType> & type)
                : AllocVarInstruction(type)
            {
            }
            virtual ILGlobalVariable * Clone() override
            {
                return new ILGlobalVariable(*this);
            }
        };

		enum class ILVariableBlockType
		{
			GlobalVar, Constant
		};

		class ILBinding
		{
		public:
			// DX11 style binding
			BindableResourceType BindingType;
			int ResourceLocalBindingIndex;

			// Vulkan style binding
			int DescriptorTableIndex;
			int DescriptorBindingIndex;
		};

		class ILVariableBlock
		{
		public:
			ILVariableBlockType Type;
			ILBinding Binding;
			String Name;
			CodePosition Position;
			EnumerableDictionary<String, RefPtr<ILGlobalVariable>> Vars;
		};

        class ILProgram : public RefObject
        {
        public:
            RefPtr<ConstantPool> ConstantPool = new Compiler::ConstantPool();
            EnumerableDictionary<String, RefPtr<ILModuleParameterSet>> ModuleParamSets;
            List<RefPtr<ILStructType>> Structs;
            EnumerableDictionary<String, RefPtr<ILFunction>> Functions;
			List<RefPtr<ILVariableBlock>> VariableBlocks;
			String ToString();
            ~ILProgram()
            {
				// explicit freeing order
                Functions = EnumerableDictionary<String, RefPtr<ILFunction>>();
				VariableBlocks = List<RefPtr<ILVariableBlock>>();
                Structs = List<RefPtr<ILStructType>>();
                ModuleParamSets = EnumerableDictionary<String, RefPtr<ILModuleParameterSet>>();
            }
        };
    }
}

#endif