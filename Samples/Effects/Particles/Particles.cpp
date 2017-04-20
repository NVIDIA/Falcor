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
#include <fstream>

const Gui::DropdownList kPixelShaders 
{
    {0, "ConstColor"},
    {1, "ColorInterp"},
    {2, "Textured"}
};

const char* kConstColorPs = "Effects/ParticleConstColor.ps.hlsl";
const char* kColorInterpPs = "Effects/ParticleInterpColor.ps.hlsl";
const char* kTexturedPs = "Effects/ParticleTexture.ps.hlsl";
const std::string kDefaultTexture = "TestParticle.png";

void Particles::initScene(std::string filename)
{
    mpScene = Scene::loadFromFile(filename, Model::LoadFlags::None, Scene::LoadFlags::StoreMaterialHistory);
    mpSceneRenderer = SceneRenderer::create(mpScene);
    mpSceneProgram = GraphicsProgram::createFromFile("", "SceneEditorSample.fs");
    std::string lights;
    getSceneLightString(mpScene.get(), lights);
    mpSceneProgram->addDefine("_LIGHT_SOURCES", lights);
    mpSceneVars = GraphicsVars::create(mpSceneProgram->getActiveVersion()->getReflector());
    setSceneLightsIntoConstantBuffer(mpScene.get(), mpSceneVars["PerFrameCB"].get());
    mpCamera = mpScene->getActiveCamera();
    mpCamController.attachCamera(mpCamera);
}

void Particles::onGuiRender()
{
    //if (mpGui->addButton("Load Scene"))
    //{
    //    std::string filename;
    //    openFileDialog("", filename);
    //    initScene(filename);
    //}

    if (mpGui->beginGroup("Create System"))
    {
        static int32_t sMaxParticles = 4096;
        mpGui->addIntVar("Max Particles", sMaxParticles, 0);
        static bool sorted = false;
        mpGui->addCheckBox("Sorted", sorted);
        mpGui->addDropdown("PixelShader", kPixelShaders, mPixelShaderIndex);
        if (mpGui->addButton("Create"))
        {
            switch ((ExamplePixelShaders)mPixelShaderIndex)
            {
            case ExamplePixelShaders::ConstColor:
            {
                ParticleSystem::SharedPtr pSys = 
                    ParticleSystem::create(mpRenderContext.get(), sMaxParticles, kConstColorPs, ParticleSystem::kDefaultSimulateShader, sorted);
                mParticleSystems.push_back(pSys);
                mPsData.push_back(vec4(0.f, 0.f, 0.f, 1.f));
                mParticleSystems[mParticleSystems.size() - 1]->getDrawVars()->getConstantBuffer(2)->
                    setBlob(&mPsData[mPsData.size() - 1].data.color, 0, sizeof(vec4));
                break;
            }
            case ExamplePixelShaders::ColorInterp:
            {
                ParticleSystem::SharedPtr pSys =
                    ParticleSystem::create(mpRenderContext.get(), sMaxParticles, kColorInterpPs, ParticleSystem::kDefaultSimulateShader, sorted);
                mParticleSystems.push_back(pSys);
                ColorInterpPsPerFrame perFrame;
                perFrame.color1 = vec4(1.f, 0.f, 0.f, 1.f);
                perFrame.colorT1 = pSys->getParticleDuration();
                perFrame.color2 = vec4(0.f, 0.f, 1.f, 1.f);
                perFrame.colorT2 = 0.f;
                mPsData.push_back(perFrame);
                mParticleSystems[mParticleSystems.size() - 1]->getDrawVars()->getConstantBuffer(2)->
                    setBlob(&mPsData[mPsData.size() - 1].data.interp, 0, sizeof(ColorInterpPsPerFrame));
                break;
            }
            case ExamplePixelShaders::Textured:
            {
                ParticleSystem::SharedPtr pSys = 
                    ParticleSystem::create(mpRenderContext.get(), sMaxParticles, kTexturedPs, ParticleSystem::kDefaultSimulateShader, sorted);
                mParticleSystems.push_back(pSys);
                mPsData.push_back(0);
                pSys->getDrawVars()->setSrv(2, mTextures[0]->getSRV());
                break;
            }
            default:
            {
                should_not_get_here();
            }
            }
        }
        mpGui->endGroup();
    }

    mpGui->addSeparator();
    mpGui->addIntVar("System index", mGuiIndex, 0, ((int32_t)mParticleSystems.size()) - 1);
    mpGui->addSeparator();

    //If there are no systems yet, don't let user touch properties
    if (mGuiIndex < 0)
        return;

    if (mpGui->beginGroup("Common Properties"))
    {
        mParticleSystems[mGuiIndex]->renderUi(mpGui.get());
        mpGui->endGroup();
    }
    if (mpGui->beginGroup("Pixel Shader Properties"))
    {
        switch ((ExamplePixelShaders)mPsData[mGuiIndex].type)
        {
        case ExamplePixelShaders::ConstColor:
        {
            if (mpGui->addRgbaColor("Color", mPsData[mGuiIndex].data.color))
            {
                mParticleSystems[mGuiIndex]->getDrawVars()->getConstantBuffer(2)->
                    setBlob(&mPsData[mGuiIndex].data.color, 0, sizeof(vec4));
            }
            break;
        }
        case ExamplePixelShaders::ColorInterp:
        {
            bool dirty = mpGui->addRgbaColor("Color1", mPsData[mGuiIndex].data.interp.color1);
            dirty |= mpGui->addFloatVar("Color T1", mPsData[mGuiIndex].data.interp.colorT1);
            dirty |= mpGui->addRgbaColor("Color2", mPsData[mGuiIndex].data.interp.color2);
            dirty |= mpGui->addFloatVar("Color T2", mPsData[mGuiIndex].data.interp.colorT2);

            if (dirty)
            {
                mParticleSystems[mGuiIndex]->getDrawVars()->getConstantBuffer(2)->
                    setBlob(&mPsData[mGuiIndex].data.interp, 0, sizeof(ColorInterpPsPerFrame));
            }

            break;
        }
        case ExamplePixelShaders::Textured:
        {
            if (mpGui->addButton("Add Texture"))
            {
                std::string filename;
                //todo put a proper filter here
                openFileDialog("", filename);
                mTextures.push_back(createTextureFromFile(filename, true, false));
                mTexDropdown.push_back({(int32_t)mTexDropdown.size(), filename });
            }

            uint32_t texIndex = mPsData[mGuiIndex].data.texIndex;
            if (mpGui->addDropdown("Texture", mTexDropdown, texIndex))
            {
                mParticleSystems[mGuiIndex]->getDrawVars()->setSrv(2, mTextures[texIndex]->getSRV());
                mPsData[mGuiIndex].data.texIndex = texIndex;
            }
            break;
        }
        default:
        {
            should_not_get_here();
            break;
        }
        }

        mpGui->endGroup();
    }
}

void Particles::onLoad()
{
    mpCamera = Camera::create();
    mpCamera->setPosition(mpCamera->getPosition() + glm::vec3(0, 5, 10));
    mpCamController.attachCamera(mpCamera);
    mTextures.push_back(createTextureFromFile(kDefaultTexture, true, false));
    mTexDropdown.push_back({ 0, kDefaultTexture });
    BlendState::Desc blendDesc;
    blendDesc.setRtBlend(0, true);
    blendDesc.setRtParams(0, BlendState::BlendOp::Add, BlendState::BlendOp::Add, BlendState::BlendFunc::SrcAlpha, BlendState::BlendFunc::OneMinusSrcAlpha,
        BlendState::BlendFunc::SrcAlpha, BlendState::BlendFunc::OneMinusSrcAlpha);
    BlendState::SharedPtr pBlend = BlendState::create(blendDesc);
    mpRenderContext->getGraphicsState()->setBlendState(pBlend);
}

void Particles::onFrameRender()
{
	const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
 	mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mpCamController.update();

    if(mpScene)
    {
        mpRenderContext->getGraphicsState()->setProgram(mpSceneProgram);
        mpRenderContext->pushGraphicsVars(mpSceneVars);

        mpSceneRenderer->renderScene(mpRenderContext.get());
        
        mpRenderContext->popGraphicsVars();
    }

    for (auto it = mParticleSystems.begin(); it != mParticleSystems.end(); ++it)
    {
        (*it)->update(mpRenderContext.get(), frameRate().getLastFrameTime(), mpCamera->getViewMatrix());
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
