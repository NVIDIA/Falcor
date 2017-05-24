#include "IL.h"
#include "CompiledProgram.h"

namespace Spire
{
    namespace Compiler
    {
        void AnalyzeSamplerUsage(CoreLib::EnumerableDictionary<ILModuleParameterInstance*, CoreLib::List<ILModuleParameterInstance*>> & samplerTextures,
            ILProgram * program, CFGNode * code, DiagnosticSink * sink);
    }
}