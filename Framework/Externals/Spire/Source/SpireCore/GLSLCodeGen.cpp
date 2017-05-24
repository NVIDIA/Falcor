#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"
#include "SamplerUsageAnalysis.h"
#include <cassert>

using namespace CoreLib::Basic;

namespace Spire
{
    namespace Compiler
    {
        class GLSLCodeGen : public CLikeCodeGen
        {
        private:
            bool useVulkanBinding = false;
            bool useSingleDescSet = false;
        protected:
            void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false) override
            {
                if (!useVulkanBinding && op->Type->IsSamplerState())
                {
                    // GLSL does not have sampler type, print 0 as placeholder
                    ctx.Body << "0";
                    return;
                }
                CLikeCodeGen::PrintOp(ctx, op, forceExpression);
            }

            void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) override
            {
                ctx.Body << "gl_Position = ";
                PrintOp(ctx, operand);
                ctx.Body << ";\n";
            }
            
            String GetTextureType(ILType * textureType)
            {
                auto texType = dynamic_cast<ILTextureType*>(textureType);
                StringBuilder sb;
                switch (texType->Flavor.Fields.Shape)
                {
                case ILTextureShape::Texture1D:
                    sb << "texture1D";
                    break;
                case ILTextureShape::Texture2D:
                    sb << "texture2D";
                    break;
                case ILTextureShape::Texture3D:
                    sb << "texture3D";
                    break;
                case ILTextureShape::TextureCube:
                    sb << "textureCube";
                    break;
                }
                if (texType->Flavor.Fields.IsMultisample)
                    sb << "MS";
                if (texType->Flavor.Fields.IsArray)
                    sb << "Array";
                if (texType->Flavor.Fields.IsShadow)
                    sb << "Shadow";
                return sb.ProduceString();
            }

            String GetSamplerType(ILType * textureType)
            {
                auto texType = dynamic_cast<ILTextureType*>(textureType);
                StringBuilder sb;
                switch (texType->Flavor.Fields.Shape)
                {
                case ILTextureShape::Texture1D:
                    sb << "sampler1D";
                    break;
                case ILTextureShape::Texture2D:
                    sb << "sampler2D";
                    break;
                case ILTextureShape::Texture3D:
                    sb << "sampler3D";
                    break;
                case ILTextureShape::TextureCube:
                    sb << "samplerCube";
                    break;
                }
                if (texType->Flavor.Fields.IsMultisample)
                    sb << "MS";
                if (texType->Flavor.Fields.IsArray)
                    sb << "Array";
                if (texType->Flavor.Fields.IsShadow)
                    sb << "Shadow";
                return sb.ProduceString();
            }

            void PrintTypeName(StringBuilder& sb, ILType* type) override
            {
                // Currently, all types are internally named based on their GLSL equivalent, so
                // outputting a type for GLSL is trivial.

                // GLSL does not have sampler type, use int as placeholder
                if (type->IsSamplerState())
                {
                    if (useVulkanBinding)
                    {
                        if (dynamic_cast<ILSamplerStateType*>(type)->IsComparison)
                            sb << "samplerShadow";
                        else
                            sb << "sampler";
                    }
                    else
                        sb << "int";
                }
                else if (type->IsTexture())
                {
                    if (useVulkanBinding)
                        sb << GetTextureType(type);
                    else
                        sb << GetSamplerType(type);
                }
                else
                    sb << type->ToString();
            }

            void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
            {
                auto printSamplerArgument = [&](ILOperand * texture, ILOperand * sampler)
                {
                    if (useVulkanBinding)
                    {
                        ctx.Body << GetSamplerType(texture->Type.Ptr()) << "(";
                        PrintOp(ctx, texture);
                        ctx.Body << ", ";
                        PrintOp(ctx, sampler);
                        ctx.Body << ")";
                    }
                    else
                    {
                        PrintOp(ctx, texture);
                    }
                    ctx.Body << ", ";
                };
                if (instr->Function == "Sample")
                {
                    if (instr->Arguments.Count() == 4)
                        ctx.Body << "textureOffset";
                    else
                        ctx.Body << "texture";
                    ctx.Body << "(";
                    printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
                    for (int i = 2; i < instr->Arguments.Count(); i++)
                    {
                        PrintOp(ctx, instr->Arguments[i].Ptr());
                        if (i < instr->Arguments.Count() - 1)
                            ctx.Body << ", ";
                    }
                    ctx.Body << ")";
                }
                else if (instr->Function == "SampleLevel")
                {
                    ctx.Body << "textureLod";
                    ctx.Body << "(";
                    printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
                    for (int i = 2; i < instr->Arguments.Count(); i++)
                    {
                        PrintOp(ctx, instr->Arguments[i].Ptr());
                        if (i < instr->Arguments.Count() - 1)
                            ctx.Body << ", ";
                    }
                    ctx.Body << ")";
                }
                else if (instr->Function == "SampleGrad")
                {
                    if (instr->Arguments.Count() == 6)
                        ctx.Body << "textureGradOffset";
                    else
                        ctx.Body << "textureGrad";
                    ctx.Body << "(";
                    printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
                    for (int i = 2; i < instr->Arguments.Count(); i++)
                    {
                        PrintOp(ctx, instr->Arguments[i].Ptr());
                        if (i < instr->Arguments.Count() - 1)
                            ctx.Body << ", ";
                    }
                    ctx.Body << ")";
                }
                else if (instr->Function == "SampleBias")
                {
                    if (instr->Arguments.Count() == 5) // loc, bias, offset
                    {
                        ctx.Body << "textureOffset(";
                        printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
                        PrintOp(ctx, instr->Arguments[2].Ptr());
                        ctx.Body << ", ";
                        PrintOp(ctx, instr->Arguments[4].Ptr());
                        ctx.Body << ", ";
                        PrintOp(ctx, instr->Arguments[3].Ptr());
                        ctx.Body << ")";
                    }
                    else
                    {
                        ctx.Body << "texture(";
                        printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
                        PrintOp(ctx, instr->Arguments[2].Ptr());
                        ctx.Body << ", ";
                        PrintOp(ctx, instr->Arguments[3].Ptr());
                        ctx.Body << ")";
                    }
                }
                else if (instr->Function == "SampleCmp")
                {
                    if (instr->Arguments.Count() == 5)
                        ctx.Body << "textureOffset(";
                    else
                        ctx.Body << "texture(";
                    printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
                    auto texType = dynamic_cast<ILTextureType*>(instr->Arguments[0]->Type.Ptr());
                    if (texType)
                    {
                        if (texType->Flavor.Fields.Shape == ILTextureShape::Texture1D && !texType->Flavor.Fields.IsArray)
                            ctx.Body << "vec2(";
                        else if (texType->Flavor.Fields.Shape == ILTextureShape::Texture2D && !texType->Flavor.Fields.IsArray)
                            ctx.Body << "vec3(";
                        else if (texType->Flavor.Fields.Shape == ILTextureShape::Texture3D || (texType->Flavor.Fields.Shape == ILTextureShape::Texture2D && texType->Flavor.Fields.IsArray) ||
                            (texType->Flavor.Fields.Shape == ILTextureShape::TextureCube && !texType->Flavor.Fields.IsArray) )
                            ctx.Body << "vec4(";
                        PrintOp(ctx, instr->Arguments[2].Ptr());
                        ctx.Body << ", ";
                        PrintOp(ctx, instr->Arguments[3].Ptr());
                        ctx.Body << ")";
                        if (instr->Arguments.Count() == 5)
                        {
                            ctx.Body << ", ";
                            PrintOp(ctx, instr->Arguments[4].Ptr());
                        }
                    }
                    ctx.Body << ")";
                }
                else
                    throw NotImplementedException("CodeGen for texture function '" + instr->Function + "' is not implemented.");
            }

            virtual void PrintHeader(StringBuilder & sb) override
            {
                sb << "#version 440\n";
                if (useBindlessTexture)
                    sb << "#extension GL_ARB_bindless_texture: require\n#extension GL_NV_gpu_shader5 : require\n";
            }

            virtual void PrintParameterReference(StringBuilder& sb, ILModuleParameterInstance * param) override
            {
                if (param->Type->GetBindableResourceType() == BindableResourceType::NonBindable)
                {
                    auto bufferName = EscapeCodeName(param->Module->BindingName);
                    sb << bufferName << "." << param->Name;
                }
                else
                {
                    sb << EscapeCodeName(param->Module->BindingName + "_" + param->Name);
                }
            }
           
            virtual void GenerateMetaData(ShaderMetaData & result, ILProgram* program, DiagnosticSink* err) override
            {
                EnumerableDictionary<ILModuleParameterInstance*, List<ILModuleParameterInstance*>> samplerTextures;
                for (auto & f : program->Functions)
                {
                    if (f.Value->IsEntryPoint)
                        AnalyzeSamplerUsage(samplerTextures, program, f.Value->Code.Ptr(), err);
                }
                if (!useVulkanBinding)
                {
                    for (auto & sampler : samplerTextures)
                    {
                        sampler.Key->BindingPoints.Clear();
                        for (auto & tex : sampler.Value)
                            sampler.Key->BindingPoints.AddRange(tex->BindingPoints);
                    }
                }
                CLikeCodeGen::GenerateMetaData(result, program, err);
            }
        public:
            GLSLCodeGen(bool vulkanBinding, bool pUseSingleDescSet)
            {
                useVulkanBinding = vulkanBinding;
                useSingleDescSet = pUseSingleDescSet;
            }
        };
        CodeGenBackend * CreateGLSLCodeGen()
        {
            return new GLSLCodeGen(false, false);
        }
        CodeGenBackend * CreateGLSL_VulkanCodeGen()
        {
            return new GLSLCodeGen(true, false);
        }
        CodeGenBackend * CreateGLSL_VulkanOneDescCodeGen()
        {
            return new GLSLCodeGen(true, true);
        }
    }
}