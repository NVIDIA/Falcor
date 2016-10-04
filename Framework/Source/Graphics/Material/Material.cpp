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
#include "Material.h"
#include "API/UniformBuffer.h"
#include "API/Texture.h"
#include "API/Buffer.h"
#include "Utils/os.h"
#include "Utils/Math/FalcorMath.h"
#include "MaterialSystem.h"

namespace Falcor
{
	uint32_t Material::sMaterialCounter = 0;
    std::vector<Material::DescId> Material::sDescIdentifier;

    // Please add your texture here every time you add another texture slot into material
    static const size_t kTextureSlots[] = {
        // layers
        offsetof(MaterialValues, layers[0]) + offsetof(MaterialLayerValues, albedo),
        offsetof(MaterialValues, layers[0]) + offsetof(MaterialLayerValues, roughness),
        offsetof(MaterialValues, layers[0]) + offsetof(MaterialLayerValues, extraParam),
        offsetof(MaterialValues, layers[1]) + offsetof(MaterialLayerValues, albedo),
        offsetof(MaterialValues, layers[1]) + offsetof(MaterialLayerValues, roughness),
        offsetof(MaterialValues, layers[1]) + offsetof(MaterialLayerValues, extraParam),
        offsetof(MaterialValues, layers[2]) + offsetof(MaterialLayerValues, albedo),
        offsetof(MaterialValues, layers[2]) + offsetof(MaterialLayerValues, roughness),
        offsetof(MaterialValues, layers[2]) + offsetof(MaterialLayerValues, extraParam),

        // modifiers
        offsetof(MaterialValues, alphaMap),
        offsetof(MaterialValues, normalMap),
        offsetof(MaterialValues, heightMap),
        offsetof(MaterialValues, ambientMap),
    };

	Material::Material(const std::string& name) : mName(name)
	{
        static_assert((sizeof(MaterialLayerValues) - sizeof(glm::vec4)) == sizeof(MaterialValue) * 3, "Please register your texture offset in kTextureSlots every time you add another texture slot into material");
        static_assert((sizeof(MaterialValues) - sizeof(glm::vec4)) == sizeof(MaterialValue) * 4 + sizeof(MaterialLayerValues) * 3, "Please register your texture offset in kTextureSlots every time you add another texture slot into material");

		mData.values.id = sMaterialCounter;
		sMaterialCounter++;
	}

    Material::SharedPtr Material::create(const std::string& name)
    {
        Material* pMaterial = new Material(name);
        return SharedPtr(pMaterial);
    }

    Material::~Material()
    {
        removeDescIdentifier();
    }
    
    void Material::resetGlobalIdCounter()
    {
        sMaterialCounter = 0;
    }

    size_t Material::getNumActiveLayers() const
	{
		size_t i = 0;
        for(; i < MatMaxLayers; ++i)
        {
            if(mData.desc.layers[i].type == MatNone)
            {
                break;
            }
        }
		return i;
	}

    const MaterialLayerValues* Material::getLayerValues(uint32_t layerIdx) const
	{
		if(layerIdx >= getNumActiveLayers())
        {
            return nullptr;
        }

		return &mData.values.layers[layerIdx];
	}

    const MaterialLayerDesc* Material::getLayerDesc(uint32_t layerIdx) const
    {
        if(layerIdx >= getNumActiveLayers())
        {
            return nullptr;
        }

        return &mData.desc.layers[layerIdx];
    }

	bool Material::addLayer(const MaterialLayerDesc& desc, const MaterialLayerValues& values)
	{
		size_t numLayers = getNumActiveLayers();
		if(numLayers >= MatMaxLayers)
		{
			Logger::log(Logger::Level::Error, "Exceeded maximum number of layers in a material");
			return false;
		}

		mData.desc.layers[numLayers] = desc;		
        mData.values.layers[numLayers] = values;

        // Update the textures flag
        mData.desc.layers[numLayers].hasAlbedoTexture = values.albedo.texture.pTexture ? true : false;
        mData.desc.layers[numLayers].hasRoughnessTexture = values.roughness.texture.pTexture ? true : false;
        mData.desc.layers[numLayers].hasExtraParamTexture = values.extraParam.texture.pTexture ? true : false;
        mDescDirty = true;

		return true;
	}

    void Material::removeLayer(uint32_t layerIdx)
    {
        if(layerIdx >= getNumActiveLayers())
        {
            assert(false);
            return;
        }
        const bool needCompaction = layerIdx + 1 < getNumActiveLayers();
        mData.desc.layers[layerIdx].type = MatNone;
        mData.values.layers[layerIdx] = MaterialLayerValues();

        /* If it's not the last material, compactify layers */
        if(needCompaction)
        {
            for(int i = 0; i < MatMaxLayers - 1; ++i)
            {
                bool hasHole = mData.desc.layers[i].type == MatNone && mData.desc.layers[i + 1].type != MatNone;
                if(!hasHole)
                {
                    continue;
                }
				memmove(&mData.desc.layers[i], &mData.desc.layers[i+1], sizeof(mData.desc.layers[0]));
                memmove(&mData.values.layers[i], &mData.values.layers[i + 1], sizeof(mData.values.layers[0]));
				mData.desc.layers[i+1].type = MatNone;
                mData.values.layers[i+1] = MaterialLayerValues();
            }
        }

        mDescDirty = true;
    }

	void Material::normalize()
	{
		float totalAlbedo = 0.f;

		/* Compute a conservative worst-case albedo from all layers */
		for(size_t i=0;i < MatMaxLayers;++i)
		{
            const MaterialLayerValues& values = mData.values.layers[i];
            const MaterialLayerDesc& desc = mData.desc.layers[i];

            if(desc.type != MatLambert && desc.type != MatConductor && desc.type != MatDielectric)
            {
                break;
            }

			// TODO: compute maximum texture albedo once there is an interface for it in the future
			float albedo = luminance(glm::vec3(values.albedo.constantColor));

			if(desc.blending == BlendAdd || desc.blending == BlendFresnel)
            {
                totalAlbedo += albedo;
            }
			else
            {
                totalAlbedo += glm::mix(totalAlbedo, albedo, values.albedo.constantColor.w);
            }
		}

		if(totalAlbedo == 0.f)
        {
            Logger::log(Logger::Level::Warning, "Material " + mName + " is pitch black");
            totalAlbedo = 1.f;
        }
		else if(totalAlbedo > 1.f)
		{
			Logger::log(Logger::Level::Warning, "Material " + mName + " is not energy conserving. Renormalizing...");

			/* Renormalize all albedos assuming linear blending between layers */
			for(size_t i = 0;i < MatMaxLayers;++i)
			{
                MaterialLayerValues& values = mData.values.layers[i];
                const MaterialLayerDesc& desc = mData.desc.layers[i];
				if (desc.type != MatLambert && desc.type != MatConductor && desc.type != MatDielectric)
                {
                    break;
                }

				vec3 newAlbedo = glm::vec3(values.albedo.constantColor);
				newAlbedo /= totalAlbedo;
				values.albedo.constantColor = glm::vec4(newAlbedo, values.albedo.constantColor.w);
			}
            totalAlbedo = 1.f;
        }

        /* Construct the normalized PMF for sampling layers based on albedos and assuming linear blending between layers */
        float currentWeight = 1.f;
        for(int i = MatMaxLayers-1;i >=0;--i)
        {
            MaterialLayerValues& values = mData.values.layers[i];
            const MaterialLayerDesc& desc = mData.desc.layers[i];
			if (desc.type != MatLambert && desc.type != MatConductor && desc.type != MatDielectric)
            {
                continue;
            }

            float albedo = luminance(glm::vec3(values.albedo.constantColor));
            /* Embed the expected probability that is based on the constant blending */
            if(desc.blending == BlendConstant)
            {
                albedo *= values.albedo.constantColor.w;
            }
            albedo *= currentWeight;

            values.pmf = albedo / totalAlbedo;

            /* Maintain the expected blending probability for the next level*/
            if(desc.blending == BlendConstant)
            {
                currentWeight = max(0.f, 1.f - currentWeight);
                assert(currentWeight > 0.f);
            }
            else
            {
                currentWeight = 1.f;
            }
        }
	}

#if _LOG_ENABLED
#define check_offset(_a) assert(pBuffer->getVariableOffset(varName + "." + #_a) == (offsetof(MaterialData, _a) + offset))
#else
#define check_offset(_a)
#endif

    void Material::setIntoUniformBuffer(UniformBuffer* pBuffer, const std::string& varName) const
    {
        finalize();
        static const size_t dataSize = sizeof(MaterialData);
        static_assert(dataSize % sizeof(glm::vec4) == 0, "Material::MaterialData size should be a multiple of 16");

        size_t offset = pBuffer->getVariableOffset(varName + ".desc.layers[0].type");

        if(offset == UniformBuffer::kInvalidUniformOffset)
        {
            Logger::log(Logger::Level::Warning, "Material::setIntoUniformBuffer() - variable \"" + varName + "\"not found in uniform buffer\n");
            return;
        }

        check_offset(values.layers[0].albedo.texture.ptr);
        check_offset(values.ambientMap.texture.ptr);
        check_offset(values.id);
        assert(offset + dataSize <= pBuffer->getBuffer()->getSize());

        bindTextures();
        pBuffer->setBlob(&mData, offset, dataSize);
    }

    static TexPtr& getTexture(const MaterialValues* mat, size_t Offset)
    {
        MaterialValue& matValue = *((MaterialValue*)((char*)mat + Offset));
        return matValue.texture;
    }

    void Material::getActiveTextures(std::vector<Texture::SharedConstPtr>& textures) const
    {
        textures.clear();
        textures.reserve(arraysize(kTextureSlots));
        for(uint32_t i = 0; i < arraysize(kTextureSlots); i++)
        {
            TexPtr& gpuTex = getTexture(&mData.values, kTextureSlots[i]);
            if(gpuTex.pTexture)
            {
                textures.push_back(gpuTex.pTexture->shared_from_this());
            }
        }
    }

	void Material::bindTextures() const
    {
        for(uint32_t i = 0; i < arraysize(kTextureSlots); i++)
        {
            TexPtr& gpuTex = getTexture(&mData.values, kTextureSlots[i]);
			gpuTex.ptr = gpuTex.pTexture ? gpuTex.pTexture->makeResident(mpSamplerOverride.get()) : 0;
        }
    }
   
    bool Material::operator==(const Material& other) const
    {
		return memcmp(&mData, &other.mData, sizeof(mData)) == 0 && mpSamplerOverride == other.mpSamplerOverride;
    }

    void Material::unloadTextures() const
    {
        for(uint32_t i = 0; i < arraysize(kTextureSlots) ; i++)
        {
            TexPtr& GpuTex = getTexture(&mData.values, kTextureSlots[i]);
			if(GpuTex.pTexture)
            {
                GpuTex.pTexture->evict(mpSamplerOverride.get());
                GpuTex.ptr = 0;
            }
        }
    }

    void Material::setNormalValue(const MaterialValue& normal)
    {
        mData.values.normalMap = normal; 
        mData.desc.hasNormalMap = normal.texture.pTexture ? true : false; 
        mDescDirty = true;
    }

    void Material::setAlphaValue(const MaterialValue& alpha) 
    { 
        mData.values.alphaMap = alpha; 
        mData.desc.hasAlphaMap = alpha.texture.pTexture ? true : false; 
        mDescDirty = true;
    }

    void Material::setAmbientValue(const MaterialValue& ambient)
    {
        mData.values.ambientMap = ambient;
        mData.desc.hasAmbientMap = ambient.texture.pTexture ? true : false;
        mDescDirty = true;
    }

    void Material::setHeightValue(const MaterialValue& height) 
    { 
        mData.values.heightMap = height; 
        mData.desc.hasHeightMap = height.texture.pTexture ? true : false; 
        mDescDirty = true;
    }

    void Material::removeDescIdentifier() const
    {
        for(size_t i = 0 ; i < sDescIdentifier.size() ; i++)
        {
            if(mDescIdentifier == sDescIdentifier[i].id)
            {
                sDescIdentifier[i].refCount--;
                if(sDescIdentifier[i].refCount == 0)
                {
                    MaterialSystem::removeMaterial(mDescIdentifier);
                    sDescIdentifier.erase(sDescIdentifier.begin() + i);
                }
            }
        }
    }

    void Material::updateDescIdentifier() const
    {
        static uint64_t identifier = 0;

        removeDescIdentifier();
        mDescDirty = false;
        for(auto& a : sDescIdentifier)
        {
            if(memcmp(&mData.desc, &a.desc, sizeof(mData.desc)) == 0)
            {
                mDescIdentifier = a.id;
                a.refCount++;
                return;
            }
        }

        // Not found, add it to the vector            
        sDescIdentifier.push_back({mData.desc, identifier, 1});
        mDescIdentifier = identifier;
        identifier++;
    }

    size_t Material::getDescIdentifier() const
    {
        finalize();
        return mDescIdentifier;
    }

    void Material::finalize() const
    {
        if(mDescDirty)
        {
            updateDescIdentifier();
            const_cast<Material*>(this)->normalize();
            mDescDirty = false;
        }
    }

#define case_return_self(_a) case _a: return #_a;

    static std::string getLayerTypeStr(uint32_t type)
    {
        switch(type)
        {
            case_return_self(MatNone);
            case_return_self(MatLambert);
            case_return_self(MatConductor);
            case_return_self(MatDielectric);
            case_return_self(MatEmissive);
            case_return_self(MatUser);
        default:
            should_not_get_here();
            return "";
        }
    }

    static std::string getLayerNdfStr(uint32_t ndf)
    {
        switch(ndf)
        {
            case_return_self(NDFBeckmann);
            case_return_self(NDFGGX);
            case_return_self(NDFUser);
        default:
            should_not_get_here();
            return "";
        }
    }

    static std::string getLayerBlendStr(uint32_t blend)
    {
        switch(blend)
        {
            case_return_self(BlendFresnel);
            case_return_self(BlendConstant);
            case_return_self(BlendAdd);
        default:
            should_not_get_here();
            return "";
        }
    }

    void Material::getMaterialDescStr(std::string& shaderDcl) const
    {
        finalize();

        shaderDcl = "{{";
        for(uint32_t layerId = 0; layerId < arraysize(mData.desc.layers); layerId++)
        {
            const MaterialLayerDesc& layer = mData.desc.layers[layerId];
            shaderDcl += '{' + getLayerTypeStr(layer.type) + ',';
            shaderDcl += getLayerNdfStr(layer.ndf) + ',';
            shaderDcl += getLayerBlendStr(layer.blending) + ',';
            shaderDcl += std::to_string(layer.hasAlbedoTexture) + ',' + std::to_string(layer.hasRoughnessTexture) + ',' + std::to_string(layer.hasExtraParamTexture) + ",{" + std::to_string(layer.pad.x) + "," + std::to_string(layer.pad.y) + '}';
            shaderDcl += '}';
            if(layerId != arraysize(mData.desc.layers) - 1)
            {
                shaderDcl += ',';
            }
        }
        shaderDcl += "},";
        shaderDcl += std::to_string(mData.desc.hasAlphaMap) + ',' + std::to_string(mData.desc.hasNormalMap) + ',' + std::to_string(mData.desc.hasHeightMap) + ',' + std::to_string(mData.desc.hasAmbientMap);
        shaderDcl += '}';
    }

}
