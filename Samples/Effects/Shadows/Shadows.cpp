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

void Shadows::onGuiRender()
{
    if (mpGui->addButton("Load Scene"))
    {
        displayLoadSceneDialog();
    }

    mpGui->addCheckBox("Update Shadow Map", mControls.updateShadowMap);
    if(mpGui->addIntVar("Cascade Count", mControls.cascadeCount, 1u, CSM_MAX_CASCADES))
    {
        for (uint32_t i = 0; i < mpCsmTech.size(); i++)
        {
            mpCsmTech[i]->setCascadeCount(mControls.cascadeCount);
        }
        createVisualizationProgram();
    }

    mpGui->addCheckBox("Visualize Cascades", mControls.visualizeCascades);
    mpGui->addCheckBox("Display Shadow Map", mControls.showShadowMap);
    mpGui->addIntVar("Displayed Cascade", mControls.displayedCascade, 0u, mControls.cascadeCount - 1);
    mpGui->addIntVar("LightIndex", mControls.lightIndex, 0u, mpScene->getLightCount() - 1);

    std::string groupName = "Light " + std::to_string(mControls.lightIndex);
    if (mpGui->beginGroup(groupName.c_str()))
    {
        mpScene->getLight(mControls.lightIndex)->setUiElements(mpGui.get());
        mpGui->endGroup();
    }
    mpCsmTech[mControls.lightIndex]->setUiElements(mpGui.get(), "CSM");
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
    mpCsmTech[mControls.lightIndex]->setUiElements(mpGui.get(), "CSM");
}

void Shadows::createScene(const std::string& filename)
{
    // Load the scene
    mpScene = Scene::loadFromFile(filename, Model::GenerateTangentSpace);

    // Create the renderer
    mpRenderer = SceneRenderer::create(mpScene);
    mpRenderer->setCameraControllerType(SceneRenderer::CameraControllerType::FirstPerson);

    mpCsmTech.resize(mpScene->getLightCount());
    for(uint32_t i = 0; i < mpScene->getLightCount(); i++)
    {
        mpCsmTech[i] = CascadedShadowMaps::create(2048, 2048, mpScene->getLight(i), mpScene, mControls.cascadeCount);
        mpCsmTech[i]->setFilterMode(CsmFilterHwPcf);
        mpCsmTech[i]->setVsmLightBleedReduction(0.3f);
    }
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
    //createScene("WorkingCitadel.fscene");
    createScene("Scenes/dragonPlane.fscene");
    //createScene("Scenes/dragonPlanePoint.fscene");
    //createScene("Scenes/dragonPlaneDir.fscene");
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
    mpRenderContext->pushGraphicsVars(mLightingPass.pProgramVars);
    
    mpRenderer->renderScene(mpRenderContext.get());

    mpRenderContext->popGraphicsVars();
    //mpRenderContext->popGraphicsState();
}

void Shadows::displayShadowMap()
{
    mShadowVisualizer.pProgramVars->setTexture("gTexture", mpCsmTech[mControls.lightIndex]->getShadowMap());
    if (mControls.cascadeCount > 1)
    {
        mShadowVisualizer.pCBuffer->setVariable("cascade", mControls.displayedCascade);
    }
    mShadowVisualizer.pProgramVars->setConstantBuffer("PerImageCB", mShadowVisualizer.pCBuffer);
    mpRenderContext->pushGraphicsVars(mShadowVisualizer.pProgramVars);
    mShadowVisualizer.pProgram->execute(mpRenderContext.get());
    mpRenderContext->popGraphicsVars();
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
            mpCsmTech[i]->setDataIntoGraphicsVars(mLightingPass.pProgramVars, var);
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
    mShadowVisualizer.pCBuffer = mShadowVisualizer.pProgramVars["PerImageCB"];
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    Shadows modelViewer;
    SampleConfig config;
    config.windowDesc.title = "Shadows Sample";
    modelViewer.run(config);
}
