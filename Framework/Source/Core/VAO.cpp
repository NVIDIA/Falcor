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
#include "VAO.h"

namespace Falcor
{
    bool checkVaoParams(const Vao::VertexBufferDescVector& vbDesc, Buffer* pIB)
    {
        // Must have at least 1 VB
        if(vbDesc.size() == 0)
        {
            Logger::log(Logger::Level::Error, "Error when creating VAO. Must have at least 1 vertex buffer");
            return false;
        }

        return true;
    }

    Vao::Vao(const VertexBufferDescVector& vbDesc, const Buffer::SharedPtr& pIB) : mpIB(pIB)
    {
        mpVBs = vbDesc;
    }

    Vao::SharedPtr Vao::create(const VertexBufferDescVector& vbDesc, const Buffer::SharedPtr& pIB)
    {
        if(checkVaoParams(vbDesc, pIB.get()) == false)
        {
            return nullptr;
        }

        SharedPtr pVao = SharedPtr(new Vao(vbDesc, pIB));
        if(pVao->initialize() == false)
        {
            pVao = nullptr;
        }

        return pVao;
    }

    Vao::ElementDesc Vao::getElementIndexByLocation(uint32_t elementLocaion) const
    {
        ElementDesc desc;

        int32_t bufferIndex = -1;
        for(uint32_t bufId = 0; bufId < getVertexBuffersCount(); ++bufId)
        {
            const auto& buffer = mpVBs[bufId];
            for(uint32_t i = 0; i < buffer.pLayout->getElementCount(); ++i)
            {
                if(buffer.pLayout->getElementShaderLocation(i) == elementLocaion)
                {
                    desc.vbIndex = bufId;
                    desc.elementIndex = i;
                    return desc;
                }
            }
        }
        return desc;
    }

}