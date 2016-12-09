/***************************************************************************
# Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
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
#include "SkyBox.h"
#include "glm/gtx/transform.hpp"
#include "Graphics/TextureHelper.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Model/ModelRenderer.h"

namespace Falcor
{
    SkyBox::UniquePtr SkyBox::create(Falcor::Texture::SharedPtr& pSkyTexture, Falcor::Sampler::SharedPtr pSampler, bool renderStereo)
    {
        UniquePtr pSkyBox = UniquePtr(new SkyBox());
        if(pSkyBox->createResources(pSkyTexture, pSampler, renderStereo) == false)
        {
            return nullptr;
        }
        return pSkyBox;
    }

    bool SkyBox::createResources(Falcor::Texture::SharedPtr& pSkyTexture, Falcor::Sampler::SharedPtr pSampler, bool renderStereo)
    {
        mpTexture = pSkyTexture;
        if(mpTexture == nullptr)
        {
            logError("Trying to create a skybox with null texture");
            return false;
        }

        mpCubeModel = Model::createFromFile("Effects/cube.obj", 0);
        if(mpCubeModel == nullptr)
        {
            logError("Failed to load cube model for SkyBox");
            return false;
        }

        mpSampler = pSampler;

        // Create the rasterizer state
        RasterizerState::Desc rastDesc;
        rastDesc.setCullMode(RasterizerState::CullMode::Front).setDepthClamp(true);
        mpNoCullRsState = RasterizerState::create(rastDesc);

        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthWriteMask(false).setDepthFunc(DepthStencilState::Func::LessEqual).setDepthTest(true);
        mpDepthStencilState = DepthStencilState::create(dsDesc);

        // Create the program
        Program::DefineList defines;
        if(renderStereo)
        {
            defines.add("_SINGLE_PASS_STEREO");
        }
        mpProgram = GraphicsProgram::createFromFile("Effects\\SkyBox.vs", "Effects\\Skybox.fs", defines);
        mpCB = ConstantBuffer::create(mpProgram, "PerFrameCB");
        mScaleOffset = mpCB->getVariableOffset("gScale");
        mTexOffset = mpCB->getVariableOffset("gSkyTex");
        mMatOffset = mpCB->getVariableOffset("gWorld");

        // Create blend state
        BlendState::Desc blendDesc;
        for(uint32_t i = 1 ; i < Fbo::getMaxColorTargetCount() ; i++)
        {
            blendDesc.setRenderTargetWriteMask(i, false, false, false, false);
        }
        blendDesc.setIndependentBlend(true);
        mpBlendState = BlendState::create(blendDesc);

        return true;
    }

    SkyBox::UniquePtr SkyBox::createFromTexCube(const std::string& textureName, bool loadAsSrgb, Falcor::Sampler::SharedPtr pSampler, bool renderStereo)
    {
        Texture::SharedPtr pTexture = createTextureFromFile(textureName, true, loadAsSrgb);
        if(pTexture == nullptr)
        {
            return nullptr;
        }
        return create(pTexture, pSampler, renderStereo);
    }

    void SkyBox::render(Falcor::RenderContext* pRenderCtx, Falcor::Camera* pCamera)
    {
        glm::mat4 world = glm::translate(pCamera->getPosition());
        mpCB->setVariable(mMatOffset, world);
        mpCB->setTexture(mTexOffset, mpTexture.get(), mpSampler.get());
        mpCB->setVariable(mScaleOffset, mScale);

//        pRenderCtx->setUniformBuffer(0, mpUbo);

        // Store the state
//         auto pOldRsState = pRenderCtx->getRasterizerState();
//         auto pOldDsState = pRenderCtx->getDepthStencilState();
//         uint32_t stencilRef = pRenderCtx->getStencilRef();
//         auto pOldBlendState = pRenderCtx->getBlendState();
//         uint32_t sampleMask = pRenderCtx->getSampleMask();
// 
//         pRenderCtx->setRasterizerState(mpNoCullRsState);
//         pRenderCtx->setDepthStencilState(mpDepthStencilState, 0);
//         pRenderCtx->setBlendState(mpBlendState, sampleMask);
// 
//         ModelRenderer::render(pRenderCtx, mpProgram.get(), mpCubeModel, pCamera, false);
// 
//         // Restore the state
//         pRenderCtx->setDepthStencilState(pOldDsState, stencilRef);
//         pRenderCtx->setRasterizerState(pOldRsState);
//         pRenderCtx->setBlendState(pOldBlendState, sampleMask);
    }
}