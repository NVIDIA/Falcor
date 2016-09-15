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
#include "Core/RenderContext.h"

namespace Falcor
{
    RenderContext::~RenderContext() = default;

    RenderContext::SharedPtr RenderContext::create()
    {
        return nullptr;
    }

    void RenderContext::applyDepthStencilState() const
    {
        
    }

    void RenderContext::applyRasterizerState() const
    {
    }

    void RenderContext::applyBlendState() const
    {
    }

    void RenderContext::applyProgram() const
    {
    }

    void RenderContext::applyVao() const
    {
    }

    void RenderContext::applyFbo() const
    {
    }

    void RenderContext::blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, bool useLinearFiltering, FboAttachmentType copyFlags, uint32_t srcIdx, uint32_t dstIdx)
	{
        UNSUPPORTED_IN_D3D12("BlitFbo");
	}

    void RenderContext::applyUniformBuffer(uint32_t Index) const
    {
    }

    void RenderContext::applyShaderStorageBuffer(uint32_t index) const
    {
        UNSUPPORTED_IN_D3D12("RenderContext::ApplyShaderStorageBuffer()");
    }

    void RenderContext::applyTopology() const
    {
    }

    void RenderContext::prepareForDrawApi() const
    {

    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        prepareForDraw();
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        prepareForDraw();
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
    }

    void RenderContext::applyViewport(uint32_t index) const
    {
    }

    void RenderContext::applyScissor(uint32_t index) const
    {
    }
}
#endif //#ifdef FALCOR_D3D12
