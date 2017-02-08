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
#include "BlendStateTest.h"
#include <iostream>

void BlendStateTest::addTests()
{
    addTestToList<TestCreate>();
    addTestToList<TestRtArray>();
}

testing_func(BlendStateTest, TestCreate)
{
    const uint32_t numBlendFactors = 10u;
    const BlendState::BlendOp blendOp = BlendState::BlendOp::Add;
    const BlendState::BlendFunc blendFunc = BlendState::BlendFunc::Zero;
    TestDesc desc;
    //RT stuff doesn't need to be thoroughly tested here, tested in TestRtArray
    desc.setRenderTargetWriteMask(0, true, true, true, true);
    desc.setRtBlend(0, true);
    desc.setRtParams(0, blendOp, blendOp, blendFunc, blendFunc, blendFunc, blendFunc);
    //Blend factor
    for (uint32_t i = 0; i < numBlendFactors; ++i)
    {
        float colorR = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float colorG = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float colorB = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float colorA = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        desc.setBlendFactor(glm::vec4(colorR, colorG, colorB, colorA));

        //Set Alpha To Coverage
        for (uint32_t j = 0; j < 2; ++j)
        {
            desc.setAlphaToCoverage(j == 0u);
            //Set Independent Blend
            for (uint32_t k = 0; k < 2; ++k)
            {
                desc.setIndependentBlend(k == 0u);
                //Create and Check blend state
                BlendState::SharedPtr blendState = BlendState::create(desc);
                if (!doStatesMatch(blendState, desc))
                {
                    return test_fail("Blend state doesn't match desc used to create");
                }
            }
        }
    }

    return TEST_PASS;
}

testing_func(BlendStateTest, TestRtArray)
{
    const uint32_t numBlendOps = 5;
    const uint32_t numBlendFuncs = 16;
    uint32_t rtIndex = 0;

    TestDesc desc;
    //rgbop
    for (uint32_t i = 0; i < numBlendOps; ++i)
    {
        BlendState::BlendOp rgbOp = static_cast<BlendState::BlendOp>(i);
        //alpha op
        for (uint32_t j = 0; j < numBlendOps; ++j)
        {
            BlendState::BlendOp alphaOp = static_cast<BlendState::BlendOp>(j);
            //srcRgbFunc
            for (uint32_t k = 0; k < numBlendFuncs; ++k)
            {
                BlendState::BlendFunc srcRgbFunc = static_cast<BlendState::BlendFunc>(k);
                //dstRgbFunc
                for (uint32_t x = 0; x < numBlendFuncs; ++x)
                {
                    BlendState::BlendFunc dstRgbFunc = static_cast<BlendState::BlendFunc>(x);
                    //srcAlphaFunc
                    for (uint32_t y = 0; y < numBlendFuncs; ++y)
                    {
                        BlendState::BlendFunc srcAlphaFunc = static_cast<BlendState::BlendFunc>(y);
                        //dstAlphaFunc
                        for (uint32_t z = 0; z < numBlendFuncs; ++z)
                        {
                            BlendState::BlendFunc dstAlphaFunc = static_cast<BlendState::BlendFunc>(z);
                            //RT Blend
                            for (uint32_t a = 0; a < 2; ++a)
                            {
                                bool rtBlend = a == 0u;
                                //RT writeMask
                                for (uint32_t b = 0; b < 13; ++b)
                                {
                                    bool writeRed = (b & 1) != 0u;
                                    bool writeBlue = (b & 2) != 0u;
                                    bool writeGreen = (b & 4) != 0u;
                                    bool writeAlpha = (b & 8) != 0u;

                                    if (rtIndex >= Fbo::getMaxColorTargetCount())
                                    {
                                        rtIndex = 0;
                                    }

                                    //Set all properties
                                    desc.setRtParams(rtIndex, rgbOp, alphaOp, srcRgbFunc, dstRgbFunc, srcAlphaFunc, dstAlphaFunc);
                                    desc.setRtBlend(rtIndex, rtBlend);
                                    desc.setRenderTargetWriteMask(rtIndex, writeRed, writeGreen, writeBlue, writeAlpha);
                                    //Create and check state
                                    BlendState::SharedPtr state = BlendState::create(desc);
                                    if (!doStatesMatch(state, desc))
                                    {
                                        return test_fail("Render target desc doesn't match ones used to create");
                                    }
                                    ++rtIndex;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return TEST_PASS;
}

bool BlendStateTest::doStatesMatch(const BlendState::SharedPtr state, const TestDesc& desc)
{
    //check settings not in the rt array
    if (state->isAlphaToCoverageEnabled() != desc.mAlphaToCoverageEnabled ||
        state->getBlendFactor() != desc.mBlendFactor ||
        state->isIndependentBlendEnabled() != desc.mEnableIndependentBlend ||
        state->getRtCount() != desc.mRtDesc.size())
    {
        return false;
    }

    //check the rt array
    for (uint32_t i = 0; i < state->getRtCount(); ++i)
    {
        BlendState::Desc::RenderTargetDesc rtDesc = state->getRtDesc(i);
        BlendState::Desc::RenderTargetDesc otherRtDesc = desc.mRtDesc[i];
        if (rtDesc.writeMask.writeRed != otherRtDesc.writeMask.writeRed ||
            rtDesc.writeMask.writeGreen != otherRtDesc.writeMask.writeGreen ||
            rtDesc.writeMask.writeBlue != otherRtDesc.writeMask.writeBlue ||
            rtDesc.writeMask.writeAlpha != otherRtDesc.writeMask.writeAlpha ||
            rtDesc.blendEnabled != otherRtDesc.blendEnabled ||
            rtDesc.rgbBlendOp != otherRtDesc.rgbBlendOp ||
            rtDesc.alphaBlendOp != otherRtDesc.alphaBlendOp ||
            rtDesc.srcRgbFunc != otherRtDesc.srcRgbFunc ||
            rtDesc.dstRgbFunc != otherRtDesc.dstRgbFunc ||
            rtDesc.srcAlphaFunc != otherRtDesc.srcAlphaFunc ||
            rtDesc.dstAlphaFunc != otherRtDesc.dstAlphaFunc)
        {
            return false;
        }

    }

    return true;
}

int main()
{
    BlendStateTest bst;
    bst.init();
    bst.run();
    return 0;
}