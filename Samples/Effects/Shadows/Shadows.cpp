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
#include "Shadows.h"

void Shadows::initUI()
{
    //TODO Fix GUI

    //mpGui->addButton("Load Scene", loadSceneCB, this);
    //mpGui->addCheckBox("Update Shadow Map", &mControls.updateShadowMap);
    //mpGui->addIntVarWithCallback("Cascade Count", setCascadeCountCB, getCascadeCountCB, this, "", 1, CSM_MAX_CASCADES);
    //mpGui->addCheckBox("Visualize Cascades", &mControls.visualizeCascades);
    //mpGui->addCheckBox("Display Shadow Map", &mControls.showShadowMap);
    //mpGui->addIntVar("Displayed Cascade", &mControls.displayedCascade, "", 0, mControls.cascadeCount - 1);

    //Gui::setGlobalHelpMessage("Sample application to load and display a model.\nUse the UI to switch between wireframe and solid mode.");
    // Load model group

    uint32_t barSize[2];
    //mpGui->getSize(barSize);
    barSize[0] += 50;
    barSize[1] += 100;
    //mpGui->setSize(barSize[0], barSize[1]);
}

void Shadows::displayLoadSceneDialog()
{
    std::string filename;
    if(openFileDialog(Scene::kFileFormatString, filename))
    {
        createScene(filename);
    }
}

void Shadows::setLightIndex(int32_t index)
{
    mControls.lightIndex = max(min(index, (int32_t)mpScene->getLightCount() - 1), 0);
    //TODO Fix Gui2 

    //mpGui->removeGroup("Light");
    //mpScene->getLight(mControls.lightIndex)->setUiElements(mpGui.get(), "Light");

    //mpGui->removeGroup("CSM");

    mpCsmTech[mControls.lightIndex]->setUiElements(mpGui.get(), "CSM");
}

void Shadows::createScene(const std::string& filename)
{
    //TODO fix gui 3

    //mpGui->removeGroup("CSM");
    //mpGui->removeGroup("Light");
    // Load the scene
    mpScene = Scene::loadFromFile(filename, Model::GenerateTangentSpace);

    // Create the renderer
    mpRenderer = SceneRenderer::create(mpScene);
    mpRenderer->setCameraControllerType(SceneRenderer::CameraControllerType::FirstPerson);

    mpCsmTech.resize(mpScene->getLightCount());
    for(uint32_t i = 0; i < mpScene->getLightCount(); i++)
    {
        mpCsmTech[i] = CascadedShadowMaps::create(2048, 2048, mpScene->getLight(i), mpScene, mControls.cascadeCount);
        mpCsmTech[i]->setFilterMode(CsmFilterVsm);
        mpCsmTech[i]->setVsmLightBleedReduction(0.3f);
    }
    //mpGui->addIntVarWithCallback("Light Index", setLightIndexCB, getLightIndexCB, this, "", 0);
    setLightIndex(0);

    // Create the main effect
    mLightingPass.pProgram = GraphicsProgram::createFromFile("Shadows.vs", "Shadows.fs");
    std::string lights;
    getSceneLightString(mpScene.get(), lights);
    mLightingPass.pProgram->addDefine("_LIGHT_SOURCES", lights);
    mLightingPass.pProgram->addDefine("_LIGHT_COUNT", std::to_string(mpScene->getLightCount()));
    mLightingPass.pState = GraphicsState::create();
    mLightingPass.pState->setProgram(mLightingPass.pProgram);
    mLightingPass.pState->setBlendState(nullptr);
    mLightingPass.pState->setDepthStencilState(nullptr);
    mLightingPass.pProgramVars = GraphicsVars::create(mLightingPass.pProgram->getActiveVersion()->getReflector());
    mLightingPass.pCBuffer = mLightingPass.pProgramVars["PerFrameCB"];
}

void Shadows::onLoad()
{
    initUI();
    createScene("Scenes/dragonplane.fscene");
    createVisualizationProgram();
}

void Shadows::runMainPass()
{
    //state
    //TODO should be push
    //mpRenderContext->setGraphicsState(mLightingPass.pState);

    //Not sure why this is necessary and the above set call(which should be a push anyway)
    //doesn't work, look into it in future
    mpRenderContext->getGraphicsState()->setProgram(mLightingPass.pProgram);

    //vars
    setSceneLightsIntoConstantBuffer(mpScene.get(), mLightingPass.pCBuffer.get());
    mLightingPass.pCBuffer->setVariable("visualizeCascades", mControls.visualizeCascades);
    mLightingPass.pCBuffer->setVariable("lightIndex", mControls.lightIndex);
    mLightingPass.pCBuffer->setVariable("camVpAtLastCsmUpdate", mCamVpAtLastCsmUpdate);
    mLightingPass.pProgramVars->setConstantBuffer("PerFrameCB", mLightingPass.pCBuffer);
    mpRenderContext->setGraphicsVars(mLightingPass.pProgramVars);

    mpRenderer->renderScene(mpRenderContext.get());

    //mpRenderContext->popGraphicsVars();
    //mpRenderContext->popGraphicsState();
}

void Shadows::displayShadowMap()
{
    mShadowVisualizer.pProgramVars->setTexture("gTexture", mpCsmTech[mControls.lightIndex]->getShadowMap());
    mShadowVisualizer.pCBuffer->setVariable("cascade", mControls.displayedCascade);
    mShadowVisualizer.pProgramVars->setConstantBuffer("PerImageCB", mShadowVisualizer.pCBuffer);
    mpRenderContext->setGraphicsVars(mShadowVisualizer.pProgramVars);
    mShadowVisualizer.pProgram->execute(mpRenderContext.get());
}

void Shadows::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpScene)
    {
        // Update the scene
        mpRenderer->update(mCurrentTime);

        // Run the shadow pass
        if(mControls.updateShadowMap)
        {
            mCamVpAtLastCsmUpdate = mpScene->getActiveCamera()->getViewProjMatrix();
            for(uint32_t i = 0; i < mpCsmTech.size(); i++)
            {
                mpCsmTech[i]->setup(mpRenderContext.get(), mpScene->getActiveCamera().get(), nullptr);
            }
        }

        for(uint32_t i = 0; i < mpCsmTech.size(); i++)
        {
            std::string var = "gCsmData[" + std::to_string(i) + "]";
            mpCsmTech[i]->setDataIntoConstantBuffer(mLightingPass.pCBuffer.get(), var);
        }

        if(mControls.showShadowMap)
        {
            displayShadowMap();
        }
        else
        {
            runMainPass();
        }
    }

    renderText(getFpsMsg(), glm::vec2(10, 10));
}

void Shadows::onShutdown()
{

}

bool Shadows::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mpRenderer->onKeyEvent(keyEvent);
}

bool Shadows::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mpRenderer->onMouseEvent(mouseEvent);
}

void Shadows::onResizeSwapChain()
{
    //Camera aspect 
    float height = (float)mpDefaultFBO->getHeight();
    float width = (float)mpDefaultFBO->getWidth();
    Camera::SharedPtr activeCamera = mpScene->getActiveCamera();
    activeCamera->setFovY(float(M_PI / 3));
    float aspectRatio = (width / height);
    activeCamera->setAspectRatio(aspectRatio);
}

void Shadows::createVisualizationProgram()
{
    // Create the shadow visualizer
    mShadowVisualizer.pProgram = FullScreenPass::create("VisualizeMap.fs");
    if(mControls.cascadeCount > 1)
    {
        mShadowVisualizer.pProgram->getProgram()->addDefine("_USE_2D_ARRAY");
        mShadowVisualizer.pCBuffer = ConstantBuffer::create(mShadowVisualizer.pProgram->getProgram(), "PerImageCB");
    }
    mShadowVisualizer.pProgramVars = GraphicsVars::create(mShadowVisualizer.pProgram->getProgram()->getActiveVersion()->getReflector());
    //mShadowVisualizer.pState->setProgram(mShadowVisualizer.pProgram->getProgram());
}

void Shadows::setCascadeCountCB(const void* pVal, void* pThis)
{
    Shadows* pShadows = (Shadows*)pThis;
    pShadows->mControls.cascadeCount = *(uint32_t*)pVal;
    for(uint32_t i = 0; i < pShadows->mpCsmTech.size(); i++)
    {
        pShadows->mpCsmTech[i]->setCascadeCount(pShadows->mControls.cascadeCount);
    }
    //pShadows->mpGui->setVarRange("Displayed Cascade", "", 0, pShadows->mControls.cascadeCount - 1);
    pShadows->createVisualizationProgram();
}

void Shadows::getCascadeCountCB(void* pVal, void* pThis)
{
    Shadows* pShadows = (Shadows*)pThis;
    *(uint32_t*)pVal = pShadows->mControls.cascadeCount;
}

void Shadows::getLightIndexCB(void* pVal, void* pThis)
{
    Shadows* pShadows = (Shadows*)pThis;
    *(uint32_t*)pVal = pShadows->mControls.lightIndex;
}

void Shadows::setLightIndexCB(const void* pVal, void* pThis)
{
    Shadows* pShadows = (Shadows*)pThis;
    pShadows->setLightIndex(*(uint32_t*)pVal);
}

void Shadows::loadSceneCB(void* pThis)
{
    Shadows* pShadows = (Shadows*)pThis;
    pShadows->displayLoadSceneDialog();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    Shadows modelViewer;
    SampleConfig config;
    config.windowDesc.title = "Shadows Sample";
    modelViewer.run(config);
}
