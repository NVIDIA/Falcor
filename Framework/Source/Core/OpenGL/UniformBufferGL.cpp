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
#ifdef FALCOR_GL
#include "Core/UniformBuffer.h"
#include "Core/ProgramVersion.h"
#include "ShaderReflectionGL.h"
#include "Core/Buffer.h"
#include "Core/Texture.h"

namespace Falcor
{
    using namespace ShaderReflection;
    UniformBuffer::~UniformBuffer() = default;

    bool UniformBuffer::apiInit(const ProgramVersion* pProgram, const std::string& bufferName, bool isUniformBuffer)
    {
        if(pProgram->getUniformBufferBinding(bufferName) == ProgramVersion::kInvalidLocation)
        {
            return false;
        }

        int32_t programID = pProgram->getApiHandle();
        GLenum bufferType = isUniformBuffer ? GL_UNIFORM_BLOCK : GL_SHADER_STORAGE_BLOCK;

        // Get the buffer size
        uint32_t blockIndex = gl_call(glGetProgramResourceIndex(programID, bufferType, bufferName.c_str()));
        assert(blockIndex != GL_INVALID_INDEX); // This has to be true
        GLenum bufferSizeEnum = GL_BUFFER_DATA_SIZE;
        int32_t size;
        gl_call(glGetProgramResourceiv(programID, bufferType, blockIndex, 1, &bufferSizeEnum, 1, nullptr, &size));
        mSize = (size_t)size;

        // Initialize the uniform map
        return reflectBufferVariables(programID, bufferName, bufferType, mSize, mVariables, mResources);
    }

    void UniformBuffer::setTextureInternal(size_t offset, const Texture* pTexture, const Sampler* pSampler) const
    {
        const uint8_t* pVar = mData.data() + offset;
        if(pTexture)
        {
            uint64_t pGpu = pTexture->makeResident(pSampler);
            *(uint64_t*)pVar = pGpu;
        }
        else
        {
            *(uint64_t*)pVar = 0;
        }
    }
}
#endif //#ifdef FALCOR_GL
