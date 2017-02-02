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
#include "postprocess.h"

using namespace Falcor;

void PostProcess::onLoad()
{
    // Create models and texture
    mpSphere = Model::createFromFile("sphere.obj", 0);
    mpTeapot = Model::createFromFile("teapot.obj", 0);

    // Create camera
    mpCamera = Camera::create();
    float nearZ = 0.1f;
    float farZ = mpSphere->getRadius() * 5000;
    mpCamera->setDepthRange(nearZ, farZ);

    // Setup controller
    mCameraController.attachCamera(mpCamera);
    mCameraController.setModelParams(mpTeapot->getCenter(), mpTeapot->getRadius(), 10.0f);

    mSkybox.pProgram = Program::createFromFile("postprocess.vs", "postprocess.fs");
    mSkybox.pProgram->addDefine("_TEXTURE_ONLY");
    mpPerFrameCB = UniformBuffer::create(mSkybox.pProgram, "PerFrameCB");
    mpEnvMapProgram = Program::createFromFile("postprocess.vs", "postprocess.fs");

    // Create the rasterizer state
    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Front);
    mSkybox.pFrontFaceCulling = RasterizerState::create(rsDesc);

    // Create the sampler state
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpTriLinearSampler = Sampler::create(samplerDesc);

    mpToneMapper = ToneMapping::create(ToneMapping::Operator::HableUc2);

    initUI();
    loadImage();
}

void PostProcess::loadImage()
{
    std::string filename;
    switch(mHdrImageIndex)
    {
    case HdrImage::AtTheWindow:
        filename = "LightProbes\\20060807_wells6_hd.hdr";
        break;
    case HdrImage::EveningSun:
        filename = "LightProbes\\hallstatt4_hd.hdr";
        break;
    case HdrImage::OvercastDay:
        filename = "LightProbes\\20050806-03_hd.hdr";
        break;
    }

    mHdrImage = createTextureFromFile(filename, true, false);
}

void PostProcess::initUI()
{
    Gui::DropdownList imageList;
    imageList.push_back({HdrImage::EveningSun, "Evening Sun"});
    imageList.push_back({HdrImage::AtTheWindow, "Window"});
    imageList.push_back({HdrImage::OvercastDay, "Overcast Day"});
    mpGui->addDropdownWithCallback("HDR Image", imageList, setHdrImage, getHdrImage, this);
    mpGui->addFloatVar("Surface Roughness", &mSurfaceRoughness, "", 0.01f, 1000, 0.01f);
    mpGui->addFloatVar("Light Intensity", &mLightIntensity, "", 0.5f, FLT_MAX, 0.1f);
    mpToneMapper->setUiElements(mpGui.get(), "HDR");
}

void GUI_CALL PostProcess::getHdrImage(void* pVal, void* pThis)
{
    PostProcess* pPP = (PostProcess*)pThis;
    *(uint32_t*)pVal = (uint32_t)pPP->mHdrImageIndex;
}

void GUI_CALL PostProcess::setHdrImage(const void* pVal, void* pThis)
{
    PostProcess* pPP = (PostProcess*)pThis;
    pPP->mHdrImageIndex = (HdrImage)*(uint32_t*)pVal;
    pPP->loadImage();
}

void PostProcess::renderMesh(const Mesh* pMesh, const Program* pProgram, RasterizerState::SharedPtr pRastState, float scale)
{
    // Update uniform-buffers data
    glm::mat4 world = glm::scale(glm::mat4(), glm::vec3(scale));
    glm::mat4 wvp = mpCamera->getProjMatrix() * mpCamera->getViewMatrix() * world;
    mpPerFrameCB->setVariable("gWorldMat", world);
    mpPerFrameCB->setVariable("gWvpMat", wvp);
    mpPerFrameCB->setVariable("gEyePosW", mpCamera->getPosition());
    mpPerFrameCB->setTexture("gEnvMap", mHdrImage.get(), mpTriLinearSampler.get());
    mpPerFrameCB->setVariable("gLightIntensity", mLightIntensity);
    mpPerFrameCB->setVariable("gSurfaceRoughness", mSurfaceRoughness);

    // Set uniform buffers
    mpRenderContext->setProgram(pProgram->getActiveVersion());

    // Just for the sake of the example, we fetch the buffer location from the program here. We could have cached it, or better yet, just use "layout(binding = <someindex>" in the shader
    mpRenderContext->setUniformBuffer(0, mpPerFrameCB);
    mpRenderContext->setRasterizerState(pRastState);

    mpRenderContext->setVao(pMesh->getVao());
    mpRenderContext->setTopology(RenderContext::Topology::TriangleList);
    mpRenderContext->drawIndexed(pMesh->getIndexCount(), 0, 0);
}

void PostProcess::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpHdrFbo->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

    mpRenderContext->pushFbo(mpHdrFbo);
    mpRenderContext->setDepthStencilState(nullptr, 0);
    mpRenderContext->setRasterizerState(nullptr);

    mCameraController.update();

    renderMesh(mpSphere->getMesh(0).get(), mSkybox.pProgram.get(), mSkybox.pFrontFaceCulling, 4500);
    renderMesh(mpTeapot->getMesh(0).get(), mpEnvMapProgram.get(), nullptr, 1);

    mpRenderContext->popFbo();

    mpToneMapper->execute(mpRenderContext.get(), mpHdrFbo, mpDefaultFBO);

    std::string Txt = getGlobalSampleMessage(true) + '\n';
    renderText(Txt, glm::vec2(10, 10));
}

void PostProcess::onShutdown()
{

}

void PostProcess::onResizeSwapChain()
{
    RenderContext::Viewport VP;
    VP.height = (float)mpDefaultFBO->getHeight();
    VP.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, VP);

    mpCamera->setFovY(float(M_PI / 8));
    mpCamera->setAspectRatio(VP.width / VP.height);

    ResourceFormat format = ResourceFormat::RGBA16Float;
    mpHdrFbo = FboHelper::create2DWithDepth(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), &format, ResourceFormat::D16Unorm);
}

bool PostProcess::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mCameraController.onKeyEvent(keyEvent);
}

bool PostProcess::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mCameraController.onMouseEvent(mouseEvent);
}

static void GUI_CALL getHdrImage(void* pVal, void* pThis);
static void GUI_CALL setHdrImage(const void* pVal, void* pThis);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    PostProcess postProcessSample;
    SampleConfig config;
    config.windowDesc.title = "Post Processing";
    postProcessSample.run(config);
}
