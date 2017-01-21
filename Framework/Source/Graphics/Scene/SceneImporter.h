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
#include <string>
#include "Externals/RapidJson/include/rapidjson/document.h"
#include "Graphics/Material/Material.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "Scene.h"

namespace Falcor
{    
    class SceneImporter
    {
    protected:
        friend class Scene;
        static Scene::SharedPtr loadScene(const std::string& filename, uint32_t modelLoadFlags, uint32_t sceneLoadFlags);

    private:
        SceneImporter() = default;
        Scene::SharedPtr load(const std::string& filename, const uint32_t& modelLoadFlags, uint32_t sceneLoadFlags);

        bool parseVersion(const rapidjson::Value& jsonVal);
        bool parseModels(const rapidjson::Value& jsonVal);
        bool parseLights(const rapidjson::Value& jsonVal);
        bool parseCameras(const rapidjson::Value& jsonVal);
        bool parseMaterials(const rapidjson::Value& jsonVal);
        bool parseAmbientIntensity(const rapidjson::Value& jsonVal);
        bool parseActiveCamera(const rapidjson::Value& jsonVal);
        bool parseCameraSpeed(const rapidjson::Value& jsonVal);
        bool parseLightingScale(const rapidjson::Value& jsonVal);
        bool parsePaths(const rapidjson::Value& jsonVal);
        bool parseUserDefinedSection(const rapidjson::Value& jsonVal);
        bool parseActivePath(const rapidjson::Value& jsonVal);
        bool parseIncludes(const rapidjson::Value& jsonVal);

        bool topLevelLoop();

        bool loadIncludeFile(const std::string& Include);

        bool createModel(const rapidjson::Value& jsonModel);
        bool createModelInstances(const rapidjson::Value& jsonVal, const Model::SharedPtr& pModel);
        bool createPointLight(const rapidjson::Value& jsonLight);
        bool createDirLight(const rapidjson::Value& jsonLight);
        ObjectPath::SharedPtr createPath(const rapidjson::Value& jsonPath);
        bool createPathFrames(ObjectPath* pPath, const rapidjson::Value& jsonFramesArray);
        bool createCamera(const rapidjson::Value& jsonCamera);

        bool createMaterial(const rapidjson::Value& jsonMaterial);
        bool createMaterialLayer(const rapidjson::Value& jsonLayer, MaterialLayerValues& layerData, MaterialLayerDesc& layerDesc);
        bool createMaterialValue(const rapidjson::Value& jsonValue, MaterialValues& matValue);
        bool createAllMaterialLayers(const rapidjson::Value& jsonLayerArray, Material* pMaterial);

        bool createMaterialLayerType(const rapidjson::Value& jsonValue, MaterialLayerDesc& matLayer);
        bool createMaterialLayerNDF(const rapidjson::Value& jsonValue, MaterialLayerDesc& matLayer);
        bool createMaterialLayerBlend(const rapidjson::Value& jsonValue, MaterialLayerDesc& matLayer);

        bool createMaterialTexture(const rapidjson::Value& jsonValue, Texture::SharedPtr& pTexture);
        bool createMaterialValueColor(const rapidjson::Value& jsonValue, glm::vec4& color);

        Scene* error(const std::string& msg);

        template<uint32_t VecSize>
        bool getFloatVec(const rapidjson::Value& jsonVal, const std::string& desc, float vec[VecSize]);
        bool getFloatVecAnySize(const rapidjson::Value& jsonVal, const std::string& desc, std::vector<float>& vec);
        rapidjson::Document mJDoc;
        Scene::SharedPtr mpScene = nullptr;
        std::string mFilename;
        std::string mDirectory;
        uint32_t mModelLoadFlags = 0;
        uint32_t mSceneLoadFlags = 0;

        struct FuncValue
        {
            const std::string token;
            decltype(&SceneImporter::parseModels) func;
        };

        static const FuncValue kFunctionTable[];
        bool validateSceneFile();
    };
}