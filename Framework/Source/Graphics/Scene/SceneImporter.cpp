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
#include "SceneImporter.h"
#include "Scene.h"
#include "Utils/OS.h"
#include "Externals/RapidJson/include/rapidjson/error/en.h"
#include <sstream>
#include <fstream>
#include "Graphics/TextureHelper.h"
#include "glm/detail/func_trigonometric.hpp"
#include "SceneExportImportCommon.h"
#include "glm/gtx/euler_angles.hpp"

namespace Falcor
{
    template<uint32_t VecSize>
    bool SceneImporter::getFloatVec(const rapidjson::Value& jsonVal, const std::string& desc, float vec[VecSize])
    {
        if(jsonVal.IsArray() == false)
        {
            error("Trying to load a vector for " + desc + ", but JValue is not an array");
            return false;
        }

        if(jsonVal.Size() != VecSize)
        {
            error("Trying to load a vector for " + desc + ", but vector size mismatches. Required size is " + std::to_string(VecSize) + ", array size is " + std::to_string(jsonVal.Size()));
            return false;
        }

        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            if(jsonVal[i].IsNumber() == false)
            {
                error("Trying to load a vector for " + desc + ", but one the elements is not a number.");
                return false;
            }

            vec[i] = (float)(jsonVal[i].GetDouble());
        }
        return true;
    }

    bool SceneImporter::getFloatVecAnySize(const rapidjson::Value& jsonVal, const std::string& desc, std::vector<float>& vec)
    {
        if (jsonVal.IsArray() == false)
        {
            error("Trying to load a vector for " + desc + ", but JValue is not an array");
            return false;
        }

        vec.resize(jsonVal.Size());
        for (uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            if (jsonVal[i].IsNumber() == false)
            {
                error("Trying to load a vector for " + desc + ", but one the elements is not a number.");
                return false;
            }

            vec[i] = (float)(jsonVal[i].GetDouble());
        }
        return true;
    }

    Scene::SharedPtr SceneImporter::loadScene(const std::string& filename, uint32_t modelLoadFlags, uint32_t sceneLoadFlags)
    {
        SceneImporter importer;
        return importer.load(filename, modelLoadFlags, sceneLoadFlags);
    }

    Scene* SceneImporter::error(const std::string& msg)
    {
        std::string err = "Error when parsing scene file \"" + mFilename + "\".\n" + msg;
#if _LOG_ENABLED
        logError(err);
#else
        msgBox(err);
#endif
        mpScene = nullptr;
        return nullptr;
    }

    bool SceneImporter::createModelInstances(const rapidjson::Value& jsonVal, const Model::SharedPtr& pModel)
    {
        if(jsonVal.IsArray() == false)
        {
            error("Model instances should be an array of objects");
            return false;
        }

        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            const auto& instance = jsonVal[i];
            glm::vec3 scaling(1, 1, 1);
            glm::vec3 translation(0, 0, 0);
            glm::vec3 rotation(0, 0, 0);
            std::string name = "Instance " + std::to_string(i);

            for(auto& m = instance.MemberBegin(); m < instance.MemberEnd(); m++)
            {
                std::string key(m->name.GetString());
                if(key == SceneKeys::kName)
                {
                    if(m->value.IsString() == false)
                    {
                        error("Model instance name should be a string value.");
                        return false;
                    }
                    name = std::string(m->value.GetString());
                }
                else if(key == SceneKeys::kTranslationVec)
                {
                    if(getFloatVec<3>(m->value, "Model instance translation vector", &translation[0]) == false)
                    {
                        return false;
                    }
                }
                else if(key == SceneKeys::kScalingVec)
                {
                    if(getFloatVec<3>(m->value, "Model instance scale vector", &scaling[0]) == false)
                    {
                        return false;
                    }
                }
                else if(key == SceneKeys::kRotationVec)
                {
                    if(getFloatVec<3>(m->value, "Model instance rotation vector", &rotation[0]) == false)
                    {
                        return false;
                    }

                    for(uint32_t c = 0; c < 3; c++)
                    {
                        rotation[c] = glm::radians(rotation[c]);
                    }
                }
                else
                {
                    error("Unknown key \"" + key + "\" when parsing model instance");
                    return false;
                }
            }

            if (isNameDuplicate(name, mInstanceMap, "model instances"))
            {
                return false;
            }
            else
            {
                auto pInstance = Scene::ModelInstance::create(pModel, translation, rotation, scaling, name);
                mInstanceMap[pInstance->getName()] = pInstance;
                mpScene->addModelInstance(pInstance);
            }
        }

        return true;
    }

    bool SceneImporter::createModel(const rapidjson::Value& jsonModel)
    {
        // Model must have at least a filename
        if(jsonModel.HasMember(SceneKeys::kFilename) == false)
        {
            error("Model must have a filename");
            return false;
        }
        const auto& modelFile = jsonModel[SceneKeys::kFilename];
        if(modelFile.IsString() == false)
        {
            error("Model filename must be a string");
            return false;
        }

        // Load the model
        auto pModel = Model::createFromFile(modelFile.GetString(), mModelLoadFlags);
        if(pModel == nullptr)
        {
            return false;
        }

        pModel->setFilename(modelFile.GetString());

        bool instanceAdded = false;

        // Loop over the other members
        for(auto& jval = jsonModel.MemberBegin(); jval != jsonModel.MemberEnd(); jval++)
        {
            std::string keyName(jval->name.GetString());
            if(keyName == SceneKeys::kFilename)
            {
                // Already handled
            }
            else if(keyName == SceneKeys::kName)
            {
                if(jval->value.IsString() == false)
                {
                    error("Model name should be a string value.");
                    return false;
                }
                pModel->setName(std::string(jval->value.GetString()));
            }
            else if (keyName == SceneKeys::kMaterialOverrides)
            {
                if (mSceneLoadFlags & Scene::LoadMaterialHistory)
                {
                    if (setMaterialOverrides(jval->value, pModel) == false)
                    {
                        return false;
                    }
                }
            }
            else if(keyName == SceneKeys::kModelInstances)
            {
                if(createModelInstances(jval->value, pModel) == false)
                {
                    return false;
                }

                instanceAdded = true;
            }
            else if(keyName == SceneKeys::kActiveAnimation)
            {
                if(jval->value.IsUint() == false)
                {
                    error("Model active animation should be an unsigned integer");
                    return false;
                }
                uint32_t activeAnimation = jval->value.GetUint();
                if(activeAnimation >= pModel->getAnimationsCount())
                {
                    std::string msg = "Warning when parsing scene file \"" + mFilename + "\".\nModel " + pModel->getName() + " was specified with active animation " + std::to_string(activeAnimation);
                    msg += ", but model only has " + std::to_string(pModel->getAnimationsCount()) + " animations. Ignoring field";
                    logWarning(msg);
                }
                else
                {
                    pModel->setActiveAnimation(activeAnimation);
                }
            }
            else
            {
                error("Invalid key found in models array. Key == " + keyName + ".");
                return false;
            }
        }

        // If no instances for the model were loaded from the scene file
        if (instanceAdded == false)
        {
            mpScene->addModelInstance(pModel, "Instance 0");
        }

        return true;
    }

    bool SceneImporter::setMaterialOverrides(const rapidjson::Value& jsonVal, const Model::SharedPtr& pModel)
    {
        if (jsonVal.IsArray() == false)
        {
            error("Material overrides should be an array of objects");
            return false;
        }

        // For each override. Each object represents one mesh and one material
        for (uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            const auto& meshOverride = jsonVal[i];

            uint32_t meshID = (uint32_t)-1;
            uint32_t materialID = (uint32_t)-1;

            // Read object
            for (auto& it = meshOverride.MemberBegin(); it < meshOverride.MemberEnd(); it++)
            {
                std::string key(it->name.GetString());

                if (key == SceneKeys::kMeshID)
                {
                    meshID = it->value.GetUint();
                }
                else if(key == SceneKeys::kMaterialID)
                {
                    materialID = it->value.GetUint();
                }
                else
                {
                    error("Unknown key \"" + key + "\" when parsing material overrides for model " + pModel->getFilename());
                    return false;
                }
            }

            if (meshID == (uint32_t)-1 || materialID == (uint32_t)-1)
            {
                error("Missing data while parsing when parsing material overrides for model " + pModel->getFilename());
                return false;
            }

            // Apply override
            auto& pMesh = pModel->getMesh(meshID);
            mpScene->getMaterialHistory()->replace(pMesh.get(), mpScene->getMaterial(materialID));
        }

        return true;
    }

    bool SceneImporter::parseModels(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsArray() == false)
        {
            error("models section should be an array of objects.");
            return false;
        }

        // Loop over the array
        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            if(createModel(jsonVal[i]) == false)
            {
                return false;
            }
        }
        return true;
    }

    uint32_t getMaterialLayerType(const std::string& type)
    {
        if(type == SceneKeys::kMaterialLambert)
        {
            return MatLambert;
        }
        else if(type == SceneKeys::kMaterialDielectric)
        {
            return MatDielectric;
        }
        else if(type == SceneKeys::kMaterialConductor)
        {
            return MatConductor;
        }
        else if(type == SceneKeys::kMaterialEmissive)
        {
            return MatEmissive;
        }
        else if(type == SceneKeys::kMaterialUser)
        {
            return MatUser;
        }
        else
        {
            return MatNone;
        }
    }

    uint32_t getMaterialLayerNDF(const std::string& ndf)
    {
        if(ndf == SceneKeys::kMaterialGGX)
        {
            return NDFGGX;
        }
        else if(ndf == SceneKeys::kMaterialBeckmann)
        {
            return NDFBeckmann;
        }
        else if(ndf == SceneKeys::kMaterialUser)
        {
            return NDFUser;
        }
        else
        {
            return -1;
        }
    }

    uint32_t getMaterialLayerBlend(const std::string& blend)
    {
        if(blend == SceneKeys::kMaterialBlendFresnel)
        {
            return BlendFresnel;
        }
        else if(blend == SceneKeys::kMaterialBlendConstant)
        {
            return BlendConstant;
        }
        else if(blend == SceneKeys::kMaterialBlendAdd)
        {
            return BlendAdd;
        }
        else
        {
            return -1;
        }
    }

    bool SceneImporter::createMaterialLayerType(const rapidjson::Value& jsonValue, Material::Layer& layerOut)
    {
        if(jsonValue.IsString() == false)
        {
            error("Material layer Type should be string");
        }

        uint32_t type = getMaterialLayerType(jsonValue.GetString());
        if(type == MatNone)
        {
            error("Unknown material layer Type '" + std::string(jsonValue.GetString()) + "'");
            return false;
        }

        layerOut.type = (Material::Layer::Type)type;
        return true;
    }

    bool SceneImporter::createMaterialLayerNDF(const rapidjson::Value& jsonValue, Material::Layer& layerOut)
    {
        if(jsonValue.IsString() == false)
        {
            error("Material layer NDF should be string");
        }

        uint32_t ndf = getMaterialLayerNDF(jsonValue.GetString());
        if(ndf == -1)
        {
            error("Unknown material layer NDF '" + std::string(jsonValue.GetString()) + "'");
            return false;
        }

        layerOut.ndf = (Material::Layer::NDF)ndf;
        return true;
    }

    bool SceneImporter::createMaterialLayerBlend(const rapidjson::Value& jsonValue, Material::Layer& layerOut)
    {
        if(jsonValue.IsString() == false)
        {
            error("Material layer NDF should be string");
        }

        uint32_t blending = getMaterialLayerBlend(jsonValue.GetString());
        if(blending == -1)
        {
            error("Unknown material layer blending '" + std::string(jsonValue.GetString()) + "'");
            return false;
        }

        layerOut.blend = (Material::Layer::Blend)blending;
        return true;
    }

    bool SceneImporter::createMaterialTexture(const rapidjson::Value& jsonValue, Texture::SharedPtr& pTexture, bool isSrgb)
    {
        if(jsonValue.IsString() == false)
        {
            error("Material texture should be a string");
            return false;
        }

        std::string filename = jsonValue.GetString();
        // Check if the file exists relative to the scene file
        std::string fullpath = mDirectory + "\\" + filename;
        if(doesFileExist(fullpath))
        {
            filename = fullpath;
        }

        pTexture = createTextureFromFile(filename, true, isSrgb);
        return (pTexture != nullptr);
    }

    bool SceneImporter::createMaterialLayer(const rapidjson::Value& jsonLayer, Material::Layer& layerOut)
    {
        if(jsonLayer.IsObject() == false)
        {
            error("Material layer should be an object");
            return false;
        }

        bool bOK = true;
        for(auto& it = jsonLayer.MemberBegin(); (it != jsonLayer.MemberEnd()) && bOK; it++)
        {
            std::string key(it->name.GetString());
            const auto& value = it->value;

            if (key == SceneKeys::kMaterialTexture)
            {
                bOK = createMaterialTexture(value, layerOut.pTexture, true);
            }
            else if(key == SceneKeys::kMaterialLayerType)
            {
                bOK = createMaterialLayerType(value, layerOut);
            }
            else if(key == SceneKeys::kMaterialNDF)
            {
                bOK = createMaterialLayerNDF(value, layerOut);
            }
            else if(key == SceneKeys::kMaterialBlend)
            {
                bOK = createMaterialLayerBlend(value, layerOut);
            }
            else if(key == SceneKeys::kMaterialAlbedo)
            {
                float* pData = reinterpret_cast<float*>(layerOut.albedo.data.data);
                getFloatVec<4>(value, SceneKeys::kMaterialAlbedo, pData);
            }
            else if(key == SceneKeys::kMaterialRoughness)
            {
                float* pData = reinterpret_cast<float*>(layerOut.roughness.data.data);
                getFloatVec<4>(value, SceneKeys::kMaterialRoughness, pData);
            }
            else if(key == SceneKeys::kMaterialExtraParam)
            {
                float* pData = reinterpret_cast<float*>(layerOut.extraParam.data.data);
                getFloatVec<4>(value, "Extra Params", pData);
            }
            else
            {
                bOK = false;
                error("Invalid key found in material layers section. Key == " + key + ".");
            }
    }

        return bOK;
    }

    bool SceneImporter::createAllMaterialLayers(const rapidjson::Value& jsonLayerArray, Material* pMaterial)
    {
        if(jsonLayerArray.IsArray() == false)
        {
            error("Material layers should be array");
            return false;
        }

        if(jsonLayerArray.Size() > MatMaxLayers)
        {
            error("Material has too many layers.");
            return false;
        }

        for(uint32_t i = 0; i < jsonLayerArray.Size(); i++)
        {
            Material::Layer layer;
            if(createMaterialLayer(jsonLayerArray[i], layer) == false)
            {
                return false;
            }

            pMaterial->addLayer(layer);
        }
        return true;
    }

    bool SceneImporter::createMaterial(const rapidjson::Value& jsonMaterial)
    {
        if(jsonMaterial.IsObject() == false)
        {
            error("Material should be an object");
            return false;
        }

        auto pMaterial = Material::create("");
        for(auto& it = jsonMaterial.MemberBegin(); it != jsonMaterial.MemberEnd(); it++)
        {
            std::string key(it->name.GetString());
            const auto& value = it->value;

            if(key == SceneKeys::kName)
            {
                if(value.IsString() == false)
                {
                    error("Material name should be a string");
                    return false;
                }
                pMaterial->setName(value.GetString());
            }
            else if(key == SceneKeys::kID)
            {
                if(value.IsUint() == false)
                {
                    error("Material ID should be an unsigned integer");
                    return false;
                }
                pMaterial->setID(value.GetUint());
            }
            else if (key == SceneKeys::kMaterialDoubleSided)
            {
                if (value.IsBool() == false)
                {
                    error("Material double-sidedness should be a bool");
                    return false;
                }
                pMaterial->setDoubleSided(value.GetBool());
            }
            else if(key == SceneKeys::kMaterialAlpha)
            {
                Texture::SharedPtr pTexture;
                if (createMaterialTexture(value, pTexture, false))
                {
                    pMaterial->setAlphaMap(pTexture);
                }
                else
                {
                    error("Material alpha map could not be loaded");
                    return false;
                }
            }
            else if(key == SceneKeys::kMaterialNormal)
            {
                Texture::SharedPtr pTexture;
                if (createMaterialTexture(value, pTexture, false))
                {
                    pMaterial->setNormalMap(pTexture);
                }
                else
                {
                    error("Material normal map could not be loaded");
                    return false;
                }
            }
            else if (key == SceneKeys::kMaterialHeight)
            {
                Texture::SharedPtr pTexture;
                if (createMaterialTexture(value, pTexture, false))
                {
                    pMaterial->setHeightMap(pTexture);
                }
                else
                {
                    error("Material height map could not be loaded");
                    return false;
                }
            }
            else if (key == SceneKeys::kMaterialAO)
            {
                Texture::SharedPtr pTexture;
                if (createMaterialTexture(value, pTexture, true))
                {
                    pMaterial->setAmbientOcclusionMap(pTexture);
                }
                else
                {
                    error("Material ambient occlusion map could not be loaded");
                    return false;
                }
            }
            else if(key == SceneKeys::kMaterialLayers)
            {
                if(createAllMaterialLayers(value, pMaterial.get()) == false)
                {
                    return false;
                }
            }
            else
            {
                error("Invalid key found in materials section. Key == " + key + ".");
                return false;
            }
        }

        mpScene->addMaterial(pMaterial);
        return true;
    }

    bool SceneImporter::parseMaterials(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsArray() == false)
        {
            error("Materials section should be an array of objects.");
            return false;
        }

        // Loop over the array
        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            if(createMaterial(jsonVal[i]) == false)
            {
                return false;
            }
        }
        return true;
    }

    bool SceneImporter::createDirLight(const rapidjson::Value& jsonLight)
    {
        auto pDirLight = DirectionalLight::create();

        for(auto& it = jsonLight.MemberBegin(); it != jsonLight.MemberEnd(); it++)
        {
            std::string key(it->name.GetString());
            const auto& value = it->value;
            if(key == SceneKeys::kName)
            {
                if(value.IsString() == false)
                {
                    error("Point light name should be a string");
                    return false;
                }
                std::string name = value.GetString();
                if(name.find(' ') != std::string::npos)
                {
                    error("Point light name can't have spaces");
                    return false;
                }
                pDirLight->setName(name);
            }
            else if(key == SceneKeys::kType)
            {
                // Don't care
            }
            else if(key == SceneKeys::kLightIntensity)
            {
                glm::vec3 intensity;
                if(getFloatVec<3>(value, "Directional light intensity", &intensity[0]) == false)
                {
                    return false;
                }
                pDirLight->setIntensity(intensity);
            }
            else if(key == SceneKeys::kLightDirection)
            {
                glm::vec3 direction;
                if(getFloatVec<3>(value, "Directional light intensity", &direction[0]) == false)
                {
                    return false;
                }
                pDirLight->setWorldDirection(direction);
            }
            else
            {
                error("Invalid key found in directional light object. Key == " + key + ".");
                return false;
            }
        }
        mpScene->addLight(pDirLight);
        return true;
    }

    bool SceneImporter::createPointLight(const rapidjson::Value& jsonLight)
    {
        auto pPointLight = PointLight::create();

        for(auto& it = jsonLight.MemberBegin(); it != jsonLight.MemberEnd(); it++)
        {
            std::string key(it->name.GetString());
            const auto& value = it->value;
            if(key == SceneKeys::kName)
            {
                if(value.IsString() == false)
                {
                    error("Dir light name should be a string");
                    return false;
                }
                std::string name = value.GetString();
                if(name.find(' ') != std::string::npos)
                {
                    error("Dir light name can't have spaces");
                    return false;
                }
                pPointLight->setName(name);
            }
            else if(key == SceneKeys::kType)
            {
                // Don't care
            }
            else if(key == SceneKeys::kLightOpeningAngle)
            {
                if(value.IsNumber() == false)
                {
                    error("Camera's FOV should be a number");
                    return false;
                }
                float angle = (float)value.GetDouble();
                // Convert to radiance
                angle = glm::radians(angle);
                pPointLight->setOpeningAngle(angle);
            }
            else if(key == SceneKeys::kLightPenumbraAngle)
            {
                if(value.IsNumber() == false)
                {
                    error("Camera's FOV should be a number");
                    return false;
                }
                float angle = (float)value.GetDouble();
                // Convert to radiance
                angle = glm::radians(angle);
                pPointLight->setPenumbraAngle(angle);
            }
            else if(key == SceneKeys::kLightIntensity)
            {
                glm::vec3 intensity;
                if(getFloatVec<3>(value, "Point light intensity", &intensity[0]) == false)
                {
                    return false;
                }
                pPointLight->setIntensity(intensity);
            }
            else if(key == SceneKeys::kLightPos)
            {
                glm::vec3 position;
                if(getFloatVec<3>(value, "Point light position", &position[0]) == false)
                {
                    return false;
                }
                pPointLight->setWorldPosition(position);
            }
            else if(key == SceneKeys::kLightDirection)
            {
                glm::vec3 dir;
                if(getFloatVec<3>(value, "Point light direction", &dir[0]) == false)
                {
                    return false;
                }
                pPointLight->setWorldDirection(dir);
            }
            else
            {
                error("Invalid key found in point light object. Key == " + key + ".");
                return false;
            }
        }

        if (isNameDuplicate(pPointLight->getName(), mLightMap, "lights"))
        {
            return false;
        }
        else
        {
            mLightMap[pPointLight->getName()] = pPointLight;
            mpScene->addLight(pPointLight);
        }

        return true;
    }

    bool SceneImporter::parseLights(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsArray() == false)
        {
            error("lights section should be an array of objects.");
            return false;
        }

        // Go over all the objects
        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            const auto& light = jsonVal[i];
            const auto& type = light.FindMember(SceneKeys::kType);
            if(type == light.MemberEnd())
            {
                error("Light source must have a type.");
                return false;
            }

            if(type->value.IsString() == false)
            {
                error("Light source Type must be a string.");
                return false;
            }

            std::string lightType(type->value.GetString());
            bool b;
            if(lightType == SceneKeys::kDirLight)
            {
                b = createDirLight(light);
            }
            else if(lightType == SceneKeys::kPointLight)
            {
                b = createPointLight(light);
            }
            else
            {
                error("Unrecognized light Type \"" + lightType + "\"");
            }

            if(b == false)
            {
                return false;
            }
        }

        return true;
    }

    bool SceneImporter::createPathFrames(ObjectPath* pPath, const rapidjson::Value& jsonFramesArray)
    {
        // an array of key frames
        if(jsonFramesArray.IsArray() == false)
        {
            error("Camera path frames should be an array of key-frame objects");
            return nullptr;
        }

        for(uint32_t i = 0; i < jsonFramesArray.Size(); i++)
        {
            float time = 0;
            glm::vec3 pos, target, up;
            for(auto& it = jsonFramesArray[i].MemberBegin(); it < jsonFramesArray[i].MemberEnd(); it++)
            {
                std::string key(it->name.GetString());
                auto& value = it->value;
                bool b = true;
                if(key == SceneKeys::kFrameTime)
                {
                    if(value.IsNumber() == false)
                    {
                        error("Camera path time should be a number");
                        b = false;
                    }

                    time = (float)value.GetDouble();
                }
                else if(key == SceneKeys::kCamPosition)
                {
                    b = getFloatVec<3>(value, "Camera path position", &pos[0]);
                }
                else if(key == SceneKeys::kCamTarget)
                {
                    b = getFloatVec<3>(value, "Camera path target", &target[0]);
                }
                else if(key == SceneKeys::kCamUp)
                {
                    b = getFloatVec<3>(value, "Camera path up vector", &up[0]);
                }

                if(b == false)
                {
                    return false;
                }
            }
            pPath->addKeyFrame(time, pos, target, up);
        }
        return true;
    }

    ObjectPath::SharedPtr SceneImporter::createPath(const rapidjson::Value& jsonPath)
    {
        auto pPath = ObjectPath::create();

        for(auto& it = jsonPath.MemberBegin(); it != jsonPath.MemberEnd(); it++)
        {
            const std::string key(it->name.GetString());
            const auto& value = it->value;

            if(key == SceneKeys::kName)
            {
                if(value.IsString() == false)
                {
                    error("Path name should be a string");
                    return nullptr;
                }

                std::string pathName(value.GetString());
                pPath->setName(pathName);
            }
            else if(key == SceneKeys::kPathLoop)
            {
                if(value.IsBool() == false)
                {
                    error("Path loop should be a boolean value");
                    return nullptr;
                }

                bool b = value.GetBool();
                pPath->setAnimationRepeat(b);
            }
            else if(key == SceneKeys::kPathFrames)
            {
                if(createPathFrames(pPath.get(), value) == false)
                {
                    return false;
                }
            }
            else if (key == SceneKeys::kAttachedObjects)
            {
                if (value.IsArray() == false)
                {
                    error("Path object list should be an array");
                    return nullptr;
                }

                for (uint32_t i = 0; i < value.Size(); i++)
                {
                    std::string type = value[i].FindMember(SceneKeys::kType)->value.GetString();
                    std::string name = value[i].FindMember(SceneKeys::kName)->value.GetString();

                    pPath->attachObject(getMovableObject(type, name));
                }
            }
            else
            {
                error("Unknown token \"" + key + "\" when parsing camera path");
                return nullptr;
            }
        }
        return pPath;
    }

    bool SceneImporter::parsePaths(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsArray() == false)
        {
            error("Paths should be an array");
            return false;
        }

        for(uint32_t PathID = 0; PathID < jsonVal.Size(); PathID++)
        {
            auto pPath = createPath(jsonVal[PathID]);
            if(pPath)
            {
                mpScene->addPath(pPath);
            }
            else
            {
                return false;
            }
        }
        return true;
    }
        
    bool SceneImporter::parseActivePath(const rapidjson::Value& jsonVal)
    {
        if (mpScene->getVersion() != 1)
        {
            return true;
        }

        // Paths should already be initialized at this stage
        if(jsonVal.IsString() == false)
        {
            error("Active path should be a string.");
            return false;
        }

        std::string activePath = jsonVal.GetString();

        // Find the path
        for(uint32_t i = 0; i < mpScene->getPathCount(); i++)
        {
            if(activePath == mpScene->getPath(i)->getName())
            {
                mpScene->getPath(i)->attachObject(mpScene->getActiveCamera());
                return true;
            }
        }

        error("Active path \"" + activePath + "\" not found." );
        return false;

    }

    bool SceneImporter::createCamera(const rapidjson::Value& jsonCamera)
    {
        auto pCamera = Camera::create();
        std::string activePath;

        // Go over all the keys
        for(auto& it = jsonCamera.MemberBegin(); it != jsonCamera.MemberEnd(); it++)
        {
            std::string key(it->name.GetString());
            const auto& value = it->value;
            if(key == SceneKeys::kName)
            {
                // Name
                if(value.IsString() == false)
                {
                    error("Camera name should be a string value");
                    return false;
                }
                pCamera->setName(value.GetString());
            }
            else if(key == SceneKeys::kCamPosition)
            {
                glm::vec3 pos;
                if(getFloatVec<3>(value, "Camera's position", &pos[0]) == false)
                {
                    return false;
                }
                pCamera->setPosition(pos);
            }
            else if(key == SceneKeys::kCamTarget)
            {
                glm::vec3 target;
                if(getFloatVec<3>(value, "Camera's target", &target[0]) == false)
                {
                    return false;
                }
                pCamera->setTarget(target);
            }
            else if(key == SceneKeys::kCamUp)
            {
                glm::vec3 up;
                if(getFloatVec<3>(value, "Camera's up vector", &up[0]) == false)
                {
                    return false;
                }
                pCamera->setUpVector(up);
            }
            else if(key == SceneKeys::kCamFovY)
            {
                if(value.IsNumber() == false)
                {
                    error("Camera's FOV should be a number");
                    return false;
                }
                float fovY = (float)value.GetDouble();
                // Convert to radiance
                fovY = glm::radians(fovY);
                pCamera->setFovY(fovY);
            }
            else if(key == SceneKeys::kCamDepthRange)
            {
                float depthRange[2];
                if(getFloatVec<2>(value, "Camera's depth-range", depthRange) == false)
                {
                    return false;
                }
                pCamera->setDepthRange(depthRange[0], depthRange[1]);
            }
            else if(key == SceneKeys::kCamAspectRatio)
            {
                if(value.IsNumber() == false)
                {
                    error("Camera's aspect ratio should be a number");
                    return false;
                }
                pCamera->setAspectRatio((float)value.GetDouble());
            }
            else
            {
                error("Invalid key found in cameras array. Key == " + key + ".");
                return false;
            }
        }

        if (isNameDuplicate(pCamera->getName(), mCameraMap, "cameras"))
        {
            return false;
        }
        else
        {
            mCameraMap[pCamera->getName()] = pCamera;
            mpScene->addCamera(pCamera);
        }

        return true;
    }

    bool SceneImporter::parseCameras(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsArray() == false)
        {
            error("cameras section should be an array of objects.");
            return false;
        }

        // New scenes are created with a default camera. Delete it.
        mpScene->deleteCamera(0);

        // Go over all the objects
        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            if(createCamera(jsonVal[i]) == false)
            {
                return false;
            }
        }

        // Set the active camera to camera 0.
        mpScene->setActiveCamera(0);

        return true;
    }


    Scene::SharedPtr SceneImporter::load(const std::string& filename, const uint32_t& modelLoadFlags, uint32_t sceneLoadFlags)
    {
        std::string fullpath;
        mFilename = filename;
        mModelLoadFlags = modelLoadFlags;
        mSceneLoadFlags = sceneLoadFlags;

        if(findFileInDataDirectories(filename, fullpath))
        {
            // Load the file
            std::ifstream fileStream(fullpath);
            std::stringstream strStream;
            strStream << fileStream.rdbuf();
            std::string jsonData = strStream.str();
            rapidjson::StringStream JStream(jsonData.c_str());

            // Get the file directory
            auto last = fullpath.find_last_of("/\\");
            mDirectory = fullpath.substr(0, last);

            // create the DOM
            mJDoc.ParseStream(JStream);

            if(mJDoc.HasParseError())
            {
                size_t line;
                line = std::count(jsonData.begin(), jsonData.begin() + mJDoc.GetErrorOffset(), '\n');
                error(std::string("JSON Parse error in line ") + std::to_string(line) + ". " + rapidjson::GetParseError_En(mJDoc.GetParseError()));
                return nullptr;
            }

            // create the scene
            mpScene = Scene::create();

            if (mSceneLoadFlags & Scene::LoadMaterialHistory)
            {
                mpScene->enableMaterialHistory();
            }

            if(topLevelLoop() == false)
            {
                return nullptr;
            }

            if (mSceneLoadFlags & Scene::GenerateAreaLights)
            {
                // Create area light(s) in the scene
                mpScene->createAreaLights();
            }
            
            return mpScene;
        }
        else
        {
            error("File not found.");
            return nullptr;
        }
    }

    bool SceneImporter::parseAmbientIntensity(const rapidjson::Value& jsonVal)
    {
        glm::vec3 ambient;
        if(getFloatVec<3>(jsonVal, SceneKeys::kAmbientIntensity, &ambient[0]))
        {
            mpScene->setAmbientIntensity(ambient);
            return true;
        }
        return false;
    }

    bool SceneImporter::parseLightingScale(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsNumber() == false)
        {
            error("Lighting scale should be a number.");
            return false;
        }

        float f = (float)(jsonVal.GetDouble());
        mpScene->setLightingScale(f);
        return true;
    }

    bool SceneImporter::parseCameraSpeed(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsNumber() == false)
        {
            error("Camera speed should be a number.");
            return false;
        }

        float f = (float)(jsonVal.GetDouble());
        mpScene->setCameraSpeed(f);
        return true;
    }

    bool SceneImporter::parseActiveCamera(const rapidjson::Value& jsonVal)
    {
        // Cameras should already be initialized at this stage
        if(jsonVal.IsString() == false)
        {
            error("Active camera should be a string.");
            return false;
        }

        std::string activeCamera = jsonVal.GetString();

        // Find the camera
        for(uint32_t i = 0; i < mpScene->getCameraCount(); i++)
        {
            if(activeCamera == mpScene->getCamera(i)->getName())
            {
                mpScene->setActiveCamera(i);
                return true;
            }
        }

        error("Active camera \"" + activeCamera + "\" not found.");
        return false;
    }

    bool SceneImporter::parseVersion(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsUint() == false)
        {
            error("value should be an unsigned integer number");
            return false;
        }
        mpScene->setVersion(jsonVal.GetUint());
        return true;
    }

    bool SceneImporter::parseUserDefinedSection(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsObject() == false)
        {
            error("User defined section should be a JSON object.");
            return false;
        }

        for(auto& it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
        {
            bool b;
            Scene::UserVariable userVar;
            std::string name(it->name.GetString());
            const auto& value = it->value;
            // Check if this is a vector
            if(value.IsArray())
            {
                for(uint32_t i = 0; i < value.Size(); i++)
                {
                    if(value[i].IsNumber() == false)
                    {
                        error("User defined section contains an array, but some of the elements are not numbers.");
                        return false;
                    }
                }

                switch(value.Size())
                {
                case 2:
                    userVar.type = Scene::UserVariable::Type::Vec2;
                    b = getFloatVec<2>(value, "custom-field vec2", &userVar.vec2[0]);
                    break;
                case 3:
                    userVar.type = Scene::UserVariable::Type::Vec3;
                    b = getFloatVec<3>(value, "custom-field vec3", &userVar.vec3[0]);
                    break;
                case 4:
                    userVar.type = Scene::UserVariable::Type::Vec4;
                    b = getFloatVec<4>(value, "custom-field vec4", &userVar.vec4[0]);
                    break;
                default:
                    userVar.type = Scene::UserVariable::Type::Vector;
                    b = getFloatVecAnySize(value, "vector of floats", userVar.vector);
                    break;
                }
                if(b == false)
                {
                    return false;
                }
            }
            else
            {
                // Not an array. Must be a literal
                // The way rapidjson works, a uint is also an int, and a 32-bit number is also a 64-bit number, so the order in which we check the Type matters
                if(value.IsUint())
                {
                    userVar.type = Scene::UserVariable::Type::Uint;
                    userVar.u32 = value.GetUint();
                }
                else if(value.IsInt())
                {
                    userVar.type = Scene::UserVariable::Type::Int;
                    userVar.i32 = value.GetInt();
                }
                else if(value.IsUint64())
                {
                    userVar.type = Scene::UserVariable::Type::Uint64;
                    userVar.u64 = value.GetUint64();
                }
                else if(value.IsInt64())
                {
                    userVar.type = Scene::UserVariable::Type::Int64;
                    userVar.i64 = value.GetInt64();
                }
                else if(value.IsDouble())
                {
                    userVar.type = Scene::UserVariable::Type::Double;
                    userVar.d64 = value.GetDouble();
                }
                else if(value.IsString())
                {
                    userVar.type = Scene::UserVariable::Type::String;
                    userVar.str = value.GetString();
                }
                else if(value.IsBool())
                {
                    userVar.type = Scene::UserVariable::Type::Bool;
                    userVar.b = value.GetBool();
                }
                else
                {
                    error("Error when parsing custom-field \"" + name + "\". Field Type invalid. Must be a literal number, string boolean or an array of 2/3/4 numbers.");
                    return false;
                }
            }
            mpScene->addUserVariable(name, userVar);
        }
        return true;
    }

    bool SceneImporter::loadIncludeFile(const std::string& include)
    {
        // Find the file
        std::string fullpath = mDirectory + '\\' + include;
        if(doesFileExist(fullpath) == false)
        {
            // Look in the data directories
            if(findFileInDataDirectories(include, fullpath) == false)
            {
                error("Can't find include file " + include);
                return false;
            }
        }

        Scene::SharedPtr pScene = SceneImporter::loadScene(fullpath, mModelLoadFlags, mSceneLoadFlags);
        if(pScene == nullptr)
        {
            return false;
        }
        mpScene->merge(pScene.get());

        return true;
    }

    bool SceneImporter::parseIncludes(const rapidjson::Value& jsonVal)
    {
        if(jsonVal.IsArray() == false)
        {
            error("Include section should be an array of strings");
        }

        for(uint32_t i = 0; i < jsonVal.Size(); i++)
        {
            if(jsonVal[i].IsString() == false)
            {
                error("Include element should be a string");
                return false;
            }

            const std::string include = jsonVal[i].GetString();
            if(loadIncludeFile(include) == false)
            {
                return false;
            }
        }
        return true;
    }

    bool SceneImporter::isNameDuplicate(const std::string& name, const ObjectMap& objectMap, const std::string& objectType) const
    {
        if (objectMap.find(name) != objectMap.end())
        {
            const std::string msg = "Multiple " + objectType + " found the same name: " + name + ".\nObjects may not attach to paths correctly.\n\nContinue anyway?";

            // If user pressed ok, return false to ignore duplicate
            return msgBox(msg, MsgBoxType::OkCancel) == MsgBoxButton::Ok ? false : true;
        }

        return false;
    }

    IMovableObject::SharedPtr SceneImporter::getMovableObject(const std::string& type, const std::string& name) const
    {
        if (type == SceneKeys::kModelInstance)
        {
            return mInstanceMap.find(name)->second;
        }
        else if (type == SceneKeys::kCamera)
        {
            return mCameraMap.find(name)->second;
        }
        else if (type == SceneKeys::kLight)
        {
            return mLightMap.find(name)->second;
        }

        should_not_get_here();
        return nullptr;
    }

    const SceneImporter::FuncValue SceneImporter::kFunctionTable[] =
    {
        // The order matters here.
        {SceneKeys::kVersion, &SceneImporter::parseVersion},
        {SceneKeys::kAmbientIntensity, &SceneImporter::parseAmbientIntensity},
        {SceneKeys::kLightingScale, &SceneImporter::parseLightingScale},
        {SceneKeys::kCameraSpeed, &SceneImporter::parseCameraSpeed},

        {SceneKeys::kMaterials, &SceneImporter::parseMaterials},
        {SceneKeys::kModels, &SceneImporter::parseModels},
        {SceneKeys::kLights, &SceneImporter::parseLights},
        {SceneKeys::kCameras, &SceneImporter::parseCameras},
        {SceneKeys::kActiveCamera, &SceneImporter::parseActiveCamera},  // Should come after ParseCameras
        {SceneKeys::kUserDefined, &SceneImporter::parseUserDefinedSection},

        {SceneKeys::kPaths, &SceneImporter::parsePaths},
        {SceneKeys::kActivePath, &SceneImporter::parseActivePath}, // Should come after ParsePaths
        {SceneKeys::kInclude, &SceneImporter::parseIncludes}
    };

    bool SceneImporter::validateSceneFile()
    {
        // Make sure the top-level is valid
        for(auto& it = mJDoc.MemberBegin(); it != mJDoc.MemberEnd(); it++)
        {
            bool found = false;
            const std::string name(it->name.GetString());

            for(uint32_t i = 0; i < arraysize(kFunctionTable); i++)
            {
                // Check that we support this value
                if(kFunctionTable[i].token == name)
                {
                    found = true;
                    break;
                }
            }

            if(found == false)
            {
                error("Invalid key found in top-level object. Key == " + std::string(it->name.GetString()) + ".");
                return false;
            }
        }
        return true;
    }

    bool SceneImporter::topLevelLoop()
    {
        if(validateSceneFile() == false)
        {
            return false;
        }

        for(uint32_t i = 0; i < arraysize(kFunctionTable); i++)
        {
            const auto& jsonMember = mJDoc.FindMember(kFunctionTable[i].token.c_str());
            if(jsonMember != mJDoc.MemberEnd())
            {
                auto a = kFunctionTable[i].func;
                if((this->*a)(jsonMember->value) == false)
                {
                    return false;
                }
            }
        }

        return true;
    }
}