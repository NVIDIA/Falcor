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

class FeatureDemo : public SampleTest
{
public:
    void onLoad() override;
    void onFrameRender() override;
    void onShutdown() override;
    void onResizeSwapChain() override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    void onGuiRender() override;

private:
    Fbo::SharedPtr mpMainFbo;
    Fbo::SharedPtr mpResolveFbo;
    Fbo::SharedPtr mpPostProcessFbo;
    struct
    {
        SkyBox::UniquePtr pEffect;
        DepthStencilState::SharedPtr pDS;
        Sampler::SharedPtr pSampler;
    } mSkyBox;

    struct
    {
        GraphicsVars::SharedPtr pVars;
        GraphicsProgram::SharedPtr pProgram;
    } mLightingPass;

    struct ShadowPass
    {
        bool updateShadowMap = true;
        CascadedShadowMaps::UniquePtr pCsm;
        glm::mat4 camVpAtLastCsmUpdate = glm::mat4();
    };
    ShadowPass mShadowPass;

    ToneMapping::UniquePtr mpToneMapper;

    struct
    {
        SSAO::UniquePtr pSSAO;
        FullScreenPass::UniquePtr pApplySSAOPass;
        GraphicsVars::SharedPtr pVars;
    } mSSAO;

    struct
    {
        ProgramVars::BindLocation perFrameCB;
        size_t lastCsmUpdateOffset;

        ProgramVars::BindLocation csmData;
        ProgramVars::BindLocation envMapTex;
        ProgramVars::BindLocation sampler;
    } mLightingBindLocs;

    struct
    {
        ProgramVars::BindLocation colorTex;
        ProgramVars::BindLocation aoMapTex;
    } mSSAOBindLocs;

    void beginFrame();
    void endFrame();
    void renderSkyBox();
    void postProcess();
    void lightingPass();
    void resolveMSAA();
    void shadowPass();
    void ambientOcclusion();

    void initSkyBox(const std::string& name);
    void initPostProcess();
    void initLightingPass();
    void initShadowPass();
    void initSSAO();
    void initEnvMap(const std::string& name);

    void initControls();
    
    GraphicsState::SharedPtr mpState;
    SceneRenderer::SharedPtr mpSceneRenderer;
    void loadModel(const std::string& filename, bool showProgressBar);
    void loadScene(const std::string& filename, bool showProgressBar);
    void initScene(Scene::SharedPtr pScene);
    void setActiveCameraAspectRatio();
    void setSceneSampler(uint32_t maxAniso);

    uint32_t mSampleCount = 4;
    Texture::SharedPtr mpEnvMap;
    Sampler::SharedPtr mpSceneSampler;

    struct ProgramControl
    {
        bool enabled;
        std::string define;
        std::string value;
    };

    enum ControlID
    {
        SuperSampling,
        DisableSpecAA,
        EnableShadows,
        EnableReflections,
        EnableSSAO,

        Count
    };

    std::vector<ProgramControl> mControls;
    void applyLightingProgramControl(ControlID controlID);

    bool mUseCameraPath = true;
    void applyCameraPathState();
    bool mOptimizedShaders = true;

    //Testing 
    void onInitializeTesting() override;
    void onBeginTestFrame() override;
};
