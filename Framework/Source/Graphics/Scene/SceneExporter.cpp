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
#include "SceneExporter.h"
#include <fstream>
#include "Externals/RapidJson/include/rapidjson/stringbuffer.h"
#include "Externals/RapidJson/include/rapidjson/prettywriter.h"
#include "SceneExportImportCommon.h"
#include "glm/detail/func_trigonometric.hpp"
#include "Utils/OS.h"

namespace Falcor
{
    bool SceneExporter::saveScene(const std::string& filename, const Scene* pScene, uint32_t exportOptions)
    {
        SceneExporter exporter(pScene, filename);
        return exporter.save(exportOptions);
    }

    template<typename T>
    void addLiteral(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const T& value)
    {
        rapidjson::Value jkey;
        jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);
        jval.AddMember(jkey, value, jallocator);
    }

    void addJsonValue(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, rapidjson::Value& value)
    {
        rapidjson::Value jkey;
        jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);
        jval.AddMember(jkey, value, jallocator);
    }

    void addString(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const std::string& value)
    {
        rapidjson::Value jstring, jkey;
        jstring.SetString(value.c_str(), (uint32_t)value.size(), jallocator);
        jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);

        jval.AddMember(jkey, jstring, jallocator);
    }

    void addBool(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, bool isValue)
    {
        rapidjson::Value jbool, jkey;
        jbool.SetBool(isValue);
        jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);

        jval.AddMember(jkey, jbool, jallocator);
    }

    template<typename T>
    void addVector(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const T& value)
    {
        rapidjson::Value jkey;
        jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);
        rapidjson::Value jvec(rapidjson::kArrayType);
        for(int32_t i = 0; i < value.length(); i++)
        {
            jvec.PushBack(value[i], jallocator);
        }

        jval.AddMember(jkey, jvec, jallocator);
    }

    bool SceneExporter::save(uint32_t exportOptions)
    {
        // create the file
        mJDoc.SetObject();

        // Write the version
        rapidjson::Value& JVal = mJDoc;
        auto& allocator = mJDoc.GetAllocator();
        addLiteral(JVal, allocator, SceneKeys::kVersion, kVersion);

        // Write everything else
        bool exportPaths = (exportOptions & ExportPaths) != 0;
        if(exportOptions & ExportGlobalSettings)    writeGlobalSettings(exportPaths);
        if(exportOptions & ExportModels)            writeModels();
        if(exportOptions & ExportLights)            writeLights();
        if(exportOptions & ExportCameras)           writeCameras();
        if(exportOptions & ExportUserDefined)       writeUserDefinedSection();
        if(exportOptions & ExportPaths)             writePaths();
        if(exportOptions & ExportMaterials)         writeMaterials();

        // Get the output string
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 4);
        mJDoc.Accept(writer);
        std::string str(buffer.GetString(), buffer.GetSize());

        // Output the file
        std::ofstream outputStream(mFilename.c_str());
        if(outputStream.fail())
        {
            logError("Can't open output scene file " + mFilename + ".\nExporting failed.");
            return false;
        }
        outputStream << str;
        outputStream.close();

        return true;
    }

    void SceneExporter::writeGlobalSettings(bool writeActivePath)
    {
        rapidjson::Value& jval = mJDoc;
        auto& Allocator = mJDoc.GetAllocator();

        addLiteral(jval, Allocator, SceneKeys::kCameraSpeed, mpScene->getCameraSpeed());
        addLiteral(jval, Allocator, SceneKeys::kLightingScale, mpScene->getLightingScale());
        addString(jval, Allocator, SceneKeys::kActiveCamera, mpScene->getActiveCamera()->getName());
        addVector(jval, Allocator, SceneKeys::kAmbientIntensity, mpScene->getAmbientIntensity());
    }

    void createModelValue(const Scene* pScene, uint32_t modelID, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jmodel)
    {
        jmodel.SetObject();
        addString(jmodel, allocator, SceneKeys::kFilename, stripDataDirectories(pScene->getModel(modelID)->getFilename()));
        addString(jmodel, allocator, SceneKeys::kName, pScene->getModel(modelID)->getName());

        if(pScene->getModel(modelID)->hasAnimations())
        {
            addLiteral(jmodel, allocator, SceneKeys::kActiveAnimation, pScene->getModel(modelID)->getActiveAnimation());
        }

        rapidjson::Value jsonInstanceArray;
        jsonInstanceArray.SetArray();
        for(uint32_t i = 0 ; i < pScene->getModelInstanceCount(modelID); i++)
        {
            rapidjson::Value jsonInstance;
            jsonInstance.SetObject();
            auto& pInstance = pScene->getModelInstance(modelID, i);

            addString(jsonInstance, allocator, SceneKeys::kName, pInstance->getName());
            addVector(jsonInstance, allocator, SceneKeys::kTranslationVec, pInstance->getTranslation());
            addVector(jsonInstance, allocator, SceneKeys::kScalingVec, pInstance->getScaling());

            // Translate rotation to degrees
            glm::vec3 rotation = pInstance->getEulerRotation();
            for(uint32_t c = 0; c < 3; c++)
            {
                rotation[c] = glm::degrees(rotation[c]);
            }
            
            addVector(jsonInstance, allocator, SceneKeys::kRotationVec, rotation);

            jsonInstanceArray.PushBack(jsonInstance, allocator);
        }

        addJsonValue(jmodel, allocator, SceneKeys::kModelInstances, jsonInstanceArray);
    }

    void SceneExporter::writeModels()
    {
        if(mpScene->getModelCount() == 0)
        {
            return;
        }

        rapidjson::Value jsonModelArray;
        jsonModelArray.SetArray();

        for(uint32_t i = 0; i < mpScene->getModelCount(); i++)
        {
            rapidjson::Value jsonModel;
            createModelValue(mpScene, i, mJDoc.GetAllocator(), jsonModel);
            jsonModelArray.PushBack(jsonModel, mJDoc.GetAllocator());
        }
        addJsonValue(mJDoc, mJDoc.GetAllocator(), SceneKeys::kModels, jsonModelArray);
    }

    void createPointLightValue(const PointLight* pLight, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonLight)
    {
        addString(jsonLight, allocator, SceneKeys::kName, pLight->getName());
        addString(jsonLight, allocator, SceneKeys::kType, SceneKeys::kPointLight);
        addVector(jsonLight, allocator, SceneKeys::kLightIntensity, pLight->getIntensity());
        addVector(jsonLight, allocator, SceneKeys::kLightPos, pLight->getWorldPosition());
        addVector(jsonLight, allocator, SceneKeys::kLightDirection, pLight->getWorldDirection());
        addLiteral(jsonLight, allocator, SceneKeys::kLightOpeningAngle, glm::degrees(pLight->getOpeningAngle()));
        addLiteral(jsonLight, allocator, SceneKeys::kLightPenumbraAngle, glm::degrees(pLight->getPenumbraAngle()));
    }

    void createDirectionalLightValue(const DirectionalLight* pLight, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonLight)
    {
        addString(jsonLight, allocator, SceneKeys::kName, pLight->getName());
        addString(jsonLight, allocator, SceneKeys::kType, SceneKeys::kDirLight);
        addVector(jsonLight, allocator, SceneKeys::kLightIntensity, pLight->getIntensity());
        addVector(jsonLight, allocator, SceneKeys::kLightDirection, pLight->getWorldDirection());
    }

    void createLightValue(const Scene* pScene, uint32_t lightID, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonLight)
    {
        jsonLight.SetObject();
        const auto pLight = pScene->getLight(lightID);

        switch(pLight->getType())
        {
        case LightPoint:
            createPointLightValue((PointLight*)pLight.get(), allocator, jsonLight);
            break;
        case LightDirectional:
            createDirectionalLightValue((DirectionalLight*)pLight.get(), allocator, jsonLight);
            break;
        default:
            should_not_get_here();
        }
    }

    void SceneExporter::writeLights()
    {
        if(mpScene->getLightCount() == 0)
        {
            return;
        }

        rapidjson::Value jsonLightsArray(rapidjson::kArrayType);

        for(uint32_t i = 0; i < mpScene->getLightCount(); i++)
        {
            rapidjson::Value jsonLight;
            createLightValue(mpScene, i, mJDoc.GetAllocator(), jsonLight);
            jsonLightsArray.PushBack(jsonLight, mJDoc.GetAllocator());
        }
        addJsonValue(mJDoc, mJDoc.GetAllocator(), SceneKeys::kLights, jsonLightsArray);
    }

    void createCameraValue(const Scene* pScene, uint32_t cameraID, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonCamera)
    {
        jsonCamera.SetObject();
        const auto pCamera = pScene->getCamera(cameraID);
        addString(jsonCamera, allocator, SceneKeys::kName, pCamera->getName());
        addVector(jsonCamera, allocator, SceneKeys::kCamPosition, pCamera->getPosition());
        addVector(jsonCamera, allocator, SceneKeys::kCamTarget, pCamera->getTarget());
        addVector(jsonCamera, allocator, SceneKeys::kCamUp, pCamera->getUpVector());
        addLiteral(jsonCamera, allocator, SceneKeys::kCamFovY, glm::degrees(pCamera->getFovY()));
        glm::vec2 depthRange;
        depthRange[0] = pCamera->getNearPlane();
        depthRange[1] = pCamera->getFarPlane();
        addVector(jsonCamera, allocator, SceneKeys::kCamDepthRange, depthRange);
        addLiteral(jsonCamera, allocator, SceneKeys::kCamAspectRatio, pCamera->getAspectRatio());
    }

    void SceneExporter::writeCameras()
    {
        if(mpScene->getCameraCount() == 0)
        {
            return;
        }

        rapidjson::Value jsonCameraArray(rapidjson::kArrayType);
        for(uint32_t i = 0; i < mpScene->getCameraCount(); i++)
        {
            rapidjson::Value jsonCamera;
            createCameraValue(mpScene, i, mJDoc.GetAllocator(), jsonCamera);
            jsonCameraArray.PushBack(jsonCamera, mJDoc.GetAllocator());
        }
        addJsonValue(mJDoc, mJDoc.GetAllocator(), SceneKeys::kCameras, jsonCameraArray);
    }

    void SceneExporter::writePaths()
    {
        if(mpScene->getPathCount() == 0)
        {
            return;
        }

        auto& allocator = mJDoc.GetAllocator();

        // Loop over the paths
        rapidjson::Value jsonPathsArray(rapidjson::kArrayType);
        for(uint32_t pathID = 0; pathID < mpScene->getPathCount(); pathID++)
        {
            const auto pPath = mpScene->getPath(pathID);
            rapidjson::Value jsonPath;
            jsonPath.SetObject();
            addString(jsonPath, allocator, SceneKeys::kName, pPath->getName());
            addBool(jsonPath, allocator, SceneKeys::kPathLoop, pPath->isRepeatOn());

            // Add the keyframes
            rapidjson::Value jsonFramesArray(rapidjson::kArrayType);
            for(uint32_t frameID = 0; frameID < pPath->getKeyFrameCount(); frameID++)
            {
                const auto& frame = pPath->getKeyFrame(frameID);
                rapidjson::Value jsonFrame(rapidjson::kObjectType);
                addLiteral(jsonFrame, allocator, SceneKeys::kFrameTime, frame.time);
                addVector(jsonFrame, allocator, SceneKeys::kCamPosition, frame.position);
                addVector(jsonFrame, allocator, SceneKeys::kCamTarget, frame.target);
                addVector(jsonFrame, allocator, SceneKeys::kCamUp, frame.up);

                jsonFramesArray.PushBack(jsonFrame, allocator);
            }

            addJsonValue(jsonPath, allocator, SceneKeys::kPathFrames, jsonFramesArray);

            // Add attached objects
            rapidjson::Value jsonObjectsArray(rapidjson::kArrayType);
            for (uint32_t i = 0; i < pPath->getAttachedObjectCount(); i++)
            {
                rapidjson::Value jsonObject(rapidjson::kObjectType);

                const auto& pMovable = pPath->getAttachedObject(i);

                const auto& pModelInstance = std::dynamic_pointer_cast<Scene::ModelInstance>(pMovable);
                const auto& pCamera = std::dynamic_pointer_cast<Camera>(pMovable);
                const auto& pLight = std::dynamic_pointer_cast<Light>(pMovable);

                if (pModelInstance != nullptr)
                {
                    addString(jsonObject, allocator, SceneKeys::kType, SceneKeys::kModelInstance);
                    addString(jsonObject, allocator, SceneKeys::kName, pModelInstance->getName());
                }
                else if (pCamera != nullptr)
                {
                    addString(jsonObject, allocator, SceneKeys::kType, SceneKeys::kCamera);
                    addString(jsonObject, allocator, SceneKeys::kName, pCamera->getName());
                }
                else if (pLight != nullptr)
                {
                    addString(jsonObject, allocator, SceneKeys::kType, SceneKeys::kLight);
                    addString(jsonObject, allocator, SceneKeys::kName, pLight->getName());
                }

                jsonObjectsArray.PushBack(jsonObject, allocator);
            }

            addJsonValue(jsonPath, allocator, SceneKeys::kAttachedObjects, jsonObjectsArray);

            // Finish path
            jsonPathsArray.PushBack(jsonPath, allocator);
        }

        addJsonValue(mJDoc, allocator, SceneKeys::kPaths, jsonPathsArray);
    }

    void SceneExporter::writeUserDefinedSection()
    {
        if(mpScene->getUserVariableCount() == 0)
        {
            return;
        }

        rapidjson::Value jsonUserValues(rapidjson::kObjectType);
        auto& allocator = mJDoc.GetAllocator();

        for(uint32_t varID = 0; varID < mpScene->getUserVariableCount(); varID++)
        {
            std::string name;
            const auto& var = mpScene->getUserVariable(varID, name);
    
            switch(var.type)
            {
            case Scene::UserVariable::Type::Int:
                addLiteral(jsonUserValues, allocator, name, var.i32);
                break;
            case Scene::UserVariable::Type::Uint:
                addLiteral(jsonUserValues, allocator, name, var.u32);
                break;
            case Scene::UserVariable::Type::Int64:
                addLiteral(jsonUserValues, allocator, name, var.i64);
                break;
            case Scene::UserVariable::Type::Uint64:
                addLiteral(jsonUserValues, allocator, name, var.u64);
                break;
            case Scene::UserVariable::Type::Double:
                addLiteral(jsonUserValues, allocator, name, var.d64);
                break;
            case Scene::UserVariable::Type::String:
                addString(jsonUserValues, allocator, name, var.str);
                break;
            case Scene::UserVariable::Type::Vec2:
                addVector(jsonUserValues, allocator, name, var.vec2);
                break;
            case Scene::UserVariable::Type::Vec3:
                addVector(jsonUserValues, allocator, name, var.vec3);
                break;
            case Scene::UserVariable::Type::Vec4:
                addVector(jsonUserValues, allocator, name, var.vec4);
                break;
            case Scene::UserVariable::Type::Bool:
                addBool(jsonUserValues, allocator, name, var.b);
                break;
            default:
                should_not_get_here();
                return;
            }
        }

        addJsonValue(mJDoc, allocator, SceneKeys::kUserDefined, jsonUserValues);
    }

    void createMaterialValue(const MaterialValues& matValue, rapidjson::Value& jsonVal, rapidjson::Document::AllocatorType& allocator)
    {
        // DISABLED_FOR_D3D12
//         jsonVal.SetObject();
//         // Constant color
//         addVector(jsonVal, allocator, SceneKeys::kMaterialColor, matValue.constantColor);
//         if(matValue.texture.pTexture)
//         {
//             std::string filename = stripDataDirectories(matValue.texture.pTexture->getSourceFilename());            
//             addString(jsonVal, allocator, SceneKeys::kMaterialTexture, filename);
//         }
    }

    const char* getMaterialLayerType(uint32_t type)
    {
        switch(type)
        {
        case MatLambert:
            return SceneKeys::kMaterialLambert;
        case MatConductor:
            return SceneKeys::kMaterialConductor;
        case MatDielectric:
            return SceneKeys::kMaterialDielectric;
        case MatEmissive:
            return SceneKeys::kMaterialEmissive;
        case MatUser:
            return SceneKeys::kMaterialUser;
        default:
            should_not_get_here();
            return "";
        }
    }
    
    const char* getMaterialLayerNDF(uint32_t ndf)
    {
        switch(ndf)
        {
        case NDFBeckmann:
            return SceneKeys::kMaterialBeckmann;
        case NDFGGX:
            return SceneKeys::kMaterialGGX;
        case NDFUser:
            return SceneKeys::kMaterialUser;
        default:
            should_not_get_here();
            return "";
        }
    }

    const char* getMaterialLayerBlending(uint32_t blend)
    {
        switch(blend)
        {
        case BlendFresnel:
            return SceneKeys::kMaterialBlendFresnel;
        case BlendConstant:
            return SceneKeys::kMaterialBlendConstant;
        case BlendAdd:
            return SceneKeys::kMaterialBlendAdd;
        default:
            should_not_get_here();
            return "";
        }
    }

    void createMaterialLayer(const MaterialLayerValues* pData, const MaterialLayerDesc* pDesc, rapidjson::Value& jsonVal, rapidjson::Document::AllocatorType& allocator)
    {
        jsonVal.SetObject();
        addString(jsonVal, allocator, SceneKeys::kMaterialLayerType, getMaterialLayerType(pDesc->type));
        addString(jsonVal, allocator, SceneKeys::kMaterialNDF, getMaterialLayerNDF(pDesc->ndf));
        addString(jsonVal, allocator, SceneKeys::kMaterialBlend, getMaterialLayerBlending(pDesc->blending));

        rapidjson::Value jsonAlbedo;
        // DISABLED_FOR_D3D12
//         createMaterialValue(pData->albedo, jsonAlbedo, allocator);
//         addJsonValue(jsonVal, allocator, SceneKeys::kMaterialAlbedo, jsonAlbedo);
// 
//         rapidjson::Value jsonRoughness;
//         createMaterialValue(pData->roughness, jsonRoughness, allocator);
//         addJsonValue(jsonVal, allocator, SceneKeys::kMaterialRoughness, jsonRoughness);
// 
//         rapidjson::Value jsonExtra;
//         createMaterialValue(pData->extraParam, jsonExtra, allocator);
//         addJsonValue(jsonVal, allocator, SceneKeys::kMaterialExtraParam, jsonExtra);
    }

    void createMaterialValue(const Material* pMaterial, rapidjson::Value& jsonMaterial, rapidjson::Document::AllocatorType& allocator)
    {
        // Name
        const std::string& name = pMaterial->getName();
        if(name.length())
        {
            addString(jsonMaterial, allocator, SceneKeys::kName, name);
        }

        // ID
        addLiteral(jsonMaterial, allocator, SceneKeys::kMaterialID, pMaterial->getId());

        // Alpha layer
        rapidjson::Value jsonAlpha;
        // DISABLED_FOR_D3D12
//         createMaterialValue(pMaterial->getAlphaValue(), jsonAlpha, allocator);
//         addJsonValue(jsonMaterial, allocator, SceneKeys::kMaterialAlpha, jsonAlpha);
// 
//         // Normal
//         rapidjson::Value jsonNormal;
//         createMaterialValue(pMaterial->getNormalMap(), jsonNormal, allocator);
//         addJsonValue(jsonMaterial, allocator, SceneKeys::kMaterialNormal, jsonNormal);
// 
//         // Height
//         rapidjson::Value jsonHeight;
//         createMaterialValue(pMaterial->getHeightMap(), jsonHeight, allocator);
//         addJsonValue(jsonMaterial, allocator, SceneKeys::kMaterialHeight, jsonHeight);

        // Loop over the layers
        if(pMaterial->getNumLayers() > 0)
        {
            rapidjson::Value jsonLayerArray(rapidjson::kArrayType);
            for(uint32_t i = 0; i < pMaterial->getNumLayers(); i++)
            {
                // DISABLED_FOR_D3D12
//                 const MaterialLayerDesc* pDesc = pMaterial->getLayerDesc(i);
//                 const MaterialLayerValues* pData = pMaterial->getLayerValues(i);
//                 rapidjson::Value jsonLayer;
//                 createMaterialLayer(pData, pDesc, jsonLayer, allocator);
//                 jsonLayerArray.PushBack(jsonLayer, allocator);
            }

            addJsonValue(jsonMaterial, allocator, SceneKeys::kMaterialLayers, jsonLayerArray);
        }
    }

    void SceneExporter::writeMaterials()
    {
        if(mpScene->getMaterialCount() == 0)
        {
            return;
        }

        auto& allocator = mJDoc.GetAllocator();
        rapidjson::Value jsonMaterialArray(rapidjson::kArrayType);

        for(uint32_t i = 0; i < mpScene->getMaterialCount(); i++)
        {
            const auto pMaterial = mpScene->getMaterial(i);
            rapidjson::Value jsonMaterial(rapidjson::kObjectType);            
            createMaterialValue(pMaterial.get(), jsonMaterial, allocator);
            jsonMaterialArray.PushBack(jsonMaterial, allocator);
        }

        addJsonValue(mJDoc, allocator, SceneKeys::kMaterials, jsonMaterialArray);
    }
}