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
#include "MaterialEditorSample.h"

void GUI_CALL MaterialEditorSample::getActiveModel(void* pVal, void* pUserData)
{
    *(MaterialEditorSample::ModelType*)pVal = ((MaterialEditorSample*)pUserData)->mActiveModel;
}

void GUI_CALL MaterialEditorSample::setActiveModel(const void* pVal, void* pUserData)
{
    MaterialEditorSample* pEditor = (MaterialEditorSample*)pUserData;
    pEditor->mActiveModel = *(MaterialEditorSample::ModelType*)pVal;
    pEditor->loadModel();
}

void GUI_CALL MaterialEditorSample::loadMaterialCB(void* pUserData)
{
    MaterialEditorSample* pEditor = (MaterialEditorSample*)pUserData;
    pEditor->loadMaterial();
}

void GUI_CALL MaterialEditorSample::resetMaterialCB(void* pUserData)
{
    MaterialEditorSample* pEditor = (MaterialEditorSample*)pUserData;
    pEditor->resetMaterial();
}

void MaterialEditorSample::loadMaterial()
{
    std::string Filename;
    if(openFileDialog("Scene files\0*.fscene\0\0", Filename))
    {
        bool bSrgb = isSrgbFormat(mpDefaultFBO->getColorTexture(0)->getFormat());
        uint32_t flags = bSrgb ? 0 : Model::AssumeLinearSpaceTextures;
        auto pScene = Scene::loadFromFile(Filename, flags);
        if(pScene)
        {
            if(pScene->getMaterialCount() == 0)
            {
                msgBox("Scene doesn't contain materials");
                return;
            }

            if(pScene->getMaterialCount() > 1)
            {
                msgBox("Scene contains more than a single material. Loading the first material");
            }
            mpMaterial = pScene->getMaterial(0);
            createEditor();
        }
    }
}

void MaterialEditorSample::resetMaterial()
{
    mpMaterial = Material::create("");
    createEditor();
}

void MaterialEditorSample::createEditor()
{
    mpEditor = nullptr;
    bool bSrgb = isSrgbFormat(mpDefaultFBO->getColorTexture(0)->getFormat());
    mpEditor = MaterialEditor::create(mpMaterial, bSrgb);
}

void MaterialEditorSample::loadModel()
{
    std::string filename;
    switch(mActiveModel)
    {
    case ModelType::Plane:
        filename = "billboard.obj";
        break;
    case ModelType::Box:
        filename = "box.obj";
        break;
    case ModelType::Sphere:
        filename = "Sphere.obj";
        break;
    case ModelType::Torus:
        filename = "torus.obj";
        break;
    default:
        should_not_get_here();
    }

    const auto Flags = Model::GenerateTangentSpace;
    mpModel = Model::createFromFile(filename, Flags);

    resetCamera();

    float Radius = mpModel->getRadius();
    float LightHeight = Falcor::max(1.0f + Radius, Radius*1.25f);
    mpPointLight->setWorldPosition(glm::vec3(0, LightHeight, 0));
}

void MaterialEditorSample::resetCamera()
{
    if(mpModel)
    {
        // update the camera position
        float radius = mpModel->getRadius();
        const glm::vec3& modelCenter = mpModel->getCenter();
        glm::vec3 camPos = modelCenter;
        camPos.z += radius * 7;

        mpCamera->setPosition(camPos);
        mpCamera->setTarget(modelCenter);
        mpCamera->setUpVector(glm::vec3(0, 1, 0));

        // Update the controllers
        mCamControl.setModelParams(modelCenter, radius, 7);

        mNearZ = std::max(0.1f, mpModel->getRadius() / 750.0f);
        mFarZ = radius * 20;
    }
}

void MaterialEditorSample::initUI()
{
    Gui::setGlobalHelpMessage("Material editor");

    mpGui->addButton("Load Material", &MaterialEditorSample::loadMaterialCB, this);
    mpGui->addSeparator();
    mpGui->addButton("Reset Material", &MaterialEditorSample::resetMaterialCB, this);
    mpGui->addSeparator();
    Gui::dropdown_list modelList;
    modelList.push_back({(uint32_t)ModelType::Plane, "Plane"});
    modelList.push_back({(uint32_t)ModelType::Box, "Box"});
    modelList.push_back({(uint32_t)ModelType::Sphere, "Sphere"});
    modelList.push_back({(uint32_t)ModelType::Torus, "Torus"});

    mpGui->addDropdownWithCallback("Select Model", modelList, &MaterialEditorSample::setActiveModel, &MaterialEditorSample::getActiveModel, this);

    const std::string LightGroup = "Lights";
    mpGui->addRgbColor("Ambient intensity", &mAmbientIntensity, LightGroup);
    mpDirLight->setUiElements(mpGui.get(), "Directional Light");
    mpGui->nestGroups(LightGroup, "Directional Light");
    mpPointLight->setUiElements(mpGui.get(), "Point Light");
    mpGui->nestGroups(LightGroup, "Point Light");

    mpGui->addCheckBox("Anisotropic Filtering", &mUseAnisoFiltering);

    uint32_t barSize[2];
    mpGui->getSize(barSize);
    barSize[0] += 50;
    barSize[1] += 100;
    mpGui->setSize(barSize[0], barSize[1]);
}

void MaterialEditorSample::onLoad()
{
    // Camera controller
    mpCamera = Camera::create();
    mCamControl.attachCamera(mpCamera);

    mpProgram = Program::createFromFile("", "MaterialEditor.fs");

    // Samplers
    Sampler::Desc SamplerDesc;
    SamplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(SamplerDesc);
    SamplerDesc.setMaxAnisotropy(8);
    m_pAnisoSampler = Sampler::create(SamplerDesc);

    // Lights
    mpDirLight = DirectionalLight::create();
    mpPointLight = PointLight::create();
    mpDirLight->setWorldDirection(glm::vec3(0.13f, 0.27f, -0.9f));

    // Uniform buffer
    mpPerFrameCB = UniformBuffer::create(mpProgram->getActiveProgramVersion().get(), "PerFrameCB");

    // Load the model
    loadModel();

    // create the editor
    mpMaterial = Material::create("New Material");
    createEditor();
    initUI();
}

void MaterialEditorSample::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpDefaultFBO->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

    if(mpModel)
    {
        mpCamera->setDepthRange(mNearZ, mFarZ);
        mCamControl.update();

        mpRenderContext->setRasterizerState(nullptr);
        mpRenderContext->setDepthStencilState(nullptr, 0);
        mpDirLight->setIntoUniformBuffer(mpPerFrameCB.get(), "gDirLight");
        mpPointLight->setIntoUniformBuffer(mpPerFrameCB.get(), "gPointLight");

        mpMaterial->overrideAllSamplers(mUseAnisoFiltering ? m_pAnisoSampler : mpLinearSampler);
        mpMaterial->setIntoUniformBuffer(mpPerFrameCB.get(), "gPrivateMaterial");

        mpPerFrameCB->setVariable("gAmbient", mAmbientIntensity);
        mpRenderContext->setUniformBuffer(0, mpPerFrameCB);
        ModelRenderer::render(mpRenderContext.get(), mpProgram.get(), mpModel, mpCamera.get());
    }

    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void MaterialEditorSample::onShutdown()
{
}

bool MaterialEditorSample::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mCamControl.onKeyEvent(keyEvent);
}

bool MaterialEditorSample::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mCamControl.onMouseEvent(mouseEvent);
}

void MaterialEditorSample::onResizeSwapChain()
{
    RenderContext::Viewport vp;
    vp.height = (float)mpDefaultFBO->getHeight();
    vp.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, vp);
    mpCamera->setFovY(float(M_PI / 8));
    mpCamera->setAspectRatio(vp.width / vp.height);
    mpCamera->setDepthRange(0, 1000);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    MaterialEditorSample materialEditor;
    SampleConfig Config;
    Config.windowDesc.title = "Material Editor";
    materialEditor.run(Config);
}
