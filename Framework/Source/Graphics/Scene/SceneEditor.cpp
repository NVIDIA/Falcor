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
        const char* kActiveModelStr = "Active Model";
        const char* kModelsStr = "Models";
        const char* kActiveInstanceStr= "Active Instance";
        const char* kActiveAnimationStr = "Active Animation";
        const char* kModelNameStr ="Model Name";
        const char* kInstanceStr= "Instance";
        const char* kCamerasStr = "Cameras";
        const char* kActiveCameraStr = "Active Camera";
        const char* kPathsStr= "Paths";
        const char* kActivePathStr = "Active Path";
    };

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

    void SceneEditor::selectActiveModel(Gui* pGui)
    {
        Gui::dropdown_list modelList;
        for (uint32_t i = 0; i < mpScene->getModelCount(); i++)
        {
            Gui::DropdownValue value;
            value.label = mpScene->getModel(i)->getName();
            value.value = i;
            modelList.push_back(value);
        }

        if (pGui->addDropdown(kActiveModelStr, modelList, mActiveModel))
        {
            mActiveModelInstance = 0;
        }
    }

    void SceneEditor::setModelName(Gui* pGui)
    {
        char modelName[1024];
        strcpy_s(modelName, mpScene->getModel(mActiveModel)->getName().c_str());
        if (pGui->addTextBox(kModelNameStr, modelName, arraysize(modelName)))
        {
            mpScene->getModel(mActiveModel)->setName(modelName);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setModelVisible(Gui* pGui)
    {
        const Scene::ModelInstance& instance = mpScene->getModelInstance(mActiveModel, mActiveModelInstance);
        bool visible = instance.isVisible;
        if (pGui->addCheckBox("Visible", visible))
        {
            mpScene->setModelInstanceVisible(mActiveModel, mActiveModelInstance, visible);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraFOV(Gui* pGui)
    {
        float fovY = degrees(mpScene->getActiveCamera()->getFovY());
        if(pGui->addFloatVar("FovY", fovY, 0, 360))
        {
            mpScene->getActiveCamera()->setFovY(radians(fovY));
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraAspectRatio(Gui* pGui)
    {
        float aspectRatio = mpScene->getActiveCamera()->getAspectRatio();
        if (pGui->addFloatVar("Aspect Ratio", aspectRatio, 0, FLT_MAX, 0.001f))
        {
            auto pCamera = mpScene->getActiveCamera();
            pCamera->setAspectRatio(aspectRatio);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraDepthRange(Gui* pGui)
    {
        if (pGui->beginGroup("Depth Range"))
        {
            auto pCamera = mpScene->getActiveCamera();
            float nearPlane = pCamera->getNearPlane();
            float farPlane = pCamera->getFarPlane();
            if (pGui->addFloatVar("Near Plane", nearPlane, 0, FLT_MAX, 0.1f) || (pGui->addFloatVar("Far Plane", farPlane, 0, FLT_MAX, 0.1f)))
            {
                pCamera->setDepthRange(nearPlane, farPlane);
                mSceneDirty = true;
            }
            pGui->endGroup();
        }

    }

    void SceneEditor::selectActivePath(Gui* pGui)
    {
        if(mPathEditor.pEditor == nullptr)
        {
            uint32_t activePath = mpScene->getActivePathIndex();
            Gui::dropdown_list pathList = getPathDropdownList(mpScene.get());
            if (pGui->addDropdown(kActivePathStr, pathList, activePath))
            {
                mpScene->setActivePath(activePath);
                mSceneDirty = true;
            }
        }
        else
        {
            std::string msg = kActivePathStr + std::string(": ") + mpScene->getPath(mPathEditor.ActivePath)->getName();
            pGui->addText(msg.c_str());
        }
    }
    
    void SceneEditor::setActiveCamera(Gui* pGui)
    {
        Gui::dropdown_list cameraList;
        for (uint32_t i = 0; i < mpScene->getCameraCount(); i++)
        {
            Gui::DropdownValue value;
            value.label = mpScene->getCamera(i)->getName();
            value.value = i;
            cameraList.push_back(value);
        }

        uint32_t camIndex = mpScene->getActiveCameraIndex();
        if (pGui->addDropdown(kActiveCameraStr, cameraList, camIndex))
        {
            mpScene->setActiveCamera(camIndex);
            if (mPathEditor.pEditor)
            {
                mPathEditor.pEditor->setCamera(mpScene->getActiveCamera());
            }
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraName(Gui* pGui)
    {
        char camName[1024];
        strcpy_s(camName, mpScene->getActiveCamera()->getName().c_str());
        if (pGui->addTextBox("Camera Name", camName, arraysize(camName)))
        {
            mpScene->getActiveCamera()->setName(camName);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraSpeed(Gui* pGui)
    {
        float speed = mpScene->getCameraSpeed();
        if(pGui->addFloatVar("Camera Speed", speed, 0, FLT_MAX, 0.1f))
        {
            mpScene->setCameraSpeed(speed);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setAmbientIntensity(Gui* pGui)
    {
        vec3 ambientIntensity = mpScene->getAmbientIntensity();
        if (pGui->addRgbColor("Ambient intensity", ambientIntensity))
        {
            mpScene->setAmbientIntensity(ambientIntensity);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setInstanceTranslation(Gui* pGui)
    {
        vec3 t = mpScene->getModelInstance(mActiveModel, mActiveModelInstance).translation;
        if (pGui->addFloat3Var("Translation", t, -10000, 10000))
        {
            mpScene->setModelInstanceTranslation(mActiveModel, mActiveModelInstance, t);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setInstanceRotation(Gui* pGui)
    {
        vec3 r = mpScene->getModelInstance(mActiveModel, mActiveModelInstance).rotation;
        r = degrees(r);
        if (pGui->addFloat3Var("Rotation", r, 0, 360))
        {
            r = radians(r);
            mpScene->setModelInstanceRotation(mActiveModel, mActiveModelInstance, r);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setInstanceScaling(Gui* pGui)
    {
        vec3 s = mpScene->getModelInstance(mActiveModel, mActiveModelInstance).scaling;
        if (pGui->addFloat3Var("Scaling", s, -10000, 10000))
        {
            mpScene->setModelInstanceScaling(mActiveModel, mActiveModelInstance, s);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraPosition(Gui* pGui)
    {
        auto& pCamera = mpScene->getActiveCamera();
        glm::vec3 position = pCamera->getPosition();
        if(pGui->addFloat3Var("Position", position, -FLT_MAX, FLT_MAX))
        {
            pCamera->setPosition(position);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraTarget(Gui* pGui)
    {
        auto& pCamera = mpScene->getActiveCamera();
        glm::vec3 target = pCamera->getTarget();
        if (pGui->addFloat3Var("Target", target, -FLT_MAX, FLT_MAX))
        {
            pCamera->setTarget(target);
            mSceneDirty = true;
        }
    }

    void SceneEditor::setCameraUp(Gui* pGui)
    {
        auto& pCamera = mpScene->getActiveCamera();
        glm::vec3 up = pCamera->getUpVector();
        if (pGui->addFloat3Var("Up", up, -FLT_MAX, FLT_MAX))
        {
            pCamera->setUpVector(up);
            mSceneDirty = true;
        }
    }

    void SceneEditor::saveScene()
    {
        std::string filename;
        if(saveFileDialog("Scene files\0*.fscene\0\0", filename))
        {
            SceneExporter::saveScene(filename, mpScene.get());
            mSceneDirty = false;
        }
    }

    void SceneEditor::pathEditorFinishedCB()
    {
        mPathEditor.pEditor = nullptr;
        mpScene->setActivePath(mPathEditor.ActivePath);
    }

    //////////////////////////////////////////////////////////////////////////
    //// End callbacks
    //////////////////////////////////////////////////////////////////////////

    SceneEditor::UniquePtr SceneEditor::create(const Scene::SharedPtr& pScene, const uint32_t modelLoadFlags)
    {
        return UniquePtr(new SceneEditor(pScene, modelLoadFlags));
    }

    SceneEditor::SceneEditor(const Scene::SharedPtr& pScene, uint32_t modelLoadFlags) : mpScene(pScene), mModelLoadFlags(modelLoadFlags) {}

    SceneEditor::~SceneEditor()
    {
        if(mSceneDirty && mpScene)
        {
            if(msgBox("Scene changed. Do you want to save the changes?", MsgBoxType::OkCancel) == MsgBoxButton::Ok)
            {
                saveScene();
            }
        }
    }

    void SceneEditor::renderModelElements(Gui* pGui)
    {
        if(pGui->beginGroup(kModelsStr))
        {
            addModel(pGui);
            if (mpScene->getModelCount())
            {
                deleteModel(pGui);
                if (mpScene->getModelCount() == 0)
                {
                    pGui->endGroup();
                    return;
                }

                pGui->addSeparator();
                selectActiveModel(pGui);
                setModelName(pGui);

                if (pGui->beginGroup(kInstanceStr))
                {
                    addModelInstance(pGui);
                    addModelInstanceRange(pGui);
                    deleteModelInstance(pGui);
                    pGui->addSeparator();
                    setModelVisible(pGui);
                    setInstanceTranslation(pGui);
                    setInstanceRotation(pGui);
                    setInstanceScaling(pGui);

                    pGui->endGroup();
                }

                renderModelAnimation(pGui);
            }
            pGui->endGroup();
        }
    }

    void SceneEditor::renderGlobalElements(Gui* pGui)
    {
        if (pGui->beginGroup("Global Settings"))
        {
            setCameraSpeed(pGui);
            setAmbientIntensity(pGui);
            pGui->endGroup();
        }
    }
    
    void SceneEditor::renderPathElements(Gui* pGui)
    {
        if(pGui->beginGroup(kPathsStr))
        {
            addPath(pGui);
            selectActivePath(pGui);
            startPathEditor(pGui);
            deletePath(pGui);
            pGui->endGroup();
        }
    }

    void SceneEditor::renderCameraElements(Gui* pGui)
    {
        if(pGui->beginGroup(kCamerasStr))
        {
            addCamera(pGui);
            setActiveCamera(pGui);
            setCameraName(pGui);
            deleteCamera(pGui);
            pGui->addSeparator();
            assert(mpScene->getCameraCount() > 0);
            setCameraAspectRatio(pGui);
            setCameraDepthRange(pGui);

            setCameraPosition(pGui);
            setCameraTarget(pGui);
            setCameraUp(pGui);
            
            pGui->endGroup();
        }
    }

    void SceneEditor::renderLightElements(Gui* pGui)
    {
        if(pGui->beginGroup("Lights"))
        {
            for (uint32_t i = 0; i < mpScene->getLightCount(); i++)
            {
                std::string name = mpScene->getLight(i)->getName();
                if (name.size() == 0)
                {
                    name = std::string("Light ") + std::to_string(i);
                }
                if (pGui->beginGroup(name.c_str()))
                {
                    mpScene->getLight(i)->setUiElements(pGui);
                    pGui->endGroup();
                }
            }
            pGui->endGroup();
        }
    }

    void SceneEditor::render(Gui* pGui)
    {
        pGui->pushWindow("Scene Editor", 300, 300, 100, 250);
        if (pGui->addButton("Export Scene"))
        {
            saveScene();
        }
        pGui->addSeparator();
        renderGlobalElements(pGui);
        renderCameraElements(pGui);
        renderPathElements(pGui);
        renderModelElements(pGui);
        renderLightElements(pGui);

        pGui->popWindow();

        if (mPathEditor.pEditor)
        {
            mPathEditor.pEditor->render(pGui);
        }
    }

    void SceneEditor::renderModelAnimation(Gui* pGui)
    {
        const auto pModel = mpScene->getModelCount() ? mpScene->getModel(mActiveModel) : nullptr;

        if (pModel && pModel->hasAnimations())
        {
            Gui::dropdown_list list(pModel->getAnimationsCount() + 1);
            list[0].label = "Bind Pose";
            list[0].value = BIND_POSE_ANIMATION_ID;

            for (uint32_t i = 0; i < pModel->getAnimationsCount(); i++)
            {
                list[i + 1].value = i;
                list[i + 1].label = pModel->getAnimationName(i);
                if (list[i + 1].label.size() == 0)
                {
                    list[i + 1].label = std::to_string(i);
                }
            }
            uint32_t activeAnim = mpScene->getModel(mActiveModel)->getActiveAnimation();
            if (pGui->addDropdown(kActiveAnimationStr, list, activeAnim)) mpScene->getModel(mActiveModel)->setActiveAnimation(activeAnim);
        }
    }
    
    void SceneEditor::addModel(Gui* pGui)
    {
        if (pGui->addButton("Add Model"))
        {
            std::string filename;
            if (openFileDialog(Model::kSupportedFileFormatsStr, filename))
            {
                auto pModel = Model::createFromFile(filename, mModelLoadFlags);
                if (pModel == nullptr)
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
                mActiveModelInstance = 0;
            }
            mSceneDirty = true;
        }
    }

    void SceneEditor::deleteModel()
    {
        mpScene->deleteModel(mActiveModel);
        mActiveModel = 0;
        mActiveModelInstance = 0;
        mSceneDirty = true;
    }

    void SceneEditor::deleteModel(Gui* pGui)
    {
        if(mpScene->getModelCount() > 0)
        {
            if (pGui->addButton("Remove Model"))
            {
                deleteModel();
            }
        }
    }

    void SceneEditor::addModelInstance(Gui* pGui)
    {
        if (pGui->addButton("Add Instance"))
        {
            const auto& Instance = mpScene->getModelInstance(mActiveModel, mActiveModelInstance);
            mActiveModelInstance = mpScene->addModelInstance(mActiveModel, Instance.name + "_0", Instance.rotation, Instance.scaling, Instance.translation);
            mSceneDirty = true;
        }
    }

    void SceneEditor::addModelInstanceRange(Gui* pGui)
    {
        pGui->addIntVar(kActiveInstanceStr, mActiveModelInstance, 0, mpScene->getModelInstanceCount(mActiveModel) - 1);
    }

    void SceneEditor::deleteModelInstance(Gui* pGui)
    {
        if(pGui->addButton("Remove Instance"))
        {
            if (mpScene->getModelInstanceCount(mActiveModel) == 1)
            {
                auto MbRes = msgBox("The active model has a single instance. Removing it will remove the model from the scene.\nContinue?", MsgBoxType::OkCancel);
                if (MbRes == MsgBoxButton::Ok)
                {
                    return deleteModel();
                }
            }

            mpScene->deleteModelInstance(mActiveModel, mActiveModelInstance);
            mActiveModelInstance = 0;
            mSceneDirty = true;
        }
    }

    void SceneEditor::addCamera(Gui* pGui)
    {
        if (pGui->addButton("Add Camera"))
        {
            auto pCamera = Camera::create();
            auto pActiveCamera = mpScene->getActiveCamera();
            *pCamera = *pActiveCamera;
            pCamera->setName(pActiveCamera->getName() + "_");
            uint32_t CamIndex = mpScene->addCamera(pCamera);
            mpScene->setActiveCamera(CamIndex);
            mSceneDirty = true;
        }
    }

    void SceneEditor::deleteCamera(Gui* pGui)
    {
        if (pGui->addButton("Remove Camera"))
        {
            if (mpScene->getCameraCount() == 1)
            {
                msgBox("The Scene has only one camera. Scenes must have at least one camera. Ignoring call.");
                return;
            }

            mpScene->deleteCamera(mpScene->getActiveCameraIndex());
            mSceneDirty = true;
        }
    }

    void SceneEditor::addPath(Gui* pGui)
    {
        if (mPathEditor.pEditor == nullptr && pGui->addButton("Add Path"))
        {
            auto pPath = ObjectPath::create();
            pPath->setName("Path " + std::to_string(mpScene->getPathCount()));
            uint32_t pathIndex = mpScene->addPath(pPath);
            mpScene->setActivePath(pathIndex);

            startPathEditor();
            mSceneDirty = true;
        }
    }

    void SceneEditor::deletePath(Gui* pGui)
    {
        uint32_t pathID = mpScene->getActivePathIndex();
        if(mPathEditor.pEditor || pathID == Scene::kFreeCameraMovement)
        {
            // Can't delete a path while the path editor is opened or if it's a free movement. Please finish editing and try again
            return;
        }

        if (pGui->addButton("Delete Path"))
        {
            mpScene->deletePath(mpScene->getActivePathIndex());
            mSceneDirty = true;
        }
    }

    void SceneEditor::startPathEditor()
    {
        mPathEditor.pEditor = PathEditor::create(mpScene->getActivePath(), mpScene->getActiveCamera(), [this]() {pathEditorFinishedCB(); });
        mSceneDirty = true;
        mpScene->setActivePath(Scene::kFreeCameraMovement);
    }

    void SceneEditor::startPathEditor(Gui* pGui)
    {
        if(mPathEditor.pEditor == nullptr)
        {
            mPathEditor.ActivePath = mpScene->getActivePathIndex();
            if (mPathEditor.ActivePath == Scene::kFreeCameraMovement)
            {
                // Free movement is not a path. Can't edit it
                return;
            }
            if (pGui->addButton("Edit Path"))
            {
                startPathEditor();
            }
        }
    }
}