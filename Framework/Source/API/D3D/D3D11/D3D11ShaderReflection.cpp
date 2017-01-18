/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "Framework.h"
#include "ShaderReflectionD3D11.h"

namespace Falcor
{
    namespace ShaderReflection
    {
        void reflectBuffers(ID3D11ShaderReflectionPtr pReflector, BufferDescMap& bufferVec)
        {
            D3D11_SHADER_DESC shaderDesc;
            pReflector->GetDesc(&shaderDesc);

            // CBs doesn't have to be continuous, we loop until we find all available buffers
            for(uint32_t found = 0, cbIndex = 0; found < shaderDesc.ConstantBuffers; cbIndex++)
            {
                D3D11_SHADER_BUFFER_DESC bufferDesc;
                ID3D11ShaderReflectionConstantBuffer* pBuffer = pReflector->GetConstantBufferByIndex(cbIndex);
                HRESULT hr = pBuffer->GetDesc(&bufferDesc);
                // Check that we found a CB
                if(SUCCEEDED(hr))
                {
                    if((bufferDesc.Type == D3D_CT_TBUFFER) || (bufferDesc.Type == D3D_CT_CBUFFER))
                    {
                        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
                        d3d_call(pReflector->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc));
                        assert(bindDesc.BindCount == 1);

                        BufferDesc falcorDesc;
                        falcorDesc.bufferSlot = bindDesc.BindPoint;
                        falcorDesc.sizeInBytes = bufferDesc.Size;
                        falcorDesc.variableCount = bufferDesc.Variables;
                        assert(bufferDesc.Type == D3D_CT_CBUFFER);  // Not sure how to handle texture buffers
                        falcorDesc.type = BufferDesc::Type::Constant;
                        bufferVec[std::string(bufferDesc.Name)] = falcorDesc;
                    }
                    found++;
                }
            }
        }

        VariableDesc::Type getVariableType(D3D_SHADER_VARIABLE_TYPE dxType, uint32_t rows, uint32_t columns)
        {
            if(dxType == D3D_SVT_BOOL)
            {
                assert(rows == 1);
                switch(columns)
                {
                case 1:
                    return VariableDesc::Type::Bool;
                case 2:
                    return VariableDesc::Type::Bool2;
                case 3:
                    return VariableDesc::Type::Bool3;
                case 4:
                    return VariableDesc::Type::Bool4;
                }
            }
            else if(dxType == D3D_SVT_UINT)
            {
                assert(rows == 1);
                switch(columns)
                {
                case 1:
                    return VariableDesc::Type::Uint;
                case 2:
                    return VariableDesc::Type::Uint2;
                case 3:
                    return VariableDesc::Type::Uint3;
                case 4:
                    return VariableDesc::Type::Uint4;
                }
            }
            else if(dxType == D3D_SVT_INT)
            {
                assert(rows == 1);
                switch(columns)
                {
                case 1:
                    return VariableDesc::Type::Int;
                case 2:
                    return VariableDesc::Type::Int2;
                case 3:
                    return VariableDesc::Type::Int3;
                case 4:
                    return VariableDesc::Type::Int4;
                }
            }
            else if(dxType == D3D_SVT_FLOAT)
            {
                switch(rows)
                {
                case 1:
                    switch(columns)
                    {
                    case 1:
                        return VariableDesc::Type::Float;
                    case 2:
                        return VariableDesc::Type::Float2;
                    case 3:
                        return VariableDesc::Type::Float3;
                    case 4:
                        return VariableDesc::Type::Float4;
                    }
                    break;
                case 2:
                    switch(columns)
                    {
                    case 2:
                        return VariableDesc::Type::Float2x2;
                    case 3:
                        return VariableDesc::Type::Float2x3;
                    case 4:
                        return VariableDesc::Type::Float2x4;
                    }
                    break;
                case 3:
                    switch(columns)
                    {
                    case 2:
                        return VariableDesc::Type::Float3x2;
                    case 3:
                        return VariableDesc::Type::Float3x3;
                    case 4:
                        return VariableDesc::Type::Float3x4;
                    }
                    break;
                case 4:
                    switch(columns)
                    {
                    case 2:
                        return VariableDesc::Type::Float4x2;
                    case 3:
                        return VariableDesc::Type::Float4x3;
                    case 4:
                        return VariableDesc::Type::Float4x4;
                    }
                    break;
                }
            }

            should_not_get_here();
            return VariableDesc::Type::Unknown;
        }

        size_t getRowCountFromType(VariableDesc::Type type)
        {
            switch(type)
            {
            case VariableDesc::Type::Unknown:
                return 0;
            case VariableDesc::Type::Bool:
            case VariableDesc::Type::Bool2:
            case VariableDesc::Type::Bool3:
            case VariableDesc::Type::Bool4:
            case VariableDesc::Type::Uint:
            case VariableDesc::Type::Uint2:
            case VariableDesc::Type::Uint3:
            case VariableDesc::Type::Uint4:
            case VariableDesc::Type::Int:
            case VariableDesc::Type::Int2:
            case VariableDesc::Type::Int3:
            case VariableDesc::Type::Int4:
            case VariableDesc::Type::Float:
            case VariableDesc::Type::Float2:
            case VariableDesc::Type::Float3:
            case VariableDesc::Type::Float4:
                return 1;
            case VariableDesc::Type::Float2x2:
            case VariableDesc::Type::Float2x3:
            case VariableDesc::Type::Float2x4:
                return 2;
            case VariableDesc::Type::Float3x2:
            case VariableDesc::Type::Float3x3:
            case VariableDesc::Type::Float3x4:
                return 3;
            case VariableDesc::Type::Float4x2:
            case VariableDesc::Type::Float4x3:
            case VariableDesc::Type::Float4x4:
                return 4;
            default:
                should_not_get_here();
                return 0;
            }
        }

        size_t calcStructSize(const VariableDescMap& variableMap)
        {
            size_t maxOffset = 0;
            VariableDesc::Type type = variableMap.begin()->second.type;

            for(const auto& a : variableMap)
            {
                if(a.second.offset > maxOffset)
                {
                    maxOffset = a.second.offset;
                    type = a.second.type;
                }
            }
            if(maxOffset > 0)
            {
                maxOffset += getRowCountFromType(type) * 16;
            }
            return maxOffset;
        }

        static void reflectType(ID3D11ShaderReflectionType* pType, VariableDescMap& variableMap, const std::string& name, size_t offset)
        {
            D3D11_SHADER_TYPE_DESC typeDesc;
            pType->GetDesc(&typeDesc);

            offset += typeDesc.Offset;

            if(typeDesc.Class == D3D_SVC_STRUCT)
            {
                // Calculate the structure size
                VariableDescMap temp;
                for(uint32_t memberID = 0; memberID < typeDesc.Members; memberID++)
                {
                    ID3D11ShaderReflectionType* pMember = pType->GetMemberTypeByIndex(memberID);
                    std::string memberName(pType->GetMemberTypeName(memberID));
                    reflectType(pMember, temp, memberName, 0);
                }
                size_t structSize = calcStructSize(temp);

                // Parse the internal variables
                for(uint32_t memberID = 0; memberID < typeDesc.Members; memberID++)
                {
                    ID3D11ShaderReflectionType* pMember = pType->GetMemberTypeByIndex(memberID);
                    std::string memberName(pType->GetMemberTypeName(memberID));

                    if(typeDesc.Elements > 0)
                    {
                        for(uint32_t i = 0; i < typeDesc.Elements; i++)
                        {
                            reflectType(pMember, variableMap, name + '[' + std::to_string(i) + "]." + memberName, offset + structSize * i);
                        }
                    }
                    else
                        reflectType(pMember, variableMap, name + "." + memberName, offset);
                }
            }
            else
            {
                VariableDesc desc;
                desc.arraySize = typeDesc.Elements;
                desc.isRowMajor = (typeDesc.Class == D3D_SVC_MATRIX_ROWS);
                desc.offset = offset;
                desc.type = getVariableType(typeDesc.Type, typeDesc.Rows, typeDesc.Columns);
                variableMap[name] = desc;
            }
        }

        static void reflectVariable(ID3D11ShaderReflectionConstantBuffer* pReflector, ID3D11ShaderReflectionVariable* pVar, VariableDescMap& variableMap)
        {
            // Get the variable name
            D3D11_SHADER_VARIABLE_DESC varDesc;
            pVar->GetDesc(&varDesc);
            std::string name(varDesc.Name);

            // Reflect the Type
            ID3D11ShaderReflectionType* pType = pVar->GetType();
            reflectType(pType, variableMap, name, varDesc.StartOffset);
        }

        void reflectBufferVariables(ID3D11ShaderReflectionConstantBuffer* pReflector, VariableDescMap& variableMap)
        {
            D3D11_SHADER_BUFFER_DESC bufferDesc;
            d3d_call(pReflector->GetDesc(&bufferDesc));

            for(uint32_t varID = 0; varID < bufferDesc.Variables; varID++)
            {
                ID3D11ShaderReflectionVariable* pVar = pReflector->GetVariableByIndex(varID);
                reflectVariable(pReflector, pVar, variableMap);
            }

            // Note(tfoley): This logic is wrong, so we disable it completely in Spire mode
#if FALCOR_USE_SPIRE_AS_COMPILER
#else
            assert(calcStructSize(variableMap) == bufferDesc.Size);
#endif
        }

#if FALCOR_USE_SPIRE_AS_COMPILER
#else
        static ShaderResourceDesc::ResourceType getResourceType(D3D_SHADER_INPUT_TYPE type)
        {
            switch(type)
            {
            case D3D_SIT_TEXTURE:
                return ShaderResourceDesc::ResourceType::Texture;
            case D3D_SIT_SAMPLER:
                return ShaderResourceDesc::ResourceType::Sampler;
            default:
                should_not_get_here();
                return ShaderResourceDesc::ResourceType::Unknown;
            }
        }

        static ShaderResourceDesc::ReturnType getReturnType(D3D_RESOURCE_RETURN_TYPE type)
        {
            switch(type)
            {
            case D3D_RETURN_TYPE_UNORM:
            case D3D_RETURN_TYPE_SNORM:
            case D3D_RETURN_TYPE_FLOAT:
                return ShaderResourceDesc::ReturnType::Float;
            case D3D_RETURN_TYPE_SINT:
                return ShaderResourceDesc::ReturnType::Int;
            case D3D_RETURN_TYPE_UINT:
                return ShaderResourceDesc::ReturnType::Uint;
            case D3D_RETURN_TYPE_DOUBLE:
                return ShaderResourceDesc::ReturnType::Double;
            default:
                should_not_get_here();
                return ShaderResourceDesc::ReturnType::Unknown;
            }
        }

        static ShaderResourceDesc::Dimensions getResourceDimensions(D3D_SRV_DIMENSION dims)
        {
            switch(dims)
            {
            case D3D_SRV_DIMENSION_BUFFER:
                return ShaderResourceDesc::Dimensions::TextureBuffer;
            case D3D_SRV_DIMENSION_TEXTURE1D:
                return ShaderResourceDesc::Dimensions::Texture1D;
            case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                return ShaderResourceDesc::Dimensions::Texture1DArray;
            case D3D_SRV_DIMENSION_TEXTURE2D:
                return ShaderResourceDesc::Dimensions::Texture2D;
            case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                return ShaderResourceDesc::Dimensions::Texture2DArray;
            case D3D_SRV_DIMENSION_TEXTURE2DMS:
                return ShaderResourceDesc::Dimensions::Texture2DMS;
            case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                return ShaderResourceDesc::Dimensions::Texture2DMSArray;
            case D3D_SRV_DIMENSION_TEXTURE3D:
                return ShaderResourceDesc::Dimensions::Texture3D;
            case D3D_SRV_DIMENSION_TEXTURECUBE:
                return ShaderResourceDesc::Dimensions::TextureCube;
            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                return ShaderResourceDesc::Dimensions::TextureCubeArray;
            default:
                should_not_get_here();
                return ShaderResourceDesc::Dimensions::Unknown;
            }
        }
#endif

        void reflectResources(ID3D11ShaderReflectionPtr pReflector, ShaderResourceDescMap& resourceMap)
        {
            D3D11_SHADER_DESC shaderDesc;
            d3d_call(pReflector->GetDesc(&shaderDesc));

            for(uint32_t i = 0; i < shaderDesc.BoundResources; i++)
            {
                D3D11_SHADER_INPUT_BIND_DESC InputDesc;
                d3d_call(pReflector->GetResourceBindingDesc(i, &InputDesc));

                // Ignore constant buffers
                if(InputDesc.Type == D3D_SIT_CBUFFER)
                {
                    continue;
                }
                ShaderResourceDesc falcorDesc;
                std::string name(InputDesc.Name);
                falcorDesc.type = getResourceType(InputDesc.Type);
                if(falcorDesc.type != ShaderResourceDesc::ResourceType::Sampler)
                {
                    falcorDesc.retType = getReturnType(InputDesc.ReturnType);
                    falcorDesc.dims = getResourceDimensions(InputDesc.Dimension);
                }
                bool isArray = name[name.length() - 1] == ']';
                falcorDesc.offset = InputDesc.BindPoint;
                falcorDesc.arraySize = isArray ? InputDesc.BindCount : 0;

                resourceMap[name] = falcorDesc;
            }
        }
    }
}
