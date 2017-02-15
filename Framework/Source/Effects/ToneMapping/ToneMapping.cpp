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
#include "ToneMapping.h"
#include "API/RenderContext.h"
#include "Graphics/FboHelper.h"

namespace Falcor
{
    static const char* kShaderFilename = "Effects\\ToneMapping.ps.hlsl";

    ToneMapping::~ToneMapping() = default;

    ToneMapping::ToneMapping(ToneMapping::Operator op)
    {
        mOperatorIndex = static_cast<uint32_t>(op);
        createLuminancePass();
        createToneMapPass(op);
        Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point);
        mpPointSampler = Sampler::create(samplerDesc);
        samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
        mpLinearSampler = Sampler::create(samplerDesc);
    }

    ToneMapping::UniquePtr ToneMapping::create(Operator op)
    {
        ToneMapping* pTM = new ToneMapping(op);
        return ToneMapping::UniquePtr(pTM);
    }

    void ToneMapping::createLuminanceFbo(Fbo::SharedPtr pSrcFbo)
    {
        bool createFbo = mpLuminanceFbo == nullptr;
        ResourceFormat srcFormat = pSrcFbo->getColorTexture(0)->getFormat();
        uint32_t bytesPerChannel = getFormatBytesPerBlock(srcFormat) / getFormatChannelCount(srcFormat);
        
        ResourceFormat luminanceFormat = (bytesPerChannel == 32) ? ResourceFormat::R32Float : ResourceFormat::R16Float;

        if(createFbo == false)
        {
            createFbo = (pSrcFbo->getWidth() != mpLuminanceFbo->getWidth()) ||
                (pSrcFbo->getHeight() != mpLuminanceFbo->getHeight()) ||
                (luminanceFormat != mpLuminanceFbo->getColorTexture(0)->getFormat());
        }

        if(createFbo)
        {
            Fbo::Desc desc;
            desc.setColorTarget(0, luminanceFormat);
            mpLuminanceFbo = FboHelper::create2D(pSrcFbo->getWidth(), pSrcFbo->getHeight(), desc, 1, Fbo::kAttachEntireMipLevel);
        }
    }

    void ToneMapping::execute(RenderContext* pRenderContext, Fbo::SharedPtr pSrc, Fbo::SharedPtr pDst)
    {
        GraphicsState::SharedPtr pState = pRenderContext->getGraphicsState();
        createLuminanceFbo(pSrc);

        //Set shared vars
        mpToneMapVars->setTexture("gColorTex", pSrc->getColorTexture(0));
        mpLuminanceVars->setTexture("gColorTex", pSrc->getColorTexture(0));
        mpToneMapVars->setSampler(1u, mpPointSampler);
        mpLuminanceVars->setSampler(1u, mpPointSampler);

        //Calculate luminance
        pRenderContext->setGraphicsVars(mpLuminanceVars);
        pState->setFbo(mpLuminanceFbo);
        mpLuminancePass->execute(pRenderContext);
        mpLuminanceFbo->getColorTexture(0)->generateMips();

        //Set Tone map vars
        if (mOperator != Operator::Clamp)
        {
            mpToneMapVars->setSampler(0u, mpLinearSampler);
            mpToneMapVars["PerImageCB"]["gMiddleGray"] = mMiddleGray;
            mpToneMapVars["PerImageCB"]["gMaxWhiteLuminance"] = mWhiteMaxLuminance;
            mpToneMapVars["PerImageCB"]["gLuminanceLod"] = mLuminanceLod;
            mpToneMapVars["PerImageCB"]["gWhiteScale"] = mWhiteScale;
            mpToneMapVars->setTexture("gLuminanceTex", mpLuminanceFbo->getColorTexture(0));
        }

        //Tone map
        pRenderContext->setGraphicsVars(mpToneMapVars);
        pState->setFbo(pDst);
        mpToneMapPass->execute(pRenderContext);
    }

    void ToneMapping::createToneMapPass(ToneMapping::Operator op)
    {
        mpToneMapPass = FullScreenPass::create(kShaderFilename);

        mOperator = op;
        switch(op)
        {
        case Operator::Clamp:
            mpToneMapPass->getProgram()->addDefine("_CLAMP");
            break;
        case Operator::Linear:
            mpToneMapPass->getProgram()->addDefine("_LINEAR");
            break;
        case Operator::Reinhard:
            mpToneMapPass->getProgram()->addDefine("_REINHARD");
            break;
        case Operator::ReinhardModified:
            mpToneMapPass->getProgram()->addDefine("_REINHARD_MOD");
            break;
        case Operator::HejiHableAlu:
            mpToneMapPass->getProgram()->addDefine("_HEJI_HABLE_ALU");
            break;
        case Operator::HableUc2:
            mpToneMapPass->getProgram()->addDefine("_HABLE_UC2");
            break;
        default:
            should_not_get_here();
        }

        mpToneMapVars = GraphicsVars::create(mpToneMapPass->getProgram()->getActiveVersion()->getReflector());
    }

    void ToneMapping::createLuminancePass()
    {
        mpLuminancePass = FullScreenPass::create(kShaderFilename);
        mpLuminancePass->getProgram()->addDefine("_LUMINANCE");
        mpLuminanceVars = GraphicsVars::create(mpLuminancePass->getProgram()->getActiveVersion()->getReflector());
    }

    void ToneMapping::setUiElements(Gui* pGui, const std::string& uiGroup)
    {

        Gui::dropdown_list opList;
        opList.push_back({(uint32_t)Operator::Clamp, "Clamp to LDR"});
        opList.push_back({(uint32_t)Operator::Linear, "Linear"});
        opList.push_back({(uint32_t)Operator::Reinhard, "Reinhard"});
        opList.push_back({(uint32_t)Operator::ReinhardModified, "Modified Reinhard"});
        opList.push_back({(uint32_t)Operator::HejiHableAlu, "Heji's approximation"});
        opList.push_back({(uint32_t)Operator::HableUc2, "Uncharted 2"});

         pGui->addDropdown("Operator", opList, mOperatorIndex);
         pGui->addFloatVar("Exposure Key", mMiddleGray, 0.0001f, 2.0f);
         pGui->addFloatVar("White Luminance", mWhiteMaxLuminance, 0.1f, FLT_MAX, 0.2f);
         pGui->addFloatVar("Luminance LOD", mLuminanceLod, 0, 16, 0.025f);
         pGui->addFloatVar("Linear White", mWhiteScale, 0, 100, 0.01f);

         if (mOperatorIndex != static_cast<uint32_t>(mOperator))
         {
             mOperator = static_cast<Operator>(mOperatorIndex);
             createToneMapPass(mOperator);
         }
    }

    void ToneMapping::setOperator(Operator op)
    {
        if(op != mOperator)
        {
            createToneMapPass(op);
        }
    }

    void ToneMapping::setMiddleGray(float middleGray)
    {
        mMiddleGray = max(0.001f, middleGray);
    }

    void ToneMapping::setWhiteMaxLuminance(float maxLuminance)
    {
        mWhiteMaxLuminance = maxLuminance;
    }

    void ToneMapping::setLuminanceLod(float lod)
    {
        mLuminanceLod = clamp(lod, 0.0f, 16.0f);
    }

    void ToneMapping::setWhiteScale(float whiteScale)
    {
        mWhiteScale = max(0.001f, whiteScale);
    }
}