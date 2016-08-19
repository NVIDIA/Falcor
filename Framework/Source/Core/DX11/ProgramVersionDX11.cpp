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
#ifdef FALCOR_DX11
#include "Core/ProgramVersion.h"
#include "ShaderReflectionDX11.h"
#include <algorithm>

namespace Falcor
{
    using namespace ShaderReflection;

    void ProgramVersion::deleteApiHandle()
    {

    }

    bool validateBufferDeclaration(const BufferDesc& prevDesc, 
        const BufferDesc& currentDesc, 
        ID3D11ShaderReflectionConstantBuffer* pPrevBuffer, 
        ID3D11ShaderReflectionConstantBuffer* pCurrentBuffer, 
        std::string& log)
    {
        bool bMatch = true;
#define error_msg(msg_) std::string(msg_) + " mismatch.\n";

#define test_field(field_, msg_)                        \
            if(prevDesc.field_ != currentDesc.field_)   \
            {                                           \
                log += error_msg(msg_);                 \
                bMatch = false;                         \
            }

        test_field(bufferSlot, "buffer slot");
        test_field(variableCount, "variable count");
        test_field(sizeInBytes, "size");
        test_field(type, "Type");
#undef test_field

        VariableDescMap prevMap, currentMap;
        reflectBufferVariables(pPrevBuffer, prevMap);
        reflectBufferVariables(pCurrentBuffer, currentMap);
        if(prevMap.size() != currentMap.size())
        {
            log += error_msg("variable count");
            bMatch = false;
        }

        auto prevVar = prevMap.begin();
        auto currentVar = currentMap.begin();
        while(bMatch && (prevVar != prevMap.end()))
        {
            if(prevVar->first != currentVar->first)
            {
                log += error_msg("variable name") + ". First seen as " + prevVar->first + ", now called " + currentVar->first;
                bMatch = false;
            }
            else
            {
#define test_field(field_, msg_)                                      \
            if(prevVar->second.field_ != currentVar->second.field_)   \
            {                                                         \
                log += error_msg(prevVar->first + " " + msg_)         \
                bMatch = false;                                       \
            }

                test_field(offset, "offset");
                test_field(arraySize, "variable count");
                test_field(isRowMajor, "row major");
                test_field(type, "Type");
#undef test_field
            }

            prevVar++;
            currentVar++;
        }

        return bMatch;
    }

    static bool validateResourceDeclarations(const ShaderResourceDescMap& programMap, const ShaderResourceDescMap& shaderMap, const Shader* pShader, const std::map<std::string, const Shader*> resourceFirstShaderMap, std::string& log)
    {
        for(const auto& shaderRes : shaderMap)
        {
            // Loop over the program map
            for(const auto programRes : programMap)
            {
                if(shaderRes.first == programRes.first)
                {
                    bool bMatch = true;
                    // Name matches. Everything else should match to.
#define test_field(field_, msg_)                                      \
            if(shaderRes.second.field_ != programRes.second.field_)   \
            {                                                         \
                log += error_msg(shaderRes.first + " " + msg_)        \
                bMatch = false;                                       \
            }

                    test_field(dims, "resource dimensions");
                    test_field(retType, "return Type");
                    test_field(type, "resource Type");
                    test_field(offset, "bind point");
                    test_field(arraySize, "array size");
#undef test_field
                    if(bMatch == false)
                    {
                        const Shader* pFirstShader = resourceFirstShaderMap.at(shaderRes.first);
                        log = "Resource '" + shaderRes.first + "' declaration mismatch between " + to_string((pFirstShader->getType())) + " shader and " + to_string(pShader->getType()) + " shader.\n" + log;
                        return false;
                    }
                    break;
                }
                else if(shaderRes.second.offset == programRes.second.offset)
                {
                    // Different names, same bind point. Error
                    std::string firstName = programRes.first;
                    std::string secondName = shaderRes.first;
                    const Shader* pFirstShader = resourceFirstShaderMap.at(firstName);
                    log = "Resource bind-point mismatch. Bind point " + std::to_string(shaderRes.second.offset) + " first use in " + to_string(pFirstShader->getType()) + " shader as ' " + firstName + "'.";
                    log += "Second use in " + to_string(pShader->getType()) + " shader as '" + secondName + "'.\nNames must match.";
                    return false;
                }
            }
        }
        return true;
    }

    static ProgramVersion::SharedPtr returnError(std::string& log, const std::string& msg, const Shader* pPrevShader, const Shader* pCurShader, ProgramVersion::SharedPtr& pProgram)
    {
        // Declarations mismatch. Return error.
        const std::string prevShaderType = to_string(pPrevShader->getType());
        const std::string curShaderType = to_string(pCurShader->getType());
        log = msg + "\nMismatch between " + prevShaderType + " shader and " + curShaderType + " shader.";
        pProgram = nullptr;
        return pProgram;
    }

    static void mergeResourceMaps(ShaderResourceDescMap& programMap, ShaderResourceDescMap& shaderMap, const Shader* pShader, std::map<std::string, const Shader*>& resourceFirstShaderMap)
    {
        ShaderResourceDescMap diff;
        std::set_difference(shaderMap.begin(), shaderMap.end(), programMap.begin(), programMap.end(), std::inserter(diff, diff.begin()), shaderMap.value_comp());
        for(const auto& d : diff)
        {
            resourceFirstShaderMap[d.first] = pShader;
        }
        programMap.insert(shaderMap.begin(), shaderMap.end());
    }

    ProgramVersion::SharedConstPtr ProgramVersion::create(const Shader::SharedPtr& pVS, 
        const Shader::SharedPtr& pFS, 
        const Shader::SharedPtr& pGS, 
        const Shader::SharedPtr& pHS, 
        const Shader::SharedPtr& pDS, 
        std::string& log, 
        const std::string& name)
    {
        // We must have at least a VS.
        if(pVS == nullptr)
        {
            log = "Program " + name + " doesn't contain a vertex-shader. That is illegal.";
            return nullptr;
        }
        SharedPtr pProgram = SharedPtr(new ProgramVersion(std::move(pVS), std::move(pFS), std::move(pGS), std::move(pHS), std::move(pDS), name));
        log = std::string();


        // Loop over all the shaders and initialize the uniform buffer bindings.
        // This is actually where verify that the shader declarations match between shaders.
        // Due to OpenGL support, Falcor assumes that CBs definition and binding points match between shader stages.

        // A map storing the first shader we encountered a buffer in.
        std::map<std::string, const Shader*> bufferFirstShaderMap;
        std::map<std::string, const Shader*> resourceFirstShaderMap;

        ShaderResourceDescMap programResources;
        for(uint32_t i = 0 ; i < kShaderCount ; i++)
        {
            const Shader* pShader = pProgram->mpShaders[i].get();

            if(pShader)
            {
                ShaderResourceDescMap shaderResources;
                reflectResources(pShader->getReflectionInterface(), shaderResources);
                if(validateResourceDeclarations(programResources, shaderResources, pShader, resourceFirstShaderMap, log) == false)
                {
                    pProgram = nullptr;
                    return pProgram;
                }

                mergeResourceMaps(programResources, shaderResources, pShader, resourceFirstShaderMap);

                // Initialize the buffer declaration
                BufferDescMap bufferDesc;
                reflectBuffers(pShader->getReflectionInterface(), bufferDesc);

                // Loop over all the buffer we found
                for(auto& buffer : bufferDesc)
                {
                    // Check if a buffer with the same name was already declared in a previous shader
                    const std::string& bufferName = buffer.first;
                    const auto& prevDesc = pProgram->mBuffersDesc.find(bufferName);
                    if(prevDesc != pProgram->mBuffersDesc.end())
                    {
                        // Make sure the current buffer declaration matches the previous declaration
                        const Shader* pPrevShader = bufferFirstShaderMap[bufferName];
                        auto pPrevBuffer = pPrevShader->getReflectionInterface()->GetConstantBufferByName(bufferName.c_str());
                        auto pCurrentBuffer = pShader->getReflectionInterface()->GetConstantBufferByName(bufferName.c_str());
                        bool bMatch = validateBufferDeclaration(prevDesc->second, buffer.second, pPrevBuffer, pCurrentBuffer, log);

                        if(bMatch == false)
                        {
                            return returnError(log, std::string("Buffer '") + buffer.first + "' declaration mismatch between shader.\n" + log, pPrevShader, pShader, pProgram);
                        }
                    }
                    else
                    {
                        // Make sure the binding point is unique
                        for(const auto& a : pProgram->mBuffersDesc)
                        {
                            if(a.second.bufferSlot == buffer.second.bufferSlot)
                            {
                                return returnError(log, std::string("Different uniform buffers use the same binding point ") + std::to_string(a.second.bufferSlot), bufferFirstShaderMap[a.first], pShader, pProgram);
                            }
                        }
                        // New buffer
                        pProgram->mBuffersDesc[bufferName] = buffer.second;
                        bufferFirstShaderMap[bufferName] = pShader;
                    }
                }
            }
        }

        return pProgram;
    }

    int32_t ProgramVersion::getAttributeLocation(const std::string& attribute) const
    {
        UNSUPPORTED_IN_DX11("CProgramVersion::GetAttributeLocation");
        return 0;
    }
}

#endif //#ifdef FALCOR_DX11
