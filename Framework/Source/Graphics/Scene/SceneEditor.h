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
#include "Utils/Gui.h"
#include "Graphics/Paths/PathEditor.h"

namespace Falcor
{
    class Scene;

    class SceneEditor
    {
    public:
        using UniquePtr = std::unique_ptr<SceneEditor>;
        using UniqueConstPtr = std::unique_ptr<const SceneEditor>;

        static UniquePtr create(const Scene::SharedPtr& pScene, const uint32_t modelLoadFlags = 0);
        ~SceneEditor();

        void setUiVisible(bool visible);
    private:
        SceneEditor(const Scene::SharedPtr& pScene, const uint32_t modelLoadFlags);
        Scene::SharedPtr mpScene;
        uint32_t mActiveModel = 0;
        int32_t mActiveModelInstance = 0;
        Gui::UniquePtr mpGui = nullptr;

        struct
        {
            uint32_t ActivePath = 0;
            PathEditor::UniquePtr pEditor = nullptr;
        } mPathEditor;

        bool mSceneDirty = false;

        void initializeUI();
        void setModelElements();
        void setCameraElements();
        void setLightElements();
        void setGlobalElements();
        void setPathElements();

        void refreshModelElements();

        static void GUI_CALL addModelCB(void* pUserData);
        static void GUI_CALL deleteModelCB(void* pUserData);

        static void GUI_CALL addModelInstanceCB(void* pUserData);
        static void GUI_CALL deleteModelInstanceCB(void* pUserData);

        static void GUI_CALL setModelNameCB(const void* pVal, void* pUserData);
        static void GUI_CALL getModelNameCB(void* pVal, void* pUserData);

        static void GUI_CALL setActiveModelCB(const void* pVal, void* pUserData);
        static void GUI_CALL getActiveModelCB(void* pVal, void* pUserData);

        static void GUI_CALL setModelVisibleCB(const void* pVal, void* pUserData);
        static void GUI_CALL getModelVisibleCB(void* pVal, void* pUserData);

        static void GUI_CALL setModelActiveAnimationCB(const void* pVal, void* pUserData);
        static void GUI_CALL getModelActiveAnimationCB(void* pVal, void* pUserData);

        static void GUI_CALL setCameraFOVCB(const void* pVal, void* pUserData);
        static void GUI_CALL getCameraFOVCB(void* pVal, void* pUserData);

        static void GUI_CALL setCameraNearPlaneCB(const void* pVal, void* pUserData);
        static void GUI_CALL getCameraNearPlaneCB(void* pVal, void* pUserData);

        static void GUI_CALL setCameraFarPlaneCB(const void* pVal, void* pUserData);
        static void GUI_CALL getCameraFarPlaneCB(void* pVal, void* pUserData);

        static void GUI_CALL setCameraAspectRatioCB(const void* pVal, void* pUserData);
        static void GUI_CALL getCameraAspectRatioCB(void* pVal, void* pUserData);

        static void GUI_CALL setAmbientIntensityCB(const void* pVal, void* pUserData);
        static void GUI_CALL getAmbientIntensityCB(void* pVal, void* pUserData);

        static void GUI_CALL addCameraCB(void* pUserData);
        static void GUI_CALL deleteCameraCB(void* pUserData);

        static void GUI_CALL setCameraNameCB(const void* pVal, void* pUserData);
        static void GUI_CALL getCameraNameCB(void* pVal, void* pUserData);

        static void GUI_CALL addPathCB(void* pUserData);
        static void GUI_CALL deletePathCB(void* pUserData);
        static void GUI_CALL editPathCB(void* pUserData);

        static void GUI_CALL setActivePathCB(const void* pVal, void* pUserData);
        static void GUI_CALL getActivePathCB(void* pVal, void* pUserData);

        static void GUI_CALL setActiveCameraCB(const void* pVal, void* pUserData);
        static void GUI_CALL getActiveCameraCB(void* pVal, void* pUserData);

        static void GUI_CALL setCameraSpeedCB(const void* pVal, void* pUserData);
        static void GUI_CALL getCameraSpeedCB(void* pVal, void* pUserData);

        template<uint32_t Channel>
        static void GUI_CALL setInstanceTranslationCB(const void* pVal, void* pUserData);
        template<uint32_t Channel>
        static void GUI_CALL getInstanceTranslationCB(void* pVal, void* pUserData);

        template<uint32_t Channel>
        static void GUI_CALL setInstanceScalingCB(const void* pVal, void* pUserData);
        template<uint32_t Channel>
        static void GUI_CALL getInstanceScalingCB(void* pVal, void* pUserData);

        template<uint32_t Channel>
        static void GUI_CALL setInstanceRotationCB(const void* pVal, void* pUserData);
        template<uint32_t Channel>
        static void GUI_CALL getInstanceRotationCB(void* pVal, void* pUserData);

        template<uint32_t Channel>
        static void GUI_CALL setCameraPositionCB(const void* pVal, void* pUserData);
        template<uint32_t Channel>
        static void GUI_CALL getCameraPositionCB(void* pVal, void* pUserData);

        template<uint32_t Channel>
        static void GUI_CALL setCameraTargetCB(const void* pVal, void* pUserData);
        template<uint32_t Channel>
        static void GUI_CALL getCameraTargetCB(void* pVal, void* pUserData);

        template<uint32_t Channel>
        static void GUI_CALL setCameraUpCB(const void* pVal, void* pUserData);
        template<uint32_t Channel>
        static void GUI_CALL getCameraUpCB(void* pVal, void* pUserData);

        static void GUI_CALL saveSceneCB(void* pUserData);

        static void pathEditorFinishedCB(void* pUserData);

        void addModel();
        void deleteModel();
        void addModelInstance();
        void deleteModelInstance();

        void addCamera();
        void deleteCamera();

        void addPath();
        void deletePath();
        void editPath();

        void setModelAnimationUI();

        void detachPath();
        void attachPath();

        uint32_t mModelLoadFlags = 0;
		uint32_t mSceneLoadFlags = 0;
    };
}