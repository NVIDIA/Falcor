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

class Shadows : public Sample
{
public:
    void onLoad() override;
    void onFrameRender() override;
    void onShutdown() override;
    void onResizeSwapChain() override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;

private:
    static void GUI_CALL getModelCB(void* pUserData);
    static void GUI_CALL setModelCB(const void* pUserData);

    void onGuiRender() override;
    void displayShadowMap();
    void runMainPass();
    void createVisualizationProgram();
    void createScene(const std::string& filename);
    void displayLoadSceneDialog();

    //TODO remove
    Texture::SharedPtr _TestTex;

    std::vector<CascadedShadowMaps::UniquePtr> mpCsmTech;
    Scene::SharedPtr mpScene;

    struct
    {
        FullScreenPass::UniquePtr pProgram;
        GraphicsVars::SharedPtr pProgramVars;
        ConstantBuffer::SharedPtr pCBuffer;
    } mShadowVisualizer;

    struct
    {
        GraphicsProgram::SharedPtr pProgram;
        GraphicsState::SharedPtr pState;
        GraphicsVars::SharedPtr pProgramVars;
        ConstantBuffer::SharedPtr pCBuffer;
    } mLightingPass;

    Sampler::SharedPtr mpLinearSampler = nullptr;

    glm::vec3 mAmbientIntensity = glm::vec3(0.1f, 0.1f, 0.1f);
    SceneRenderer::UniquePtr mpRenderer;
    glm::mat4 mCamVpAtLastCsmUpdate;

    struct Controls
    {
        bool updateShadowMap = true;
        bool showShadowMap = false;
        bool visualizeCascades = false;
        int32_t displayedCascade = 0;
        //TODO the default for this should be 4
        uint32_t cascadeCount = 1;
        int32_t lightIndex = 0;
    };
    Controls mControls;

    void setLightIndex(int32_t index);

    static void GUI_CALL loadSceneCB(void* pThis);
    static void GUI_CALL getCascadeCountCB(void* pVal, void* pThis);
    static void GUI_CALL setCascadeCountCB(const void* pVal, void* pThis);

    static void GUI_CALL getLightIndexCB(void* pVal, void* pThis);
    static void GUI_CALL setLightIndexCB(const void* pVal, void* pThis);
};