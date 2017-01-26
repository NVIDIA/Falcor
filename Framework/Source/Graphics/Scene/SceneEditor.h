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
#include <vector>
#include <set>
#include "Graphics/Paths/PathEditor.h"
#include "Utils/Picking/Picking.h"

namespace Falcor
{
    class Scene;
    class Gui;

    class SceneEditor
    {
    public:
        using UniquePtr = std::unique_ptr<SceneEditor>;
        using UniqueConstPtr = std::unique_ptr<const SceneEditor>;

        static UniquePtr create(const Scene::SharedPtr& pScene, const uint32_t modelLoadFlags = 0);
        void renderGui(Gui* pGui);
        ~SceneEditor();

        void renderSelection();
        bool onMouseEvent(const MouseEvent& mouseEvent);
        bool onKeyEvent(const KeyboardEvent& keyEvent);
        void onResizeSwapChain();

        void setActiveModelInstance(const Scene::ModelInstance::SharedPtr& pModelInstance);

    private:
        SceneEditor(const Scene::SharedPtr& pScene, const uint32_t modelLoadFlags);
        Scene::SharedPtr mpScene;
        uint32_t mActiveModel = 0;
        int32_t mActiveModelInstance = 0;

        struct
        {
            uint32_t ActivePath = 0;
            PathEditor::UniquePtr pEditor;
        } mPathEditor;

        bool mSceneDirty = false;

        // Main GUI functions
        void renderModelElements(Gui* pGui);
        void renderCameraElements(Gui* pGui);
        void renderLightElements(Gui* pGui);
        void renderGlobalElements(Gui* pGui);
        void renderPathElements(Gui* pGui);

        // Model functions
        void addModel(Gui* pGui);
        void deleteModel(Gui* pGui);
        void deleteModel();
        void setModelName(Gui* pGui);
        void setModelVisible(Gui* pGui);
        void selectActiveModel(Gui* pGui);

        // Model instance
        void addModelInstance(Gui* pGui);
        void addModelInstanceRange(Gui* pGui);
        void deleteModelInstance(Gui* pGui);
        void setInstanceTranslation(Gui* pGui);
        void setInstanceScaling(Gui* pGui);
        void setInstanceRotation(Gui* pGui);


        // Camera functions
        void setCameraFOV(Gui* pGui);
        void setCameraDepthRange(Gui* pGui);
        void setCameraAspectRatio(Gui* pGui);
        void setCameraName(Gui* pGui);
        void setCameraSpeed(Gui* pGui);
        void addCamera(Gui* pGui);
        void deleteCamera(Gui* pGui);
        void setActiveCamera(Gui* pGui);
        void setCameraPosition(Gui* pGui);
        void setCameraTarget(Gui* pGui);
        void setCameraUp(Gui* pGui);

        // Paths
        void pathEditorFinishedCB();
        void addPath(Gui* pGui);
        void selectActivePath(Gui* pGui);
        void deletePath(Gui* pGui);
        void startPathEditor(Gui* pGui);
        void startPathEditor();

        // Global functions
        void setAmbientIntensity(Gui* pGui);
        void saveScene();

        void renderModelAnimation(Gui* pGui);

        uint32_t mModelLoadFlags = 0;
        uint32_t mSceneLoadFlags = 0;

        // Gets and caches Euler rotations from model instances in the scene
        void initializeEulerRotationsCache();

        std::vector<std::vector<glm::vec3>> mInstanceEulerRotations;

        // Picking
        void addToSelection(const Scene::ModelInstance::SharedPtr& pModelInstance, bool append);
        void deselect();

        RenderContext::SharedPtr mpRenderContext;
        Picking::UniquePtr mpPicking;

        bool mControlDown = false;
        CpuTimer mMouseHoldTimer;
        
        GraphicsState::SharedPtr mpGraphicsState;
        GraphicsProgram::SharedPtr mpWireframeProgram;
        RasterizerState::SharedPtr mpBiasWireframeRS;
        DepthStencilState::SharedPtr mpDepthTestDS;

        Scene::SharedPtr mpSelectionScene; // Holds selection in model selection mode
        SceneRenderer::UniquePtr mpSelectionSceneRenderer;
        std::set<Scene::ModelInstance*> mSelectedInstances;
    };
}