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
#include "Utils/StringUtils.h"

namespace Falcor
{
    ProgramReflection::SharedPtr ProgramReflection::create(const ProgramVersion* pProgramVersion, std::string& log)
    {
        SharedPtr pReflection = SharedPtr(new ProgramReflection);
        return pReflection->init(pProgramVersion, log) ? pReflection : nullptr;
    }

    uint32_t ProgramReflection::getUniformBufferBinding(const std::string& name) const
    {
        return kInvalidLocation;
    }

    bool ProgramReflection::reflectFragmentOutputs(const ProgramVersion* pProgVer, std::string& log)
    {
        return true;
    }

    bool ProgramReflection::reflectVertexAttributes(const ProgramVersion* pProgVer, std::string& log)
    {
        return true;
    }
  
    bool ProgramReflection::reflectResources(const ProgramVersion* pProgVer, std::string& log)
    {
        return true;
    }

    bool ProgramReflection::init(const ProgramVersion* pProgVer, std::string& log)
    {
        bool b = true;
        b = b && reflectBuffers(pProgVer, log);
        b = b && reflectVertexAttributes(pProgVer, log);
        b = b && reflectFragmentOutputs(pProgVer, log);
        b = b && reflectResources(pProgVer, log);
        return b;
    }

    const ProgramReflection::Variable* ProgramReflection::BufferReflection::getVariableData(const std::string& name, size_t& offset, bool allowNonIndexedArray) const
    {
        const std::string msg = "Error when getting uniform data\"" + name + "\" from uniform buffer \"" + mName + "\".\n";
        uint32_t arrayIndex = 0;

        // Look for the uniform
        auto& var = mVariables.find(name);

#ifdef FALCOR_DX11
        if(var == mVariables.end())
        {
            // Textures might come from our struct. Try again.
            std::string texName = name + ".t";
            var = mVariables.find(texName);
        }
#endif
        if(var == mVariables.end())
        {
            // The name might contain an array index. Remove the last array index and search again
            std::string nameV2 = removeLastArrayIndex(name);
            var = mVariables.find(nameV2);

            if(var == mVariables.end())
            {
                Logger::log(Logger::Level::Error, msg + "Uniform not found.");
                return nullptr;
            }

            const auto& data = var->second;
            if(data.arraySize == 0)
            {
                // Not an array, so can't have an array index
                Logger::log(Logger::Level::Error, msg + "Uniform is not an array, so name can't include an array index.");
                return nullptr;
            }

            // We know we have an array index. Make sure it's in range
            std::string indexStr = name.substr(nameV2.length() + 1);
            char* pEndPtr;
            arrayIndex = strtol(indexStr.c_str(), &pEndPtr, 0);
            if(*pEndPtr != ']')
            {
                Logger::log(Logger::Level::Error, msg + "Array index must be a literal number (no whitespace are allowed)");
                return nullptr;
            }

            if(arrayIndex >= data.arraySize)
            {
                Logger::log(Logger::Level::Error, msg + "Array index (" + std::to_string(arrayIndex) + ") out-of-range. Array size == " + std::to_string(data.arraySize) + ".");
                return nullptr;
            }
        }
        else if((allowNonIndexedArray == false) && (var->second.arraySize > 0))
        {
            // Variable name should contain an explicit array index (for N-dim arrays, N indices), but the index was missing
            Logger::log(Logger::Level::Error, msg + "Expecting to find explicit array index in uniform name (for N-dimensional array, N indices must be specified).");
            return nullptr;
        }

        const auto* pData = &var->second;
        offset = pData->offset + pData->arrayStride * arrayIndex;
        return pData;
    }

    const ProgramReflection::Variable* ProgramReflection::BufferReflection::getVariableData(const std::string& name, bool allowNonIndexedArray) const
    {
        size_t t;
        return getVariableData(name, t, allowNonIndexedArray);
    }

    ProgramReflection::BufferReflection::SharedConstPtr ProgramReflection::getUniformBufferDesc(uint32_t bindLocation) const
    {
        auto& desc = mUniformBuffers.descMap.find(bindLocation);
        if(desc == mUniformBuffers.descMap.end())
        {
            return nullptr;
        }
        return desc->second;
    }

    ProgramReflection::BufferReflection::SharedConstPtr ProgramReflection::getUniformBufferDesc(const std::string& name) const
    {
        uint32_t bindLoc = getUniformBufferBinding(name);
        if(bindLoc != kInvalidLocation)
        {
            return getUniformBufferDesc(bindLoc);
        }
        return nullptr;
    }

    const ProgramReflection::Resource* ProgramReflection::BufferReflection::getResourceData(const std::string& name) const
    {
        auto& it = mResources.find(name);
        return it == mResources.end() ? nullptr : &(it->second);
    }

    ProgramReflection::BufferReflection::BufferReflection(const std::string& name, Type type, size_t size, size_t varCount, const VariableMap& varMap, const ResourceMap& resourceMap) : 
        mName(name),
        mType(type),
        mSizeInBytes(size),
        mVariableCount(varCount),
        mVariables(varMap),
        mResources(resourceMap)
    {

    }

    ProgramReflection::BufferReflection::SharedPtr ProgramReflection::BufferReflection::create(const std::string& name, Type type, size_t size, size_t varCount, const VariableMap& varMap, const ResourceMap& resourceMap)
    {
        return SharedPtr(new BufferReflection(name, type, size, varCount, varMap, resourceMap));
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