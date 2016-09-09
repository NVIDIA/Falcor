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
#include "Core/ProgramVersion.h"
#include "Core/Buffer.h"
#include "Core/RenderContext.h"

namespace Falcor
{
    ProgramVars::ProgramVars(const ProgramVersion* pProgramVer, bool createBuffers)
    {
        mpReflection = pProgramVer->getReflector();

        // Initialize the UBO map. We always do it, to mark which slots are used in the shader.
        const auto& uboMap = mpReflection->getUniformBufferMap();
        for(auto& ubo : uboMap)
        {
            uint32_t index = ubo.first;
            // Only create the buffer if needed
            mUniformBuffers[index] = createBuffers ? UniformBuffer::create(ubo.second) : nullptr;
        }
    }

    ProgramVars::SharedPtr ProgramVars::create(const ProgramVersion* pProgramVer, bool createBuffers)
    {
        return SharedPtr(new ProgramVars(pProgramVer, createBuffers));
    }

    UniformBuffer::SharedPtr ProgramVars::getUniformBuffer(const std::string& name) const
    {
        uint32_t bindLocation = mpReflection->getUniformBufferBinding(name);
        if(bindLocation == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Error, "Can't find uniform buffer named \"" + name + "\"");
            return nullptr;
        }
        return getUniformBuffer(bindLocation);
    }

    UniformBuffer::SharedPtr ProgramVars::getUniformBuffer(uint32_t index) const
    {
        auto& it = mUniformBuffers.find(index);
        if(it == mUniformBuffers.end())
        {
            return nullptr;
        }
        return it->second;
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
        const auto& desc = mpReflection->getUniformBufferDesc(index);
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
        uint32_t loc = mpReflection->getUniformBufferBinding(name);
        if(loc == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Warning, "Uniform buffer \"" + name + "\" was not found. Ignoring bindUniformBuffer() call.");
            return ErrorCode::NotFound;
        }

        return bindUniformBuffer(loc, pUbo);
    }

    void ProgramVars::setIntoContext(RenderContext* pContext) const
    {
        for(auto& buf : mUniformBuffers)
        {
            pContext->setUniformBuffer(buf.first, buf.second);
        }
    }
}