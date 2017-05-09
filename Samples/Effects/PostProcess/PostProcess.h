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
#include "SampleTest.h"
using namespace Falcor;

class PostProcess : public SampleTest
{
public:
    void onLoad() override;
    void onFrameRender() override;
    void onShutdown() override;
    void onResizeSwapChain() override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;

private:
    Model::SharedPtr mpTeapot;
    Texture::SharedPtr mHdrImage;
    ModelViewCameraController mCameraController;
    Camera::SharedPtr mpCamera;
    float mLightIntensity = 1.0f;
    float mSurfaceRoughness = 5.0f;

    void onGuiRender() override;
    void renderTeapot();

    Sampler::SharedPtr mpTriLinearSampler;
    GraphicsProgram::SharedPtr mpMainProg = nullptr;
    GraphicsVars::SharedPtr mpProgramVars = nullptr;
    GraphicsState::SharedPtr mpGraphicsState = nullptr;

    SkyBox::UniquePtr mpSkyBox;

    enum HdrImage
    {
        EveningSun,
        OvercastDay,
        AtTheWindow
    };
    static const Gui::DropdownList kImageList;

    HdrImage mHdrImageIndex = HdrImage::EveningSun;
    Fbo::SharedPtr mpHdrFbo;
    ToneMapping::UniquePtr mpToneMapper;
    SceneRenderer::SharedPtr mpSceneRenderer;

    void loadImage();

    //testing
    void onInitializeTesting() override;
    void onEndTestFrame() override;
    std::vector<uint32_t> mChangeModeFrames;
    std::vector<uint32_t>::iterator mChangeModeIt;
    uint32_t mToneMapOperatorIndex;
};
