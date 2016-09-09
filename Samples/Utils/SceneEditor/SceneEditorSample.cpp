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

void SceneEditorSample::createSceneCallback(void* pUserData)
{
    SceneEditorSample* pEditor = (SceneEditorSample*)pUserData;
    pEditor->createScene();
}

void SceneEditorSample::loadSceneCallback(void* pUserData)
{
    SceneEditorSample* pEditor = (SceneEditorSample*)pUserData;
    pEditor->loadScene();
}

void SceneEditorSample::initUI()
{
    mpGui->addButton("Create New Scene", &SceneEditorSample::createSceneCallback, this);
    mpGui->addButton("Load Scene", &SceneEditorSample::loadSceneCallback, this);
}

void SceneEditorSample::onLoad()
{
    initUI();
}

void SceneEditorSample::reset()
{
    mpProgram = nullptr;
    mpLightBuffer = nullptr;
}

void SceneEditorSample::initNewScene()
{
    if(mpScene)
    {
        mpRenderer = SceneRenderer::create(mpScene);
        mpEditor = nullptr;    // Need to do that for the UI to work correctly
        mpEditor = SceneEditor::create(mpScene);

        mpProgram = Program::createFromFile("", "SceneEditorSample.fs");
        std::string lights;
        getSceneLightString(mpScene.get(), lights);
        mpProgram->addDefine("_LIGHT_SOURCES", lights);
        mpLightBuffer = UniformBuffer::create(mpProgram->getActiveVersion().get(), "PerFrameCB");
    }
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
    mpDefaultFBO->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpScene)
    {
        mpRenderContext->setBlendState(nullptr);
        mpRenderContext->setDepthStencilState(nullptr, 0);
        setSceneLightsIntoUniformBuffer(mpScene.get(), mpLightBuffer.get());
        mpRenderContext->setUniformBuffer(0, mpLightBuffer);
        mpRenderer->update(mCurrentTime);
        mpRenderer->renderScene(mpRenderContext.get(), mpProgram.get());
    }

    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void SceneEditorSample::onShutdown()
{
    reset();
}

bool SceneEditorSample::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mpRenderer ? mpRenderer->onKeyEvent(keyEvent) : false;
}

bool SceneEditorSample::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mpRenderer ? mpRenderer->onMouseEvent(mouseEvent) : false;
}

void SceneEditorSample::onDataReload()
{

}

void SceneEditorSample::onResizeSwapChain()
{
    RenderContext::Viewport vp;
    vp.height = (float)mpDefaultFBO->getHeight();
    vp.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, vp);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    SceneEditorSample sceneEditor;
    SampleConfig config;
    config.windowDesc.title = "Scene Editor";
    config.freezeTimeOnStartup = true;
    sceneEditor.run(config);
}
