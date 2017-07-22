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

    SkyBox::UniquePtr SkyBox::create(Texture::SharedPtr& pSkyTexture, Sampler::SharedPtr pSampler, bool renderStereo)
    {
        UniquePtr pSkyBox = UniquePtr(new SkyBox());
        if(pSkyBox->createResources(pSkyTexture, pSampler, renderStereo) == false)
        {
            return nullptr;
        }
        return pSkyBox;
    }

    bool SkyBox::createResources(Texture::SharedPtr& pTexture, Sampler::SharedPtr pSampler, bool renderStereo)
    {
        if(pTexture == nullptr)
        {
            logError("Trying to create a skybox with null texture");
            return false;
        }

        mpTexture = pTexture;

        mpCubeModel = Model::createFromFile("Effects/cube.obj");
        if(mpCubeModel == nullptr)
        {
            logError("Failed to load cube model for SkyBox");
            return false;
        }

        // Create the program
        Program::DefineList defines;
        if(renderStereo)
        {
            defines.add("_SINGLE_PASS_STEREO");
        }

        assert(mpTexture->getType() == Texture::Type::TextureCube || mpTexture->getType() == Texture::Type::Texture2D);
        if (mpTexture->getType() == Texture::Type::Texture2D)
        {
            defines.add("_SPHERICAL_MAP");
        }

        mpProgram = GraphicsProgram::createFromFile("Effects\\SkyBox.vs.slang", "Effects\\Skybox.ps.slang", defines);
        mpVars = GraphicsVars::create(mpProgram->getActiveVersion()->getReflector());

        mBindLocations.perFrameCB = getBufferBindLocation(mpProgram->getActiveVersion()->getReflector().get(), "PerFrameCB");
        mBindLocations.texture = getResourceBindLocation(mpProgram->getActiveVersion()->getReflector().get(), "gTexture");
        mBindLocations.sampler= getResourceBindLocation(mpProgram->getActiveVersion()->getReflector().get(), "gSampler");

        ConstantBuffer::SharedPtr& pCB = mpVars->getConstantBuffer(mBindLocations.perFrameCB.regSpace, mBindLocations.perFrameCB.baseRegIndex, 0);
        mScaleOffset = pCB->getVariableOffset("gScale");
        mMatOffset = pCB->getVariableOffset("gWorld");

        mpVars->setSrv(mBindLocations.texture.regSpace, mBindLocations.texture.baseRegIndex, 0, mpTexture->getSRV());
        mpVars->setSampler(mBindLocations.sampler.regSpace, mBindLocations.sampler.baseRegIndex, 0, pSampler);

        // Create state
        mpState = GraphicsState::create();
        BlendState::Desc blendDesc;
        for(uint32_t i = 1 ; i < Fbo::getMaxColorTargetCount() ; i++)
        {
            blendDesc.setRenderTargetWriteMask(i, false, false, false, false);
        }
        blendDesc.setIndependentBlend(true);
        mpState->setBlendState(BlendState::create(blendDesc));

        // Create the rasterizer state
        RasterizerState::Desc rastDesc;
        rastDesc.setCullMode(RasterizerState::CullMode::Front).setDepthClamp(true);
        mpState->setRasterizerState(RasterizerState::create(rastDesc));

        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthWriteMask(false).setDepthFunc(DepthStencilState::Func::LessEqual).setDepthTest(true);
        mpState->setDepthStencilState(DepthStencilState::create(dsDesc));
        mpState->setProgram(mpProgram);

        return true;
    }

    Sampler::SharedPtr SkyBox::getSampler() const
    {
        return mpVars->getSampler(mBindLocations.sampler.regSpace, mBindLocations.sampler.baseRegIndex, 0);
    }

    Texture::SharedPtr SkyBox::getTexture() const
    {
        return mpTexture;
    }

    void SkyBox::setSampler(Sampler::SharedPtr pSampler)
    {
        mpVars->setSampler(mBindLocations.sampler.regSpace, mBindLocations.sampler.baseRegIndex, 0, pSampler);
    }

    SkyBox::UniquePtr SkyBox::createFromTexture(const std::string& textureName, bool loadAsSrgb, Sampler::SharedPtr pSampler, bool renderStereo)
    {
        Texture::SharedPtr pTexture = createTextureFromFile(textureName, false, loadAsSrgb);
        if(pTexture == nullptr)
        {
            return nullptr;
        }
        return create(pTexture, pSampler, renderStereo);
    }

    void SkyBox::render(RenderContext* pRenderCtx, Camera* pCamera)
    {
        glm::mat4 world = glm::translate(pCamera->getPosition());
        ConstantBuffer::SharedPtr& pCB = mpVars->getConstantBuffer(mBindLocations.perFrameCB.regSpace, mBindLocations.perFrameCB.baseRegIndex, 0);
        pCB->setVariable(mMatOffset, world);
        pCB->setVariable(mScaleOffset, mScale);

        mpState->setFbo(pRenderCtx->getGraphicsState()->getFbo());
        pRenderCtx->pushGraphicsVars(mpVars);
        pRenderCtx->pushGraphicsState(mpState);

        ModelRenderer::render(pRenderCtx, mpCubeModel, pCamera, false);

        pRenderCtx->popGraphicsVars();
        pRenderCtx->popGraphicsState();
    }
}