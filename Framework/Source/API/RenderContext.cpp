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
#include "RenderContext.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "BlendState.h"
#include "FBO.h"
#include "Device.h"

namespace Falcor
{
    RenderContext::BlitData RenderContext::sBlitData;
    CommandSignatureHandle RenderContext::spDrawCommandSig = nullptr;
    CommandSignatureHandle RenderContext::spDrawIndexCommandSig = nullptr;

    RenderContext::~RenderContext()
    {
        releaseBlitData();
    }

    void RenderContext::pushGraphicsState(const GraphicsState::SharedPtr& pState)
    {
        mPipelineStateStack.push(mpGraphicsState);
        setGraphicsState(pState);
    }

    void RenderContext::popGraphicsState()
    {
        if (mPipelineStateStack.empty())
        {
            logWarning("Can't pop from the PipelineState stack. The stack is empty");
            return;
        }

        setGraphicsState(mPipelineStateStack.top());
        mPipelineStateStack.pop();
    }

    void RenderContext::pushGraphicsVars(const GraphicsVars::SharedPtr& pVars)
    {
        mpGraphicsVarsStack.push(mpGraphicsVars);
        setGraphicsVars(pVars);
    }

    void RenderContext::popGraphicsVars()
    {
        if (mpGraphicsVarsStack.empty())
        {
            logWarning("Can't pop from the graphics vars stack. The stack is empty");
            return;
        }

        setGraphicsVars(mpGraphicsVarsStack.top());
        mpGraphicsVarsStack.pop();
    }

    void RenderContext::initBlitData()
    {
        if (sBlitData.pVars == nullptr)
        {
            sBlitData.pPass = FullScreenPass::create("Framework/Shaders/Blit.vs.slang", "Framework/Shaders/Blit.ps.slang");
            sBlitData.pVars = GraphicsVars::create(sBlitData.pPass->getProgram()->getActiveVersion()->getReflector());
            sBlitData.pState = GraphicsState::create();

            sBlitData.pSrcRectBuffer = sBlitData.pVars->getConstantBuffer("SrcRectCB");
            sBlitData.offsetVarOffset = (uint32_t)sBlitData.pSrcRectBuffer->getVariableOffset("gOffset");
            sBlitData.scaleVarOffset = (uint32_t)sBlitData.pSrcRectBuffer->getVariableOffset("gScale");
            sBlitData.prevSrcRectOffset = vec2(-1.0f);
            sBlitData.prevSrcReftScale = vec2(-1.0f);

            Sampler::Desc desc;
            desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
            sBlitData.pLinearSampler = Sampler::create(desc);
            desc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
            sBlitData.pPointSampler = Sampler::create(desc);
            assert(sBlitData.pPass->getProgram()->getActiveVersion()->getReflector()->getResourceDesc("gTex")->regIndex == 0);
        }
    }

    void RenderContext::releaseBlitData()
    {
        sBlitData.pSrcRectBuffer = nullptr;
        sBlitData.pVars = nullptr;
        sBlitData.pPass = nullptr;
        sBlitData.pState = nullptr;
    }

    void RenderContext::blit(ShaderResourceView::SharedPtr pSrc, RenderTargetView::SharedPtr pDst, const uvec4& srcRect, const uvec4& dstRect, Sampler::Filter filter)
    {
        initBlitData(); // This has to be here and can't be in the constructor. FullScreenPass will allocate some buffers which depends on the ResourceAllocator which depends on the fence inside the RenderContext. Dependencies are fun!
        if (filter == Sampler::Filter::Linear)
        {
            sBlitData.pVars->setSampler(0, sBlitData.pLinearSampler);
        }
        else
        {
            sBlitData.pVars->setSampler(0, sBlitData.pPointSampler);
        }

        assert(pSrc->getViewInfo().arraySize == 1 && pSrc->getViewInfo().mipCount == 1);
        assert(pDst->getViewInfo().arraySize == 1 && pDst->getViewInfo().mipCount == 1);

        const Texture* pSrcTexture = dynamic_cast<const Texture*>(pSrc->getResource());
        const Texture* pDstTexture = dynamic_cast<const Texture*>(pDst->getResource());
        assert(pSrcTexture != nullptr && pDstTexture != nullptr);

        vec2 srcRectOffset(0.0f);
        vec2 srcRectScale(1.0f);
        GraphicsState::Viewport dstViewport(0.0f, 0.0f, (float)pDstTexture->getWidth(), (float)pDstTexture->getHeight(), 0.0f, 1.0f);

        // If src rect specified
        if (srcRect.x != (uint32_t)-1)
        {
            const vec2 srcSize(pSrcTexture->getWidth(), pSrcTexture->getHeight());
            srcRectOffset = vec2(srcRect.x, srcRect.y) / srcSize;
            srcRectScale = vec2(srcRect.z - srcRect.x, srcRect.w - srcRect.y) / srcSize;
        }

        // If dest rect specified
        if (dstRect.x != (uint32_t)-1)
        {
            dstViewport = GraphicsState::Viewport((float)dstRect.x, (float)dstRect.y, (float)(dstRect.z - dstRect.x), (float)(dstRect.w - dstRect.y), 0.0f, 1.0f);
        }

        // Update buffer/state
        if (srcRectOffset != sBlitData.prevSrcRectOffset)
        {
            sBlitData.pSrcRectBuffer->setVariable(sBlitData.offsetVarOffset, srcRectOffset);
            sBlitData.prevSrcRectOffset = srcRectOffset;
        }

        if (srcRectScale != sBlitData.prevSrcReftScale)
        {
            sBlitData.pSrcRectBuffer->setVariable(sBlitData.scaleVarOffset, srcRectScale);
            sBlitData.prevSrcReftScale = srcRectScale;
        }

        sBlitData.pState->setViewport(0, dstViewport);

        pushGraphicsState(sBlitData.pState);
        pushGraphicsVars(sBlitData.pVars);

        if (pSrcTexture->getSampleCount() > 1)
        {
            sBlitData.pPass->getProgram()->addDefine("SAMPLE_COUNT", std::to_string(pSrcTexture->getSampleCount()));
        }
        else
        {
            sBlitData.pPass->getProgram()->removeDefine("SAMPLE_COUNT");
        }

        Fbo::SharedPtr pFbo = Fbo::create();
        Texture::SharedPtr pSharedTex = std::const_pointer_cast<Texture>(pDstTexture->shared_from_this());
        pFbo->attachColorTarget(pSharedTex, 0, pDst->getViewInfo().mostDetailedMip, pDst->getViewInfo().firstArraySlice, pDst->getViewInfo().arraySize);
        sBlitData.pState->pushFbo(pFbo, false);
        sBlitData.pVars->setSrv(0, pSrc);
        sBlitData.pPass->execute(this);

        // Release the resources we bound
        sBlitData.pVars->setSrv(0, nullptr);
        sBlitData.pState->popFbo(false);
        popGraphicsState();
        popGraphicsVars();
    }
}
