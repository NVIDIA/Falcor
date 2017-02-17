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
#include "SceneEditorSample.h"

void SceneEditorSample::onGuiRender()
{
    mpGui->addSeparator();

    if (mpGui->addButton("Create New Scene"))
    {
        createScene();
    }
    if (mpGui->addButton("Load Scene"))
    {
        loadScene();
    }
    if(mpEditor)
    {
        mpEditor->renderGui(mpGui.get());
        mpGui->addCheckBox("Preview Camera", mCameraLiveViewMode);
    }
}

void SceneEditorSample::onResizeSwapChain()
{
    if (mpEditor)
    {
        mpEditor->onResizeSwapChain();
    }
}

void SceneEditorSample::onLoad()
{
}

void SceneEditorSample::reset()
{
    mpProgram = nullptr;
    mpVars = nullptr;
}

void SceneEditorSample::initNewScene()
{
    if(mpScene)
    {
        mpRenderer = SceneRenderer::create(mpScene);
        mpEditor = SceneEditor::create(mpScene, Model::GenerateTangentSpace);

        initShader();
    }
}

void SceneEditorSample::initShader()
{
    mpProgram = GraphicsProgram::createFromFile("", "SceneEditorSample.fs");
    std::string lights;
    getSceneLightString(mpScene.get(), lights);
    mpProgram->addDefine("_LIGHT_SOURCES", lights);
    mpVars = GraphicsVars::create(mpProgram->getActiveVersion()->getReflector());
}

void SceneEditorSample::loadScene()
{
    std::string Filename;
    if(openFileDialog("Scene files\0*.fscene\0\0", Filename))
    {
        reset();
        mpScene = Scene::loadFromFile(Filename, Model::GenerateTangentSpace);
        initNewScene();
    }
}

void SceneEditorSample::createScene()
{
    reset();
    mpScene = Scene::create((float)mpDefaultFBO->getWidth()/(float)mpDefaultFBO->getHeight());
    initNewScene();
}

std::string PrintVec3(const glm::vec3& v)
{
    std::string s("(");
    s += std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ")";
    return s;
}

void SceneEditorSample::onFrameRender()
{

    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpScene)
    {
        // If lights changed, recompile shader
        if (mScenePrevLightCount != mpScene->getLightCount())
        {
            initShader();
            mScenePrevLightCount = mpScene->getLightCount();
        }

        mpDefaultPipelineState->setBlendState(nullptr);
        mpDefaultPipelineState->setDepthStencilState(nullptr);
        setSceneLightsIntoConstantBuffer(mpScene.get(), mpVars["PerFrameCB"].get());
        mpRenderContext->setGraphicsVars(mpVars);
        mpDefaultPipelineState->setProgram(mpProgram);

        mpEditor->update(mCurrentTime);
        mpRenderer->update(mCurrentTime);

        const auto& pCamera = mCameraLiveViewMode ? mpScene->getActiveCamera() : mpEditor->getEditorCamera();
        mpRenderer->renderScene(mpRenderContext.get(), pCamera.get());
    }

    if (mpEditor && mCameraLiveViewMode == false)
    {
        mpEditor->render(mpRenderContext.get());
    }
}

void SceneEditorSample::onShutdown()
{
    reset();
}

bool SceneEditorSample::onKeyEvent(const KeyboardEvent& keyEvent)
{
    if (mCameraLiveViewMode)
    {
        return mpRenderer->onKeyEvent(keyEvent);
    }

    return mpEditor ? mpEditor->onKeyEvent(keyEvent) : false;
}

bool SceneEditorSample::onMouseEvent(const MouseEvent& mouseEvent)
{
    if (mCameraLiveViewMode)
    {
        return mpRenderer->onMouseEvent(mouseEvent);
    }

    return mpEditor ? mpEditor->onMouseEvent(mpRenderContext.get(), mouseEvent) : false;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    SceneEditorSample sceneEditor;
    SampleConfig config;
    config.windowDesc.title = "Scene Editor";
    config.freezeTimeOnStartup = true;
    sceneEditor.run(config);
}
