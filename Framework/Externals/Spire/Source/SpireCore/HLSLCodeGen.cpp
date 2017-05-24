#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"

#include <cassert>

using namespace CoreLib::Basic;

namespace Spire
{
    namespace Compiler
    {
        class HLSLCodeGen : public CLikeCodeGen
        {
        protected:
            void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) override
            {
                ctx.Body << "stage_output.sv_position = ";
                PrintOp(ctx, operand);
                ctx.Body << ";\n";
            }

            void PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1) override
            {
                // The matrix-vector, vector-matrix, and matrix-matrix product
                // operation is written with the `*` operator in GLSL, but
                // is handled by the built-in function `mul()` in HLSL.
                //
                // This function is called by the code generator for that op
                // and allows us to print it appropriately.

                ctx.Body << "mul(";
                PrintOp(ctx, op1);
                ctx.Body << ", ";
                PrintOp(ctx, op0);
                ctx.Body << ")";
            }

            void PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name) override
            {
                // Currently, all types are internally named based on their GLSL equivalent, so
                // for HLSL output we go ahead and maintain a big table to remap the names.
                //
                // Note: for right now, this is just a linear array, with no particular sorting.
                // Eventually it should be turned into a hash table for performance, or at least
                // just be kept sorted so that we can use a binary search.
                //
                // Note 2: Well, actually, the Right Answer is for the type representation to
                // be better than just a string, so that we don't have to do this string->string map.
                static const struct {
                    char const* glslName;
                    char const* hlslName;
                } kNameRemaps[] =
                {
                    { "vec2", "*float2" },
                    { "vec3", "*float3" },
                    { "vec4", "*float4" },

                    { "ivec2", "*int2" },
                    { "ivec3", "*int3" },
                    { "ivec4", "*int4" },

                    { "uvec2", "*uint2" },
                    { "uvec3", "*uint3" },
                    { "uvec4", "*uint4" },

                    { "mat3", "float3x3" },
                    { "mat4", "float4x4" },

                    { "sampler2D", "Texture2D" },
                    { "sampler2DArray", "Texture2DArray" },
                    { "samplerCube", "TextureCube" },
                    { "sampler2DShadow", "Texture2D" },
                    { "sampler2DArrayShadow", "Texture2DArray" },
                    { "samplerCubeShadow", "TextureCube" },
                };

                for(auto remap : kNameRemaps)
                {
                    if(strcmp(name.Buffer(), remap.glslName) == 0)
                    {
                        char const* hlslName = remap.hlslName;
                        if(*hlslName == '*')
                        {
                            hlslName++;

                            // Note(tfoley): The specific case we are dealing with right
                            // now is that constructing a vector from a scalar value
                            // *must* be expressed as a cast in HLSL, while in GLSL
                            // it *must* be expressed as a constructor call. We
                            // intercept the call to a constructor here in the
                            // specific case where it has one argument, and print
                            // it differently
                            if(instr->Arguments.Count() == 1)
                            {
                                ctx.Body << "((" << hlslName << ") ";
                                PrintOp(ctx, instr->Arguments[0].Ptr());
                                ctx.Body << ")";
                                return;
                            }
                        }

                        PrintDefaultCallInstrExpr(ctx, instr, hlslName);
                        return;
                    }
                }

                PrintDefaultCallInstrExpr(ctx, instr, name);
            }

            void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
            {
                // texture functions are defined based on HLSL, so this is trivial
                // internally, texObj.Sample(sampler_obj, uv, ..) is represented as Sample(texObj, sampler_obj, uv, ...)
                // so we need to lift first argument to the front
                PrintOp(ctx, instr->Arguments[0].Ptr(), true);
                ctx.Body << "." << instr->Function;
                ctx.Body << "(";
                for (int i = 1; i < instr->Arguments.Count(); i++)
                {
                    PrintOp(ctx, instr->Arguments[i].Ptr());
                    if (i < instr->Arguments.Count() - 1)
                        ctx.Body << ", ";
                }
                ctx.Body << ")";
            }
			
            void PrintTypeName(StringBuilder& sb, ILType* type) override
            {
                // Currently, all types are internally named based on their GLSL equivalent, so
                // for HLSL output we go ahead and maintain a big table to remap the names.
                //
                // Note: for right now, this is just a linear array, with no particular sorting.
                // Eventually it should be turned into a hash table for performance, or at least
                // just be kept sorted so that we can use a binary search.
                //
                // Note 2: Well, actually, the Right Answer is for the type representation to
                // be better than just a string, so that we don't have to do this string->string map.
                static const struct {
                    char const* glslName;
                    char const* hlslName;
                } kNameRemaps[] =
                {
                    { "vec2", "float2" },
                    { "vec3", "float3" },
                    { "vec4", "float4" },

                    { "ivec2", "int2" },
                    { "ivec3", "int3" },
                    { "ivec4", "int4" },

                    { "uvec2", "uint2" },
                    { "uvec3", "uint3" },
                    { "uvec4", "uint4" },

                    { "mat3", "float3x3" },
                    { "mat4", "float4x4" },

                    { "sampler2D", "Texture2D" },
                    { "sampler2DArray", "Texture2DArray" },
                    { "samplerCube", "TextureCube" },
                    { "sampler2DShadow", "Texture2D" },
                    { "sampler2DArrayShadow", "Texture2DArray" },
                    { "samplerCubeShadow", "TextureCube" },
                    { "sampler3D", "Texture3D" }
                };

                String typeName = type->ToString();
                for(auto remap : kNameRemaps)
                {
                    if(strcmp(typeName.Buffer(), remap.glslName) == 0)
                    {
                        sb << remap.hlslName;
                        return;
                    }
                }

                // If we don't find the type in our map, then that either means we missed a case,
                // or this is a user-defined type. I don't see an obvious way to check which of
                // those cases we are in, so we will just fall back to outputting the "GLSL name" here.
                sb << type->ToString();
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

            void PrintHeader(StringBuilder& sb)
            {
                // The way that we assign semantics may generate a warning,
                // and rather than clear it up with more complicated codegen,
                // we choose to just disable it (since we control all the
                // semantics anyway).
                sb << "#pragma warning(disable: 3576)\n";

                // In order to be a portable shading language, Spire needs
                // to make some basic decisions about the semantics of
                // matrix operations so they are consistent between APIs.
                //
                // The default interpretation in each language is:
                //   GLSL: column-major storage, column-major semantics
                //   HLSL: column-major storage, row-major semantics
                //
                // We can't change the semantics, but we *can* change
                // the storage layout, and making it be row-major in
                // HLSL ensures that the actual behavior of a shader
                // is consistent between APIs.
                sb << "#pragma pack_matrix( row_major )\n";
            }
        };

        CodeGenBackend * CreateHLSLCodeGen()
        {
            return new HLSLCodeGen();
        }
    }
}