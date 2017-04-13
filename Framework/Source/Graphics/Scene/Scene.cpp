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
#include "Scene.h"
#include "SceneImporter.h"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Falcor
{
    uint32_t Scene::sSceneCounter = 0;

    const Scene::UserVariable Scene::kInvalidVar;

    const char* Scene::kFileFormatString = "Scene files\0*.fscene\0\0";

    Scene::SharedPtr Scene::loadFromFile(const std::string& filename, const uint32_t& modelLoadFlags, uint32_t sceneLoadFlags)
    {
        return SceneImporter::loadScene(filename, modelLoadFlags, sceneLoadFlags);
    }

    Scene::SharedPtr Scene::create()
    {
        return SharedPtr(new Scene());
    }

    Scene::Scene() : mId(sSceneCounter++)
    {
        // Reset all global id counters recursively
        Model::resetGlobalIdCounter();
        Light::resetGlobalIdCounter();
    }

    Scene::~Scene() = default;

//     void Scene::updateExtents()
//     {
//         if (mExtentsDirty)
//         {
//             mExtentsDirty = false;
// 
//             mRadius = 0.f;
//             float k = 0.f;
//             mCenter = vec3(0, 0, 0);
//             for (uint32_t i = 0; i < getModelCount(); ++i)
//             {
//                 const auto& model = getModel(i);
//                 const float r = model->getRadius();
//                 const vec3 c = model->getCenter();
//                 for (uint32_t j = 0; j < getModelInstanceCount(i); ++j)
//                 {
//                     const auto& inst = getModelInstance(i, j);
//                     const vec3 instC = vec3(vec4(c, 1.f) * inst.transformMatrix);
//                     const float instR = r * max(inst.scaling.x, max(inst.scaling.y, inst.scaling.z));
// 
//                     if (k == 0.f)
//                     {
//                         mCenter = instC;
//                         mRadius = instR;
//                     }
//                     else
//                     {
//                         vec3 dir = instC - mCenter;
//                         if (length(dir) > 1e-6f)
//                             dir = normalize(dir);
//                         vec3 a = mCenter - dir * mRadius;
//                         vec3 b = instC + dir * instR;
// 
//                         mCenter = (a + b) * 0.5f;
//                         mRadius = length(a - b);
//                     }
//                     k++;
//                 }
//             }
// 
//             // Update light extents
//             for (auto& light : mpLights)
//             {
//                 if (light->getType() == LightDirectional)
//                 {
//                     auto pDirLight = std::dynamic_pointer_cast<DirectionalLight>(light);
//                     pDirLight->setWorldParams(getCenter(), getRadius());
//                 }
//             }
//         }
//     }

    bool Scene::update(double currentTime, CameraController* cameraController)
    {
		updateLights();

        for (auto& path : mpPaths)
        {
            path->animate(currentTime);
        }

        // Ignore the elapsed time we got from the user. This will allow camera movement in cases where the time is frozen
        if(cameraController)
        {
            cameraController->attachCamera(getActiveCamera());
            cameraController->setCameraSpeed(getCameraSpeed());
            return cameraController->update();
        }

        return false;
    }

    void Scene::deleteModel(uint32_t modelID)
    {
        if (mpMaterialHistory != nullptr)
        {
            mpMaterialHistory->onModelRemoved(getModel(modelID).get());
        }

        // Delete entire vector of instances
        mModels.erase(mModels.begin() + modelID);
    }

    void Scene::deleteAllModels()
    {
        mModels.clear();
    }

    uint32_t Scene::getModelInstanceCount(uint32_t modelID) const
    {
        return (uint32_t)(mModels[modelID].size());
    }

    void Scene::addModelInstance(const Model::SharedPtr& pModel, const std::string& instanceName, const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scaling)
    {
        int32_t modelID = -1;

        // Linear search from the end. Instances are usually added in order by model
        for (int32_t i = (int32_t)mModels.size() - 1; i >= 0; i--)
        {
            if (mModels[i][0]->getObject() == pModel)
            {
                modelID = i;
                break;
            }
        }

        // If model not found, new model, add new instance vector
        if (modelID == -1)
        {
            mModels.push_back(ModelInstanceList());
            modelID = (int32_t)mModels.size() - 1;
        }

        mModels[modelID].push_back(ModelInstance::create(pModel, translation, rotation, scaling, instanceName));
    }

    void Scene::addModelInstance(const ModelInstance::SharedPtr& pInstance)
    {
        // Checking for existing instance list for model
        for (uint32_t modelID = 0; modelID < (uint32_t)mModels.size(); modelID++)
        {
            // If found, add to that list
            if (getModel(modelID) == pInstance->getObject())
            {
                mModels[modelID].push_back(pInstance);
                return;
            }
        }

        // If not found, add a new list
        mModels.emplace_back();
        mModels.back().push_back(pInstance);
    }

    void Scene::deleteModelInstance(uint32_t modelID, uint32_t instanceID)
    {
        // Delete instance
        auto& instances = mModels[modelID];

        instances.erase(instances.begin() + instanceID);

        // If no instances are left, delete the vector
        if (instances.empty())
        {
            deleteModel(modelID);
        }
    }

    const Scene::UserVariable& Scene::getUserVariable(const std::string& name)
    {
        const auto& a = mUserVars.find(name);
        if(a == mUserVars.end())
        {
            logWarning("Can't find user variable " + name + " in scene.");
            return kInvalidVar;
        }
        else
        {
            return a->second;
        }
    }

    const Scene::UserVariable& Scene::getUserVariable(uint32_t varID, std::string& varName) const
    {
        for(const auto& a : mUserVars)
        {
            if(varID == 0)
            {
                varName = a.first;
                return a.second;
            }
            --varID;
        }

        should_not_get_here();
        varName = "";
        return mUserVars.begin()->second;
    }

    uint32_t Scene::addLight(const Light::SharedPtr& pLight)
    {
        mpLights.push_back(pLight);
		updateLights();
        return (uint32_t)mpLights.size() - 1;
    }

    void Scene::deleteLight(uint32_t lightID)
    {
        mpLights.erase(mpLights.begin() + lightID);
		updateLights();
    }

	void Scene::updateLights()
	{
		lightEnvironment.setLightList(mpLights);
		lightEnvironment.setAmbient(mAmbientIntensity);
	}

    void Scene::deleteMaterial(uint32_t materialID)
    {
        if (mpMaterialHistory != nullptr)
        {
            mpMaterialHistory->onMaterialRemoved(getMaterial(materialID).get());
        }

        mpMaterials.erase(mpMaterials.begin() + materialID);
    }

    void Scene::enableMaterialHistory()
    {
        if (mpMaterialHistory == nullptr)
        {
            mpMaterialHistory = MaterialHistory::create();
        }
    }

    void Scene::disableMaterialHistory()
    {
        mpMaterialHistory = nullptr;
    }

    uint32_t Scene::addPath(const ObjectPath::SharedPtr& pPath)
    { 
        mpPaths.push_back(pPath); 
        return (uint32_t)mpPaths.size() - 1; 
    }

    void Scene::deletePath(uint32_t pathID)
    {
        mpPaths.erase(mpPaths.begin() + pathID);
    }

    uint32_t Scene::addCamera(const Camera::SharedPtr& pCamera)
    {
        mCameras.push_back(pCamera); 
        return (uint32_t)mCameras.size() - 1;
    }

    void Scene::deleteCamera(uint32_t cameraID)
    {
        mCameras.erase(mCameras.begin() + cameraID);

        if(cameraID == mActiveCameraID)
        {
            mActiveCameraID = 0;
        }
    }

    void Scene::setActiveCamera(uint32_t camID)
    {
        mActiveCameraID = camID;
    }

    void Scene::merge(const Scene* pFrom)
    {
#define merge(name_) name_.insert(name_.end(), pFrom->name_.begin(), pFrom->name_.end());

        merge(mModels);
        merge(mpLights);
        merge(mpPaths);
        merge(mpMaterials);
        merge(mCameras);
#undef merge
        mUserVars.insert(pFrom->mUserVars.begin(), pFrom->mUserVars.end());
    }

    void Scene::createAreaLights()
    {
        // Clean up area light(s) before adding
        deleteAreaLights();

        // Go through all models in the scene
        for (uint32_t modelId = 0; modelId < getModelCount(); ++modelId)
        {
            const Model::SharedPtr& pModel = getModel(modelId);
            if (pModel)
            {
                // Retrieve model instances for this model
                for (uint32_t modelInstanceId = 0; modelInstanceId < getModelInstanceCount(modelId); ++modelInstanceId)
                {
                    // #TODO This should probably create per model instance
                    AreaLight::createAreaLightsForModel(pModel, mpLights);
                }
            }
        }
    }

    void Scene::deleteAreaLights()
    {
        // Clean up the list before adding
        std::vector<Light::SharedPtr>::iterator it = mpLights.begin();

        for (; it != mpLights.end();)
        {
            if ((*it)->getType() == LightArea)
            {
                it = mpLights.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
