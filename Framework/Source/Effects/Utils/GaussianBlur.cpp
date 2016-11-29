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
#include "GaussianBlur.h"
#include "API/RenderContext.h"
#include "Graphics/FboHelper.h"

namespace Falcor
{
    static std::string kShaderFilename("Effects\\GaussianBlur.fs");

    GaussianBlur::~GaussianBlur() = default;

    GaussianBlur::GaussianBlur(uint32_t kernelSize) : mKernelSize(kernelSize)
    {
        Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
        mpSampler = Sampler::create(samplerDesc);
    }

    GaussianBlur::UniquePtr GaussianBlur::create(uint32_t kernelSize)
    {
        GaussianBlur* pBlur = new GaussianBlur(kernelSize);
        return GaussianBlur::UniquePtr(pBlur);
    }

    void GaussianBlur::createTmpFbo(const Texture* pSrc)
    {
        bool createFbo = mpTmpFbo == nullptr;
        ResourceFormat srcFormat = pSrc->getFormat();

        if(createFbo == false)
        {
            createFbo = (pSrc->getWidth() != mpTmpFbo->getWidth()) ||
                (pSrc->getHeight() != mpTmpFbo->getHeight()) ||
                (srcFormat != mpTmpFbo->getColorTexture(0)->getFormat()) ||
                pSrc->getArraySize() != mpTmpFbo->getColorTexture(0)->getArraySize();
        }

        if(createFbo)
        {
            Fbo::Desc fboDesc;
            fboDesc.setColorTarget(0, srcFormat);
            mpTmpFbo = FboHelper::create2D(pSrc->getWidth(), pSrc->getHeight(), fboDesc, pSrc->getArraySize());
            createProgram();
        }
    }

    void GaussianBlur::createProgram()
    {
        Program::DefineList defines;
        defines.add("_KERNEL_WIDTH", std::to_string(mKernelSize));
        if(mpTmpFbo->getColorTexture(0)->getArraySize() > 1)
        {
            defines.add("_USE_TEX2D_ARRAY");
        }

        uint32_t arraySize = mpTmpFbo->getColorTexture(0)->getArraySize();
        uint32_t layerMask = (arraySize > 1) ? ((1 << arraySize) - 1) : 0;
        mpHorizontalBlur = FullScreenPass::create(kShaderFilename, defines, true, true, layerMask);
        mpHorizontalBlur->getProgram()->addDefine("_HORIZONTAL_BLUR");
        mpVerticalBlur = FullScreenPass::create(kShaderFilename, defines, true, true, layerMask);
        mpVerticalBlur->getProgram()->addDefine("_VERTICAL_BLUR");
        mpCb = ConstantBuffer::create(mpVerticalBlur->getProgram(), "PerImageCB");
    }

    void GaussianBlur::execute(RenderContext* pRenderContext, const Texture* pSrc, Fbo::SharedPtr pDst)
    {
        createTmpFbo(pSrc);
        uint32_t arraySize = pSrc->getArraySize();
        PipelineState::Viewport vp;
        vp.originX = 0;
        vp.originY = 0;
        vp.height = (float)mpTmpFbo->getHeight();
        vp.width = (float)mpTmpFbo->getWidth();
        vp.minDepth = 0;
        vp.maxDepth = 1;

        PipelineState* pState = pRenderContext->getPipelineState().get();
        for(uint32_t i = 0; i < arraySize; i++)
        {
            pState->pushViewport(i, vp);
        }

        // Horizontal pass
        mpCb->setTexture(0, pSrc, mpSampler.get(), false);
        pState->pushFbo(mpTmpFbo);
        // DISABLED_FOR_D3D12
//        pRenderContext->setUniformBuffer(0, mpUbo);
        mpHorizontalBlur->execute(pRenderContext);

        // Vertical pass
        mpCb->setTexture(0, mpTmpFbo->getColorTexture(0).get(), mpSampler.get(), false);
        pState->setFbo(pDst);
        mpVerticalBlur->execute(pRenderContext);

        pState->popFbo();
        for(uint32_t i = 0; i < arraySize; i++)
        {
            pState->popViewport(i);
        }
    }   
}