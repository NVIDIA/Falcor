// CLikeCodeGen.h
#ifndef SPIRE_C_LIKE_CODE_GEN_H
#define SPIRE_C_LIKE_CODE_GEN_H

//
// This file implements the shared logic for code generation in C-like
// languages, such as GLSL and HLSL.
//

#include "CodeGenBackend.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib::Basic;

        class CLikeCodeGen;

        class CodeGenContext
        {
        public:
            CLikeCodeGen * codeGen;
            HashSet<String> GeneratedDefinitions;
            Dictionary<String, String> SubstituteNames;
            Dictionary<ILOperand*, String> VarName;
            CompileResult * Result = nullptr;
            HashSet<String> UsedVarNames;
            int BufferAllocator = 0;
            StringBuilder Body, Header, GlobalHeader;
            List<ILType*> Arguments;
            String ReturnVarName;
            String GenerateCodeName(String name, String prefix)
            {
                StringBuilder nameBuilder;
                int startPos = 0;
                if (name.StartsWith("_sys_"))
                    startPos = name.IndexOf('_', 5) + 1;
                nameBuilder << prefix;
                for (int i = startPos; i < name.Length(); i++)
                {
                    if ((name[i] >= 'a' && name[i] <= 'z') || 
                        (name[i] >= 'A' && name[i] <= 'Z') ||
                        name[i] == '_' || 
                        (name[i] >= '0' && name[i] <= '9'))
                    {
                        nameBuilder << name[i];
                    }
                    else
                        nameBuilder << '_';
                }
                auto rs = nameBuilder.ToString();
                int i = 0;
                while (UsedVarNames.Contains(rs))
                {
                    i++;
                    rs = nameBuilder.ToString() + String(i);
                }
                UsedVarNames.Add(rs);

                return rs;
            }
            String DefineVariable(ILOperand * op);
        };

        class CLikeCodeGen : public CodeGenBackend
        {
        protected:
            HashSet<String> intrinsicTextureFunctions;
            bool useBindlessTexture = false;
            DiagnosticSink * errWriter;

            virtual void PrintParameterReference(StringBuilder& sb, ILModuleParameterInstance * param) = 0;
            
            virtual void PrintTypeName(StringBuilder& sb, ILType* type) = 0;
            virtual void PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name);
            virtual void PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1);
            virtual void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) = 0;
            virtual void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr) = 0;
            virtual void PrintGlobalVarBlock(StringBuilder & sb, ILVariableBlock * gvarBlock);
            virtual void PrintHeader(StringBuilder & sb) = 0;

            // Helpers for printing call instructions
            void PrintDefaultCallInstrArgs(CodeGenContext & ctx, CallInstruction * instr);
            void PrintDefaultCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr, String const& name);

        public:
            DiagnosticSink* getSink() { return this->errWriter; }
            void PrintType(StringBuilder & sbCode, ILType* type);

            void PrintDef(StringBuilder & sbCode, ILType* type, const String & name);

            String GetFuncOriginalName(const String & name);

            virtual void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false);
            void PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr);
            void PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr);
            void PrintUnaryInstrExpr(CodeGenContext & ctx, UnaryInstruction * instr);
            void PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr);
            void PrintAllocVarInstrExpr(CodeGenContext & ctx, AllocVarInstruction * instr);
            void PrintAllocVarInstr(CodeGenContext & ctx, AllocVarInstruction * instr);
            void PrintFetchArgInstrExpr(CodeGenContext & ctx, FetchArgInstruction * instr);
            void PrintFetchArgInstr(CodeGenContext & ctx, FetchArgInstruction * instr);
            void PrintSelectInstrExpr(CodeGenContext & ctx, SelectInstruction * instr);
            void PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr);
            void PrintCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr);
            void PrintCallInstr(CodeGenContext & ctx, CallInstruction * instr);
            bool AppearAsExpression(ILInstruction & instr, bool force);
            void PrintSwizzleInstrExpr(CodeGenContext & ctx, SwizzleInstruction * swizzle);
            void PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr);
            void PrintInstr(CodeGenContext & ctx, ILInstruction & instr);
            void GenerateCode(CodeGenContext & context, CFGNode * code);

        public:
            CLikeCodeGen();
            virtual void GenerateMetaData(ShaderMetaData & result, ILProgram* program, DiagnosticSink * err);
            virtual CompiledShaderSource GenerateShader(const CompileOptions & options, ILProgram * program, DiagnosticSink * err) override;
            void GenerateStructs(StringBuilder & sb, ILProgram * program);
            
            void GenerateFunctionDeclaration(StringBuilder & sbCode, ILFunction * function);
            void GenerateFunction(StringBuilder & sbCode, ILFunction * function);
        };
    }
}

#endif // SPIRE_C_LIKE_CODE_GEN_H
