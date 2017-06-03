#include "IL.h"
#include "../CoreLib/LibIO.h"
#include "Syntax.h"
#include "CompiledProgram.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib::IO;

        ILBaseType BaseTypeFromString(CoreLib::Text::TokenReader & parser)
        {
            if (parser.LookAhead("int"))
                return ILBaseType::Int;
            if (parser.LookAhead("float"))
                return ILBaseType::Float;
            if (parser.LookAhead("bool"))
                return ILBaseType::Bool;
            if (parser.LookAhead("uint"))
                return ILBaseType::UInt;
            if (parser.LookAhead("uint64"))
                return ILBaseType::UInt64;
            if (parser.LookAhead("void"))
                return ILBaseType::Void;
            return ILBaseType::Void;
        }

        int RoundToAlignment(int offset, int alignment)
        {
            int remainder = offset % alignment;
            if (remainder == 0)
                return offset;
            else
                return offset + (alignment - remainder);
        }

        int GetMaxResourceBindings(BindableResourceType type)
        {
            switch (type)
            {
            case BindableResourceType::Texture:
                return 32;
            case BindableResourceType::Sampler:
                return 32;
            case BindableResourceType::Buffer:
                return 16;
            case BindableResourceType::StorageBuffer:
                return 16;
            }
            return 0;
        }

        const char * ILBaseTypeToString(ILBaseType type)
        {
            switch (type)
            {
            case ILBaseType::Int:
                return "int";
            case ILBaseType::UInt:
                return "uint";
            case ILBaseType::Bool:
                return "bool";
            case ILBaseType::Float:
                return "float";
            case ILBaseType::Void:
                return "void";
            default:
                return "?unknowntype";
            }
        }

        const char * ILTextureShapeToString(ILTextureShape shape)
        {
            switch (shape)
            {
            case ILTextureShape::Texture1D:
                return "Texture1D";
            case ILTextureShape::Texture2D:
                return "Texture2D";
            case ILTextureShape::Texture3D:
                return "Texture3D";
            case ILTextureShape::TextureCube:
                return "TextureCube";
            }
            return nullptr;
        }

        int SizeofBaseType(ILBaseType type)
        {
            if (type == ILBaseType::Int)
                return 4;
            if (type == ILBaseType::UInt)
                return 4;
            else if (type == ILBaseType::Float)
                return 4;
            else if (type == ILBaseType::Bool)
                return 4;
            else
                return 0;
        }

        bool ILType::IsBool()
        {
            auto basicType = dynamic_cast<ILBasicType*>(this);
            if (basicType)
                return basicType->Type == ILBaseType::Bool;
            else
                return false;
        }

        bool ILType::IsInt()
        {
            auto basicType = dynamic_cast<ILBasicType*>(this);
            if (basicType)
                return basicType->Type == ILBaseType::Int;
            else
                return false;
        }

        bool ILType::IsUInt()
        {
            auto basicType = dynamic_cast<ILBasicType*>(this);
            if (basicType)
                return basicType->Type == ILBaseType::UInt;
            else
                return false;
        }

        bool ILType::IsIntegral()
        {
            auto basicType = dynamic_cast<ILBasicType*>(this);
            if (basicType)
                return basicType->Type == ILBaseType::Int || basicType->Type == ILBaseType::UInt || basicType->Type == ILBaseType::Bool;
            else
                return false;
        }

        bool ILType::IsVoid()
        {
            auto basicType = dynamic_cast<ILBasicType*>(this);
            if (basicType)
                return basicType->Type == ILBaseType::Void;
            else
                return false;
        }

        bool ILType::IsFloat()
        {
            auto basicType = dynamic_cast<ILBasicType*>(this);
            if (basicType)
                return basicType->Type == ILBaseType::Float;
            else
                return false;
        }

        bool ILType::IsBoolVector()
        {
            auto basicType = dynamic_cast<ILVectorType*>(this);
            if (basicType)
                return basicType->BaseType == ILBaseType::Bool;
            else
                return false;
        }

        bool ILType::IsIntVector()
        {
            auto basicType = dynamic_cast<ILVectorType*>(this);
            if (basicType)
                return basicType->BaseType == ILBaseType::Int;
            else
                return false;
        }

        bool ILType::IsUIntVector()
        {
            auto basicType = dynamic_cast<ILVectorType*>(this);
            if (basicType)
                return basicType->BaseType == ILBaseType::UInt;
            else
                return false;
        }

        bool ILType::IsFloatVector()
        {
            auto basicType = dynamic_cast<ILVectorType*>(this);
            if (basicType)
                return basicType->BaseType == ILBaseType::Float;
            else
                return false;
        }

        bool ILType::IsFloatMatrix()
        {
            auto basicType = dynamic_cast<ILMatrixType*>(this);
            if (basicType)
                return basicType->BaseType == ILBaseType::Float;
            else
                return false;
        }

        bool ILType::IsNonShadowTexture()
        {
            auto basicType = dynamic_cast<ILTextureType*>(this);
            if (basicType)
                return !basicType->Flavor.Fields.IsShadow;
            else
                return false;
        }

        ILBasicType * ILType::AsBasicType()
        {
            return dynamic_cast<ILBasicType*>(this);
        }

        ILVectorType * ILType::AsVectorType()
        {
            return dynamic_cast<ILVectorType*>(this);
        }

        ILMatrixType * ILType::AsMatrixType()
        {
            return dynamic_cast<ILMatrixType*>(this);
        }

        ILTextureType * ILType::AsTextureType()
        {
            return dynamic_cast<ILTextureType*>(this);
        }

        ILPointerLikeType * ILType::AsPointerLikeType()
        {
            return dynamic_cast<ILPointerLikeType*>(this);
        }

        ILArrayLikeType * ILType::AsArrayLikeType()
        {
            return dynamic_cast<ILArrayLikeType*>(this);
        }

        bool ILType::IsTexture()
        {
            auto basicType = dynamic_cast<ILTextureType*>(this);
            if (basicType)
                return true;
            else
                return false;
        }

        bool ILType::IsSamplerState()
        {
            auto basicType = dynamic_cast<ILSamplerStateType*>(this);
			if (basicType)
				return true;
			else
				return false;
        }

        RefPtr<ILType> DeserializeBasicType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("basic");
            auto rs = new ILBasicType(BaseTypeFromString(reader));
            reader.ReadWord();
            return rs;
        }
        RefPtr<ILVectorType> DeserializeVectorType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("vector");
            reader.Read("<");
            RefPtr<ILVectorType> vecType = new ILVectorType();
            vecType->BaseType = BaseTypeFromString(reader);
            reader.Read(",");
            vecType->Size = reader.ReadInt();
            reader.Read(">");
            return vecType;
        }
		RefPtr<ILSamplerStateType> DeserializeSamplerType(CoreLib::Text::TokenReader & reader)
		{
			reader.Read("SamplerState");
			reader.Read("(");
			auto rs = new ILSamplerStateType();
			rs->IsComparison = reader.ReadInt() != 0;
			reader.Read(")");
			return rs;
		}
        RefPtr<ILMatrixType> DeserializeMatrixType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("matrix");
            reader.Read("<");
            RefPtr<ILMatrixType> vecType = new ILMatrixType();
            vecType->BaseType = BaseTypeFromString(reader);
            reader.Read(",");
            vecType->Size[0] = reader.ReadInt();
            reader.Read(",");
            vecType->Size[1] = reader.ReadInt();
            reader.Read(">");
            return vecType;
        }
        RefPtr<ILType> DeserializeStructType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("struct");
            RefPtr<ILStructType> rs = new ILStructType();
            rs->TypeName = reader.ReadToken().Content;
            reader.Read("(");
            while (reader.LookAhead(")"))
            {
                ILStructType::ILStructField field;
                field.FieldName = reader.ReadToken().Content;
                reader.Read(":");
                field.Type = ILType::Deserialize(reader);
                reader.Read(";");
            }
            reader.Read(")");
            return rs;
        }
        RefPtr<ILType> DeserializeArrayType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("array");
            reader.Read("(");
            RefPtr<ILArrayType> rs = new ILArrayType();
            rs->BaseType = ILType::Deserialize(reader);
            reader.Read(",");
            rs->ArrayLength = reader.ReadInt();
            reader.Read(")");
            return rs;
        }
        RefPtr<ILType> DeserializeArrayLikeType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("array_like");
            reader.Read("<");
            RefPtr<ILArrayLikeType> rs = new ILArrayLikeType();
            rs->BaseType = ILType::Deserialize(reader);
            reader.Read(",");
            rs->Name = (ILArrayLikeTypeName)reader.ReadInt();
            reader.Read(">");
            return rs;
        }
        RefPtr<ILType> DeserializePointerLikeType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("ptr_like");
            reader.Read("<");
            RefPtr<ILArrayLikeType> rs = new ILArrayLikeType();
            rs->BaseType = ILType::Deserialize(reader);
            reader.Read(",");
            rs->Name = (ILArrayLikeTypeName)reader.ReadInt();
            reader.Read(">");
            return rs;
        }
        RefPtr<ILType> DeserializeTextureType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("texture");
            reader.Read("<");
            RefPtr<ILTextureType> rs = new ILTextureType();
            rs->BaseType = ILType::Deserialize(reader);
            reader.Read(",");
            rs->Flavor.Bits = reader.ReadUInt();
            reader.Read(">");
            return rs;
        }
        RefPtr<ILType> DeserializeGenericType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("generic");
            RefPtr<ILGenericType> rs = new ILGenericType();
            rs->GenericTypeName = reader.ReadWord();
            reader.Read("(");
            rs->BaseType = ILType::Deserialize(reader);
            reader.Read(")");
            return rs;
        }
        RefPtr<ILType> DeserializeRecordType(CoreLib::Text::TokenReader & reader)
        {
            reader.Read("record");
            RefPtr<ILRecordType> rs = new ILRecordType();
            rs->TypeName = reader.ReadWord();
            return rs;
        }
        RefPtr<ILType> ILType::Deserialize(CoreLib::Text::TokenReader & reader)
        {
			if (reader.LookAhead("basic"))
				return DeserializeBasicType(reader);
			else if (reader.LookAhead("struct"))
				return DeserializeStructType(reader);
			else if (reader.LookAhead("array"))
				return DeserializeArrayType(reader);
			else if (reader.LookAhead("array_like"))
				return DeserializeArrayLikeType(reader);
			else if (reader.LookAhead("ptr_like"))
				return DeserializePointerLikeType(reader);
			else if (reader.LookAhead("texture"))
				return DeserializeTextureType(reader);
			else if (reader.LookAhead("vector"))
				return DeserializeVectorType(reader);
			else if (reader.LookAhead("matrix"))
				return DeserializeMatrixType(reader);
			else if (reader.LookAhead("generic"))
				return DeserializeGenericType(reader);
			else if (reader.LookAhead("record"))
				return DeserializeRecordType(reader);
			else if (reader.LookAhead("SamplerState"))
				return DeserializeSamplerType(reader);
            return nullptr;
        }

        bool CFGNode::HasPhiInstruction()
        {
            return headInstr && headInstr->GetNext() && headInstr->GetNext()->Is<PhiInstruction>();
        }

        ILInstruction * CFGNode::GetFirstNonPhiInstruction()
        {
            for (auto & instr : *this)
            {
                if (!instr.Is<PhiInstruction>())
                    return &instr;
            }
            return tailInstr;
        }

        int NamingCounter = 0;

        void CFGNode::NameAllInstructions()
        {
            // name all operands
            StringBuilder numBuilder;
            for (auto & instr : GetAllInstructions())
            {
                numBuilder.Clear();
                for (auto & c : instr.Name)
                {
                    if (c >= '0' && c <= '9')
                        numBuilder.Append(c);
                    else
                        numBuilder.Clear();
                }
                auto num = numBuilder.ToString();
                if (num.Length())
                {
                    int id = StringToInt(num);
                    NamingCounter = Math::Max(NamingCounter, id + 1);
                }
            }
            HashSet<String> existingNames;
            for (auto & instr : GetAllInstructions())
            {
                if (instr.Name.Length() == 0)
                    instr.Name = String("t") + String(NamingCounter++, 16);
                else
                {
                    int counter = 1;
                    String newName = instr.Name;
                    while (existingNames.Contains(newName))
                    {
                        newName = instr.Name + String(counter);
                        counter++;
                    }
                    instr.Name = newName;
                }
                existingNames.Add(instr.Name);
            }
        }

        void CFGNode::DebugPrint()
        {
            printf("===========\n");
            for (auto& instr : *this)
            {
                printf("%S\n", instr.ToString().ToWString());
            }
            printf("===========\n");
        }

        LoadInstruction::LoadInstruction(ILOperand * dest)
        {
            Deterministic = false;
            Operand = dest;
            Type = dest->Type->Clone();
            if (!Spire::Compiler::Is<AllocVarInstruction>(dest) && !Spire::Compiler::Is<FetchArgInstruction>(dest))
                throw "invalid address operand";
        }
        void SubInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitSubInstruction(this);
        }
        void MulInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitMulInstruction(this);
        }
        void DivInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitDivInstruction(this);
        }
        void ModInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitModInstruction(this);
        }
        void AndInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitAndInstruction(this);
        }
        void OrInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitOrInstruction(this);
        }
        void BitAndInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitBitAndInstruction(this);
        }
        void BitOrInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitBitOrInstruction(this);
        }
        void BitXorInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitBitXorInstruction(this);
        }
        void ShlInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitShlInstruction(this);
        }
        void ShrInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitShrInstruction(this);
        }
        void CmpgtInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCmpgtInstruction(this);
        }
        void CmpgeInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCmpgeInstruction(this);
        }
        void CmpltInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCmpltInstruction(this);
        }
        void CmpleInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCmpleInstruction(this);
        }
        void CmpeqlInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCmpeqlInstruction(this);
        }
        void CmpneqInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCmpneqInstruction(this);
        }
        void CopyInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCopyInstruction(this);
        }
        void LoadInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitLoadInstruction(this);
        }
        void StoreInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitStoreInstruction(this);
        }
        void AllocVarInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitAllocVarInstruction(this);
        }
        void FetchArgInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitFetchArgInstruction(this);
        }
        void PhiInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitPhiInstruction(this);
        }
        void SelectInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitSelectInstruction(this);
        }
        void CallInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitCallInstruction(this);
        }
        void SwitchInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitSwitchInstruction(this);
        }
        void NotInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitNotInstruction(this);
        }
        void NegInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitNegInstruction(this);
        }
        void BitNotInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitBitNotInstruction(this);
        }
        void AddInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitAddInstruction(this);
        }
        void MemberAccessInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitMemberAccessInstruction(this);
        }
        AllInstructionsIterator & AllInstructionsIterator::operator++()
        {
            if (subBlockPtr < curInstr->GetSubBlockCount())
            {
                StackItem item;
                item.instr = curInstr;
                item.subBlockPtr = subBlockPtr + 1;
                stack.Add(item);
                curInstr = curInstr->GetSubBlock(subBlockPtr)->begin().Current;
                subBlockPtr = 0;
            }
            else
                curInstr = curInstr->GetNext();
            while (curInstr->GetNext() == nullptr && stack.Count() > 0)
            {
                auto item = stack.Last();
                stack.RemoveAt(stack.Count() - 1);
                curInstr = item.instr;
                subBlockPtr = item.subBlockPtr;
                if (subBlockPtr >= curInstr->GetSubBlockCount())
                {
                    subBlockPtr = 0;
                    curInstr = curInstr->GetNext();
                }
            }
            return *this;
        }
        AllInstructionsIterator AllInstructionsCollection::begin()
        {
            return AllInstructionsIterator(node->begin().Current);
        }
        AllInstructionsIterator AllInstructionsCollection::end()
        {
            return AllInstructionsIterator(node->end().Current);
        }
        ILType * ILStructType::Clone()
        {
            auto rs = new ILStructType(*this);
            rs->Members.Clear();
            for (auto & m : Members)
            {
                ILStructField f;
                f.FieldName = m.FieldName;
                f.Type = m.Type->Clone();
                rs->Members.Add(f);
            }
            return rs;
        }
        String ILStructType::ToString()
        {
            return TypeName;
        }
        bool ILStructType::Equals(ILType * type)
        {
            auto st = dynamic_cast<ILStructType*>(type);
            if (st && st->TypeName == this->TypeName)
                return true;
            return false;
        }
        ILType * ILRecordType::Clone()
        {
            auto rs = new ILRecordType(*this);
            rs->Members.Clear();
            for (auto & m : Members)
            {
                ILObjectDefinition f;
                f.Type = m.Value.Type->Clone();
                f.Name = m.Value.Name;
                f.Attributes = m.Value.Attributes;
                rs->Members.Add(m.Key, f);
            }
            return rs;
        }
        String ILRecordType::ToString()
        {
            return TypeName;
        }
        bool ILRecordType::Equals(ILType * type)
        {
            auto recType = dynamic_cast<ILRecordType*>(type);
            if (recType)
                return TypeName == recType->TypeName;
            else
                return false;
        }
        void DiscardInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitDiscardInstruction(this);
        }
        void SwizzleInstruction::Accept(InstructionVisitor * visitor)
        {
            visitor->VisitSwizzleInstruction(this);
        }
		String ILProgram::ToString()
		{
			
			StringBuilder sb;
			for (auto & s : Structs)
			{
                s->Serialize(sb);
                sb << "\n";
			}
			for (auto & vb : VariableBlocks)
			{
				switch (vb->Type)
				{
				case ILVariableBlockType::Constant:
					sb << "cbuffer {\n";
					break;
				default:
					sb << "vars {\n";
					break;
				}
				for (auto & v : vb->Vars)
				{
					sb << v.Value->Type->ToString() << " " << v.Key;
					if (v.Value->Code)
					{
						sb << " {\n";
						sb << v.Value->Code->ToString() << "\n}\n";
					}
					else
						sb << ";\n";
				}
				sb << "}\n";
			}
			for (auto & f : Functions)
			{
				sb << "func " << f.Key << "(";
				for (auto & param : f.Value->Parameters)
					sb << param.Value->Type->ToString() << " " << param.Key << ", ";
				sb << ") : " << f.Value->ReturnType->ToString();
				sb << "\n{\n" << f.Value->Code->ToString() << "\n}\n";
			}
			return sb.ProduceString();
		}
		ILType * ILSamplerStateType::Clone()
		{
			return new ILSamplerStateType(*this);
		}
		String ILSamplerStateType::ToString()
		{
			if (IsComparison)
				return "SamplerComparisonState";
			else
				return "SamplerState";
		}
		bool ILSamplerStateType::Equals(ILType * type)
		{
			auto sampler = dynamic_cast<ILSamplerStateType*>(type);
			if (sampler)
				return sampler->IsComparison == IsComparison;
			return false;
		}
}
}