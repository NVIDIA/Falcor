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
#include "RasterizerStateTest.h"
#include <iostream>
#include <limits>

RasterizerStateTest::RasterizerStateTest() :mpRasterizerState(nullptr)
{
    REGISTER_NAME;
}

void RasterizerStateTest::addTests()
{
    addTestToList<TestCreate>();
    addTestToList<TestCreateStress>();
}

TESTING_FUNC(RasterizerStateTest, TestCreate)
{
    TestDesc desc;
    desc.setCullMode(RasterizerState::CullMode::None);
    desc.setFillMode(RasterizerState::FillMode::Solid);
    desc.setFrontCounterCW(true);
    desc.setDepthBias(27, 42.6f);
    desc.setDepthClamp(false);
    desc.setLineAntiAliasing(true);
    desc.setScissorTest(false);
    desc.setConservativeRasterization(true);
    desc.setForcedSampleCount(false);

    RasterizerState::SharedPtr state = RasterizerState::create(desc);
    if (doStatesMatch(state, desc))
        return TestData(TestResult::Pass, mName);
    else
        return TestData(TestResult::Fail, mName, "Rasterizer state doesn't match desc used to create it");
}

TESTING_FUNC(RasterizerStateTest, TestCreateStress)
{
    TestDesc desc;
    desc.setCullMode(RasterizerState::CullMode::Front);
    desc.setFillMode(RasterizerState::FillMode::Wireframe);
    desc.setFrontCounterCW(false);
    desc.setDepthBias(std::numeric_limits<int32_t>::min(), std::numeric_limits<float>::max());
    desc.setDepthClamp(true);
    desc.setLineAntiAliasing(false);
    desc.setScissorTest(true);
    desc.setConservativeRasterization(false);
    desc.setForcedSampleCount(true);

    RasterizerState::SharedPtr state = RasterizerState::create(desc);
    if (!doStatesMatch(state, desc))
        return TestData(TestResult::Fail, mName + "Rasterizer state doesn't match desc used to create it");
    
    desc.setDepthBias(std::numeric_limits<int32_t>::max(), std::numeric_limits<float>::min());
    state = nullptr;
    state = RasterizerState::create(desc);
    if (doStatesMatch(state, desc))
        return TestData(TestResult::Pass, mName);
    else
        return TestData(TestResult::Fail, mName, "Rasterizer state doesn't match desc used to create it");
}

bool RasterizerStateTest::doStatesMatch(const RasterizerState::SharedPtr state, const TestDesc& desc)
{
    return state->getCullMode() == desc.mCullMode &&
        state->getFillMode() == desc.mFillMode &&
        state->isFrontCounterCW() == desc.mIsFrontCcw &&
        state->getSlopeScaledDepthBias() == desc.mSlopeScaledDepthBias &&
        state->getDepthBias() == desc.mDepthBias &&
        state->isDepthClampEnabled() == desc.mClampDepth &&
        state->isScissorTestEnabled() == desc.mScissorEnabled &&
        state->isLineAntiAliasingEnabled() == desc.mEnableLinesAA &&
        state->isConservativeRasterizationEnabled() == desc.mConservativeRaster &&
        state->getForcedSampleCount() == desc.mForcedSampleCount;
}

int main()
{
    RasterizerStateTest rst;
    rst.init();
    rst.run();
    return 0;
}