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
#include "Framework.h"
#include "scene.h"
#include "SceneEditor.h"
#include "Utils/Gui.h"
#include "glm/detail/func_trigonometric.hpp"
#include "Utils/OS.h"
#include "SceneExporter.h"
#include "Graphics/Model/AnimationController.h"
#include "Graphics/Paths/PathEditor.h"

namespace Falcor
{
    namespace
    {
        const std::string kActiveModelStr("Active Model");
        const std::string kModelsStr("Models");
        const std::string kActiveInstanceStr("Active Instance");
        const std::string kActiveAnimationStr("Active Animation");
        const std::string kModelNameStr("Model Name");
        const std::string kInstanceStr("Instance");

        const std::string kCamerasStr("Cameras");
        const std::string kActiveCameraStr("Active Camera");   

        const std::string kPathsStr("Paths");
        const std::string kActivePathStr("Active Path");
    };

    Gui::dropdown_list getModelDropdownList(const Scene* pScene)
    {
        Gui::dropdown_list modelList;
        for(uint32_t i = 0; i < pScene->getModelCount(); i++)
        {
            Gui::DropdownValue value;
            value.label = pScene->getModel(i)->getName();
            value.value = i;
            modelList.push_back(value);
        }
        return modelList;
    }

    Gui::dropdown_list getCameraDropdownList(const Scene* pScene)
    {
        Gui::dropdown_list cameraList;

        for(uint32_t i = 0; i < pScene->getCameraCount(); i++)
        {
            Gui::DropdownValue value;
            value.label = pScene->getCamera(i)->getName();
            value.value = i;
            cameraList.push_back(value);
        }
        return cameraList;
    }

    Gui::dropdown_list getPathDropdownList(const Scene* pScene)
    {
        Gui::dropdown_list pathList;
        static const Gui::DropdownValue kFreeMove{(int)Scene::kFreeCameraMovement, "Free Movement"};
        pathList.push_back(kFreeMove);

        for(uint32_t i = 0; i < pScene->getPathCount(); i++)
        {
            Gui::DropdownValue value;
            value.label = pScene->getPath(i)->getName();
            value.value = i;
            pathList.push_back(value);
        }

        return pathList;
    }
    //////////////////////////////////////////////////////////////////////////
    //// Callbacks
    //////////////////////////////////////////////////////////////////////////
    void SceneEditor::addModelCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->addModel();
    }

    void SceneEditor::deleteModelCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->deleteModel();
    }

    void SceneEditor::addModelInstanceCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->addModelInstance();
    }

    void SceneEditor::getModelActiveAnimationCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        const auto pModel = pEditor->mpScene->getModel(pEditor->mActiveModel);
        *(uint32_t*)pVal = pModel->getActiveAnimation();
    }

    void SceneEditor::setModelActiveAnimationCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pModel = pEditor->mpScene->getModel(pEditor->mActiveModel);
        pModel->setActiveAnimation(*(uint32_t*)pVal);
    }

    void SceneEditor::deleteModelInstanceCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->deleteModelInstance();
    }

    void SceneEditor::getActiveModelCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(int32_t*)pVal = pEditor->mActiveModel;
    }

    void SceneEditor::setActiveModelCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mActiveModelInstance = 0;
        pEditor->mActiveModel = *(int32_t*)pVal;

        pEditor->refreshModelElements();        
    }

    void SceneEditor::getModelNameCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(std::string*)pVal = pEditor->mpScene->getModel(pEditor->mActiveModel)->getName();
    }

    void SceneEditor::setModelNameCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mpScene->getModel(pEditor->mActiveModel)->setName(*(std::string*)pVal);
        pEditor->refreshModelElements();

        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getModelVisibleCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(bool*)pVal = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance).isVisible;
    }

    void SceneEditor::setModelVisibleCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mpScene->setModelInstanceVisible(pEditor->mActiveModel, pEditor->mActiveModelInstance, *(bool*)pVal);
    }

    void SceneEditor::getCameraFOVCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        float fovy = pEditor->mpScene->getActiveCamera()->getFovY();
        fovy = glm::degrees(fovy);
        *(float*)pVal = fovy;
    }

    void SceneEditor::setCameraFOVCB(const void* pVal, void* pUserData)
    {
        float fovy = *(float*)pVal;
        fovy = glm::radians(fovy);

        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mpScene->getActiveCamera()->setFovY(fovy);
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getCameraAspectRatioCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(float*)pVal = pEditor->mpScene->getActiveCamera()->getAspectRatio();
    }

    void SceneEditor::setCameraAspectRatioCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pCamera = pEditor->mpScene->getActiveCamera();
        pCamera->setAspectRatio(*(float*)pVal);
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getCameraFarPlaneCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(float*)pVal = pEditor->mpScene->getActiveCamera()->getFarPlane();
    }

    void SceneEditor::setCameraFarPlaneCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pCamera = pEditor->mpScene->getActiveCamera();
        pCamera->setDepthRange(pCamera->getNearPlane(), *(float*)pVal);
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getCameraNearPlaneCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(float*)pVal = pEditor->mpScene->getActiveCamera()->getNearPlane();
    }

    void SceneEditor::setCameraNearPlaneCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pCamera = pEditor->mpScene->getActiveCamera();
        pCamera->setDepthRange(*(float*)pVal, pCamera->getFarPlane());
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getActivePathCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        if(pEditor->mPathEditor.pEditor)
        {
            *(uint32_t*)pVal = pEditor->mPathEditor.ActivePath;
        }
        else
        {
            *(uint32_t*)pVal = pEditor->mpScene->getActivePathIndex();
        }
    }

    void SceneEditor::setActivePathCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        if(pEditor->mPathEditor.pEditor)
        {
            msgBox("Can't change path while the path editor is opened. Please finish editing and try again.");
            return;
        }

        auto pScene = pEditor->mpScene;

        pScene->setActivePath(*(uint32_t*)pVal);
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getActiveCameraCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(uint32_t*)pVal = pEditor->mpScene->getActiveCameraIndex();
    }

    void SceneEditor::setActiveCameraCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pScene = pEditor->mpScene;

        pScene->setActiveCamera(*(uint32_t*)pVal);
        if(pEditor->mPathEditor.pEditor)
        {
            pEditor->mPathEditor.pEditor->setCamera(pEditor->mpScene->getActiveCamera());
        }
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getCameraNameCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(std::string*)pVal = pEditor->mpScene->getActiveCamera()->getName();
    }

    void SceneEditor::setCameraNameCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mpScene->getActiveCamera()->setName(*(std::string*)pVal);
        pEditor->mSceneDirty = true;
        pEditor->mpGui->setDropdownValues(kActiveCameraStr, kCamerasStr, getCameraDropdownList(pEditor->mpScene.get()));
    }

    void SceneEditor::addCameraCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->addCamera();
    }

    void SceneEditor::deleteCameraCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->deleteCamera();
    }

    void SceneEditor::addPathCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->addPath();
    }

    void SceneEditor::deletePathCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->deletePath();
    }

    void SceneEditor::editPathCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->editPath();
    }

    void SceneEditor::getCameraSpeedCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(float*)pVal = pEditor->mpScene->getCameraSpeed();
    }

    void SceneEditor::setCameraSpeedCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mpScene->setCameraSpeed(*(float*)pVal);
        pEditor->mSceneDirty = true;
    }

    void SceneEditor::getAmbientIntensityCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        *(glm::vec3*)pVal = pEditor->mpScene->getAmbientIntensity();
    }

    void SceneEditor::setAmbientIntensityCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mpScene->setAmbientIntensity(*(glm::vec3*)pVal);
        pEditor->mSceneDirty = true;
    }

    template<uint32_t channel>
    void SceneEditor::setInstanceTranslationCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto& instance = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance);
        glm::vec3 translate = instance.translation;
        translate[channel] = *(float*)pVal;
        pEditor->mpScene->setModelInstanceTranslation(pEditor->mActiveModel, pEditor->mActiveModelInstance, translate);
        pEditor->mSceneDirty = true;
    }

    template<uint32_t channel>
    void SceneEditor::getInstanceTranslationCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto& instance = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance);
        *(float*)pVal = instance.translation[channel];
    }

    template<uint32_t channel>
    void SceneEditor::setInstanceRotationCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto& instance = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance);
        float rad = glm::radians(*(float*)pVal);
        glm::vec3 rotation = instance.rotation;
        rotation[channel] = rad;
        pEditor->mpScene->setModelInstanceRotation(pEditor->mActiveModel, pEditor->mActiveModelInstance, rotation);
        pEditor->mSceneDirty = true;
    }

    template<uint32_t channel>
    void SceneEditor::getInstanceRotationCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto& instance = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance);
        float deg = glm::degrees(instance.rotation[channel]);
        *(float*)pVal = deg;
    }

    template<uint32_t channel>
    void SceneEditor::setInstanceScalingCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto& instance = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance);
        glm::vec3 scale = instance.scaling;
        scale[channel] = *(float*)pVal;
        pEditor->mpScene->setModelInstanceScaling(pEditor->mActiveModel, pEditor->mActiveModelInstance, scale);
        pEditor->mSceneDirty = true;
    }

    template<uint32_t channel>
    void SceneEditor::getInstanceScalingCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto& instance = pEditor->mpScene->getModelInstance(pEditor->mActiveModel, pEditor->mActiveModelInstance);
        *(float*)pVal = instance.scaling[channel];
    }

    template<uint32_t channel>
    void SceneEditor::getCameraPositionCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        const auto pCamera = pEditor->mpScene->getActiveCamera();
        *(float*)pVal = pCamera->getPosition()[channel];
    }

    template<uint32_t channel>
    void SceneEditor::setCameraPositionCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pCamera = pEditor->mpScene->getActiveCamera();
        auto pos = pCamera->getPosition();
        pos[channel] = *(float*)pVal;
        pCamera->setPosition(pos);

        pEditor->mSceneDirty = true;
    }

    template<uint32_t channel>
    void SceneEditor::getCameraTargetCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        const auto pCamera = pEditor->mpScene->getActiveCamera();
        *(float*)pVal = pCamera->getTargetPosition()[channel];
    }

    template<uint32_t channel>
    void SceneEditor::setCameraTargetCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pCamera = pEditor->mpScene->getActiveCamera();
        auto target = pCamera->getTargetPosition();
        target[channel] = *(float*)pVal;
        pCamera->setTarget(target);

        pEditor->mSceneDirty = true;
    }

    template<uint32_t channel>
    void SceneEditor::getCameraUpCB(void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        const auto pCamera = pEditor->mpScene->getActiveCamera();
        *(float*)pVal = pCamera->getUpVector()[channel];
    }

    template<uint32_t channel>
    void SceneEditor::setCameraUpCB(const void* pVal, void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        auto pCamera = pEditor->mpScene->getActiveCamera();
        auto up = pCamera->getUpVector();
        up[channel] = *(float*)pVal;
        pCamera->setUpVector(up);

        pEditor->mSceneDirty = true;
    }

    void SceneEditor::saveSceneCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        std::string filename;
        if(saveFileDialog("Scene files\0*.fscene\0\0", filename))
        {
            SceneExporter::saveScene(filename, pEditor->mpScene.get());
            pEditor->mSceneDirty = false;
        }
    }

    void SceneEditor::pathEditorFinishedCB(void* pUserData)
    {
        SceneEditor* pEditor = (SceneEditor*)pUserData;
        pEditor->mPathEditor.pEditor = nullptr;
        pEditor->mpGui->setDropdownValues(kActivePathStr, kPathsStr, getPathDropdownList(pEditor->mpScene.get()));
        pEditor->mpScene->setActivePath(pEditor->mPathEditor.ActivePath);
    }

    //////////////////////////////////////////////////////////////////////////
    //// End callbacks
    //////////////////////////////////////////////////////////////////////////

    SceneEditor::UniquePtr SceneEditor::create(const Scene::SharedPtr& pScene, const uint32_t ModelLoadFlags)
    {
        return UniquePtr(new SceneEditor(pScene, ModelLoadFlags));
    }

    SceneEditor::SceneEditor(const Scene::SharedPtr& pScene, uint32_t ModelLoadFlags) : mpScene(pScene), mModelLoadFlags(mModelLoadFlags)
    {
        mPathEditor.pEditor = nullptr;
        initializeUI();
    }

    SceneEditor::~SceneEditor()
    {
        if(mSceneDirty && mpScene)
        {
            if(msgBox("Scene changed. Do you want to save the changes?", MsgBoxType::OkCancel) == MsgBoxButton::Ok)
            {
                saveSceneCB(this);
            }
        }
    }

    void SceneEditor::refreshModelElements()
    {
        bool isVisible = (mpScene->getModelCount() > 0);
        
        // set visibility
        mpGui->setVarVisibility(kActiveModelStr, kModelsStr, isVisible);
        mpGui->setVarVisibility(kModelNameStr, kModelsStr, isVisible);
        mpGui->setVarVisibility(kModelNameStr, kModelsStr, isVisible);
        mpGui->setGroupVisibility(kInstanceStr, isVisible);

        setModelAnimationUI();

        if(isVisible)
        {
            mpGui->setDropdownValues(kActiveModelStr, kModelsStr, getModelDropdownList(mpScene.get()));
            mpGui->setVarRange(kActiveInstanceStr, kInstanceStr, 0, mpScene->getModelInstanceCount(mActiveModel) - 1);
        }
    }

    void SceneEditor::setModelElements()
    {
        mpGui->addButton("Add Model", &SceneEditor::addModelCB, this, kModelsStr);
        mpGui->addButton("Remove Model", &SceneEditor::deleteModelCB, this, kModelsStr);
        mpGui->addSeparator(kModelsStr);


        // Set models
        static const Gui::dropdown_list kDummy = {{0, ""}};
        mpGui->addDropdownWithCallback(kActiveModelStr, kDummy, &SceneEditor::setActiveModelCB, &SceneEditor::getActiveModelCB, this, kModelsStr);
        mpGui->addTextBoxWithCallback(kModelNameStr, &SceneEditor::setModelNameCB, &SceneEditor::getModelNameCB, this, kModelsStr);

        mpGui->addButton("Add Instance", &SceneEditor::addModelInstanceCB, this, kInstanceStr);
        mpGui->addIntVar(kActiveInstanceStr, &mActiveModelInstance, kInstanceStr, 0, 0, 1);
        mpGui->addButton("Remove Instance", &SceneEditor::deleteModelInstanceCB, this, kInstanceStr);
        mpGui->addSeparator(kInstanceStr);

        mpGui->addCheckBoxWithCallback("Visible", &SceneEditor::setModelVisibleCB, &SceneEditor::getModelVisibleCB, this, kInstanceStr);

        // Translation
        const std::string kTranslationStr("Translation");
        mpGui->addFloatVarWithCallback("x", &SceneEditor::setInstanceTranslationCB<0>, &SceneEditor::getInstanceTranslationCB<0>, this, kTranslationStr);
        mpGui->addFloatVarWithCallback("y", &SceneEditor::setInstanceTranslationCB<1>, &SceneEditor::getInstanceTranslationCB<1>, this, kTranslationStr);
        mpGui->addFloatVarWithCallback("z", &SceneEditor::setInstanceTranslationCB<2>, &SceneEditor::getInstanceTranslationCB<2>, this, kTranslationStr);
        mpGui->nestGroups(kInstanceStr, kTranslationStr);

        // Rotation
        const std::string kRotationStr("Rotation");
        mpGui->addFloatVarWithCallback("Yaw", &SceneEditor::setInstanceRotationCB<0>, &SceneEditor::getInstanceRotationCB<0>, this, kRotationStr, -360, 360, 0.1f);
        mpGui->addFloatVarWithCallback("Pitch", &SceneEditor::setInstanceRotationCB<1>, &SceneEditor::getInstanceRotationCB<1>, this, kRotationStr, -360, 360, 0.1f);
        mpGui->addFloatVarWithCallback("Roll", &SceneEditor::setInstanceRotationCB<2>, &SceneEditor::getInstanceRotationCB<2>, this, kRotationStr, -360, 360, 0.1f);
        mpGui->nestGroups(kInstanceStr, kRotationStr);

        // Scaling
        const std::string kScalingStr("Scaling");
        mpGui->addFloatVarWithCallback("x", &SceneEditor::setInstanceScalingCB<0>, &SceneEditor::getInstanceScalingCB<0>, this, kScalingStr, 0.001f, FLT_MAX, 0.001f);
        mpGui->addFloatVarWithCallback("y", &SceneEditor::setInstanceScalingCB<1>, &SceneEditor::getInstanceScalingCB<1>, this, kScalingStr, 0.001f, FLT_MAX, 0.001f);
        mpGui->addFloatVarWithCallback("z", &SceneEditor::setInstanceScalingCB<2>, &SceneEditor::getInstanceScalingCB<2>, this, kScalingStr, 0.001f, FLT_MAX, 0.001f);
        mpGui->nestGroups(kInstanceStr, kScalingStr);

        mpGui->nestGroups(kModelsStr, kInstanceStr);

        mpGui->addDropdownWithCallback(kActiveAnimationStr, kDummy, &SceneEditor::setModelActiveAnimationCB, &SceneEditor::getModelActiveAnimationCB, this, kModelsStr);

        mpGui->addSeparator();

        refreshModelElements();
    }

    void SceneEditor::setGlobalElements()
    {
        mpGui->addFloatVarWithCallback("Camera Speed", &SceneEditor::setCameraSpeedCB, &SceneEditor::getCameraSpeedCB, this, "Global Settings", 0.1f, FLT_MAX, 0.1f);
        mpGui->addRgbColorWithCallback("Ambient intensity", &SceneEditor::setAmbientIntensityCB, &SceneEditor::getAmbientIntensityCB, this, "Global Settings");
        mpGui->addSeparator();
    }
    
    void SceneEditor::setPathElements()
    {
        mpGui->addButton("Add Path", &SceneEditor::addPathCB, this, kPathsStr);

        Gui::dropdown_list pathList = getPathDropdownList(mpScene.get());
        mpGui->addDropdownWithCallback(kActivePathStr, pathList, &SceneEditor::setActivePathCB, &SceneEditor::getActivePathCB, this, kPathsStr);

        mpGui->addButton("Edit Path", &SceneEditor::editPathCB, this, kPathsStr);
        mpGui->addButton("Delete Path", &SceneEditor::deletePathCB, this, kPathsStr);
        mpGui->addSeparator();
    }

    void SceneEditor::setCameraElements()
    {
        mpGui->addButton("Add Camera", &SceneEditor::addCameraCB, this, kCamerasStr);

        Gui::dropdown_list cameraList = getCameraDropdownList(mpScene.get());

        mpGui->addDropdownWithCallback(kActiveCameraStr, cameraList, &SceneEditor::setActiveCameraCB, &SceneEditor::getActiveCameraCB, this, kCamerasStr);
        mpGui->addTextBoxWithCallback("Camera Name", &SceneEditor::setCameraNameCB, &SceneEditor::getCameraNameCB, this, kCamerasStr);

        mpGui->addButton("Remove Camera", &SceneEditor::deleteCameraCB, this, kCamerasStr);
        mpGui->addSeparator(kCamerasStr);
        assert(mpScene->getCameraCount() > 0);

        auto pCamera = mpScene->getActiveCamera();

        mpGui->addFloatVarWithCallback("Aspect Ratio", &SceneEditor::setCameraAspectRatioCB, &SceneEditor::getCameraAspectRatioCB, this, kCamerasStr, 0, FLT_MAX, 0.001f);
        mpGui->addFloatVarWithCallback("FOV-Y", &SceneEditor::setCameraFOVCB, &SceneEditor::getCameraFOVCB, this, kCamerasStr, 0, 360, 0.001f);
        mpGui->addFloatVarWithCallback("Near Plane", &SceneEditor::setCameraNearPlaneCB, &SceneEditor::getCameraNearPlaneCB, this, "Depth Range", 0, FLT_MAX, 0.1f);
        mpGui->addFloatVarWithCallback("Far Plane", &SceneEditor::setCameraFarPlaneCB, &SceneEditor::getCameraFarPlaneCB, this, "Depth Range", 0, FLT_MAX, 0.1f);
        mpGui->nestGroups(kCamerasStr, "Depth Range");

        // Translation
        const std::string kPositionStr("Position");
        mpGui->addFloatVarWithCallback("x", &SceneEditor::setCameraPositionCB<0>, &SceneEditor::getCameraPositionCB<0>, this, kPositionStr);
        mpGui->addFloatVarWithCallback("y", &SceneEditor::setCameraPositionCB<1>, &SceneEditor::getCameraPositionCB<1>, this, kPositionStr);
        mpGui->addFloatVarWithCallback("z", &SceneEditor::setCameraPositionCB<2>, &SceneEditor::getCameraPositionCB<2>, this, kPositionStr);
        mpGui->nestGroups(kCamerasStr, kPositionStr);

        // Target
        const std::string kTargetStr("Target");
        mpGui->addFloatVarWithCallback("x", &SceneEditor::setCameraTargetCB<0>, &SceneEditor::getCameraTargetCB<0>, this, kTargetStr);
        mpGui->addFloatVarWithCallback("y", &SceneEditor::setCameraTargetCB<1>, &SceneEditor::getCameraTargetCB<1>, this, kTargetStr);
        mpGui->addFloatVarWithCallback("z", &SceneEditor::setCameraTargetCB<2>, &SceneEditor::getCameraTargetCB<2>, this, kTargetStr);
        mpGui->nestGroups(kCamerasStr, kTargetStr);

        // Up
        const std::string kUpStr("Up");
        mpGui->addFloatVarWithCallback("x", &SceneEditor::setCameraUpCB<0>, &SceneEditor::getCameraUpCB<0>, this, kUpStr);
        mpGui->addFloatVarWithCallback("y", &SceneEditor::setCameraUpCB<1>, &SceneEditor::getCameraUpCB<1>, this, kUpStr);
        mpGui->addFloatVarWithCallback("z", &SceneEditor::setCameraUpCB<2>, &SceneEditor::getCameraUpCB<2>, this, kUpStr);
        mpGui->nestGroups(kCamerasStr, kUpStr);

        mpGui->addSeparator();
    }

    void SceneEditor::setLightElements()
    {
        for(uint32_t i = 0; i < mpScene->getLightCount(); i++)
        {
            std::string name = mpScene->getLight(i)->getName();
            if(name.size() == 0)
            {
                name = std::string("Light ") + std::to_string(i);
            }
            mpScene->getLight(i)->setUiElements(mpGui.get(), name);
            mpGui->nestGroups("Lights", name);
        }

        mpGui->addSeparator();
    }

    void SceneEditor::initializeUI()
    {
        mpGui = Gui::create("Scene Editor");
        mpGui->setSize(350, 300);
        mpGui->setPosition(50, 300);

        mpGui->addButton("Export Scene", &SceneEditor::saveSceneCB, this);
        mpGui->addSeparator();
        setGlobalElements();
        setCameraElements();
        setPathElements();
        setModelElements();
        setLightElements();
    }

    void SceneEditor::setModelAnimationUI()
    {
        const auto pModel = mpScene->getModelCount() ? mpScene->getModel(mActiveModel) : nullptr;

        if(pModel && pModel->hasAnimations())
        {
            Gui::dropdown_list list(pModel->getAnimationsCount() + 1);
            list[0].label = "Bind Pose";
            list[0].value = BIND_POSE_ANIMATION_ID;

            for(uint32_t i = 0; i < pModel->getAnimationsCount(); i++)
            {
                list[i + 1].value = i;
                list[i + 1].label = pModel->getAnimationName(i);
                if(list[i + 1].label.size() == 0)
                {
                    list[i + 1].label = std::to_string(i);
                }
            }
            mpGui->setDropdownValues(kActiveAnimationStr, kModelsStr, list);
            mpGui->setVarVisibility(kActiveAnimationStr, kModelsStr, true);
        }
        else
        {
            mpGui->setVarVisibility(kActiveAnimationStr, kModelsStr, false);
        }
    }

    void SceneEditor::setUiVisible(bool bVisible)
    {
        mpGui->setVisibility(bVisible);
    }
    
    void SceneEditor::addModel()
    {
        std::string filename;
        if(openFileDialog(Model::kSupportedFileFormatsStr, filename))
        {
            auto pModel = Model::createFromFile(filename, mModelLoadFlags);
            if(pModel == nullptr)
            {
                Logger::log(Logger::Level::Error, "Error when trying to load model " + filename);
                return;
            }

            // Get the model name
            size_t offset = filename.find_last_of("/\\");
            std::string modelName = (offset == std::string::npos) ? filename : filename.substr(offset + 1);

            // Remove the last extension
            offset = modelName.find_last_of(".");
            modelName = (offset == std::string::npos) ? modelName : modelName.substr(0, offset);

            pModel->setName(modelName);
            mActiveModel = mpScene->addModel(pModel, filename, true);

            refreshModelElements();
            setActiveModelCB(&mActiveModel, this);
        }
        mSceneDirty = true;
    }

    void SceneEditor::deleteModel()
    {
        if(mpScene->getModelCount() > 0)
        {
            mpScene->deleteModel(mActiveModel);
            mActiveModel = 0;
            mSceneDirty = true;
            refreshModelElements();
            setActiveModelCB(&mActiveModel, this);
        }
    }

    void SceneEditor::addModelInstance()
    {
        const auto& Instance = mpScene->getModelInstance(mActiveModel, mActiveModelInstance);
        mActiveModelInstance = mpScene->addModelInstance(mActiveModel, Instance.name + "_0", Instance.rotation, Instance.scaling, Instance.translation);
        refreshModelElements();

        mSceneDirty = true;
    }

    void SceneEditor::deleteModelInstance()
    {
        if(mpScene->getModelInstanceCount(mActiveModel) == 1)
        {
            auto MbRes = msgBox("The active model has a single instance. Removing it will remove the model from the scene.\nContiune?", MsgBoxType::OkCancel);
            if(MbRes == MsgBoxButton::Ok)
            {
                return deleteModel();
            }
        }

        mpScene->deleteModelInstance(mActiveModel, mActiveModelInstance);
        refreshModelElements();
        mActiveModelInstance = 0;
        mSceneDirty = true;
    }

    void SceneEditor::addCamera()
    {
        auto pCamera = Camera::create();
        auto pActiveCamera = mpScene->getActiveCamera();
        *pCamera = *pActiveCamera;
        pCamera->setName(pActiveCamera->getName() + "_");
        uint32_t CamIndex = mpScene->addCamera(pCamera);
        mpScene->setActiveCamera(CamIndex);

        mpGui->setDropdownValues(kActiveCameraStr, kCamerasStr, getCameraDropdownList(mpScene.get()));
        mSceneDirty = true;
    }

    void SceneEditor::deleteCamera()
    {
        if(mpScene->getCameraCount() == 1)
        {
            msgBox("The Scene has only one camera. Scenes must have at least one camera. Ignoring call.");
            return;
        }

        mpScene->deleteCamera(mpScene->getActiveCameraIndex());
        mpGui->setDropdownValues(kActiveCameraStr, kCamerasStr, getCameraDropdownList(mpScene.get()));
        mSceneDirty = true;
    }

    void SceneEditor::addPath()
    {
        if(mPathEditor.pEditor)
        {
            msgBox("Can't add a path while the path editor is opened. Please finish editing and try again.");
            return;
        }

        auto pPath = ObjectPath::create();
        pPath->setName("Path " + std::to_string(mpScene->getPathCount()));
        uint32_t pathIndex = mpScene->addPath(pPath);
        mpScene->setActivePath(pathIndex);

        mpGui->setDropdownValues(kActivePathStr, kPathsStr, getPathDropdownList(mpScene.get()));

        editPath();
        mSceneDirty = true;
    }

    void SceneEditor::deletePath()
    {
        if(mPathEditor.pEditor)
        {
            msgBox("Can't delete a path while the path editor is opened. Please finish editing and try again.");
            return;
        }

        uint32_t pathID = mpScene->getActivePathIndex();
        if(pathID == Scene::kFreeCameraMovement)
        {
            msgBox("Free movement is not a path. Can't delete it.");
            return;
        }

        mpScene->deletePath(mpScene->getActivePathIndex());
        mpGui->setDropdownValues(kActivePathStr, kPathsStr, getPathDropdownList(mpScene.get()));
        mSceneDirty = true;
    }

    void SceneEditor::editPath()
    {
        if(mPathEditor.pEditor)
        {
            // No need to do anything, since user can't change a path while editing is in progress.
            return;
        }

        mPathEditor.ActivePath = mpScene->getActivePathIndex();
        if(mPathEditor.ActivePath == Scene::kFreeCameraMovement)
        {
            msgBox("Free movement is not a path. Can't edit it.");
            return;
        }
        mPathEditor.pEditor = PathEditor::create(mpScene->getActivePath(), mpScene->getActiveCamera(), &SceneEditor::pathEditorFinishedCB, this);
        mSceneDirty = true;
        mpScene->setActivePath(Scene::kFreeCameraMovement);
    }
}