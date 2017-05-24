#include "SamplerUsageAnalysis.h"

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib;

        void AnalyzeSamplerUsageImpl(EnumerableDictionary<ILModuleParameterInstance*, List<ILModuleParameterInstance*>> & samplerTextures,
            ILProgram * program,
            CFGNode * code,
            DiagnosticSink * sink,
            HashSet<ILFunction*> & processedFunctions,
            Dictionary<int, ILModuleParameterInstance*> & paramValues)
        {
            for (auto & instr : code->GetAllInstructions())
            {
                auto call = dynamic_cast<CallInstruction*>(&instr);
                if (call)
                {
                    if (call->Function.StartsWith("Sample") && !program->Functions.ContainsKey(call->Function))
                    {
                        auto textureOp = call->Arguments[0].Ptr();
                        auto samplerOp = call->Arguments[1].Ptr();
                        ILModuleParameterInstance * textureInstance = nullptr;
                        ILModuleParameterInstance * samplerInstance = nullptr;
                        if (auto texArg = dynamic_cast<FetchArgInstruction*>(textureOp))
                            paramValues.TryGetValue(texArg->ArgId, textureInstance);
                        else if (auto texArg1 = dynamic_cast<ILModuleParameterInstance*>(textureOp))
                            textureInstance = texArg1;
                        if (auto samplerArg = dynamic_cast<FetchArgInstruction*>(samplerOp))
                            paramValues.TryGetValue(samplerArg->ArgId, samplerInstance);
                        else if (auto samplerArg1 = dynamic_cast<ILModuleParameterInstance*>(samplerOp))
                            samplerInstance = samplerArg1;
                        if (textureInstance && samplerInstance)
                        {
                            auto list = samplerTextures.TryGetValue(samplerInstance);
                            if (!list)
                            {
                                samplerTextures.Add(samplerInstance, List<ILModuleParameterInstance*>());
                                list = samplerTextures.TryGetValue(samplerInstance);
                            }
                            list->Add(textureInstance);
                        }
                        else
                        {
                            SPIRE_INTERNAL_ERROR(sink, instr.Position);
                        }
                    }
                    else
                    {
                        RefPtr<ILFunction> func = nullptr;
                        if (program->Functions.TryGetValue(call->Function, func))
                        {
                            if (!processedFunctions.Contains(func.Ptr()))
                            {
                                // trace into the function call
                                Dictionary<int, ILModuleParameterInstance*> args;
                                for (int i = 0; i < call->Arguments.Count(); i++)
                                {
                                    if (auto arg = dynamic_cast<FetchArgInstruction*>(call->Arguments[i].Ptr()))
                                    {
                                        ILModuleParameterInstance * argValue = nullptr;
                                        if (paramValues.TryGetValue(arg->ArgId, argValue))
                                            args[i + 1] = argValue;
                                    }
                                    else if (auto argValue = dynamic_cast<ILModuleParameterInstance*>(call->Arguments[i].Ptr()))
                                    {
                                        args[i + 1] = argValue;
                                    }
                                }
                                if (func->Code)
                                {
                                    processedFunctions.Add(func.Ptr());
                                    AnalyzeSamplerUsageImpl(samplerTextures, program, func->Code.Ptr(), sink,
                                        processedFunctions, args);
                                    processedFunctions.Remove(func.Ptr());
                                }
                            }
                        }
                    }
                }
            }
        }

        void AnalyzeSamplerUsage(EnumerableDictionary<ILModuleParameterInstance*, List<ILModuleParameterInstance*>> & samplerTextures, 
            ILProgram * program, CFGNode * code, DiagnosticSink * sink)
        {
            Dictionary<int, ILModuleParameterInstance*> params;
            HashSet<ILFunction*> processedFuncs;
            AnalyzeSamplerUsageImpl(samplerTextures, program, code, sink, processedFuncs, params);
        }
    }
}