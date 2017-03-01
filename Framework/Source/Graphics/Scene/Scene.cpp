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
#include <iostream>
#include "glm/ext.hpp"

namespace Falcor
{
	uint32_t Scene::sSceneCounter = 0;

    const Scene::UserVariable Scene::kInvalidVar;

    const char* Scene::kFileFormatString = "Falcor Scene File\n*.fscene\0\0";

    Scene::SharedPtr Scene::loadFromFile(const std::string& filename, const uint32_t& modelLoadFlags, uint32_t sceneLoadFlags)
    {
        return SceneImporter::loadScene(filename, modelLoadFlags, sceneLoadFlags);
    }

    Scene::SharedPtr Scene::create(float cameraAspectRatio)
    {
        return SharedPtr(new Scene(cameraAspectRatio));
    }

	Scene::Scene(float cameraAspectRatio) : mId(sSceneCounter++)
    {
        // Reset all global id counters recursively
        Model::resetGlobalIdCounter();
        Light::resetGlobalIdCounter();
        // create a default camera
        auto pCamera = Camera::create();
        pCamera->setAspectRatio(cameraAspectRatio);
        pCamera->setName("Default");
        addCamera(pCamera);
        mDirty = true;
    }

    Scene::~Scene() = default;

    bool Scene::update(double currentTime, CameraController* cameraController)
    {
        auto pCamera = getActiveCamera();
        auto pActivePath = getActivePath();

        bool bChanged = false;

        if(pActivePath)
        {
            pActivePath->animate(currentTime);
            bChanged = true;
        }
        // Ignore the elapsed time we got from the user. This will allow camera movement in cases where the time is frozen
        if(cameraController)
        {
            cameraController->attachCamera(pCamera);
            cameraController->setCameraSpeed(getCameraSpeed());
            return cameraController->update();
        }

        // Recompute scene extents if dirty
        if(mDirty)
        {
            mDirty = false;

            mRadius = 0.f;
            float k = 0.f;
            mCenter = vec3(0, 0, 0);
            for(uint32_t i = 0; i<getModelCount(); ++i)
            {
                const auto& model = getModel(i);
                const float r = model->getRadius();
                const vec3 c = model->getCenter();
                for(uint32_t j = 0; j<getModelInstanceCount(i); ++j)
                {
                    const auto& inst = getModelInstance(i, j);
                    const vec3 instC = vec3(vec4(c, 1.f) * inst.transformMatrix);
                    const float instR = r * max(inst.scaling.x, max(inst.scaling.y, inst.scaling.z));

                    if(k == 0.f)
                    {
                        mCenter = instC;
                        mRadius = instR;
                    }
                    else
                    {
                        vec3 dir = instC - mCenter;
                        if(length(dir) > 1e-6f)
                            dir = normalize(dir);
                        vec3 a = mCenter - dir * mRadius;
                        vec3 b = instC + dir * instR;

                        mCenter = (a + b) * 0.5f;
                        mRadius = length(a - b);
                    }
                    k++;
                }
            }

            // Update light extents
            for(auto& light : mpLights)
            {
                if(light->getType() == LightDirectional)
                {
                    auto pDirLight = std::dynamic_pointer_cast<DirectionalLight>(light);
                    pDirLight->setWorldParams(getCenter(), getRadius());
                }
            }

            bChanged = true;
        }

        return bChanged;
    }

    vec3 Scene::getCenter()
    {
        if(mDirty)
            update(0.f);
        return mCenter;
    }

    float Scene::getRadius()
    {
        if(mDirty)
            update(0.f);
        return mRadius;
    }

    uint32_t Scene::addModelInstance(uint32_t modelID, const std::string& name, const glm::vec3& rotate, const glm::vec3& scale, const glm::vec3& translate)
    {
        ModelInstance instance;
        instance.scaling = scale;
        instance.rotation = rotate;
        instance.translation = translate;
        instance.name = name;
        mModels[modelID].instances.push_back(instance);
        calculateModelInstanceMatrix(modelID, (uint32_t)mModels[modelID].instances.size() - 1);

        mDirty = true;
        return (uint32_t)mModels[modelID].instances.size() - 1;
    }

    void Scene::deleteModelInstance(uint32_t modelID, uint32_t instanceID)
    {
        auto& instances = mModels[modelID].instances;
        instances.erase(instances.begin() + instanceID);
        mDirty = true;
    }

    void Scene::calculateModelInstanceMatrix(uint32_t modelID, uint32_t instanceID)
    {
        ModelInstance& instance = mModels[modelID].instances[instanceID];
        glm::mat4 translation = glm::translate(glm::mat4(), instance.translation);
        glm::mat4 scaling = glm::scale(glm::mat4(), instance.scaling);
        glm::mat4 rotation = glm::yawPitchRoll(instance.rotation[0], instance.rotation[1], instance.rotation[2]);

        instance.transformMatrix = translation * scaling * rotation;
    }

    const Scene::UserVariable& Scene::getUserVariable(const std::string& name)
    {
        const auto& a = mUserVars.find(name);
        if(a == mUserVars.end())
        {
            Logger::log(Logger::Level::Warning, "Can't find user variable " + name + " in scene.");
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

    uint32_t Scene::addModel(const Model::SharedPtr& pModel, const std::string& filename, bool createIdentityInstance)
    {
        mModels.push_back(ModelData(pModel, filename)); 
		uint32_t modelID = (uint32_t)mModels.size() - 1;
		if (createIdentityInstance)
		{
			addModelInstance(modelID, pModel->getName(), vec3(0.0), vec3(1.0), vec3(0.0));
		}
        mDirty = true;
        return modelID;
    }

    void Scene::deleteModel(uint32_t modelID)
    {
        mModels.erase(mModels.begin() + modelID);
        mDirty = true;
    }

    uint32_t Scene::addLight(const Light::SharedPtr& pLight)
    {
        mpLights.push_back(pLight);
        if(pLight->getType() == LightDirectional)
        {
            auto pDirLight = std::dynamic_pointer_cast<DirectionalLight>(pLight);
            pDirLight->setWorldParams(getCenter(), getRadius());
        }
        mDirty = true;
        return (uint32_t)mpLights.size() - 1;
    }

    void Scene::deleteLight(uint32_t lightID)
    {
        mpLights.erase(mpLights.begin() + lightID);
        mDirty = true;
    }

    uint32_t Scene::addPath(const ObjectPath::SharedPtr& pPath) 
    { 
        mpPaths.push_back(pPath); 
        return (uint32_t)mpPaths.size() - 1; 
    }

    void Scene::deletePath(uint32_t pathID)
    {
        mpPaths.erase(mpPaths.begin() + pathID);
        if(mActivePathID == pathID)
        {
            // No need to detach the camera from the path, since the path is destroyed anyway
            mActivePathID = mpPaths.size() ? 0 : kFreeCameraMovement;
            attachActiveCameraToPath();
        }
    }

    uint32_t Scene::addCamera(const Camera::SharedPtr& pCamera)
    {
        mCameras.push_back(pCamera); 
        return (uint32_t)mCameras.size() - 1;
    }

    void Scene::deleteCamera(uint32_t cameraID)
    {
        if(cameraID == mActiveCameraID)
        {
            detachActiveCameraFromPath();
        }

        mCameras.erase(mCameras.begin() + cameraID);

        if(cameraID == mActiveCameraID)
        {
            // Don't call SetActiveCamera(), since it will try to detach the current active camera, which might be invalid
            mActiveCameraID = 0;
            attachActiveCameraToPath();
        }
    }

    void Scene::setModelInstanceTranslation(uint32_t modelID, uint32_t instanceID, const glm::vec3& translation)
    {
        mModels[modelID].instances[instanceID].translation = translation;
        calculateModelInstanceMatrix(modelID, instanceID);
        mDirty = true;
    }

    void Scene::setModelInstanceRotation(uint32_t modelID, uint32_t instanceID, const glm::vec3& rotation)
    {
        mModels[modelID].instances[instanceID].rotation = rotation;
        calculateModelInstanceMatrix(modelID, instanceID);
        mDirty = true;
    }

    void Scene::setModelInstanceScaling(uint32_t modelID, uint32_t instanceID, const glm::vec3& scaling)
    {
        mModels[modelID].instances[instanceID].scaling = scaling;
        calculateModelInstanceMatrix(modelID, instanceID);
        mDirty = true;
    }

    void Scene::setActiveCamera(uint32_t camID)
    {
        detachActiveCameraFromPath();
        mActiveCameraID = camID;
        attachActiveCameraToPath();
    }

    void Scene::setActivePath(uint32_t pathID)
    {
        detachActiveCameraFromPath();
        mActivePathID = pathID;
        attachActiveCameraToPath();
    }

    void Scene::attachActiveCameraToPath()
    {
        auto pPath = getActivePath();
        if(pPath)
        {
            auto pCamera = getActiveCamera();
            pPath->attachObject(pCamera);
        }
    }

    void Scene::detachActiveCameraFromPath()
    {
        auto pPath = getActivePath();
        if(pPath)
        {
            auto pCamera = getActiveCamera();
            pPath->detachObject(pCamera);
        }
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
        mDirty = true;
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
					// FIXME
					const Scene::ModelInstance& modelInstance = getModelInstance(modelId, modelInstanceId);
					Falcor::createAreaLightsForModel(pModel.get(), mpLights);
                    mDirty = true;
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
        mDirty = true;
    }
}
