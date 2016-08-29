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
#include "NormalMapFiltering.h"

void NormalMapFiltering::initUI()
{
    Gui::setGlobalHelpMessage("Sample application to load and display a model.\nUse the UI to switch between wireframe and solid mode.");

    const Scene* pScene = mpRenderer->getScene();
    for(uint32_t i = 0; i < pScene->getLightCount(); i++)
    {
        std::string group = "Light " + std::to_string(i);
        pScene->getLight(i)->setUiElements(mpGui.get(), group);
    }
    mpGui->addCheckBox("Lean Map", &mUseLeanMap);
}

void setProgramDefines(Program* pProgram, bool leanMap, const Scene* pScene, uint32_t leanMapCount)
{
    std::string lights;
    pProgram->clearDefines();
    getSceneLightString(pScene, lights);
    pProgram->addDefine("_LIGHT_SOURCES", lights);
    if(leanMap)
    {
        pProgram->addDefine("_MS_USER_NORMAL_MAPPING");
        pProgram->addDefine("_LEAN_MAP_COUNT", std::to_string(leanMapCount));
    }
}

void NormalMapFiltering::onLoad()
{
    Scene::SharedPtr pScene = Scene::loadFromFile("scenes\\ogre.fscene", Model::GenerateTangentSpace);
    if(pScene == nullptr)
    {
        exit(1);
    }
    mpRenderer = SceneRenderer::create(pScene);
    mpLeanMap = LeanMap::create(pScene.get());
    mpProgram = Program::createFromFile("", "NormalMapFiltering.fs");
    
    mUseLeanMap = true;
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);
    pScene->getModel(0)->bindSamplerToMaterials(mpLinearSampler);

    setProgramDefines(mpProgram.get(), mUseLeanMap, pScene.get(), mpLeanMap->getRequiredLeanMapShaderArraySize());
    mpLightBuffer = UniformBuffer::create(mpProgram->getActiveProgramVersion().get(), "PerFrameCB");
    mpLeanMapBuffer = UniformBuffer::create(mpProgram->getActiveProgramVersion().get(), "LeanMapsCB");
    mpLeanMap->setIntoUniformBuffer(mpLeanMapBuffer.get(), 0, mpLinearSampler.get());
    mCameraController.attachCamera(pScene->getCamera(0));
    mCameraController.setModelParams(pScene->getModel(0)->getCenter(), pScene->getModel(0)->getRadius(), 4);
    
    initUI();
}

void NormalMapFiltering::onFrameRender()
{
    setProgramDefines(mpProgram.get(), mUseLeanMap, mpRenderer->getScene(), mpLeanMap->getRequiredLeanMapShaderArraySize());

    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpDefaultFBO->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

    mpRenderContext->setBlendState(nullptr);
    mpRenderContext->setDepthStencilState(nullptr, 0);
    setSceneLightsIntoUniformBuffer(mpRenderer->getScene(), mpLightBuffer.get());
    mpRenderContext->setUniformBuffer(0, mpLightBuffer);
    mpRenderContext->setUniformBuffer(1, mpLeanMapBuffer);
    mCameraController.update();
    mpRenderer->renderScene(mpRenderContext.get(), mpProgram.get());

    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void NormalMapFiltering::onShutdown()
{

}

bool NormalMapFiltering::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mCameraController.onKeyEvent(keyEvent);
}

bool NormalMapFiltering::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mCameraController.onMouseEvent(mouseEvent);
}

void NormalMapFiltering::onResizeSwapChain()
{
    RenderContext::Viewport vp;
    vp.height = (float)mpDefaultFBO->getHeight();
    vp.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, vp);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    NormalMapFiltering sample;
    SampleConfig config;
    config.windowDesc.title = "Normal Map Filtering";
    config.windowDesc.swapChainDesc.width = 1600;
    config.windowDesc.swapChainDesc.height = 1024;
    sample.run(config);
}
