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
#include "EnvMap.h"

void EnvMap::initUI()
{
    Gui::setGlobalHelpMessage("Sample application to load and display a model.\nUse the UI to switch between wireframe and solid mode.");
    mpGui->addButton("Load TexCube", loadTextureCB, this, "");
    mpGui->addFloatVarWithCallback("Cubemap Scale", setScaleCB, getScaleCB, this, "", 0.01f, FLT_MAX, 0.01f);
}

void EnvMap::onLoad()
{
    mpCamera = Camera::create();
    mpCameraController = SixDoFCameraController::SharedPtr(new SixDoFCameraController);
    mpCameraController->attachCamera(mpCamera);
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpTriLinearSampler = Sampler::create(samplerDesc);
    initUI();
}

void EnvMap::loadTexture()
{
    std::string filename;
    if(openFileDialog("DDS files\0*.dds\0\0", filename))
    {
        mpSkybox = SkyBox::createFromTexCube(filename, true, mpTriLinearSampler);
    }
}

void EnvMap::loadTextureCB(void* pThis)
{
    EnvMap* pEnvMap = (EnvMap*)pThis;
    pEnvMap->loadTexture();
}

void EnvMap::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpDefaultFBO->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpSkybox)
    {
        mpCameraController->update();
        mpSkybox->render(mpRenderContext.get(), mpCamera.get());
    }
    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void EnvMap::onShutdown()
{

}

bool EnvMap::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mpCameraController->onKeyEvent(keyEvent);
}

bool EnvMap::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mpCameraController->onMouseEvent(mouseEvent);
}

void EnvMap::onDataReload()
{

}

void GUI_CALL EnvMap::getScaleCB(void* pData, void* pThis)
{
    EnvMap* pEnv = (EnvMap*)pThis;
    if(pEnv->mpSkybox)
    {
        *(float*)pData = pEnv->mpSkybox->getScale();
    }
}
void GUI_CALL EnvMap::setScaleCB(const void* pData, void* pThis)
{
    EnvMap* pEnv = (EnvMap*)pThis;
    if(pEnv->mpSkybox)
    {
        pEnv->mpSkybox->setScale(*(float*)pData);
    }
}


void EnvMap::onResizeSwapChain()
{
    RenderContext::Viewport vp;
    vp.height = (float)mpDefaultFBO->getHeight();
    vp.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, vp);
    mpCamera->setFovY(float(M_PI / 8));
    mpCamera->setAspectRatio(vp.width / vp.height);
    mpCamera->setDepthRange(0.01f, 1000);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    EnvMap sample;
    SampleConfig config;
    config.windowDesc.title = "Skybox Sample";
    sample.run(config);
}
