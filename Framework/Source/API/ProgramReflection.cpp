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

#include "Externals/Spire/Spire.h"

namespace Falcor
{
    ProgramReflection::SharedPtr ProgramReflection::create(const ProgramVersion* pProgramVersion, std::string& log)
    {
        SharedPtr pReflection = SharedPtr(new ProgramReflection);
        return pReflection->init(pProgramVersion, log) ? pReflection : nullptr;
    }

    ProgramReflection::SharedPtr ProgramReflection::createFromSpire(
        SpireCompilationContext*    pSpireContext,
        SpireShader*                pSpireShader,
        std::string& log)
    {
        SharedPtr pReflection = SharedPtr(new ProgramReflection);
        return pReflection->initFromSpire(pSpireContext, pSpireShader, log) ? pReflection : nullptr;
    }


    ProgramReflection::BindLocation ProgramReflection::getBufferBinding(const std::string& name) const
    {
        // Names are unique regardless of buffer type. Search in each map
        for (const auto& desc : mBuffers)
        {
            auto& it = desc.nameMap.find(name);
            if (it != desc.nameMap.end())
            {
                return it->second;
            }
        }

        static const BindLocation invalidBind(kInvalidLocation, ShaderAccess::Undefined);
        return invalidBind;
    }

    bool ProgramReflection::init(const ProgramVersion* pProgVer, std::string& log)
    {
        bool b = true;
        b = b && reflectResources(pProgVer, log);
        b = b && reflectVertexAttributes(pProgVer, log);
        b = b && reflectFragmentOutputs(pProgVer, log);
        return b;
    }

    static ProgramReflection::Variable::Type getSpireVariableType(
        char const* spireTypeName)
    {
        static const struct
        {
            char const*                         name;
            ProgramReflection::Variable::Type   type;
        } kEntries[] = {

        #define ENTRY(SPIRE_NAME, FALCOR_NAME) \
            { #SPIRE_NAME, ProgramReflection::Variable::Type::FALCOR_NAME }

            ENTRY(float,    Float),
            ENTRY(vec3,     Float3),
            ENTRY(uint,     Uint),
            ENTRY(uvec3,    Uint3),
            ENTRY(mat4, Float4x4),

        #undef ENTRY

            { nullptr },
        };

        for(auto ee = kEntries; ee->name; ++ee)
        {
            if(strcmp(ee->name, spireTypeName) == 0)
                return ee->type;
        }
        assert(!"unimplemented");
        return ProgramReflection::Variable::Type::Unknown;
    }

    ProgramReflection::BufferReflection::SharedPtr ProgramReflection::BufferReflection::create(SpireModule* componentClass)
    {
        char const* bufferName = spGetModuleName(componentClass);
        auto bufferType = BufferReflection::Type::Constant;
        auto shaderAccess = ShaderAccess::Read;
        uint32_t bindPoint = 0;
        uint32_t bindSpace = 0;

        ProgramReflection::VariableMap varMap;

        // loop over variables to fill them in...
        int paramCount = spModuleGetParameterCount(componentClass);
        for( int pp = 0; pp < paramCount; ++pp )
        {
            SpireComponentInfo spireVarInfo;
            spModuleGetParameter(componentClass, pp, &spireVarInfo);

            char const* varName = spireVarInfo.Name;
 
            switch( spireVarInfo.BindableResourceType )
            {
            case SPIRE_NON_BINDABLE:
                {
                    // An ordinary uniform
                    ProgramReflection::Variable varInfo;
                    varInfo.arraySize = 0;
                    varInfo.isRowMajor = true;
                    varInfo.location = spireVarInfo.Offset;
                    varInfo.type = getSpireVariableType(spireVarInfo.TypeName);
                    varMap[varName] = varInfo;
                }
                break;

            default:
            case SPIRE_TEXTURE:
            case SPIRE_SAMPLER:
            case SPIRE_UNIFORM_BUFFER:
            case SPIRE_STORAGE_BUFFER:
                assert(!"unimplemented");
                break;
            }

        }

        // TODO: confirm that this is how to get the right size!
        size_t bufferSize = spModuleGetParameterBufferSize(componentClass);

        ProgramReflection::BindLocation bindLocation(bindPoint, shaderAccess);
        auto bufferReflection = ProgramReflection::BufferReflection::create(
            bufferName,
            bindPoint,
            bindSpace,
            bufferType,
            bufferSize,
            varMap,
            ProgramReflection::ResourceMap(),
            shaderAccess);
        bufferReflection->setShaderMask(0xFFFFFFFF);

        return bufferReflection;
    }


    bool ProgramReflection::initFromSpire(
        SpireCompilationContext*    pSpireContext,
        SpireShader*                pSpireShader,
        std::string& log)
    {
        int componentCount = spShaderGetParameterCount(pSpireShader);
        for(int cc = 0; cc < componentCount; ++cc)
        {
            char const* componentClassName = spShaderGetParameterType(pSpireShader, cc);
            SpireModule* componentClass = spFindModule(pSpireContext, componentClassName);
            if(!componentClass)
                continue;

            char const* bufferName = spShaderGetParameterName(pSpireShader, cc);

            auto& bufferDesc = mBuffers[(uint32_t)BufferReflection::Type::Constant];

            // Create the buffer reflection
            auto bufferReflection = ProgramReflection::BufferReflection::create(componentClass);

            auto shaderAccess = ShaderAccess::Read;
            uint32_t bindPoint = spShaderGetParameterBinding(pSpireShader, cc);
            uint32_t bindSpace = 0;

            // clobber some stuff in the reflection computed from the component class
            bufferReflection->mRegIndex = bindPoint;
            bufferReflection->mRegSpace = bindSpace;

            ProgramReflection::BindLocation bindLocation(bindPoint, shaderAccess);

            bufferDesc.nameMap[bufferName] = bindLocation;
            bufferDesc.descMap[bindLocation] = bufferReflection;
        }

        return true;
    }

    const ProgramReflection::Variable* ProgramReflection::BufferReflection::getVariableData(const std::string& name, size_t& offset, bool allowNonIndexedArray) const
    {
        const std::string msg = "Error when getting variable data \"" + name + "\" from buffer \"" + mName + "\".\n";
        uint32_t arrayIndex = 0;

        // Look for the variable
        auto& var = mVariables.find(name);

#ifdef FALCOR_DX11
        if (var == mVariables.end())
        {
            // Textures might come from our struct. Try again.
            std::string texName = name + ".t";
            var = mVariables.find(texName);
        }
#endif
        if (var == mVariables.end())
        {
            // The name might contain an array index. Remove the last array index and search again
            std::string nameV2 = removeLastArrayIndex(name);
            var = mVariables.find(nameV2);

            if (var == mVariables.end())
            {
                logError(msg + "Variable not found.");
                return nullptr;
            }

            const auto& data = var->second;
            if (data.arraySize == 0)
            {
                // Not an array, so can't have an array index
                logError(msg + "Variable is not an array, so name can't include an array index.");
                return nullptr;
            }

            // We know we have an array index. Make sure it's in range
            std::string indexStr = name.substr(nameV2.length() + 1);
            char* pEndPtr;
            arrayIndex = strtol(indexStr.c_str(), &pEndPtr, 0);
            if (*pEndPtr != ']')
            {
                logError(msg + "Array index must be a literal number (no whitespace are allowed)");
                return nullptr;
            }

            if (arrayIndex >= data.arraySize)
            {
                logError(msg + "Array index (" + std::to_string(arrayIndex) + ") out-of-range. Array size == " + std::to_string(data.arraySize) + ".");
                return nullptr;
            }
        }
        else if ((allowNonIndexedArray == false) && (var->second.arraySize > 0))
        {
            // Variable name should contain an explicit array index (for N-dim arrays, N indices), but the index was missing
            logError(msg + "Expecting to find explicit array index in variable name (for N-dimensional array, N indices must be specified).");
            return nullptr;
        }

        const auto* pData = &var->second;
        offset = pData->location + pData->arrayStride * arrayIndex;
        return pData;
    }

    const ProgramReflection::Variable* ProgramReflection::BufferReflection::getVariableData(const std::string& name, bool allowNonIndexedArray) const
    {
        size_t t;
        return getVariableData(name, t, allowNonIndexedArray);
    }

    ProgramReflection::BufferReflection::SharedConstPtr ProgramReflection::getBufferDesc(uint32_t bindLocation, ShaderAccess shaderAccess, BufferReflection::Type bufferType) const
    {
        const auto& descMap = mBuffers[uint32_t(bufferType)].descMap;
        auto& desc = descMap.find({ bindLocation, shaderAccess });
        if (desc == descMap.end())
        {
            return nullptr;
        }
        return desc->second;
    }

    ProgramReflection::BufferReflection::SharedConstPtr ProgramReflection::getBufferDesc(const std::string& name, BufferReflection::Type bufferType) const
    {
        BindLocation bindLoc = getBufferBinding(name);
        if (bindLoc.regIndex != kInvalidLocation)
        {
            return getBufferDesc(bindLoc.regIndex, bindLoc.shaderAccess, bufferType);
        }
        return nullptr;
    }

    const ProgramReflection::Resource* ProgramReflection::BufferReflection::getResourceData(const std::string& name) const
    {
        auto& it = mResources.find(name);
        return it == mResources.end() ? nullptr : &(it->second);
    }

    ProgramReflection::BufferReflection::BufferReflection(const std::string& name, uint32_t registerIndex, uint32_t regSpace, Type type, size_t size, const VariableMap& varMap, const ResourceMap& resourceMap, ShaderAccess shaderAccess) :
        mName(name),
        mType(type),
        mSizeInBytes(size),
        mVariables(varMap),
        mResources(resourceMap),
        mRegIndex(registerIndex),
        mShaderAccess(shaderAccess)
    {

    }

    ProgramReflection::BufferReflection::SharedPtr ProgramReflection::BufferReflection::create(const std::string& name, uint32_t regIndex, uint32_t regSpace, Type type, size_t size, const VariableMap& varMap, const ResourceMap& resourceMap, ShaderAccess shaderAccess)
    {
        assert(regSpace == 0);
        return SharedPtr(new BufferReflection(name, regIndex, regSpace, type, size, varMap, resourceMap, shaderAccess));
    }

    const ProgramReflection::Variable* ProgramReflection::getVertexAttribute(const std::string& name) const
    {
        const auto& it = mVertAttr.find(name);
        return (it == mVertAttr.end()) ? nullptr : &(it->second);
    }

    const ProgramReflection::Variable* ProgramReflection::getFragmentOutput(const std::string& name) const
    {
        const auto& it = mFragOut.find(name);
        return (it == mFragOut.end()) ? nullptr : &(it->second);
    }

    const ProgramReflection::Resource* ProgramReflection::getResourceDesc(const std::string& name) const
    {
        const auto& it = mResources.find(name);
        const ProgramReflection::Resource* pRes = (it == mResources.end()) ? nullptr : &(it->second);

        if (pRes == nullptr)
        {
            // Check if this is the internal struct
#ifdef FALCOR_D3D
            const auto& it = mResources.find(name + ".t");
            pRes = (it == mResources.end()) ? nullptr : &(it->second);
#endif
            if(pRes == nullptr)
            {
                logError("Can't find resource '" + name + "' in program");
            }
        }
        return pRes;
    }
}