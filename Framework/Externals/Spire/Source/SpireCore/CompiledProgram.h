#ifndef BAKER_SL_COMPILED_PROGRAM_H
#define BAKER_SL_COMPILED_PROGRAM_H

#include "../CoreLib/Basic.h"
#include "Diagnostics.h"
#include "IL.h"
#include "Syntax.h"
#include "TypeLayout.h"

namespace Spire
{
    namespace Compiler
    {
        class ShaderMetaData
        {
        public:
            CoreLib::String ShaderName;
            CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<ILModuleParameterSet>> ParameterSets; // bindingName->DescSet
        };

        class StageSource
        {
        public:
            String MainCode;
            List<unsigned char> BinaryCode;
        };

        class CompiledShaderSource
        {
        public:
            EnumerableDictionary<String, StageSource> Stages;
            ShaderMetaData MetaData;
        };

        void IndentString(StringBuilder & sb, String src);

        struct TranslationUnitResult
        {
            String outputSource;
        };

        class CompileResult
        {
        public:
            DiagnosticSink* mSink = nullptr;

            String ScheduleFile;
            RefPtr<ILProgram> Program;
            EnumerableDictionary<String, CompiledShaderSource> CompiledSource; // shader -> stage -> code
            
            // Per-translation-unit results
            List<TranslationUnitResult> translationUnits;

#if 0
            void PrintDiagnostics()
            {
                for (int i = 0; i < sink.diagnostics.Count(); i++)
                {
                    fprintf(stderr, "%S(%d): %s %d: %S\n",
                        sink.diagnostics[i].Position.FileName.ToWString(),
                        sink.diagnostics[i].Position.Line,
                        getSeverityName(sink.diagnostics[i].severity),
                        sink.diagnostics[i].ErrorID,
                        sink.diagnostics[i].Message.ToWString());
                }
            }
#endif

            CompileResult()
            {}
            ~CompileResult()
            {
            }
            DiagnosticSink * GetErrorWriter()
            {
                return mSink;
            }
            int GetErrorCount()
            {
                return mSink->GetErrorCount();
            }
        };

    }
}

#endif