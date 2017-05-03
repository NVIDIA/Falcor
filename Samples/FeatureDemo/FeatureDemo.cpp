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
#include "FeatureDemo.h"
#include "API/D3D/FalcorD3D.h"

void FeatureDemo::initLightingPass()
{
    mLightingPass.pProgram = GraphicsProgram::createFromFile("FeatureDemo.vs.hlsl", "FeatureDemo.ps.hlsl");
    mLightingPass.pProgram->addDefine("_LIGHT_COUNT", std::to_string(mpSceneRenderer->getScene()->getLightCount()));
    initControls();
    mLightingPass.pVars = GraphicsVars::create(mLightingPass.pProgram->getActiveVersion()->getReflector());
}

void FeatureDemo::initShadowPass()
{
    mShadowPass.pCsm = CascadedShadowMaps::create(2048, 2048, mpSceneRenderer->getScene()->getLight(0), mpSceneRenderer->getScene()->shared_from_this(), 4);
    mShadowPass.pCsm->setFilterMode(CsmFilterEvsm2);
    mShadowPass.pCsm->setVsmLightBleedReduction(0.3f);
}

void FeatureDemo::initSSAO()
{
    mSSAO.pSSAO = SSAO::create(uvec2(1024));
    mSSAO.pApplySSAOPass = FullScreenPass::create("ApplyAO.ps.hlsl");
    mSSAO.pVars = GraphicsVars::create(mSSAO.pApplySSAOPass->getProgram()->getActiveVersion()->getReflector());

    Sampler::Desc desc;
    desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mSSAO.pVars->setSampler("gSampler", Sampler::create(desc));
}

void FeatureDemo::initScene(Scene::SharedPtr pScene)
{
    if (pScene->getCameraCount() == 0)
    {
        // Place the camera above the center, looking slightly downwards
        const Model* pModel = pScene->getModel(0).get();
        Camera::SharedPtr pCamera = Camera::create();

        vec3 position = pModel->getCenter();
        float radius = pModel->getRadius();
        position.y += 0.1f * radius;
        pScene->setCameraSpeed(radius * 0.03f);

        pCamera->setPosition(position);
        pCamera->setTarget(position + vec3(0, -0.3f, -radius));
        pCamera->setDepthRange(0.1f, radius * 10);

        pScene->addCamera(pCamera);
    }

    if (pScene->getLightCount() == 0)
    {
        // Create a directional light
        DirectionalLight::SharedPtr pDirLight = DirectionalLight::create();
        pDirLight->setWorldDirection(vec3(-0.189f, -0.861f, -0.471f));
        pDirLight->setIntensity(vec3(1, 1, 0.985f) * 10.0f);
        pDirLight->setName("DirLight");
        pScene->addLight(pDirLight);
        pScene->setAmbientIntensity(vec3(0.1f));
    }

    mpSceneRenderer = SceneRenderer::create(pScene);
    mpSceneRenderer->setCameraControllerType(SceneRenderer::CameraControllerType::FirstPerson);
    setActiveCameraAspectRatio();
    initLightingPass();
    initShadowPass();
    initSSAO();
}

void FeatureDemo::loadModel(const std::string& filename)
{
    ProgressBar::SharedPtr pBar = ProgressBar::create("Loading Model");
    Model::SharedPtr pModel = Model::createFromFile(filename.c_str());
    if (!pModel) return;
    pModel->bindSamplerToMaterials(mSkyBox.pEffect->getSampler());
    Scene::SharedPtr pScene = Scene::create();
    pScene->addModelInstance(pModel, "instance");

    initScene(pScene);
}

void FeatureDemo::loadScene(const std::string& filename)
{
    ProgressBar::SharedPtr pBar = ProgressBar::create("Loading Scene", 100);
    Scene::SharedPtr pScene = Scene::loadFromFile(filename);

    if (pScene != nullptr)
    {
        initScene(pScene);
    }
}

void FeatureDemo::initSkyBox()
{
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mSkyBox.pSampler = Sampler::create(samplerDesc);
    mSkyBox.pEffect = SkyBox::createFromTexture("LightProbes\\10-Shiodome_Stairs_3k.dds", true, mSkyBox.pSampler);
    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthFunc(DepthStencilState::Func::Always);
    mSkyBox.pDS = DepthStencilState::create(dsDesc);
}

void FeatureDemo::initPostProcess()
{
    mpToneMapper = ToneMapping::create(ToneMapping::Operator::HableUc2);
}

void FeatureDemo::onLoad()
{
    mpState = GraphicsState::create();

    initSkyBox();
    initPostProcess();
    init_tests();
}

void FeatureDemo::renderSkyBox()
{
    mpState->setDepthStencilState(mSkyBox.pDS);
    mSkyBox.pEffect->render(mpRenderContext.get(), mpSceneRenderer->getScene()->getActiveCamera().get());
    mpState->setDepthStencilState(nullptr);
}

void FeatureDemo::beginFrame()
{
    mpRenderContext->pushGraphicsState(mpState);
    mpState->setFbo(mpMainFbo);
    mpRenderContext->clearFbo(mpMainFbo.get(), glm::vec4(), 1, 0, FboAttachmentType::All);
    mpRenderContext->clearFbo(mpPostProcessFbo.get(), glm::vec4(), 1, 0, FboAttachmentType::Color);
}

void FeatureDemo::endFrame()
{
    mpRenderContext->popGraphicsState();
}

void FeatureDemo::postProcess()
{
    mpToneMapper->execute(mpRenderContext.get(), mpResolveFbo, mControls[EnableSSAO].enabled ? mpPostProcessFbo : mpDefaultFBO);
}

void FeatureDemo::lightingPass()
{
    mpState->setProgram(mLightingPass.pProgram);
    mpRenderContext->setGraphicsVars(mLightingPass.pVars);
    ConstantBuffer::SharedPtr pCB = mLightingPass.pVars->getConstantBuffer("PerFrameCB");
    if(mControls[ControlID::EnableShadows].enabled)
    {
        pCB["camVpAtLastCsmUpdate"] = mShadowPass.camVpAtLastCsmUpdate;
        mShadowPass.pCsm->setDataIntoGraphicsVars(mLightingPass.pVars, "gCsmData");
    }
    if (mControls[EnableReflections].enabled)
    {
        mLightingPass.pVars->setTexture("gEnvMap", mSkyBox.pEffect->getTexture());
        mLightingPass.pVars->setSampler("gSampler", mSkyBox.pEffect->getSampler());
    }
    mpSceneRenderer->renderScene(mpRenderContext.get());
}

void FeatureDemo::resolveMSAA()
{
    mpRenderContext->blit(mpMainFbo->getColorTexture(0)->getSRV(), mpResolveFbo->getRenderTargetView(0));
    mpRenderContext->blit(mpMainFbo->getColorTexture(1)->getSRV(), mpResolveFbo->getRenderTargetView(1));
    mpRenderContext->blit(mpMainFbo->getDepthStencilTexture()->getSRV(), mpResolveFbo->getRenderTargetView(2));
}

void FeatureDemo::shadowPass()
{
    if (mControls[EnableShadows].enabled && mShadowPass.updateShadowMap)
    {
        mShadowPass.camVpAtLastCsmUpdate = mpSceneRenderer->getScene()->getActiveCamera()->getViewProjMatrix();
        mShadowPass.pCsm->setup(mpRenderContext.get(), mpSceneRenderer->getScene()->getActiveCamera().get(), nullptr);
    }
}

void FeatureDemo::ambientOcclusion()
{
    if (mControls[EnableSSAO].enabled)
    {
        Texture::SharedPtr pAOMap = mSSAO.pSSAO->generateAOMap(mpRenderContext.get(), mpSceneRenderer->getScene()->getActiveCamera().get(), mpResolveFbo->getColorTexture(2), mpResolveFbo->getColorTexture(1));
        mSSAO.pVars->setTexture("gColor", mpPostProcessFbo->getColorTexture(0));
        mSSAO.pVars->setTexture("gAOMap", pAOMap);

        mpRenderContext->getGraphicsState()->setFbo(mpDefaultFBO);
        mpRenderContext->setGraphicsVars(mSSAO.pVars);

        mSSAO.pApplySSAOPass->execute(mpRenderContext.get());
    }
}

void FeatureDemo::onFrameRender()
{
    if(mpSceneRenderer)
    {
        beginFrame();

        mpSceneRenderer->update(mCurrentTime);
        shadowPass();
        renderSkyBox();
        lightingPass();
        resolveMSAA();
        postProcess();
        ambientOcclusion();
        endFrame();
    }
    else
    {
        mpRenderContext->clearFbo(mpDefaultFBO.get(), vec4(0.2f, 0.4f, 0.5f, 1), 1, 0);
    }

    run_test();
}

void FeatureDemo::onShutdown()
{

}

bool FeatureDemo::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mpSceneRenderer ? mpSceneRenderer->onKeyEvent(keyEvent) : false;
}

bool FeatureDemo::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mpSceneRenderer ? mpSceneRenderer->onMouseEvent(mouseEvent) : true;
}

void FeatureDemo::onResizeSwapChain()
{
    // Create the main FBO
    uint32_t w = mpDefaultFBO->getWidth();
    uint32_t h = mpDefaultFBO->getHeight();

    Fbo::Desc fboDesc;
    fboDesc.setColorTarget(0, ResourceFormat::RGBA8UnormSrgb);
    mpPostProcessFbo = FboHelper::create2D(w, h, fboDesc);

    fboDesc.setColorTarget(0, ResourceFormat::RGBA32Float).setColorTarget(1, ResourceFormat::RGBA8Unorm).setColorTarget(2, ResourceFormat::R32Float);
    mpResolveFbo = FboHelper::create2D(w, h, fboDesc);

    fboDesc.setSampleCount(mSampleCount).setColorTarget(2, ResourceFormat::Unknown).setDepthStencilTarget(ResourceFormat::D32Float);
    mpMainFbo = FboHelper::create2D(w, h, fboDesc);

    if(mpSceneRenderer)
    {
        setActiveCameraAspectRatio();
    }
}

void FeatureDemo::setActiveCameraAspectRatio()
{
    uint32_t w = mpDefaultFBO->getWidth();
    uint32_t h = mpDefaultFBO->getHeight();
    mpSceneRenderer->getScene()->getActiveCamera()->setAspectRatio((float)w / (float)h);
}

void FeatureDemo::onInitializeTestingArgs(const ArgList& args)
{
    mUniformDt = args.argExists("uniformdt");

    std::vector<ArgList::Arg> model = args.getValues("loadmodel");
    if (!model.empty())
    {
        loadModel(model[0].asString());
    }

    std::vector<ArgList::Arg> scene = args.getValues("loadscene");
    if (!scene.empty())
    {
        loadScene(scene[0].asString());
    }

    std::vector<ArgList::Arg> cameraPos = args.getValues("camerapos");
    if (!cameraPos.empty())
    {
        mpSceneRenderer->getScene()->getActiveCamera()->setPosition(glm::vec3(cameraPos[0].asFloat(), cameraPos[1].asFloat(), cameraPos[2].asFloat()));
    }

    std::vector<ArgList::Arg> cameraTarget = args.getValues("cameratarget");
    if (!cameraTarget.empty())
    {
        mpSceneRenderer->getScene()->getActiveCamera()->setTarget(glm::vec3(cameraTarget[0].asFloat(), cameraTarget[1].asFloat(), cameraTarget[2].asFloat()));
    }
}

void FeatureDemo::onRunTestTask(const FrameRate&)
{
    if (mUniformDt)
    {
        mUniformGlobalTime += 0.016f;
        mCurrentTime = mUniformGlobalTime;
    }
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    FeatureDemo sample;
    SampleConfig config;
    config.windowDesc.title = "Falcor Feature Demo";
    config.windowDesc.resizableWindow = false;
    sample.run(config);
}
