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

    std::vector<ParticleSystem::SharedPtr> mParticleSystems;
    Camera::SharedPtr mpCamera;
    FirstPersonCameraController mpCamController;

    struct GuiData
    {
        int32_t mSystemIndex = -1;
        uint32_t mPixelShaderIndex = 0;
        bool mSortSystem = false;
        int32_t mMaxParticles = 4096;
        Gui::DropdownList mTexDropdown;
    } mGuiData;

    union PSData
    {
        PSData() : color(vec4(1.f, 1.f, 1.f, 1.f)) {}
        ~PSData() {}
        PSData(vec4 newColor) : color(newColor) {};
        PSData(ColorInterpPsPerFrame interpData) : interp(interpData) {};
        PSData(uint32_t newTexIndex) : texIndex(newTexIndex) {};
        vec4 color;
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
        PixelShaderData(vec4 color) : type(ExamplePixelShaders::ConstColor), data(color) {}
        PixelShaderData(ColorInterpPsPerFrame interpData) : type(ExamplePixelShaders::ColorInterp), data(interpData) {}
        PixelShaderData(uint32_t texIndex) : type(ExamplePixelShaders::Textured), data(texIndex) {}
        ExamplePixelShaders type;
        PSData data;
    };

    std::vector<PixelShaderData> mPsData;
    std::vector<Texture::SharedPtr> mTextures;
    uint32_t mTexIndex = 0;
    //gui var for setting the max particles of created particle systems
};
