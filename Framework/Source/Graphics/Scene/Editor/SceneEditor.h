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
#include "Graphics/Scene/Editor/Gizmo.h"
#include "Graphics/Scene/Editor/SceneEditorRenderer.h"

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

        const Camera::SharedPtr& getEditorCamera() const { return mpEditorScene->getActiveCamera(); }

        void update(double currentTime);
        void render();

        bool onMouseEvent(const MouseEvent& mouseEvent);
        bool onKeyEvent(const KeyboardEvent& keyEvent);
        void onResizeSwapChain();

    private:

        SceneEditor(const Scene::SharedPtr& pScene, const uint32_t modelLoadFlags);
        Scene::SharedPtr mpScene;

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

        // Light functions
        void addPointLight(Gui* pGui);
        void addDirectionalLight(Gui* pGui);

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

        // #TODO get from user
        RenderContext::SharedPtr mpRenderContext;

        //
        // Initialization
        //

        // Initializes Editor helper-scenes, Picking, and Rendering
        void initializeEditorScenes();

        // Initializes Editor's representation of the scene being edited
        void initializeEditorObjects();

        // Model Instance Euler Angle Helpers
        const glm::vec3& getActiveInstanceEulerRotation();
        void setActiveInstanceEulerRotation(const glm::vec3& rotation);
        std::vector<std::vector<glm::vec3>> mInstanceEulerRotations;

        //
        // Editor Objects
        //
        static const float kCameraModelScale;
        static const float kLightModelScale;

        // Update transform of gizmos and camera models
        void updateEditorObjectTransforms();

        // Helper function to update the transform of a single camera model from it's respective camera in the master scene
        void updateCameraModelTransform(uint32_t cameraID);

        // Rebuilds the lookup map between light model instances and master scene point lights
        void rebuildLightIDMap();

        // Helper to apply gizmo transforms to the object being edited
        void applyGizmoTransform();

        // Types of objects selectable in the editor
        enum class ObjectType
        {
            None,
            Model,
            Camera,
            Light
        };

        //
        // Picking
        //

        void select(const Scene::ModelInstance::SharedPtr& pModelInstance);
        void deselect();

        void setActiveModelInstance(const Scene::ModelInstance::SharedPtr& pModelInstance);

        // ID's in master scene
        uint32_t mSelectedModel = 0;
        int32_t mSelectedModelInstance = 0;
        uint32_t mSelectedCamera = 0;
        uint32_t mSelectedLight = 0;

        Picking::UniquePtr mpScenePicker;

        std::set<Scene::ModelInstance*> mSelectedInstances;
        ObjectType mSelectedObjectType = ObjectType::None;

        CpuTimer mMouseHoldTimer;

        //
        // Gizmos
        //

        static const Gui::RadioButtonGroup kGizmoSelectionButtons;

        void setActiveGizmo(Gizmo::Type type, bool show);

        bool mGizmoBeingDragged = false;
        Gizmo::Type mActiveGizmoType = Gizmo::Type::Translate;
        Gizmo::Gizmos mGizmos;

        //
        // Editor
        //

        // Find instance ID of a model instance in the editor scene. Returns uint -1 if not found.
        uint32_t findEditorModelInstanceID(uint32_t modelID, const Scene::ModelInstance::SharedPtr& pInstance) const;

        // Wireframe Rendering
        GraphicsState::SharedPtr mpSelectionGraphicsState;
        GraphicsProgram::SharedPtr mpColorProgram;
        GraphicsVars::SharedPtr mpColorProgramVars;

        // Separate scene for rendering selected model wireframe
        Scene::SharedPtr mpSelectionScene;
        SceneRenderer::UniquePtr mpSelectionSceneRenderer;

        // Separate scene for editor gizmos and objects
        Scene::SharedPtr mpEditorScene;
        SceneEditorRenderer::UniquePtr mpEditorSceneRenderer;
        Picking::UniquePtr mpEditorPicker;

        Model::SharedPtr mpCameraModel;
        Model::SharedPtr mpLightModel;

        uint32_t mEditorCameraModelID = (uint32_t)-1;
        uint32_t mEditorLightModelID = (uint32_t)-1;

        // Maps between light models and master scene light ID
        std::unordered_map<uint32_t, uint32_t> mLightIDEditorToScene;
        std::unordered_map<uint32_t, uint32_t> mLightIDSceneToEditor;
    };
}