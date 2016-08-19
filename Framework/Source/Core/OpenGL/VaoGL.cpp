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
#include "Core/VAO.h"
#include "Core/Buffer.h"
#include "Core/VertexLayout.h"
#include <map>

namespace Falcor
{
    bool checkVaoParams(const Vao::VertexBufferDescVector& vbDesc, Buffer* pIB);

    static bool shouldBindAttribAsInteger(ResourceFormat format)
    {
        auto Type = getFormatType(format);
        return (Type == FormatType::Sint) || (Type == FormatType::Uint);
    }

    bool Vao::initialize()
    {
        gl_call(glCreateVertexArrays(1, &mApiHandle));

        if(mpIB)
        {
            gl_call(glVertexArrayElementBuffer(mApiHandle, mpIB->getApiHandle()));
        }

        for(uint32_t vbID = 0; vbID < (uint32_t)mpVBs.size(); vbID++)
        {   
            auto pVB = mpVBs[vbID].pBuffer;
            uint32_t stride = mpVBs[vbID].stride;
            const VertexLayout* pLayout = mpVBs[vbID].pLayout.get();

            if(pVB == nullptr)
            {
                Logger::log(Logger::Level::Error, "Error when creating VAO. One of the vertex-buffers is null. Ignoring elements related to that buffer.");
                continue;
            }

            // Bind the VB
            BufferHandle vbHandle = pVB->getApiHandle();
            gl_call(glVertexArrayVertexBuffer(mApiHandle, vbID, vbHandle, 0, stride));

            for(uint32_t elem = 0 ; elem < pLayout->getElementCount() ; elem++)
            {
                // Bind the buffer
                uint32_t shaderLocation = pLayout->getElementShaderLocation(elem);
                if (shaderLocation != VertexLayout::kInvalidShaderLocation) 
                {
                    gl_call(glEnableVertexArrayAttrib(mApiHandle, shaderLocation));
                    gl_call(glVertexArrayAttribBinding(mApiHandle, shaderLocation, vbID));

                    ResourceFormat elemFormat = pLayout->getElementFormat(elem);
                    uint32_t channelCount = getFormatChannelCount(elemFormat);
                    GLenum glFormat = getGlFormatType(elemFormat);
                    uint32_t elemOffset = pLayout->getElementOffset(elem);

                    for(uint32_t index = 0; index < pLayout->getElementArraySize(elem); index++)
                    {
                        if(shouldBindAttribAsInteger(elemFormat))
                        {
                            gl_call(glVertexArrayAttribIFormat(mApiHandle, shaderLocation, channelCount, glFormat, elemOffset));
                        }
                        else
                        {
                            gl_call(glVertexArrayAttribFormat(mApiHandle, shaderLocation, channelCount, glFormat, GL_TRUE, elemOffset));
                        }
                        shaderLocation++;
                        elemOffset += getFormatBytesPerBlock(elemFormat);
                    }
                }
            }
        }
        return true;
    }

    Vao::~Vao()
    {
        glDeleteVertexArrays(1, &mApiHandle);
    }

    VaoHandle Vao::getApiHandle() const
    {
        return mApiHandle;
    }
}
#endif //#ifdef FALCOR_GL
