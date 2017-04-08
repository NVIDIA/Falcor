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
#include "Particles.h"
#include "API/D3D/FalcorD3D.h"
#include <limits>

void Particles::onGuiRender()
{
    if (mpGui->beginGroup("Create System"))
    {
        static int32_t sMaxParticles = 512;
        mpGui->addIntVar("Max Particles", sMaxParticles, 0);
        if (mpGui->addButton("Create"))
        {
            mParticleSystems.push_back(ParticleSystem::create(mpRenderContext.get(), sMaxParticles, "Effects/ParticleConstColor.ps.hlsl"));
        }
        mpGui->endGroup();
    }

    if (mpGui->beginGroup("Edit Properties"))
    {
        mpGui->addIntVar("System index", mGuiIndex, 0, ((int32_t)mParticleSystems.size()) - 1);
        mpGui->addSeparator();
        mParticleSystems[mGuiIndex]->renderUi(mpGui.get());
    }
}

void Particles::onLoad()
{
    mParticleSystems.push_back(ParticleSystem::create(mpRenderContext.get(), 16 * 1024));
    mpCamera = Camera::create();
    mpCamera->setPosition(mpCamera->getPosition() + glm::vec3(0, 5, 10));
    mpCamController.attachCamera(mpCamera);
    mpTex = createTextureFromFile("C:/Users/clavelle/Desktop/waterdrop.png", true, false);
    mParticleSystems[0]->getDrawVars()->setSrv(2, mpTex->getSRV());
}

void Particles::onFrameRender()
{
	const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
 	mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mpCamController.update();

    for (auto it = mParticleSystems.begin(); it != mParticleSystems.end(); ++it)
    {
        (*it)->update(mpRenderContext.get(), frameRate().getLastFrameTime());
        (*it)->render(mpRenderContext.get(), mpCamera->getViewMatrix(), mpCamera->getProjMatrix());
    }
}

void Particles::onShutdown()
{
}

bool Particles::onKeyEvent(const KeyboardEvent& keyEvent)
{
    mpCamController.onKeyEvent(keyEvent);
    return false;
}

bool Particles::onMouseEvent(const MouseEvent& mouseEvent)
{
    mpCamController.onMouseEvent(mouseEvent);
    return false;
}

void Particles::onDataReload()
{
}

void Particles::onResizeSwapChain()
{
    float height = (float)mpDefaultFBO->getHeight();
    float width = (float)mpDefaultFBO->getWidth();

    mpCamera->setFocalLength(21.0f);
    float aspectRatio = (width / height);
    mpCamera->setAspectRatio(aspectRatio);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    Particles sample;
    SampleConfig config;
    config.windowDesc.title = "Particles";
    config.windowDesc.resizableWindow = true;
    sample.run(config);
}
