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

void StereoRendering::initUI()
{

    Gui::setGlobalHelpMessage("Sample application to load and display a model.\nUse the UI to switch between wireframe and solid mode.");
    mpGui->addButton("Load Scene", &StereoRendering::loadSceneCB, this);
    Gui::DropdownList submitModeList;
    submitModeList.push_back({(int)SceneRenderer::RenderMode::Mono, "Render to Screen"});

    if(VRSystem::instance())
    {
        mpGui->addCheckBox("Display VR FBO", &mShowStereoViews);
        submitModeList.push_back({(int)SceneRenderer::RenderMode::SinglePassStereo, "Single Pass Stereo"});
    }
    mpGui->addDropdownWithCallback("Submission Mode", submitModeList, setRenderModeCB, getRenderModeCB, this);
}

void StereoRendering::initVR()
{
    VRDisplay* pDisplay = VRSystem::instance()->getHMD().get();

    // Create the FBOs
    glm::ivec2 renderSize = pDisplay->getRecommendedRenderSize();
    ResourceFormat colorFormat = ResourceFormat::RGBA8UnormSrgb;
    mpVrFbo = VrFbo::create(&colorFormat, 1, ResourceFormat::D24Unorm);
}

void StereoRendering::submitSinglePassStereo()
{
    PROFILE(SPS);

    // Set the viewport
    mpVrFbo->pushViewport(mpRenderContext.get(), 0);

    // Clear the FBO
    mpVrFbo->getFbo()->clear(kClearColor, 1.0f, 0, FboAttachmentType::All);
    mpRenderContext->pushFbo(mpVrFbo->getFbo());

    // Render
    mpSceneRenderer->renderScene(mpRenderContext.get(), mpProgram.get());

    // Restore the state
    mpRenderContext->popFbo();
    mpRenderContext->popViewport(0);

    // Submit the views and display them
    mpVrFbo->submitToHmd();
    blitTexture(mpVrFbo->getEyeResourceView(VRDisplay::Eye::Left).get(), 0);
    blitTexture(mpVrFbo->getEyeResourceView(VRDisplay::Eye::Right).get(), mpDefaultFBO->getWidth() / 2);
}

void StereoRendering::submitToScreen()
{
    mpSceneRenderer->renderScene(mpRenderContext.get(), mpProgram.get());
}

void StereoRendering::loadSceneCB(void* pThis)
{
    StereoRendering* pSR = (StereoRendering*)pThis;
    pSR->loadScene();
}

void StereoRendering::setRenderMode()
{
    if(mpScene)
    {
        std::string lights;
        getSceneLightString(mpScene.get(), lights);
        mpProgram->addDefine("_LIGHT_SOURCES", lights);
        switch(mRenderMode)
        {
        case SceneRenderer::RenderMode::SinglePassStereo:
            mpProgram->addDefine("_SINGLE_PASS_STEREO");
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
        mpScene = Scene::loadFromFile(Filename, Model::GenerateTangentSpace);
        mpSceneRenderer = SceneRenderer::create(mpScene);

        mpProgram = Program::createFromFile("", "StereoRendering.fs");
        setRenderMode();
        mpUniformBuffer = UniformBuffer::create(mpProgram, "PerFrameCB");
    }
}

void StereoRendering::onLoad()
{
    initUI();
    initVR();

    mpBlit = FullScreenPass::create("blit.fs");
    mpBlitUbo = UniformBuffer::create(mpBlit->getProgram(), "PerImageCB");
}

void StereoRendering::blitTexture(const Texture* pTexture, uint32_t xStart)
{
    if(mShowStereoViews)
    {
        RenderContext::Viewport vp;
        vp.originX = (float)xStart;
        vp.originY = 0;
        vp.height = (float)mpDefaultFBO->getHeight();
        vp.width = (float)(mpDefaultFBO->getWidth() / 2);
        vp.minDepth = 0;
        vp.maxDepth = 1;
        mpRenderContext->pushViewport(0, vp);
        mpBlitUbo->setTexture(0, pTexture, nullptr);
        mpRenderContext->setUniformBuffer(0, mpBlitUbo);
        mpBlit->execute(mpRenderContext.get());
        mpRenderContext->popViewport(0);
    }
}

void StereoRendering::onFrameRender()
{
    mpDefaultFBO->clear(kClearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpSceneRenderer)
    {
        setSceneLightsIntoUniformBuffer(mpScene.get(), mpUniformBuffer.get());
        mpRenderContext->setUniformBuffer(0, mpUniformBuffer);
        mpSceneRenderer->update(mCurrentTime);

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

    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

bool StereoRendering::onKeyEvent(const KeyboardEvent& keyEvent)
{
    if(keyEvent.key == KeyboardEvent::Key::Space && keyEvent.type == KeyboardEvent::Type::KeyPressed)
    {
        mRenderMode = (mRenderMode == SceneRenderer::RenderMode::SinglePassStereo) ? SceneRenderer::RenderMode::Mono : SceneRenderer::RenderMode::SinglePassStereo;
        setRenderModeCB(&mRenderMode, this);
        return true;
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
    RenderContext::Viewport vp;
    vp.height = (float)mpDefaultFBO->getHeight();
    vp.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, vp);
}

void StereoRendering::getRenderModeCB(void* pVal, void* pUserData)
{
    StereoRendering* pThis = (StereoRendering*)pUserData;
    *(uint32_t*)pVal = (uint32_t)pThis->mRenderMode;
}

void StereoRendering::setRenderModeCB(const void* pVal, void* pUserData)
{
    StereoRendering* pThis = (StereoRendering*)pUserData;
    pThis->mRenderMode = (SceneRenderer::RenderMode)*(uint32_t*)pVal;
    pThis->setRenderMode();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    StereoRendering sample;
    SampleConfig config;
    config.windowDesc.title = "Stereo Rendering";
    config.windowDesc.swapChainDesc.height = 1024;
    config.windowDesc.swapChainDesc.width = 1600;
    config.enableVR = true;
    sample.run(config);
}
