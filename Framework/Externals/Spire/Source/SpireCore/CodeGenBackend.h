#ifndef CODE_GEN_BACKEND_H
#define CODE_GEN_BACKEND_H

#include "../CoreLib/Basic.h"
#include "CompiledProgram.h"

namespace Spire
{
    namespace Compiler
    {		
        class CompileOptions;

        class CodeGenBackend : public CoreLib::Basic::Object
        {
        public:
            virtual CompiledShaderSource GenerateShader(const CompileOptions & options, ILProgram * program, DiagnosticSink * err) = 0;
        };

        CodeGenBackend * CreateGLSLCodeGen();
        CodeGenBackend * CreateGLSL_VulkanCodeGen();
        CodeGenBackend * CreateGLSL_VulkanOneDescCodeGen();
        CodeGenBackend * CreateHLSLCodeGen();
    }
}

#endif