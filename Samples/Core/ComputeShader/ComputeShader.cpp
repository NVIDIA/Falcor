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
#include "ComputeShader.h"
#include "API/D3D/FalcorD3D.h"

void ComputeShader::onGuiRender()
{
    if (mpGui->addButton("Load Image"))
    {
        loadImage();
    }
    mpGui->addCheckBox("Pixelate", mbPixelate);
}

void ComputeShader::onLoad()
{
    mpProg = ComputeProgram::createFromFile("compute.hlsl");
    mpState = ComputeState::create();
    mpState->setProgram(mpProg);
    mpProgVars = ProgramVars::create(mpProg->getActiveVersion()->getReflector());
    mpBlitPass = FullScreenPass::create("blit.fs");
    mpBlitVars = ProgramVars::create(mpBlitPass->getProgram()->getActiveVersion()->getReflector());

    mpTmpTexture = Texture::create2D(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), mpDefaultFBO->getColorTexture(0)->getFormat(), 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
}

void ComputeShader::loadImage()
{
    std::string filename;
    if(openFileDialog("Supported Formats\0*.jpg;*.bmp;*.dds;*.png;*.tiff;*.tif;*.tga\0\0", filename))
    {
        auto fboFormat = mpDefaultFBO->getColorTexture(0)->getFormat();
        mpImage = createTextureFromFile(filename, false, isSrgbFormat(fboFormat));
 
        resizeSwapChain(mpImage->getWidth(), mpImage->getHeight());
        mpTmpTexture = Texture::create2D(mpImage->getWidth(), mpImage->getHeight(), ResourceFormat::RGBA8Unorm, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
        mpProgVars->setTexture("gInput", mpImage);
    }
}

void ComputeShader::onFrameRender()
{
	const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearRtv(mpTmpTexture->getRTV().get(), clearColor);

    if(mpImage)
    {
        if (mbPixelate)
        {
            mpProg->addDefine("_PIXELATE");
        }
        else
        {
            mpProg->removeDefine("_PIXELATE");
        }
        mpProgVars->setUav("gOutput", mpTmpTexture);

        mpRenderContext->setComputeState(mpState);
        mpRenderContext->setComputeVars(mpProgVars);

        uint32_t w = (mpImage->getWidth() / 16) + 1;
        uint32_t h = (mpImage->getHeight() / 16) + 1;
        mpRenderContext->dispatch(w, h, 1);
    }

    // FIXME: should be part of the render-context
    mpBlitVars->setTexture("gTexture", mpTmpTexture);
    mpRenderContext->setProgramVariables(mpBlitVars);
    mpBlitPass->execute(mpRenderContext.get());
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ComputeShader sample;
    SampleConfig config;
    config.windowDesc.title = "Falcor Project Template";
    config.windowDesc.resizableWindow = true;
    config.deviceDesc.depthFormat = ResourceFormat::Unknown;
    sample.run(config);
}
