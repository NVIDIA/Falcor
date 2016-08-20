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
#include "ShaderToy.h"

ShaderToy::~ShaderToy()
{
}

void ShaderToy::initUI()
{
    Gui::setGlobalHelpMessage("Sample application to load ShaderToys.");
}

void ShaderToy::onLoad()
{
    // create rasterizer state
    RasterizerState::Desc rsDesc;
    mpNoCullRastState = RasterizerState::create(rsDesc);

    // Depth test
    DepthStencilState::Desc dsDesc;
	dsDesc.setDepthTest(false);
	mpNoDepthDS = DepthStencilState::create(dsDesc);

    // Blend state
    BlendState::Desc blendDesc;
    mpOpaqueBS = BlendState::create(blendDesc);

    // Texture sampler
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
    mpLinearSampler = Sampler::create(samplerDesc);

    // Load shaders
    mpMainPass = FullScreenPass::create("toyContainer.fs");

    // Create Constant buffer
    mpToyCB = UniformBuffer::create(mpMainPass->getProgram()->getActiveProgramVersion().get(), "ToyCB");

    // Get buffer finding
    mToyCBBinding = mpMainPass->getProgram()->getActiveProgramVersion()->getUniformBufferBinding("ToyCB");

    initUI();
}

void ShaderToy::onFrameRender()
{
    // set up framebuffer and viewport
    auto width = mpDefaultFBO->getWidth();
    auto height = mpDefaultFBO->getHeight();
    mpRenderContext->setFbo(mpDefaultFBO);
    RenderContext::Viewport vp;
    vp.width = float(width);
    vp.height = float(height);
    mpRenderContext->setViewport(0, vp);

    // set up per-frame constants

    // iResolution
    glm::vec2 iResolution = glm::vec2((float)width, (float)height);
    mpToyCB->setVariable("iResolution", iResolution);

    // iGlobalTime
    float iGlobalTime = (float)mCurrentTime;  
    mpToyCB->setVariable("iGlobalTime", iGlobalTime);

    // run final pass
    mpToyCB->uploadToGPU();
    mpRenderContext->setUniformBuffer(mToyCBBinding, mpToyCB);
    mpMainPass->execute(mpRenderContext.get());

    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void ShaderToy::onShutdown()
{
}

bool ShaderToy::onKeyEvent(const KeyboardEvent& keyEvent)
{
    bool bHandled = false;
    {
        if(keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            //switch(keyEvent.key)
            //{
            //default:
            //    bHandled = false;
            //}
        }
    }
    return bHandled;
}

bool ShaderToy::onMouseEvent(const MouseEvent& mouseEvent)
{
    bool bHandled = false;
    return bHandled;
}

void ShaderToy::onResizeSwapChain()
{
    uint32_t width = mpDefaultFBO->getWidth();
    uint32_t height = mpDefaultFBO->getHeight();

    mAspectRatio = (float(width) / float(height));
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ShaderToy sample;
    SampleConfig config;
    config.windowDesc.swapChainDesc.width = 1280;
    config.windowDesc.swapChainDesc.height = 720;
    config.windowDesc.swapChainDesc.isSrgb = false;
    config.enableVsync = true;
    config.windowDesc.resizableWindow = true;
    config.windowDesc.title = "Falcor Shader Toy";
    sample.run(config);
}
