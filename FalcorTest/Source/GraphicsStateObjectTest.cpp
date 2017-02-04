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
#include "GraphicsStateObjectTest.h"

void GraphicsStateObjectTest::addTests()
{
    addTestToList<TestCreate>();
}

testing_func(GraphicsStateObjectTest, TestCreate)
{
    //This crashes on debug w/ e_invalid arg in create() in apiInit() when calling CreateGraphicsPipelineState
    //No idea why, tried to emulate what is being done in GraphicsState::getGSO but can't seem to get it to work.
    //It works and fails (rather than crashing) in release so it would seem the GraphicsStateObject is being 
    //created despite e_invalidArg being the HRESULT returned by the creation. Not sure why fails in release,
    //can't really debug in release.

    GraphicsStateObject::Desc desc;
    GraphicsProgram::SharedPtr program = GraphicsProgram::createFromFile("", "Simple.ps.hlsl");
    RootSignature::SharedPtr rootSig = RootSignature::create(program->getActiveVersion()->getReflector().get());

    //Maybe the layout is the wrong thing? tried to make a dummy layout here but it still crashes on create
    float buffer[10] = { 0.4f };
    Buffer::SharedPtr vBuf = Buffer::create(10u, Resource::BindFlags::Vertex, Buffer::CpuAccess::Read, buffer);
    VertexLayout::SharedPtr vLayout = VertexLayout::create();
    VertexBufferLayout::SharedPtr vBufLayout = VertexBufferLayout::create();
    vBufLayout->addElement(VERTEX_POSITION_NAME, 0u, ResourceFormat::RGB32Float, 10u, VERTEX_POSITION_LOC);
    vLayout->addBufferLayout(0u, vBufLayout);

    Fbo::Desc fboDesc;
    BlendState::SharedPtr blendState = BlendState::create(BlendState::Desc());
    RasterizerState::SharedPtr rasterizerState = RasterizerState::create(RasterizerState::Desc());
    DepthStencilState::SharedPtr depthState = DepthStencilState::create(DepthStencilState::Desc());

    //Don't think I need to do anything too in depth with each of these, they will have their own testing classes
    desc.setProgramVersion(program->getActiveVersion());
    desc.setRootSignature(rootSig);
    desc.setVertexLayout(vLayout);
    desc.setFboFormats(fboDesc);
    desc.setBlendState(blendState);
    desc.setRasterizerState(rasterizerState);
    desc.setDepthStencilState(depthState);

    const uint32_t numRandomMasks = 10;
    const uint32 numPrimTypes = 5;
    for (uint32_t i = 0; i < numRandomMasks; ++i)
    {
        uint32_t sampleMask = static_cast<uint32_t>(rand());
        desc.setSampleMask(sampleMask);
        //Start at 1, skip over undefined, which asserts
        for (uint32_t j = 1; j < numPrimTypes; ++j)
        {
            GraphicsStateObject::PrimitiveType primType = static_cast<GraphicsStateObject::PrimitiveType>(j);

            GraphicsStateObject::SharedPtr gObj = GraphicsStateObject::create(desc);
            if (gObj->getDesc().getBlendState()->getApiHandle() != blendState->getApiHandle() ||
                gObj->getDesc().getDepthStencilState()->getApiHandle() != depthState->getApiHandle() ||
                gObj->getDesc().getFboDesc().getColorTargetFormat(0) != fboDesc.getColorTargetFormat(0) ||
                gObj->getDesc().getPrimitiveType() != primType ||
                //would rather check api handle than name for program version, gives unresolved external
                gObj->getDesc().getProgramVersion()->getName() != program->getActiveVersion()->getName() ||
                gObj->getDesc().getRasterizerState()->getApiHandle() != rasterizerState->getApiHandle() ||
                gObj->getDesc().getSampleMask() != sampleMask ||
                gObj->getDesc().getVertexLayout()->getBufferLayout(0)->getStride() != vLayout->getBufferLayout(0)->getStride())
            {
                return test_fail("Graphics State Object does not match desc used to create it");
            }
        }
    }

    return TEST_PASS;
}

int main()
{
    GraphicsStateObjectTest gsot;
    gsot.init(true);
    gsot.run();
    return 0;
}