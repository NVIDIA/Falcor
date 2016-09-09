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
#pragma once
#include "Framework.h"
#include "ProgramReflection.h"

namespace Falcor
{
    ProgramReflection::SharedPtr ProgramReflection::create(const ProgramVersion* pProgramVersion)
    {
        return SharedPtr(new ProgramReflection);
    }

    uint32_t ProgramReflection::getUniformBufferBinding(const std::string& name) const
    {
        return kInvalidLocation;
    }

    //         // Loop over all the shaders and initialize the uniform buffer bindings.
//         // This is actually where verify that the shader declarations match between shaders.
//         // Due to OpenGL support, Falcor assumes that CBs definition and binding points match between shader stages.
// 
//         // A map storing the first shader we encountered a buffer in.
//         std::map<std::string, const Shader*> bufferFirstShaderMap;
//         std::map<std::string, const Shader*> resourceFirstShaderMap;
// 
//         ShaderResourceDescMap programResources;
//         for(uint32_t i = 0 ; i < kShaderCount ; i++)
//         {
//             const Shader* pShader = pProgram->mpShaders[i].get();
// 
//             if(pShader)
//             {
//                 ShaderResourceDescMap shaderResources;
//                 reflectResources(pShader->getReflectionInterface(), shaderResources);
//                 if(validateResourceDeclarations(programResources, shaderResources, pShader, resourceFirstShaderMap, log) == false)
//                 {
//                     pProgram = nullptr;
//                     return pProgram;
//                 }
// 
//                 mergeResourceMaps(programResources, shaderResources, pShader, resourceFirstShaderMap);
// 
//                 // Initialize the buffer declaration
//                 BufferDescMap bufferDesc;
//                 reflectBuffers(pShader->getReflectionInterface(), bufferDesc);
// 
//                 // Loop over all the buffer we found
//                 for(auto& buffer : bufferDesc)
//                 {
//                     // Check if a buffer with the same name was already declared in a previous shader
//                     const std::string& bufferName = buffer.first;
//                     const auto& prevDesc = pProgram->mBuffersDesc.find(bufferName);
//                     if(prevDesc != pProgram->mBuffersDesc.end())
//                     {
//                         // Make sure the current buffer declaration matches the previous declaration
//                         const Shader* pPrevShader = bufferFirstShaderMap[bufferName];
//                         auto pPrevBuffer = pPrevShader->getReflectionInterface()->GetConstantBufferByName(bufferName.c_str());
//                         auto pCurrentBuffer = pShader->getReflectionInterface()->GetConstantBufferByName(bufferName.c_str());
//                         bool bMatch = validateBufferDeclaration(prevDesc->second, buffer.second, pPrevBuffer, pCurrentBuffer, log);
// 
//                         if(bMatch == false)
//                         {
//                             return returnError(log, std::string("Buffer '") + buffer.first + "' declaration mismatch between shader.\n" + log, pPrevShader, pShader, pProgram);
//                         }
//                     }
//                     else
//                     {
//                         // Make sure the binding point is unique
//                         for(const auto& a : pProgram->mBuffersDesc)
//                         {
//                             if(a.second.bufferSlot == buffer.second.bufferSlot)
//                             {
//                                 return returnError(log, std::string("Different uniform buffers use the same binding point ") + std::to_string(a.second.bufferSlot), bufferFirstShaderMap[a.first], pShader, pProgram);
//                             }
//                         }
//                         // New buffer
//                         pProgram->mBuffersDesc[bufferName] = buffer.second;
//                         bufferFirstShaderMap[bufferName] = pShader;
//                     }
//                 }
//             }
//         }
}