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
#pragma once
#include "Falcor.h"

using namespace Falcor;

class Particles : public Sample
{
public:
    void onLoad() override;
    void onFrameRender() override;
    void onShutdown() override;
    void onResizeSwapChain() override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    void onDataReload() override;
    void onGuiRender() override;

private:
    enum class ExamplePixelShaders
    {
        ConstColor = 0,
        ColorInterp = 1,
        Textured = 2,
        Count 
    };

    ComputeContext::SharedPtr mpComputeContext;
    std::vector<ParticleSystem::SharedPtr> mParticleSystems;
    int32_t mGuiIndex = 0;
    uint32_t mPixelShaderIndex = 0;
    Camera::SharedPtr mpCamera;
    FirstPersonCameraController mpCamController;
    Texture::SharedPtr mpTex;
    static const uint32_t guiTextBufferSize = 200;
    char mGuiTextBuffer[guiTextBufferSize];
    //This seems kinda messy. I imagine user would wrap particle system in their own class to store additional 
    //custom properties like these
    union PSData
    {
        PSData() : color(vec3(1.f, 1.f, 1.f)) {}
        ~PSData() {}
        PSData(vec3 newColor) : color(newColor) {};
        PSData(ColorInterpPsPerFrame interpData) : interp(interpData) {};
        PSData(uint32_t newTexIndex) : texIndex(newTexIndex) {};
        vec3 color;
        ColorInterpPsPerFrame interp;
        uint32_t texIndex;
    };

    struct PixelShaderData
    {
        PixelShaderData(const PixelShaderData& rhs) 
        {
            type = rhs.type;
            switch (type)
            {
            case ExamplePixelShaders::ConstColor: data.color = rhs.data.color; return;
            case ExamplePixelShaders::ColorInterp: data.interp = rhs.data.interp; return;
            case ExamplePixelShaders::Textured: data.texIndex = rhs.data.texIndex; return;
            default: should_not_get_here();
            }
        }
        PixelShaderData(vec3 color) : type(ExamplePixelShaders::ConstColor), data(color) {}
        PixelShaderData(ColorInterpPsPerFrame interpData) : type(ExamplePixelShaders::ColorInterp), data(interpData) {}
        PixelShaderData(uint32_t texIndex) : type(ExamplePixelShaders::Textured), data(texIndex) {}
        ExamplePixelShaders type;
        PSData data;
    };

    std::vector<PixelShaderData> mPsData;
    std::vector<Texture::SharedPtr> mTextures;
    Gui::DropdownList mTexDropdown;
    uint32_t mTexIndex = 0;

    void initScene(std::string filename);

    //Hacked in temporarily for screenshot
    Scene::SharedPtr mpScene;
    SceneRenderer::SharedPtr mpSceneRenderer;
    GraphicsVars::SharedPtr mpSceneVars;
    GraphicsProgram::SharedPtr mpSceneProgram;
};
