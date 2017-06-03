#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"
#include "ShaderCompiler.h"

using namespace CoreLib::Basic;

namespace Spire
{
    namespace Compiler
    {
        void CLikeCodeGen::PrintType(StringBuilder & sbCode, ILType* type)
        {
            PrintTypeName(sbCode, type);
        }

        void CLikeCodeGen::PrintDef(StringBuilder & sbCode, ILType* type, const String & name)
        {
            PrintType(sbCode, type);
            sbCode << " ";
            sbCode << name;
            if (name.Length() == 0)
                throw InvalidProgramException("unnamed instruction.");
        }
        
        String CLikeCodeGen::GetFuncOriginalName(const String & name)
        {
            String originalName;
            int splitPos = name.IndexOf('@');
            if (splitPos == 0)
                return name;
            if (splitPos != -1)
                originalName = name.SubString(0, splitPos);
            else
                originalName = name;
            return originalName;
        }

        void CLikeCodeGen::PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression)
        {
            auto makeFloat = [](float v)
            {
                String rs(v, "%.12e");
                if (!rs.Contains('.') && !rs.Contains('e') && !rs.Contains('E'))
                    rs = rs + ".0";
                if (rs.StartsWith("-"))
                    rs = "(" + rs + ")";
                return rs;
            };
            
            if (auto c = dynamic_cast<ILConstOperand*>(op))
            {
                auto type = c->Type.Ptr();
                if (type->IsFloat())
                    ctx.Body << makeFloat(c->FloatValues[0]);
                else if (type->IsInt())
                    ctx.Body << (c->IntValues[0]);
                else if (type->IsBool())
                    ctx.Body << ((c->IntValues[0] != 0) ? "true" : "false");
                else if (auto baseType = dynamic_cast<ILVectorType*>(type))
                {
                    PrintType(ctx.Body, baseType);
                    ctx.Body << "(";
                    for (int i = 0; i < baseType->Size; i++)
                    {
                        if (baseType->BaseType == ILBaseType::Float)
                            ctx.Body << makeFloat(c->FloatValues[i]);
                        else if (baseType->BaseType == ILBaseType::Int)
                            ctx.Body << c->IntValues[i];
                        else if (baseType->BaseType == ILBaseType::Bool)
                            ctx.Body << (c->IntValues[i] ? "true" : "false");
                        else if (baseType->BaseType == ILBaseType::UInt)
                            ctx.Body << c->IntValues[i] << "u";
                        else if (baseType->BaseType == ILBaseType::UInt64)
                            ctx.Body << c->IntValues[i];
                        if (i != baseType->Size - 1)
                            ctx.Body << ", ";
                    }
                    ctx.Body << ")";
                }
                else if (auto matrixType = dynamic_cast<ILMatrixType*>(type))
                {
                    PrintType(ctx.Body, matrixType);
                    ctx.Body << "(";
                    for (int i = 0; i < matrixType->Size[0] * matrixType->Size[1]; i++)
                    {
                        if (matrixType->BaseType == ILBaseType::Float)
                            ctx.Body << makeFloat(c->FloatValues[i]);
                        else if (matrixType->BaseType == ILBaseType::Int)
                            ctx.Body << c->IntValues[i];
                        else if (matrixType->BaseType == ILBaseType::Bool)
                            ctx.Body << (c->IntValues[i] ? "true" : "false");
                        else if (matrixType->BaseType == ILBaseType::UInt)
                            ctx.Body << c->IntValues[i] << "u";
                        else if (matrixType->BaseType == ILBaseType::UInt64)
                            ctx.Body << c->IntValues[i];
                        if (i != matrixType->Size[0] * matrixType->Size[1] - 1)
                            ctx.Body << ", ";
                    }
                    ctx.Body << ")";
                }
                else
                    throw InvalidOperationException("Illegal constant.");
            }
            else if (auto instr = dynamic_cast<ILInstruction*>(op))
            {
                if (AppearAsExpression(*instr, forceExpression))
                {
                    PrintInstrExpr(ctx, *instr);
                }
                else
                {
                    if (forceExpression)
                        throw InvalidProgramException("cannot generate code block as an expression.");
                    String substituteName;
                    if (ctx.SubstituteNames.TryGetValue(instr->Name, substituteName))
                        ctx.Body << substituteName;
                    else
                        ctx.Body << instr->Name;
                }
            }
            else if (auto param = dynamic_cast<ILModuleParameterInstance*>(op))
            {
                PrintParameterReference(ctx.Body, param);
            }
            else
                throw InvalidOperationException("Unsupported operand type.");
        }

        static bool IsMatrix(ILOperand* operand)
        {
            auto type = operand->Type;
            // TODO(tfoley): This needs to be expanded once other matrix types are supported
            return type->IsFloatMatrix();
        }

        void CLikeCodeGen::PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1)
        {
            ctx.Body << "(";
            PrintOp(ctx, op0);
            ctx.Body << " * ";
            PrintOp(ctx, op1);
            ctx.Body << ")";
        }

        void CLikeCodeGen::PrintGlobalVarBlock(StringBuilder & sb, ILVariableBlock * gvarBlock)
        {
			if (gvarBlock->Vars.Count())
			{
				if (gvarBlock->Type == ILVariableBlockType::Constant)
				{
					//TODO: emit bindings
					sb << "cbuffer {\n";
				}
				for (auto gvar : gvarBlock->Vars)
				{
					PrintType(sb, gvar.Value->Type.Ptr());
					sb << " " << gvar.Value->Name << ";\n";
				}
				if (gvarBlock->Type == ILVariableBlockType::Constant)
				{
					//TODO: emit bindings
					sb << "};\n";
				}
			}
        }

        void CLikeCodeGen::PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr)
        {
            if (instr->Is<StoreInstruction>())
            {
                auto op0 = instr->Operands[0].Ptr();
                auto op1 = instr->Operands[1].Ptr();
                ctx.Body << "(";
                PrintOp(ctx, op0);
                ctx.Body << " = ";
                PrintOp(ctx, op1);
                ctx.Body << ")";
                return;
            }
            auto op0 = instr->Operands[0].Ptr();
            auto op1 = instr->Operands[1].Ptr();
            if (instr->Is<StoreInstruction>())
            {
                throw InvalidOperationException("store instruction cannot appear as expression.");
            }
            if (instr->Is<MemberAccessInstruction>())
            {
                PrintOp(ctx, op0);
                bool printDefault = true;
                auto vecType = op0->Type->AsVectorType();
                if (vecType)
                {
                    if (auto c = dynamic_cast<ILConstOperand*>(op1))
                    {
                        switch (c->IntValues[0])
                        {
                        case 0:
                            ctx.Body << ".x";
                            break;
                        case 1:
                            ctx.Body << ".y";
                            break;
                        case 2:
                            ctx.Body << ".z";
                            break;
                        case 3:
                            ctx.Body << ".w";
                            break;
                        default:
                            throw InvalidOperationException("Invalid member access.");
                        }
                        printDefault = false;
                    }
                }
                else if (auto structType = dynamic_cast<ILStructType*>(op0->Type.Ptr()))
                {
                    if (auto c = dynamic_cast<ILConstOperand*>(op1))
                    {
                        ctx.Body << "." << structType->Members[c->IntValues[0]].FieldName;
                    }
                    printDefault = false;
                }
                if (printDefault)
                {
                    ctx.Body << "[";
                    PrintOp(ctx, op1);
                    ctx.Body << "]";
                }
                return;
            }
            const char * op = "";
            if (instr->Is<AddInstruction>())
            {
                op = "+";
            }
            else if (instr->Is<SubInstruction>())
            {
                op = "-";
            }
            else if (instr->Is<MulInstruction>())
            {
                // For matrix-matrix, matrix-vector, and vector-matrix `*`,
                // GLSL performs a linear-algebraic inner product, while HLSL
                // always does element-wise product. We need to give the
                // codegen backend a change to handle this case.
                if(IsMatrix(op0) || IsMatrix(op1))
                {
                    PrintMatrixMulInstrExpr(ctx, op0, op1);
                    return;
                }

                op = "*";
            }
            else if (instr->Is<DivInstruction>())
            {
                op = "/";
            }
            else if (instr->Is<ModInstruction>())
            {
                op = "%";
            }
            else if (instr->Is<ShlInstruction>())
            {
                op = "<<";
            }
            else if (instr->Is<ShrInstruction>())
            {
                op = ">>";
            }
            else if (instr->Is<CmpeqlInstruction>())
            {
                op = "==";
                //ctx.Body << "int";
            }
            else if (instr->Is<CmpgeInstruction>())
            {
                op = ">=";
                //ctx.Body << "int";
            }
            else if (instr->Is<CmpgtInstruction>())
            {
                op = ">";
                //ctx.Body << "int";
            }
            else if (instr->Is<CmpleInstruction>())
            {
                op = "<=";
                //ctx.Body << "int";
            }
            else if (instr->Is<CmpltInstruction>())
            {
                op = "<";
                //ctx.Body << "int";
            }
            else if (instr->Is<CmpneqInstruction>())
            {
                op = "!=";
                //ctx.Body << "int";
            }
            else if (instr->Is<AndInstruction>())
            {
                op = "&&";
            }
            else if (instr->Is<OrInstruction>())
            {
                op = "||";
            }
            else if (instr->Is<BitXorInstruction>())
            {
                op = "^";
            }
            else if (instr->Is<BitAndInstruction>())
            {
                op = "&";
            }
            else if (instr->Is<BitOrInstruction>())
            {
                op = "|";
            }
            else
                throw InvalidProgramException("unsupported binary instruction.");
            ctx.Body << "(";
            PrintOp(ctx, op0);
            ctx.Body << " " << op << " ";
            PrintOp(ctx, op1);
            ctx.Body << ")";
        }

        void CLikeCodeGen::PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr)
        {
            auto op0 = instr->Operands[0].Ptr();
            auto op1 = instr->Operands[1].Ptr();
            if (instr->Is<StoreInstruction>())
            {
                PrintOp(ctx, op0);
                ctx.Body << " = ";
                PrintOp(ctx, op1);
                ctx.Body << ";\n";
                return;
            }
            auto varName = ctx.DefineVariable(instr);
            ctx.Body << varName << " = ";
            PrintBinaryInstrExpr(ctx, instr);
            ctx.Body << ";\n";
        }

        void CLikeCodeGen::PrintUnaryInstrExpr(CodeGenContext & ctx, UnaryInstruction * instr)
        {
            auto op0 = instr->Operand.Ptr();
            if (instr->Is<LoadInstruction>())
            {
                PrintOp(ctx, op0);
                return;
            }
            else if (instr->Is<SwizzleInstruction>())
            {
                PrintSwizzleInstrExpr(ctx, instr->As<SwizzleInstruction>());
                return;
            }
            const char * op = "";
            if (instr->Is<BitNotInstruction>())
                op = "~";
            else if (instr->Is<CopyInstruction>())
                op = "";
            else if (instr->Is<NegInstruction>())
                op = "-";
            else if (instr->Is<NotInstruction>())
                op = "!";
            else
                throw InvalidProgramException("unsupported unary instruction.");
            ctx.Body << "(" << op;
            PrintOp(ctx, op0);
            ctx.Body << ")";
        }

        void CLikeCodeGen::PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr)
        {
            auto varName = ctx.DefineVariable(instr);
            ctx.Body << varName << " = ";
            PrintUnaryInstrExpr(ctx, instr);
            ctx.Body << ";\n";
        }

        void CLikeCodeGen::PrintAllocVarInstrExpr(CodeGenContext & ctx, AllocVarInstruction * instr)
        {
            ctx.Body << instr->Name;
        }

        void CLikeCodeGen::PrintAllocVarInstr(CodeGenContext & ctx, AllocVarInstruction * instr)
        {
            ctx.DefineVariable(instr);
        }

        void CLikeCodeGen::PrintFetchArgInstrExpr(CodeGenContext & ctx, FetchArgInstruction * instr)
        {
            ctx.Body << instr->Name;
        }

        void CLikeCodeGen::PrintFetchArgInstr(CodeGenContext & ctx, FetchArgInstruction * instr)
        {
            if (instr->ArgId == 0)
            {
                ctx.ReturnVarName = ctx.DefineVariable(instr);
            }
        }

        void CLikeCodeGen::PrintSelectInstrExpr(CodeGenContext & ctx, SelectInstruction * instr)
        {
            ctx.Body << "(";
            PrintOp(ctx, instr->Operands[0].Ptr());
            ctx.Body << "?";
            PrintOp(ctx, instr->Operands[1].Ptr());
            ctx.Body << ":";
            PrintOp(ctx, instr->Operands[2].Ptr());
            ctx.Body << ")";
        }

        void CLikeCodeGen::PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr)
        {
            auto varName = ctx.DefineVariable(instr);
            ctx.Body << varName << " = ";
            PrintSelectInstrExpr(ctx, instr);
            ctx.Body << ";\n";
        }

        void CLikeCodeGen::PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name)
        {
            PrintDefaultCallInstrExpr(ctx, instr, name);
        }

        void CLikeCodeGen::PrintDefaultCallInstrArgs(CodeGenContext & ctx, CallInstruction * instr)
        {
            ctx.Body << "(";
            int id = 0;
            for (auto & arg : instr->Arguments)
            {
                PrintOp(ctx, arg.Ptr());
                if (id != instr->Arguments.Count() - 1)
                    ctx.Body << ", ";
                id++;
            }
            ctx.Body << ")";
        }


        void CLikeCodeGen::PrintDefaultCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr, String const& callName)
        {
            if (callName == "__init") // special casing for constructors
                PrintType(ctx.Body, instr->Type.Ptr());
            else
                ctx.Body << callName;
            PrintDefaultCallInstrArgs(ctx, instr);
        }

        void CLikeCodeGen::PrintCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr)
        {
            if (instr->Arguments.Count() > 0 && instr->Arguments.First()->Type->IsTexture() && intrinsicTextureFunctions.Contains(instr->Function))
            {
                PrintTextureCall(ctx, instr);
                return;
            }
            String callName;
            callName = GetFuncOriginalName(instr->Function);
            PrintCallInstrExprForTarget(ctx, instr, callName);
        }

        void CLikeCodeGen::PrintCallInstr(CodeGenContext & ctx, CallInstruction * instr)
        {
            if (!instr->Type->IsVoid())
            {
                auto varName = ctx.DefineVariable(instr);
                ctx.Body << varName;
                ctx.Body << " = ";
            }
            PrintCallInstrExpr(ctx, instr);
            ctx.Body << ";\n";
        }

        bool CLikeCodeGen::AppearAsExpression(ILInstruction & instr, bool force)
        {
            if (instr.Is<MemberAccessInstruction>())
                return true;
            if (instr.Is<ILGlobalVariable>())
                return true;
            if (auto arg = instr.As<FetchArgInstruction>())
            {
                if (arg->ArgId == 0)
                    return false;
            }
            if (instr.Is<StoreInstruction>() && force)
                return true;

            return (instr.Users.Count() <= 1 && !instr.HasSideEffect()
                && !instr.Is<AllocVarInstruction>())
                || instr.Is<FetchArgInstruction>();
        }

        void CLikeCodeGen::PrintSwizzleInstrExpr(CodeGenContext & ctx, SwizzleInstruction * swizzle)
        {
            PrintOp(ctx, swizzle->Operand.Ptr());
            ctx.Body << "." << swizzle->SwizzleString;
        }

        void CLikeCodeGen::PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr)
        {
            if (auto binInstr = instr.As<BinaryInstruction>())
                PrintBinaryInstrExpr(ctx, binInstr);
            else if (auto unaryInstr = instr.As<UnaryInstruction>())
                PrintUnaryInstrExpr(ctx, unaryInstr);
            else if (auto allocVar = instr.As<AllocVarInstruction>())
                PrintAllocVarInstrExpr(ctx, allocVar);
            else if (auto fetchArg = instr.As<FetchArgInstruction>())
                PrintFetchArgInstrExpr(ctx, fetchArg);
            else if (auto select = instr.As<SelectInstruction>())
                PrintSelectInstrExpr(ctx, select);
            else if (auto call = instr.As<CallInstruction>())
                PrintCallInstrExpr(ctx, call);
        }

        void CLikeCodeGen::PrintInstr(CodeGenContext & ctx, ILInstruction & instr)
        {
            // ctx.Body << "// " << instr.ToString() << ";\n";
            if (!AppearAsExpression(instr, false))
            {
                if (auto binInstr = instr.As<BinaryInstruction>())
                    PrintBinaryInstr(ctx, binInstr);
                else if (auto unaryInstr = instr.As<UnaryInstruction>())
                    PrintUnaryInstr(ctx, unaryInstr);
                else if (auto allocVar = instr.As<AllocVarInstruction>())
                    PrintAllocVarInstr(ctx, allocVar);
                else if (auto fetchArg = instr.As<FetchArgInstruction>())
                    PrintFetchArgInstr(ctx, fetchArg);
                else if (auto select = instr.As<SelectInstruction>())
                    PrintSelectInstr(ctx, select);
                else if (auto call = instr.As<CallInstruction>())
                    PrintCallInstr(ctx, call);
                    
            }
        }
        void CLikeCodeGen::GenerateCode(CodeGenContext & context, CFGNode * code)
        {
            for (auto & instr : *code)
            {
                if (auto ifInstr = instr.As<IfInstruction>())
                {
                    context.Body << "if (bool(";
                    PrintOp(context, ifInstr->Operand.Ptr());
                    context.Body << "))\n{\n";
                    GenerateCode(context, ifInstr->TrueCode.Ptr());
                    context.Body << "}\n";
                    if (ifInstr->FalseCode)
                    {
                        context.Body << "else\n{\n";
                        GenerateCode(context, ifInstr->FalseCode.Ptr());
                        context.Body << "}\n";
                    }
                }
                else if (auto forInstr = instr.As<ForInstruction>())
                {
                    context.Body << "for (";
                    if (forInstr->InitialCode)
                        PrintOp(context, forInstr->InitialCode->GetLastInstruction(), true);
                    context.Body << "; ";
                    if (forInstr->ConditionCode)
                        PrintOp(context, forInstr->ConditionCode->GetLastInstruction(), true);
                    context.Body << "; ";
                    if (forInstr->SideEffectCode)
                        PrintOp(context, forInstr->SideEffectCode->GetLastInstruction(), true);
                    context.Body << ")\n{\n";
                    GenerateCode(context, forInstr->BodyCode.Ptr());
                    context.Body << "}\n";
                }
                else if (auto doInstr = instr.As<DoInstruction>())
                {
                    context.Body << "do\n{\n";
                    GenerateCode(context, doInstr->BodyCode.Ptr());
                    context.Body << "} while (bool(";
                    PrintOp(context, doInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
                    context.Body << "));\n";
                }
                else if (auto whileInstr = instr.As<WhileInstruction>())
                {
                    context.Body << "while (bool(";
                    PrintOp(context, whileInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
                    context.Body << "))\n{\n";
                    GenerateCode(context, whileInstr->BodyCode.Ptr());
                    context.Body << "}\n";
                }
                else if (auto ret = instr.As<ReturnInstruction>())
                {
                    context.Body << "return ";
                    PrintOp(context, ret->Operand.Ptr());
                    context.Body << ";\n";
                }
                else if (instr.Is<BreakInstruction>())
                {
                    context.Body << "break;\n";
                }
                else if (instr.Is<ContinueInstruction>())
                {
                    context.Body << "continue;\n";
                }
                else if (instr.Is<DiscardInstruction>())
                {
                    context.Body << "discard;\n";
                }
                else
                    PrintInstr(context, instr);
            }
        }

        CLikeCodeGen::CLikeCodeGen()
        {
            intrinsicTextureFunctions.Add("Sample");
            intrinsicTextureFunctions.Add("SampleBias");
            intrinsicTextureFunctions.Add("SampleGrad");
            intrinsicTextureFunctions.Add("SampleLevel");
            intrinsicTextureFunctions.Add("SampleCmp");
        }

        void CLikeCodeGen::GenerateMetaData(ShaderMetaData & result, ILProgram * program, DiagnosticSink * /*err*/)
        {
            result.ParameterSets = program->ModuleParamSets;
        }

        CompiledShaderSource CLikeCodeGen::GenerateShader(const CompileOptions & /*options*/, ILProgram * program, DiagnosticSink * err)
        {
            this->errWriter = err;
            StringBuilder sbCode;
            CompiledShaderSource rs;
            PrintHeader(sbCode);
            GenerateStructs(sbCode, program);
			for (auto gvar : program->VariableBlocks)
			{
                PrintGlobalVarBlock(sbCode, gvar.Ptr());
			}
            for (auto func : program->Functions)
            {
                GenerateFunctionDeclaration(sbCode, func.Value.Ptr());
                sbCode << ";\n";
            }
            for (auto func : program->Functions)
                GenerateFunction(sbCode, func.Value.Ptr());
            StageSource src;
            src.MainCode = sbCode.ProduceString();
            rs.Stages.Add("src", src);
            GenerateMetaData(rs.MetaData, program, err);
            return rs;
        }

        void CLikeCodeGen::GenerateStructs(StringBuilder & sb, ILProgram * program)
        {
            for (auto & st : program->Structs)
            {
                if (!st->IsIntrinsic)
                {
                    sb << "struct " << st->TypeName << "\n{\n";
                    for (auto & f : st->Members)
                    {
                        sb << f.Type->ToString();
                        sb << " " << f.FieldName << ";\n";
                    }
                    sb << "};\n";
                }
            }
        }

        void CLikeCodeGen::GenerateFunctionDeclaration(StringBuilder & sbCode, ILFunction * function)
        {
            function->Code->NameAllInstructions();
            auto retType = function->ReturnType.Ptr();
            if (retType)
                PrintType(sbCode, retType);
            else
                sbCode << "void";
            sbCode << " " << GetFuncOriginalName(function->Name) << "(";
            int id = 0;
            auto paramIter = function->Parameters.begin();
            for (auto & instr : *function->Code)
            {
                if (auto arg = instr.As<FetchArgInstruction>())
                {
                    if (arg->ArgId != 0)
                    {
                        if (id > 0)
                        {
                            sbCode << ", ";
                        }
                        auto qualifier = (*paramIter).Value->Qualifier;
                        if (qualifier == ParameterQualifier::InOut)
                            sbCode << "inout ";
                        else if (qualifier == ParameterQualifier::Out)
                            sbCode << "out ";
                        else if (qualifier == ParameterQualifier::Uniform)
                            sbCode << "uniform ";
                        PrintDef(sbCode, arg->Type.Ptr(), arg->Name);
                        id++;
                    }
                    ++paramIter;
                }
            }
            sbCode << ")";
        }
        void CLikeCodeGen::GenerateFunction(StringBuilder & sbCode, ILFunction * function)
        {
            CodeGenContext ctx;
            ctx.codeGen = this;
            ctx.UsedVarNames.Clear();
            ctx.Body.Clear();
            ctx.Header.Clear();
            ctx.Arguments.Clear();
            ctx.ReturnVarName = "";
            ctx.VarName.Clear();
                
            function->Code->NameAllInstructions();
            GenerateFunctionDeclaration(sbCode, function);
            sbCode << "\n{\n";
            GenerateCode(ctx, function->Code.Ptr());
            sbCode << ctx.Header.ToString() << ctx.Body.ToString();
            if (ctx.ReturnVarName.Length())
                sbCode << "return " << ctx.ReturnVarName << ";\n";
            sbCode << "}\n";
        }

        String CodeGenContext::DefineVariable(ILOperand * op)
        {
            String rs;
            if (VarName.TryGetValue(op, rs))
            {
                return rs;
            }
            else
            {
                auto name = GenerateCodeName(op->Name, "");
                codeGen->PrintDef(Header, op->Type.Ptr(), name);
                if (op->Type->IsInt() || op->Type->IsUInt())
                {
                    Header << " = 0";
                }
                Header << ";\n";
                VarName.Add(op, name);
                op->Name = name;
                return op->Name;
            }
        }
    }
}