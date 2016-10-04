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
#include "ProgramVars.h"
#include "API/Buffer.h"
#include "API/RenderContext.h"

namespace Falcor
{
    template<typename BufferType>
    static void initializeBuffersMap(std::unordered_map<uint32_t, typename BufferType::SharedPtr>& bufferMap, bool createBuffers, const ProgramReflection::BufferMap& reflectionMap)
    {
        for(auto& buf : reflectionMap)
        {
            uint32_t index = buf.first;
            // Only create the buffer if needed
            bufferMap[index] = createBuffers ? BufferType::create(buf.second) : nullptr;
        }
    }

    ProgramVars::ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig)
    {
        mpReflector = pReflector;

        // Initialize the UBO and SSBO maps. We always do it, to mark which slots are used in the shader.
        initializeBuffersMap<UniformBuffer>(mUniformBuffers, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Constant));
        initializeBuffersMap<ShaderStorageBuffer>(mSSBO, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::UnorderedAccess));
        mpRootSignature = pRootSig ? pRootSig : RootSignature::createFromReflection(pReflector.get());
    }

    ProgramVars::SharedPtr ProgramVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig)
    {
        return SharedPtr(new ProgramVars(pReflector, createBuffers, pRootSig));
    }

    template<typename BufferClass>
    typename BufferClass::SharedPtr getBufferCommon(uint32_t index, const std::unordered_map<uint32_t, typename BufferClass::SharedPtr>& bufferMap)
    {
        auto& it = bufferMap.find(index);
        if(it == bufferMap.end())
        {
            return nullptr;
        }
        return it->second;
    }

    template<typename BufferClass, ProgramReflection::BufferReflection::Type bufferType>
    typename BufferClass::SharedPtr getBufferCommon(const std::string& name, const ProgramReflection* pReflector, const std::unordered_map<uint32_t, typename BufferClass::SharedPtr>& bufferMap)
    {
        uint32_t bindLocation = pReflector->getBufferBinding(name);

        if(bindLocation == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Error, "Can't find buffer named \"" + name + "\"");
            return nullptr;
        }

        auto& pDesc = pReflector->getBufferDesc(name, bufferType);

        if(pDesc->getType() != bufferType)
        {
            Logger::log(Logger::Level::Error, "Buffer \"" + name + "\" is a " + to_string(pDesc->getType()) + " buffer, while requesting for " + to_string(bufferType) + " buffer");
            return nullptr;
        }


        return getBufferCommon<BufferClass>(bindLocation, bufferMap);
    }

    UniformBuffer::SharedPtr ProgramVars::getUniformBuffer(const std::string& name) const
    {
        return getBufferCommon<UniformBuffer, ProgramReflection::BufferReflection::Type::Constant>(name, mpReflector.get(), mUniformBuffers);
    }

    UniformBuffer::SharedPtr ProgramVars::getUniformBuffer(uint32_t index) const
    {
        return getBufferCommon<UniformBuffer>(index, mUniformBuffers);
    }

    ShaderStorageBuffer::SharedPtr ProgramVars::getShaderStorageBuffer(const std::string& name) const
    {
        return getBufferCommon<ShaderStorageBuffer, ProgramReflection::BufferReflection::Type::UnorderedAccess>(name, mpReflector.get(), mSSBO);
    }

    ShaderStorageBuffer::SharedPtr ProgramVars::getShaderStorageBuffer(uint32_t index) const
    {
        return getBufferCommon<ShaderStorageBuffer>(index, mSSBO);
    }

    ErrorCode ProgramVars::bindUniformBuffer(uint32_t index, const UniformBuffer::SharedPtr& pUbo)
    {
        // Check that the index is valid
        if(mUniformBuffers.find(index) == mUniformBuffers.end())
        {
            Logger::log(Logger::Level::Warning, "No uniform buffer was found at index " + std::to_string(index) + ". Ignoring bindUniformBuffer() call.");
            return ErrorCode::NotFound;
        }

        // Just need to make sure the buffer is large enough
        const auto& desc = mpReflector->getBufferDesc(index, ProgramReflection::BufferReflection::Type::Constant);
        if(desc->getRequiredSize() > pUbo->getBuffer()->getSize())
        {
            Logger::log(Logger::Level::Error, "Can't bind uniform-buffer. Size mismatch.");
            return ErrorCode::SizeMismatch;
        }

        mUniformBuffers[index] = pUbo;
        return ErrorCode::None;
    }

    ErrorCode ProgramVars::bindUniformBuffer(const std::string& name, const UniformBuffer::SharedPtr& pUbo)
    {
        // Find the buffer
        uint32_t loc = mpReflector->getBufferBinding(name);
        if(loc == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Warning, "Uniform buffer \"" + name + "\" was not found. Ignoring bindUniformBuffer() call.");
            return ErrorCode::NotFound;
        }

        return bindUniformBuffer(loc, pUbo);
    }

    ErrorCode ProgramVars::setTexture(uint32_t index, const Texture::SharedConstPtr& pTexture)
    {
        return ErrorCode::None;
    }
}