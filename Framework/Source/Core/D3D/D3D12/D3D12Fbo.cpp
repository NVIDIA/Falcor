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
#ifdef FALCOR_D3D12
#include "Framework.h"
#include "Core/FBO.h"

namespace Falcor
{
    Fbo::Fbo(bool initApiHandle)
    {
        mApiHandle = -1;
    }

    Fbo::~Fbo()
    {
    }

    uint32_t Fbo::getApiHandle() const
    {
        UNSUPPORTED_IN_D3D12("Fbo::getApiHandle()");
        return mApiHandle;
    }

    uint32_t Fbo::getMaxColorTargetCount()
    {
        return D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    }

    void Fbo::applyColorAttachment(uint32_t rtIndex)
    {
    }

    void Fbo::applyDepthAttachment()
    {
    }

    bool Fbo::checkStatus() const
    {
        return true;
    }
    
    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::vec4& color) const
    {
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::uvec4& color) const
    {
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::ivec4& color) const
    {
    }

    void Fbo::captureToFile(uint32_t rtIndex, const std::string& filename, Bitmap::FileFormat fileFormat)
    {
    }

    void Fbo::clearDepthStencil(float depth, uint8_t stencil, bool clearDepth, bool clearStencil) const
    {
    }
}
#endif //#ifdef FALCOR_D3D12
