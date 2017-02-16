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

const Gui::DropdownList PostProcess::kImageList = { { HdrImage::EveningSun, "Evening Sun" },
{ HdrImage::AtTheWindow, "Window" }, { HdrImage::OvercastDay, "Overcast Day" } };

void PostProcess::onLoad()
{
    //Create model and camera
    mpTeapot = Model::createFromFile("teapot.obj", 0u);
    mpCamera = Camera::create();
    float nearZ = 0.1f;
    float farZ = mpTeapot->getRadius() * 5000;
    mpCamera->setDepthRange(nearZ, farZ);

    //Setup controller
    mCameraController.attachCamera(mpCamera);
    mCameraController.setModelParams(mpTeapot->getCenter(), mpTeapot->getRadius(), 10.0f);    
    
    //Program
    mpProgram = GraphicsProgram::createFromFile("PostProcess.vs.hlsl", "Postprocess.ps.hlsl");
    mpProgramVars = GraphicsVars::create(mpProgram->getActiveVersion()->getReflector());
    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setFbo(mpDefaultFBO);

    //Sampler
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    Sampler::SharedPtr triLinear = Sampler::create(samplerDesc);
    mpProgramVars->setSampler(0, triLinear);

    mpToneMapper = ToneMapping::create(ToneMapping::Operator::HableUc2);

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

    mHdrImage = createTextureFromFile(filename, false, false, Resource::BindFlags::ShaderResource);
}

void PostProcess::onGuiRender()
{
    uint32_t uHdrIndex = static_cast<uint32_t>(mHdrImageIndex);
    if (mpGui->addDropdown("HdrImage", kImageList, uHdrIndex))
    {
        mHdrImageIndex = static_cast<HdrImage>(uHdrIndex);
        loadImage();
    }
    mpGui->addFloatVar("Surface Roughness", mSurfaceRoughness, 0.01f, 1000, 0.01f);
    mpGui->addFloatVar("Light Intensity", mLightIntensity, 0.5f, FLT_MAX, 0.1f);
    mpToneMapper->setUiElements(mpGui.get(), "HDR");
}

void PostProcess::renderMesh(const Mesh* pMesh, GraphicsProgram::SharedPtr pProgram, RasterizerState::SharedPtr pRastState, float scale)
{
    //Update vars
    glm::mat4 world = glm::scale(glm::mat4(), glm::vec3(scale));
    glm::mat4 wvp = mpCamera->getProjMatrix() * mpCamera->getViewMatrix() * world;
    ConstantBuffer::SharedPtr pPerFrameCB = mpProgramVars["PerFrameCB"];
    pPerFrameCB["gWorldMat"] = world;
    pPerFrameCB["gWvpMat"] = wvp;
    pPerFrameCB["gEyePosW"] = mpCamera->getPosition();
    pPerFrameCB["gLightIntensity"] = mLightIntensity;
    pPerFrameCB["gSurfaceRoughness"] = mSurfaceRoughness;
    mpProgramVars->setTexture("gEnvMap", mHdrImage);

    //Set Gfx state
    mpGraphicsState->setVao(pMesh->getVao());
    mpGraphicsState->setRasterizerState(pRastState);
    mpGraphicsState->setProgram(mpProgram);
    mpRenderContext->setGraphicsState(mpGraphicsState);
    mpRenderContext->setGraphicsVars(mpProgramVars);
    mpRenderContext->drawIndexed(pMesh->getIndexCount(), 0, 0);
}

void PostProcess::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mpHdrFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mCameraController.update();

    //Render teapot to hdr fbo
    mpGraphicsState->pushFbo(mpHdrFbo);
    renderMesh(mpTeapot->getMesh(0).get(), mpProgram, nullptr, 1);
    mpGraphicsState->popFbo();

    //Run tone mapping
    mpToneMapper->execute(mpRenderContext.get(), mpHdrFbo, mpDefaultFBO);

    std::string Txt = getFpsMsg() + '\n';
    renderText(Txt, glm::vec2(10, 10));
}

void PostProcess::onShutdown()
{

}

void PostProcess::onResizeSwapChain()
{
    //Camera aspect 
    float height = (float)mpDefaultFBO->getHeight();
    float width = (float)mpDefaultFBO->getWidth();
    mpCamera->setFovY(float(M_PI / 3));
    float aspectRatio = (width / height);
    mpCamera->setAspectRatio(aspectRatio);

    //recreate hdr fbo
    ResourceFormat format = ResourceFormat::RGBA32Float;
    Fbo::Desc desc;
    desc.setDepthStencilTarget(ResourceFormat::D16Unorm);
    desc.setColorTarget(0u, format);
    mpHdrFbo = FboHelper::create2D(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), desc);
}

bool PostProcess::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mCameraController.onKeyEvent(keyEvent);
}

bool PostProcess::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mCameraController.onMouseEvent(mouseEvent);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    PostProcess postProcessSample;
    SampleConfig config;
    config.windowDesc.title = "Post Processing";
    postProcessSample.run(config);
}
