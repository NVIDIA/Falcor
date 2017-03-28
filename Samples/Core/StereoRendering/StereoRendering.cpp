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
#include "StereoRendering.h"

static const glm::vec4 kClearColor(0.38f, 0.52f, 0.10f, 1);


void StereoRendering::onGuiRender()
{
    if (mpGui->addButton("Load Scene"))
    {
        loadScene();
    }
    Gui::DropdownList submitModeList;
    submitModeList.push_back({ (int)SceneRenderer::RenderMode::Mono, "Render to Screen" });
    if(VRSystem::instance() && gpDevice->isExtensionSupported(""))
    {
        submitModeList.push_back({(int)SceneRenderer::RenderMode::SinglePassStereo, "Single Pass Stereo"});
        mpGui->addCheckBox("Display VR FBO", mShowStereoViews);
    }
    if (mpGui->addDropdown("Submission Mode", submitModeList, (uint32_t&)mRenderMode))
    {
        setRenderMode();
    }
}

void StereoRendering::initVR()
{
    if (VRSystem::instance())
    {
        VRDisplay* pDisplay = VRSystem::instance()->getHMD().get();

        // Create the FBOs
        Fbo::Desc vrFboDesc;

        vrFboDesc.setColorTarget(0, mpDefaultFBO->getColorTexture(0)->getFormat());
        vrFboDesc.setDepthStencilTarget(mpDefaultFBO->getDepthStencilTexture()->getFormat());

        mpVrFbo = VrFbo::create(vrFboDesc);

        static bool first = true;
        if (Device::isExtensionSupported("") == false && first)
        {
            first = false;
            msgBox("The sample relies on NVIDIA's Single Pass Stereo extension to operate correctly.\nMake sure you have an NVIDIA GPU, you enabled NVAPI support in FalcorConfig.h at that you downloaded the NVAPI SDK.\nCheck the readme for instructions");
        }
    }
    else
    {
        msgBox("Can't initialize the VR system. Make sure that your HMD is connected properly");
    }
}

void StereoRendering::submitSinglePassStereo()
{
    PROFILE(SPS);

    // Clear the FBO
    mpRenderContext->clearFbo(mpVrFbo->getFbo().get(), kClearColor, 1.0f, 0, FboAttachmentType::All);
    
    // update state
    mpGraphicsState->setProgram(mpProgram);
    mpGraphicsState->setFbo(mpVrFbo->getFbo());
    mpRenderContext->pushGraphicsState(mpGraphicsState);
    mpRenderContext->setGraphicsVars(mpProgramVars);

    // Render
    mpSceneRenderer->renderScene(mpRenderContext.get());

    // Restore the state
    mpRenderContext->popGraphicsState();

    // Submit the views and display them
    mpVrFbo->submitToHmd(mpRenderContext.get());
    blitTexture(mpVrFbo->getEyeResourceView(VRDisplay::Eye::Left), 0);
    blitTexture(mpVrFbo->getEyeResourceView(VRDisplay::Eye::Right), mpDefaultFBO->getWidth() / 2);
}

void StereoRendering::submitToScreen()
{
    mpGraphicsState->setProgram(mpProgram);
    mpGraphicsState->setFbo(mpDefaultFBO);
    mpRenderContext->setGraphicsState(mpGraphicsState);
    mpRenderContext->setGraphicsVars(mpProgramVars);
    mpSceneRenderer->renderScene(mpRenderContext.get());
}

void StereoRendering::setRenderMode()
{
    if(mpScene)
    {
        std::string lights;
        getSceneLightString(mpScene.get(), lights);
        mpProgram->addDefine("_LIGHT_SOURCES", lights);
        mpGraphicsState->toggleSinglePassStereo(false);
        switch(mRenderMode)
        {
        case SceneRenderer::RenderMode::SinglePassStereo:
            mpProgram->addDefine("_SINGLE_PASS_STEREO");
            mpGraphicsState->toggleSinglePassStereo(true);
            break;
        }
        mpSceneRenderer->setRenderMode(mRenderMode);
    }
}

void StereoRendering::loadScene()
{
    std::string Filename;
    if(openFileDialog("Scene files\0*.fscene\0\0", Filename))
    {
        loadScene(Filename);
    }
}

void StereoRendering::loadScene(const std::string& filename)
{
    mpScene = Scene::loadFromFile(filename);
    mpSceneRenderer = SceneRenderer::create(mpScene);
    mpProgram = GraphicsProgram::createFromFile("", "StereoRendering.ps.hlsl");
    setRenderMode();
    mpProgramVars = GraphicsVars::create(mpProgram->getActiveVersion()->getReflector());
    for (uint32_t m = 0; m < mpScene->getModelCount(); m++)
    {
        mpScene->getModel(m)->bindSamplerToMaterials(mpTriLinearSampler);
    }
}

void StereoRendering::onLoad()
{
    initVR();

    mpGraphicsState = GraphicsState::create();

    mpBlit = FullScreenPass::create("blit.fs");
    mpBlitVars = GraphicsVars::create(mpBlit->getProgram()->getActiveVersion()->getReflector());
    setRenderMode();

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpTriLinearSampler = Sampler::create(samplerDesc);
}

void StereoRendering::blitTexture(Texture::SharedPtr pTexture, uint32_t xStart)
{
    if(mShowStereoViews)
    {
        uint32_t w = mpDefaultFBO->getWidth() / 2;
        uint32_t h = mpDefaultFBO->getHeight();
        GraphicsState::Viewport vp(float(xStart), 0, float(w), float(h), 0, 1);
        mpGraphicsState->setViewport(0, vp, true);
        
        mpGraphicsState->setFbo(mpDefaultFBO, false);
        mpRenderContext->setGraphicsState(mpGraphicsState);
        mpBlitVars->setTexture("gTexture", pTexture);
        mpRenderContext->setGraphicsVars(mpBlitVars);  

        mpBlit->execute(mpRenderContext.get());
    }
}

void StereoRendering::onFrameRender()
{
    static uint32_t frameCount = 0u;

    mpRenderContext->clearFbo(mpDefaultFBO.get(), kClearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpSceneRenderer)
    {      
        mpSceneRenderer->update(mCurrentTime);

        setSceneLightsIntoConstantBuffer(mpScene.get(), mpProgramVars->getConstantBuffer(0).get());

        switch(mRenderMode)
        {
        case SceneRenderer::RenderMode::Mono:
            submitToScreen();
            break;
        case SceneRenderer::RenderMode::SinglePassStereo:
            submitSinglePassStereo();
            break;
        default:
            should_not_get_here();
        }
    }

    std::string message = getFpsMsg();
    message += "\nFrame counter: " + std::to_string(frameCount);

    renderText(message, glm::vec2(10, 10));

    frameCount++;
}

bool StereoRendering::onKeyEvent(const KeyboardEvent& keyEvent)
{
    if(keyEvent.key == KeyboardEvent::Key::Space && keyEvent.type == KeyboardEvent::Type::KeyPressed)
    {
        if (VRSystem::instance() && gpDevice->isExtensionSupported(""))
        {
            mRenderMode = (mRenderMode == SceneRenderer::RenderMode::SinglePassStereo) ? SceneRenderer::RenderMode::Mono : SceneRenderer::RenderMode::SinglePassStereo;
            setRenderMode();
            return true;
        }
    }
    return mpSceneRenderer ? mpSceneRenderer->onKeyEvent(keyEvent) : false;
}

bool StereoRendering::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mpSceneRenderer ? mpSceneRenderer->onMouseEvent(mouseEvent) : false;
}

void StereoRendering::onDataReload()
{

}

void StereoRendering::onResizeSwapChain()
{
    initVR();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    StereoRendering sample;
    SampleConfig config;
    config.windowDesc.title = "Stereo Rendering";
    config.windowDesc.height = 1024;
    config.windowDesc.width = 1600;
    config.windowDesc.resizableWindow = true;
    config.enableVR = true;
    sample.run(config);
}
