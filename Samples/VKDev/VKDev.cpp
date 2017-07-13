/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
#ifdef FALCOR_VK
#include "VKDev.h"

void VKDev::onGuiRender()
{
    mpGui->addButton("foo!");
}

void VKDev::onLoad()
{
    resizeSwapChain(1000, 1000);
    mpPass = FullScreenPass::create("FullScreenPass.vs.glsl", "FullScreenPass.fs.glsl");
    mpVars = GraphicsVars::create(mpPass->getProgram()->getActiveVersion()->getReflector());

    Sampler::Desc sampler;
    sampler.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point);
    mpVars->setSampler("gSampler", Sampler::create(sampler));
    mpVars->setTexture("gTex", createTextureFromFile("C:\\Users\\nbenty\\Pictures\\ff7.jpg", false, true));
    mpTypedBuffer = TypedBuffer<uint32_t>::create(1);
    mpTex3D = Texture::create3D(10, 1, 1, ResourceFormat::R32Uint, 1, nullptr, Resource::BindFlags::UnorderedAccess);
    mpVars->setTypedBuffer("typedBuffer", mpTypedBuffer);
    mpVars->setTexture("typed3D", mpTex3D);
}

void VKDev::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mp4SampleFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::Color);
    mpDefaultPipelineState->pushFbo(mp4SampleFbo);

    StructuredBuffer::SharedPtr pCountBuffer = mpVars->getStructuredBuffer("outImage");
    pCountBuffer[0]["count"] = 0u;

    mpRenderContext->clearUAV(mpTypedBuffer->getUAV().get(), uvec4(0));
    mpRenderContext->clearUAV(mpTex3D->getUAV().get(), uvec4(0));

    mpVars["PerFrameCB"]["offset"] = mTexOffset;
    mpRenderContext->setGraphicsVars(mpVars);
    mpPass->execute(mpRenderContext.get());
    mpRenderContext->blit(mp4SampleFbo->getColorTexture(0)->getSRV(), mpDefaultFBO->getRenderTargetView(0));

    mpDefaultPipelineState->popFbo();
    std::string count;
    count += "Structured Buffer =   " + std::to_string((uint32_t)pCountBuffer[0]["count"]) + "\n";
    const uint32_t* pTyped = (uint32_t*)mpTypedBuffer->map(Buffer::MapType::Read);
    count += "Typed Buffer      =   " + std::to_string(pTyped[0]) + "\n";
    const auto& tex3D = mpRenderContext->readTextureSubresource(mpTex3D.get(), 0);
    pTyped = (uint32_t*)tex3D.data();
    count += "Texture 3D        =   " + std::to_string(pTyped[5]) + "\n";
    mpTypedBuffer->unmap();
    renderText(count, vec2(250, 20));
}

void VKDev::onShutdown()
{

}

bool VKDev::onKeyEvent(const KeyboardEvent& keyEvent)
{
    if (keyEvent.type == KeyboardEvent::Type::KeyPressed)
    {
        switch (keyEvent.key)
        {
        case KeyboardEvent::Key::W:
            mTexOffset.y += 0.1f;
            break;
        case KeyboardEvent::Key::S:
            mTexOffset.y -= 0.1f;
            break;
        case KeyboardEvent::Key::D:
            mTexOffset.x += 0.1f;
            break;
        case KeyboardEvent::Key::A:
            mTexOffset.x -= 0.1f;
            break;
        default:
            return false;
        }
        return true;
    }
    return false;
}

bool VKDev::onMouseEvent(const MouseEvent& mouseEvent)
{
    return false;
}

void VKDev::onDataReload()
{

}

void VKDev::onResizeSwapChain()
{
    Fbo::Desc fboDesc;
    fboDesc.setSampleCount(4).setColorTarget(0, mpDefaultFBO->getColorTexture(0)->getFormat());
    mp4SampleFbo = FboHelper::create2D(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), fboDesc);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    VKDev sample;
    SampleConfig config;
    config.windowDesc.title = "Vulkan Sandbox";
    config.windowDesc.resizableWindow = true;
    sample.run(config);
}
#else
#include <Windows.h>
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    return 0;
}
#endif