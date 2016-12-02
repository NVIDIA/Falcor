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
#include "ShaderBuffers.h"

void ShaderBuffersSample::onGuiRender()
{
     mpGui->addDirectionWidget("Light Direction", mLightData.worldDir);
     mpGui->addRgbColor("Light intensity", mLightData.intensity);
     mpGui->addRgbColor("Surface Color", mSurfaceColor);
     mpGui->addCheckBox("Count FS invocations", mCountPixelShaderInvocations);
}

Vao::SharedConstPtr ShaderBuffersSample::getVao()
{
    auto pMesh = mpModel->getMesh(0);
    auto pVao = pMesh->getVao();
    return pVao;
}

void ShaderBuffersSample::onLoad()
{
    mpCamera = Camera::create();

    // create the program
    mpProgram = Program::createFromFile("ShaderBuffers.vs", "ShaderBuffers.fs");

    // Load the model
    mpModel = Model::createFromFile("teapot.obj", 0);

    // Plane has only one mesh, get the VAO now
    mpVao = getVao();
    auto pMesh = mpModel->getMesh(0);
    mIndexCount = pMesh->getIndexCount();

    // Initialize uniform-buffers data
    mLightData.intensity = glm::vec3(1, 1, 1);
    mLightData.worldDir = glm::vec3(0, -1, 0);

    // Set camera parameters
    glm::vec3 center = mpModel->getCenter();
    float radius = mpModel->getRadius();

    float nearZ = 0.1f;
    float farZ = radius * 10;
    mpCamera->setDepthRange(nearZ, farZ);

    // Initialize the camera controller
    mCameraController.attachCamera(mpCamera);
    mCameraController.setModelParams(center, radius, radius * 10);

    // create the uniform buffers
    mpProgramVars = ProgramVars::create(mpProgram->getActiveVersion()->getReflector());
    uint32_t z = 0;
    mpInvocationsBuffer = Buffer::create(sizeof(uint32_t), Buffer::BindFlags::UnorderedAccess, Buffer::CpuAccess::Read, &z);
    mpProgramVars->attachBuffer("gInvocationBuffer", mpInvocationsBuffer);

    // create pipeline cache
    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpDefaultPipelineState->setRasterizerState(RasterizerState::create(rsDesc));

    // Depth test
    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthTest(true);
    mpDefaultPipelineState->setDepthStencilState(DepthStencilState::create(dsDesc));
    mpDefaultPipelineState->setFbo(mpDefaultFBO);
    mpDefaultPipelineState->setVao(mpVao);
    mpDefaultPipelineState->setProgram(mpProgram);
}

void ShaderBuffersSample::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mCameraController.update();

    // Update uniform-buffers data
    mpProgramVars["PerFrameCB"]["m.worldMat"] = glm::mat4();
    glm::mat4 wvp = mpCamera->getViewProjMatrix();
    mpProgramVars["PerFrameCB"]["m.wvpMat"] = wvp;
    mpProgramVars["PerFrameCB"]["surfaceColor"] = mSurfaceColor;

    mpProgramVars["LightCB"]["worldDir"] = mLightData.worldDir;
    mpProgramVars["LightCB"]["intensity"] = mLightData.intensity;
    
    // Set uniform buffers
    mpRenderContext->setProgramVariables(mpProgramVars);
    mpRenderContext->drawIndexed(mIndexCount, 0, 0);

     std::string msg = getFpsMsg() + '\n';
    if(mCountPixelShaderInvocations)
    {
        uint32_t* pData = (uint32_t*)mpInvocationsBuffer->map(Buffer::MapType::Read);
        std::string msg = "PS was invoked " + std::to_string(*pData) + " times";
        mpInvocationsBuffer->unmap();
        renderText(msg, vec2(600, 100));

        mpRenderContext->clearUAV(mpInvocationsBuffer, uvec4(0));
    }
}

void ShaderBuffersSample::onDataReload()
{
    mpVao = getVao();
}

bool ShaderBuffersSample::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mCameraController.onKeyEvent(keyEvent);
}

bool ShaderBuffersSample::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mCameraController.onMouseEvent(mouseEvent);
}

void ShaderBuffersSample::onResizeSwapChain()
{
    float height = (float)mpDefaultFBO->getHeight();
    float width = (float)mpDefaultFBO->getWidth();

    mpCamera->setFovY(float(M_PI / 8));
    mpCamera->setAspectRatio(width / height);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ShaderBuffersSample buffersSample;
    SampleConfig config;
    config.windowDesc.title = "Shader Buffers";
    buffersSample.run(config);
}
