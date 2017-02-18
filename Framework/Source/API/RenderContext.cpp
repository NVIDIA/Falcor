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

namespace Falcor
{
    RenderContext::BlitData RenderContext::sBlitData;

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
            sBlitData.pPass = FullScreenPass::create("Framework/Shaders/Blit.hlsl");
            sBlitData.pVars = GraphicsVars::create(sBlitData.pPass->getProgram()->getActiveVersion()->getReflector());
            sBlitData.pState = GraphicsState::create();
            Sampler::Desc desc;
            desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
            sBlitData.pVars->setSampler("gSampler", Sampler::create(desc));
            assert(sBlitData.pPass->getProgram()->getActiveVersion()->getReflector()->getResourceDesc("gTex")->regIndex == 0);
        }
    }

    void RenderContext::releaseBlitData()
    {
        sBlitData.pVars = nullptr;
        sBlitData.pPass = nullptr;
        sBlitData.pState = nullptr;
    }

    void RenderContext::blit(ShaderResourceView::SharedPtr pSrc, RenderTargetView::SharedPtr pDst)
    {
        initBlitData(); // This has to be here and can't be in the constructor. FullScreenPass will allocate some buffers which depends on the ResourceAllocator which depends on the fence inside the RenderContext. Dependencies are fun!

        assert(pSrc->getViewInfo().arraySize == 1 && pSrc->getViewInfo().mipCount == 1);
        assert(pDst->getViewInfo().arraySize == 1 && pDst->getViewInfo().mipCount == 1);

        pushGraphicsState(sBlitData.pState);
        pushGraphicsVars(sBlitData.pVars);

        const Texture* pSrcTex = dynamic_cast<const Texture*>(pSrc->getResource());
        assert(pSrcTex);
        if (pSrcTex->getSampleCount() > 1)
        {
            sBlitData.pPass->getProgram()->addDefine("SAMPLE_COUNT", std::to_string(pSrcTex->getSampleCount()));
        }
        else
        {
            sBlitData.pPass->getProgram()->removeDefine("SAMPLE_COUNT");
        }

        Fbo::SharedPtr pFbo = Fbo::create();
        const Texture* pTexture = dynamic_cast<const Texture*>(pDst->getResource());
        assert(pTexture);
        Texture::SharedPtr pSharedTex = std::const_pointer_cast<Texture>(pTexture->shared_from_this());
        pFbo->attachColorTarget(pSharedTex, 0, pDst->getViewInfo().mostDetailedMip, pDst->getViewInfo().firstArraySlice, pDst->getViewInfo().arraySize);
        sBlitData.pState->setFbo(pFbo);
        sBlitData.pVars->setSrv(0, pSrc);
        sBlitData.pPass->execute(this);

        // Release the resources we bound
        sBlitData.pVars->setSrv(0, nullptr);

        popGraphicsState();
        popGraphicsVars();
    }
}
