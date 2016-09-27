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
#include "ParallelReduction.h"
#include "Graphics/FboHelper.h"
#include "Core/RenderContext.h"
#include "glm/vec2.hpp"

namespace Falcor
{
    const char* fsFilename = "Framework/ParallelReduction.fs";

    ParallelReduction::ParallelReduction(ParallelReduction::Type reductionType, uint32_t readbackLatency, uint32_t width, uint32_t height) : mReductionType(reductionType)
    {
        ResourceFormat texFormat;
        Program::DefineList defines;
        defines.add("_TILE_SIZE", std::to_string(kTileSize));
        switch(reductionType)
        {
        case Type::MinMax:
           texFormat = ResourceFormat::RG32Float;
           defines.add("_MIN_MAX_REDUCTION");
           break;
        default:
            should_not_get_here();
            return;
        }

        Sampler::Desc samplerDesc;
        samplerDesc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp).setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point).setLodParams(0, 0, 0);
        mpPointSampler = Sampler::create(samplerDesc);

        mpResultFbo.resize(readbackLatency + 1);
        for(auto& pFbo : mpResultFbo)
        {
            Fbo::Desc fboDesc;
            fboDesc.setColorFormat(0, texFormat);
            pFbo = FboHelper::create2D(1, 1, fboDesc);
        }
        mpFirstIterProg = FullScreenPass::create(fsFilename, defines);
        mpFirstIterProg->getProgram()->addDefine("_FIRST_ITERATION");
        mpRestIterProg = FullScreenPass::create(fsFilename, defines);
        mpUbo = UniformBuffer::create(mpFirstIterProg->getProgram()->getActiveProgramVersion().get(), "PerImageCB");

        // Calculate the number of reduction passes
        if(width > kTileSize || height > kTileSize)
        {
            while(width > 1 || height > 1)
            {
                width = (width + kTileSize - 1) / kTileSize;;
                height = (height + kTileSize - 1) / kTileSize;;

                width = max(width, 1u);
                height = max(height, 1u);

                Fbo::Desc fboDesc;
                fboDesc.setColorFormat(0, texFormat);
                mpTmpResultFbo.push_back(FboHelper::create2D(width, height, fboDesc));
            }
        }
    }

    ParallelReduction::UniquePtr ParallelReduction::create(Type reductionType, uint32_t readbackLatency, uint32_t width, uint32_t height)
    {
        return ParallelReduction::UniquePtr(new ParallelReduction(reductionType, readbackLatency, width, height));
    }

    void runProgram(RenderContext* pRenderCtx, const Texture* pInput, const FullScreenPass* pProgram, Fbo::SharedPtr pDst, UniformBuffer::SharedPtr pUbo, Sampler::SharedPtr pPointSampler)
    {
        // Bind the input texture
        pUbo->setTexture(0, pInput, pPointSampler.get());
        pRenderCtx->setUniformBuffer(0, pUbo);

        // Set draw params
        pRenderCtx->pushFbo(pDst);
        RenderContext::Viewport vp;
        vp.height = (float)pDst->getHeight();
        vp.width = (float)pDst->getWidth();
        pRenderCtx->pushViewport(0, vp);

        // Launch the program
        pProgram->execute(pRenderCtx);

        // Restore state
        pRenderCtx->popViewport(0);
        pRenderCtx->popFbo();
    }

    glm::vec4 ParallelReduction::reduce(RenderContext* pRenderCtx, const Texture* pInput)
    {
        const FullScreenPass* pProgram = mpFirstIterProg.get();

        for(size_t i = 0; i < mpTmpResultFbo.size(); i++)
        {
            runProgram(pRenderCtx, pInput, pProgram, mpTmpResultFbo[i], mpUbo, mpPointSampler);
            pProgram = mpRestIterProg.get();
            pInput = mpTmpResultFbo[i]->getColorTexture(0).get();
        }

        runProgram(pRenderCtx, pInput, pProgram, mpResultFbo[mCurFbo], mpUbo, mpPointSampler);

        // Read back the results
        mCurFbo = (mCurFbo + 1) % mpResultFbo.size();

        uint32_t bytesToRead = 0;
        glm::vec4 result(0);
        switch(mReductionType)
        {
        case Type::MinMax:
            bytesToRead = sizeof(glm::vec2);
            break;
        default:
            should_not_get_here();
        }

        mpResultFbo[mCurFbo]->getColorTexture(0)->readSubresourceData(&result, bytesToRead, 0, 0);
        return result;
    }
}