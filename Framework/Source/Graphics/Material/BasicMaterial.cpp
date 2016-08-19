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
#include "BasicMaterial.h"
#include "Data/HostDeviceData.h"

namespace Falcor
{
    Material::SharedPtr BasicMaterial::convertToMaterial()
    {
        Material::SharedPtr pMaterial = Material::create("");
        std::string name = "BasicMaterial" + std::to_string(pMaterial->getId());
        pMaterial->setName(name);

        size_t currentLayer = 0;

        /* Parse diffuse layer */
        if(luminance(diffuseColor) > 0.f || pTextures[MapType::DiffuseMap] != nullptr)
        {
            MaterialLayerDesc desc;
            MaterialLayerValues data;

            desc.type = MatLambert;
            desc.blending = BlendAdd;

            vec3 albedo = diffuseColor;
            /* Special case: if albedo color is zero but texture is not, ignore the color */
            if(luminance(albedo) == 0.f)
            {
                albedo = vec3(1.f);
            }

            data.albedo.constantColor = vec4(albedo, 1.f);
            data.albedo.texture.pTexture = pTextures[MapType::DiffuseMap];
            desc.hasAlbedoTexture = (data.albedo.texture.pTexture != nullptr);
            pMaterial->addLayer(desc, data);
        }

        /* Parse conductor layer */
        if(luminance(specularColor) > 0.f || pTextures[MapType::SpecularMap] != nullptr)
        {
            MaterialLayerDesc desc;
            MaterialLayerValues data;

            desc.type = MatConductor;
            /* Uniform mixture blending */
            desc.blending = BlendAdd;

            vec3 specular = specularColor;
            /* Special case: if albedo color is zero but texture is not, ignore the color */
            if(luminance(specularColor) == 0.f)
            {
                specular = vec3(1.f);
            }

            data.albedo.constantColor = vec4(specular, 1.f);
            data.albedo.texture.pTexture = pTextures[MapType::SpecularMap];
            desc.hasAlbedoTexture = (data.albedo.texture.pTexture != nullptr);

            data.roughness.constantColor = vec4(convertShininessToRoughness(shininess));
            data.roughness.texture.pTexture = pTextures[MapType::ShininessMap];
            desc.hasRoughnessTexture = (data.roughness.texture.pTexture != nullptr);

            /* Average chrome IoR and kappa */
            data.extraParam.constantColor = vec4(3.f, 4.2f, 0.f, 0.f);
            pMaterial->addLayer(desc, data);
        }

        /* Parse dielectric layer */
        if((luminance(transparentColor) * (1.f - opacity) > 0.f) && currentLayer < MatMaxLayers)
        {
            MaterialLayerDesc desc;
            MaterialLayerValues data;

            desc.type = MatDielectric;
            desc.blending = BlendFresnel;

            data.albedo.constantColor = vec4(transparentColor * (1.f - opacity), 1.f);

            /* Always assume a smooth dielectric */
            data.roughness.constantColor = vec4(0.f);
            data.extraParam.constantColor = vec4(IoR, 0.f, 0.f, 0.f);

            pMaterial->addLayer(desc, data);
        }

        /* Parse emissive layer */
        if(luminance(emissiveColor) > 0.f || pTextures[MapType::EmissiveMap] != nullptr)
        {
            MaterialLayerDesc desc;
            MaterialLayerValues data;

            desc.type = MatEmissive;
            desc.blending = BlendAdd;

            vec3 albedo = emissiveColor;
            /* Special case: if albedo color is zero but texture is not, ignore the color */
            if(luminance(albedo) == 0.f)
            {
                albedo = vec3(1.f);
            }

            data.albedo.constantColor = vec4(albedo, 1.f);
            data.albedo.texture.pTexture = pTextures[MapType::EmissiveMap];
            desc.hasAlbedoTexture = (data.albedo.texture.pTexture != nullptr);
            pMaterial->addLayer(desc, data);
        }

        /* Initialize modifiers */
        MaterialValue normalVal;
        normalVal.texture.pTexture = pTextures[MapType::NormalMap];
        pMaterial->setNormalValue(normalVal);

        MaterialValue alphaVal;
        alphaVal.texture.pTexture = pTextures[MapType::AlphaMap];
        alphaVal.constantColor = glm::vec4(0.5f);
        pMaterial->setAlphaValue(alphaVal);

        MaterialValue ambientVal;
        ambientVal.texture.pTexture = pTextures[MapType::AmbientMap];
        ambientVal.constantColor = glm::vec4(0.5f);
        pMaterial->setAmbientValue(ambientVal);

        MaterialValue heightVal;
        heightVal.constantColor.x = bumpScale;
        heightVal.constantColor.y = bumpOffset;
        heightVal.texture.pTexture = pTextures[MapType::HeightMap];
        pMaterial->setHeightValue(heightVal);

        pMaterial->finalize();

        return pMaterial;
    }

    static inline uint32_t getLayersCountByType(const Material* pMaterial, uint32_t matType)
    {
        uint32_t numLayers = 0;
        int32_t layerId = MatMaxLayers;
        while(--layerId >= 0)
        {
            const MaterialLayerDesc* pDesc = pMaterial->getLayerDesc(layerId);
            if(pDesc->type == matType)
            {
                numLayers++;
            }
        }

        return numLayers;
    }

    static inline const MaterialLayerValues* getLayerDataFromType(const Material* pMaterial, uint32_t matType)
    {
        int32_t layerId = MatMaxLayers;
        while(--layerId >= 0)
        {
            const MaterialLayerDesc* pDesc = pMaterial->getLayerDesc(layerId);

            if(pDesc->type == matType)
            {
                return pMaterial->getLayerValues(layerId);
            }
        }

        return nullptr;
    }

    void BasicMaterial::initializeFromMaterial(const Material* pMaterial)
    {
        if(getLayersCountByType(pMaterial, MatLambert) > 1 ||
            getLayersCountByType(pMaterial, MatConductor) > 1 ||
            getLayersCountByType(pMaterial, MatDielectric) > 1)
        {
            Logger::log(Logger::Level::Warning, "BasicMaterial::initializeFromMaterial(): Material " + pMaterial->getName() + " was exported with loss of data");
        }

        /* Parse diffuse layer */
        const MaterialLayerValues* pDiffuseData = pMaterial->getLayerValues(MatLambert);
        if(pDiffuseData)
        {
            pTextures[MapType::DiffuseMap] = pDiffuseData->albedo.texture.pTexture;
            diffuseColor = vec3(pDiffuseData->albedo.constantColor);
            if(pDiffuseData->roughness.texture.pTexture)
            {
                Logger::log(Logger::Level::Warning, "BasicMaterial::initializeFromMaterial(): Material " + pMaterial->getName() + " has an unsupported diffuse roughness texture");
            }
        }

        /* Parse emissive layer */
        const MaterialLayerValues* pEmissiveData = pMaterial->getLayerValues(MatEmissive);
        if(pEmissiveData)
        {
            pTextures[MapType::EmissiveMap] = pEmissiveData->albedo.texture.pTexture;
            emissiveColor = vec3(pEmissiveData->albedo.constantColor);
            if(pEmissiveData->roughness.texture.pTexture)
            {
                Logger::log(Logger::Level::Warning, "BasicMaterial::initializeFromMaterial(): Material " + pMaterial->getName() + " has an unsupported emissive roughness texture");
            }
        }

        /* Parse specular layer */
        const MaterialLayerValues* pSpecData = pMaterial->getLayerValues(MatConductor);
        if(pSpecData)
        {
            pTextures[MapType::SpecularMap] = pSpecData->albedo.texture.pTexture;
            specularColor = vec3(pSpecData->albedo.constantColor);
            pTextures[MapType::ShininessMap] = pSpecData->roughness.texture.pTexture;
            shininess = convertRoughnessToShininess(pSpecData->roughness.constantColor.x);
        }

        /* Parse transparent layer */
        const MaterialLayerValues* pTransData = pMaterial->getLayerValues(MatDielectric);
        if(pTransData)
        {
            transparentColor = vec3(pTransData->albedo.constantColor);
            IoR = pTransData->extraParam.constantColor.x;
            if(pTransData->albedo.texture.pTexture)
            {
                Logger::log(Logger::Level::Warning, "Material::GetObsoleteMaterial: Material " + pMaterial->getName() + " has an unsupported transparency roughness texture");
            }
            if(pTransData->roughness.texture.pTexture)
            {
                Logger::log(Logger::Level::Warning, "Material::GetObsoleteMaterial: Material " + pMaterial->getName() + " has an unsupported transparency roughness texture");
            }
        }

        /* Parse modifiers */
        const MaterialValue& alphaMap = pMaterial->getAlphaValue();
        if(alphaMap.texture.pTexture)
        {
            pTextures[MapType::AlphaMap] = alphaMap.texture.pTexture;
            opacity = alphaMap.constantColor.x;
        }

        const MaterialValue& ambientMap = pMaterial->getAmbientValue();
        if(ambientMap.texture.pTexture)
        {
            pTextures[MapType::AmbientMap] = ambientMap.texture.pTexture;
        }
        const MaterialValue& normalMap = pMaterial->getNormalValue();
        if(normalMap.texture.pTexture)
        {
            pTextures[MapType::NormalMap] = normalMap.texture.pTexture;
        }

        const MaterialValue& heightMap = pMaterial->getHeightValue();
        if(heightMap.texture.pTexture)
        {
            pTextures[MapType::HeightMap] = heightMap.texture.pTexture;
            bumpScale = heightMap.constantColor.x;
            bumpOffset = heightMap.constantColor.y;
        }
    }
}