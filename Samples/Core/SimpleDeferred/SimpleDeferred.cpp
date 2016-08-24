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
#include "SimpleDeferred.h"

SimpleDeferred::~SimpleDeferred()
{
}

void GUI_CALL SimpleDeferred::loadModelCallback(void* pUserData)
{
    SimpleDeferred* pViewer = reinterpret_cast<SimpleDeferred*>(pUserData);
    pViewer->loadModel();
}

CameraController& SimpleDeferred::getActiveCameraController()
{
    switch(mCameraType)
    {
    case SimpleDeferred::ModelViewCamera:
        return mModelViewCameraController;
    case SimpleDeferred::FirstPersonCamera:
        return mFirstPersonCameraController;
    default:
        should_not_get_here();
        return mFirstPersonCameraController;
    }
}

void SimpleDeferred::loadModelFromFile(const std::string& filename)
{
    uint32_t flags = mCompressTextures ? Model::CompressTextures : 0;
    flags |= mGenerateTangentSpace ? Model::GenerateTangentSpace : 0;
    auto fboFormat = mpDefaultFBO->getColorTexture(0)->getFormat();
    flags |= isSrgbFormat(fboFormat) ? 0 : Model::AssumeLinearSpaceTextures;

    mpModel = Model::createFromFile(filename, flags);

    if(mpModel == nullptr)
    {
        msgBox("Could not load model");
        return;
    }
    setModelUIElements();
    resetCamera();

    float Radius = mpModel->getRadius();
    mpPointLight->setWorldPosition(glm::vec3(0, Radius*1.25f, 0));
}

void SimpleDeferred::loadModel()
{
    std::string filename;
    if(openFileDialog("Supported Formats\0*.obj;*.bin;*.dae;*.x;*.md5mesh\0\0", filename))
    {
        loadModelFromFile(filename);
    }
}

void SimpleDeferred::initUI()
{
    Gui::setGlobalHelpMessage("Sample application to load and display a model.\nUse the UI to switch between wireframe and solid mode.");
    // Load model group
    mpGui->addButton("Load Model", &SimpleDeferred::loadModelCallback, this);
    const std::string LoadOptions = "Load Options";
    mpGui->addCheckBox("Compress Textures", &mCompressTextures, LoadOptions);
    mpGui->addCheckBox("Generate Tangent Space", &mGenerateTangentSpace, LoadOptions);

    Gui::dropdown_list debugModeList;
    debugModeList.push_back({ 0, "Disabled" });
    debugModeList.push_back({ 1, "Positions" });
    debugModeList.push_back({ 2, "Normals" });
    debugModeList.push_back({ 3, "Albedo" });
    debugModeList.push_back({ 4, "Illumination" });
    mpGui->addDropdown("Debug mode", debugModeList, &mDebugMode);

    Gui::dropdown_list cullList;
    cullList.push_back({0, "No Culling"});
    cullList.push_back({1, "Backface Culling"});
    cullList.push_back({2, "Frontface Culling"});
    mpGui->addDropdown("Cull Mode", cullList, &mCullMode);

    const std::string lightGroup = "Lights";
    mpGui->addRgbColor("Ambient intensity", &mAmbientIntensity, lightGroup);
    mpDirLight->setUiElements(mpGui.get(), "Directional Light");
    mpGui->nestGroups(lightGroup, "Directional Light");
    mpPointLight->setUiElements(mpGui.get(), "Point Light");
    mpGui->nestGroups(lightGroup, "Point Light");

    Gui::dropdown_list cameraList;
    cameraList.push_back({ModelViewCamera, "Model-View"});
    cameraList.push_back({FirstPersonCamera, "First-Person"});
    mpGui->addDropdown("Camera Type", cameraList, &mCameraType);


    uint32_t barSize[2];
    mpGui->getSize(barSize);
    barSize[0] += 50;
    barSize[1] += 100;
    mpGui->setSize(barSize[0], barSize[1]);
}

void SimpleDeferred::setModelUIElements()
{
    bool bAnim = mpModel && mpModel->hasAnimations();
    static const std::string animateStr = "Animate";
    static const std::string activeAnimStr = "Active Animation";

    if(bAnim)
    {
        mActiveAnimationID = sBindPoseAnimationID;

        mpGui->addCheckBox(animateStr, &mAnimate);
        Gui::dropdown_list list;
        list.resize(mpModel->getAnimationsCount() + 1);
        list[0].label = "Bind Pose";
        list[0].value = sBindPoseAnimationID;

        for(uint32_t i = 0; i < mpModel->getAnimationsCount(); i++)
        {
            list[i + 1].value = i;
            list[i + 1].label = mpModel->getAnimationName(i);
            if(list[i + 1].label.size() == 0)
            {
                list[i + 1].label = std::to_string(i);
            }
        }
        mpGui->addDropdownWithCallback(activeAnimStr, list, setActiveAnimationCB, getActiveAnimationCB, this);
    }
    else
    {
        mpGui->removeVar(animateStr);
        mpGui->removeVar(activeAnimStr);
    }

    mpGui->removeVar("Near Plane", "Depth Range");
    mpGui->removeVar("Far Plane", "Depth Range");
    const float MinDepth = mpModel->getRadius() * 1 / 1000;
    mpGui->addFloatVar("Near Plane", &mNearZ, "Depth Range", MinDepth, mpModel->getRadius() * 15, MinDepth * 5);
    mpGui->addFloatVar("Far Plane", &mFarZ, "Depth Range", MinDepth, mpModel->getRadius() * 15, MinDepth * 5);
}

void SimpleDeferred::onLoad()
{
    mpCamera = Camera::create();

	mpDeferredPassProgram = Program::createFromFile("", "DeferredPass.fs");

    mpLightingPass = FullScreenPass::create("LightingPass.fs");

    // create rasterizer state
    RasterizerState::Desc rsDesc;
    mpCullRastState[0] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpCullRastState[1] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::Front);
    mpCullRastState[2] = RasterizerState::create(rsDesc);

    // Depth test
    DepthStencilState::Desc dsDesc;
	dsDesc.setDepthTest(false);
	mpNoDepthDS = DepthStencilState::create(dsDesc);
    dsDesc.setDepthTest(true);
    mpDepthTestDS = DepthStencilState::create(dsDesc);

    // Blend state
    BlendState::Desc blendDesc;
    mpOpaqueBS = BlendState::create(blendDesc);

    mModelViewCameraController.attachCamera(mpCamera);
    mFirstPersonCameraController.attachCamera(mpCamera);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
    mpLinearSampler = Sampler::create(samplerDesc);

    mpPointLight = PointLight::create();
    mpDirLight = DirectionalLight::create();
    mpDirLight->setWorldDirection(glm::vec3(-0.5f, -0.2f, -1.0f));

    mpDeferredPerFrameCB = UniformBuffer::create(mpDeferredPassProgram->getActiveProgramVersion().get(), "PerFrameCB");
    mpLightingFrameCB = UniformBuffer::create(mpLightingPass->getProgram()->getActiveProgramVersion().get(), "PerImageCB");

    // Load default model
    loadModelFromFile("Ogre/bs_rest.obj");

    initUI();
}

void SimpleDeferred::onFrameRender()
{
    uint32_t width = mpDefaultFBO->getWidth();
    uint32_t height = mpDefaultFBO->getHeight();

    Fbo::SharedPtr& pCurrentFbo = mpDisplayFBO ? mpDisplayFBO : mpDefaultFBO;
	if(mpDisplayFBO)
	{
		RenderContext::Viewport vp;
		vp.width = float(width / mDisplayScaling);
		vp.height = float(height / mDisplayScaling);
		mpRenderContext->setViewport(0, vp);
	}

    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);

    mpGBufferFbo->clear(glm::vec4(-1.f), 1.0f, 0, FboAttachmentType::Color | FboAttachmentType::Depth);

    // G-Buffer pass
    if(mpModel)
    {
        mpRenderContext->setFbo(mpGBufferFbo);

        mpCamera->setDepthRange(mNearZ, mFarZ);
        CameraController& ActiveController = getActiveCameraController();
        ActiveController.update();

        // Animate
        if(mAnimate)
        {
            PROFILE(Animate);
            mpModel->animate(mCurrentTime);
        }

        // Set render state
        mpRenderContext->setRasterizerState(mpCullRastState[mCullMode]);
        mpRenderContext->setDepthStencilState(mpDepthTestDS, 0);

        // Render model
        mpModel->bindSamplerToMaterials(mpLinearSampler);
        mpRenderContext->setUniformBuffer(0, mpDeferredPerFrameCB);
        ModelRenderer::render(mpRenderContext.get(), mpDeferredPassProgram.get(), mpModel, mpCamera.get());
    }

    // Lighting pass (fullscreen quad)
    {
		mpRenderContext->setFbo(pCurrentFbo);
		pCurrentFbo->clear(clearColor, 1.0f, 0, FboAttachmentType::Color);

        // Reset render state
        mpRenderContext->setRasterizerState(mpCullRastState[0]);
        mpRenderContext->setBlendState(mpOpaqueBS);
        mpRenderContext->setDepthStencilState(mpNoDepthDS, 0);

        // Set lighting params
        mpLightingFrameCB->setVariable("gAmbient", mAmbientIntensity);
        mpDirLight->setIntoUniformBuffer(mpLightingFrameCB.get(), "gDirLight");
        mpPointLight->setIntoUniformBuffer(mpLightingFrameCB.get(), "gPointLight");

        // Set GBuffer as input
        mpLightingFrameCB->setTexture("gGBuf0", mpGBufferFbo->getColorTexture(0).get(), nullptr);
        mpLightingFrameCB->setTexture("gGBuf1", mpGBufferFbo->getColorTexture(1).get(), nullptr);
        mpLightingFrameCB->setTexture("gGBuf2", mpGBufferFbo->getColorTexture(2).get(), nullptr);

        // Debug mode
        mpLightingFrameCB->setVariable("gDebugMode", (uint32_t)mDebugMode);

        // Kick it off
        mpRenderContext->setUniformBuffer(0, mpLightingFrameCB);
        mpLightingPass->execute(mpRenderContext.get());
    }

	// Display upscaling pass
	if(mpDisplayFBO)
	{
		assert(mDisplayScaling > 1);
		mpRenderContext->blitFbo(mpDisplayFBO.get(), mpDefaultFBO.get(),
									glm::ivec4(0, 0, width / mDisplayScaling, height / mDisplayScaling),
									glm::ivec4(0, 0, width, height));
		// Set the actual FBO
		mpRenderContext->setFbo(mpDefaultFBO);
		RenderContext::Viewport vp;
		vp.width = float(width);
		vp.height = float(height);
		mpRenderContext->setViewport(0, vp);
	}

    renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void SimpleDeferred::onShutdown()
{

}

bool SimpleDeferred::onKeyEvent(const KeyboardEvent& keyEvent)
{
    bool bHandled = getActiveCameraController().onKeyEvent(keyEvent);
    if(bHandled == false)
    {
        if(keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            switch(keyEvent.key)
            {
            case KeyboardEvent::Key::R:
                resetCamera();
                break;
            default:
                bHandled = false;
            }
        }
    }
    return bHandled;
}

bool SimpleDeferred::onMouseEvent(const MouseEvent& mouseEvent)
{
    return getActiveCameraController().onMouseEvent(mouseEvent);
}

void SimpleDeferred::onResizeSwapChain()
{
    uint32_t width = mpDefaultFBO->getWidth();
    uint32_t height = mpDefaultFBO->getHeight();

    mpCamera->setFovY(float(M_PI / 3));
    mAspectRatio = (float(width) / float(height));
    mpCamera->setAspectRatio(mAspectRatio);

	// Rule-of-thumb detection if we need to scale to high-dip display (>150dpi)
	mDisplayScaling = (getDisplayDpi() > 150) ? 2 : 1;

	// create a lower-res default FBO in case if we need to scale the results
	mpDisplayFBO = nullptr;
	if(mDisplayScaling > 1)
	{
		Falcor::ResourceFormat fbFormat[1] = { Falcor::ResourceFormat::RGBA8UnormSrgb};
        mpDisplayFBO = FboHelper::create2DWithDepth(width / mDisplayScaling, height / mDisplayScaling, fbFormat, Falcor::ResourceFormat::D24UnormS8);
	}
	RenderContext::Viewport vp;
	vp.width = float(width / mDisplayScaling);
	vp.height = float(height / mDisplayScaling);
	mpRenderContext->setViewport(0, vp);

    // create G-Buffer
    const glm::vec4 clearColor(0.f, 0.f, 0.f, 0.f);
    Falcor::ResourceFormat GBufferFBFormat[3] = { Falcor::ResourceFormat::RGBA16Float, Falcor::ResourceFormat::RGBA16Float, Falcor::ResourceFormat::RGBA16Float };
    mpGBufferFbo = FboHelper::create2DWithDepth(width / mDisplayScaling, height / mDisplayScaling, GBufferFBFormat, Falcor::ResourceFormat::D32Float, 1, 3);
    mpGBufferFbo->clear(clearColor, 1.0f, 0, FboAttachmentType::All);
}

void SimpleDeferred::resetCamera()
{
    if(mpModel)
    {
        // update the camera position
        float radius = mpModel->getRadius();
        const glm::vec3& modelCenter = mpModel->getCenter();
        glm::vec3 camPos = modelCenter;
        camPos.z += radius * 4;

        mpCamera->setPosition(camPos);
        mpCamera->setTarget(modelCenter);
        mpCamera->setUpVector(glm::vec3(0, 1, 0));

        // Update the controllers
        mModelViewCameraController.setModelParams(modelCenter, radius, 4);
        mFirstPersonCameraController.setCameraSpeed(radius*0.25f);
        mNearZ = std::max(0.1f, mpModel->getRadius() / 750.0f);
        mFarZ = radius * 10;
    }
}

void SimpleDeferred::getActiveAnimationCB(void* pVal, void* pUserData)
{
    SimpleDeferred* pViewer = (SimpleDeferred*)pUserData;
    *(uint32_t*)pVal = pViewer->mActiveAnimationID;
}

void SimpleDeferred::setActiveAnimationCB(const void* pVal, void* pUserData)
{
    SimpleDeferred* pViewer = (SimpleDeferred*)pUserData;
    pViewer->mActiveAnimationID = *(uint32_t*)pVal;
    pViewer->mpModel->setActiveAnimation(pViewer->mActiveAnimationID);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    SimpleDeferred sample;
    SampleConfig config;
    config.windowDesc.swapChainDesc.width = 1280;
    config.windowDesc.swapChainDesc.height = 720;
    config.windowDesc.resizableWindow = true;
    config.windowDesc.title = "Simple Deferred";
    sample.run(config);
}
