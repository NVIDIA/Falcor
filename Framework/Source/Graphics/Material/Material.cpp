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
#include "API/ConstantBuffer.h"
#include "API/Texture.h"
#include "API/Buffer.h"
#include "Utils/os.h"
#include "Utils/Math/FalcorMath.h"
#include "MaterialSystem.h"
#include "API/ProgramVars.h"
#include <sstream>

namespace Falcor
{
    uint32_t Material::sMaterialCounter = 0;
    std::vector<Material::DescId> Material::sDescIdentifier;

	bool Material::UseGeneralShader = true;

    Material::Material(const std::string& name) : mName(name)
    {
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

    uint32_t Material::getNumLayers() const
    {
        finalize();
        uint32_t i = 0;
        for(; i < MatMaxLayers; ++i)
        {
            if(mData.desc.layers[i].type == MatNone)
            {
                break;
            }
        }
        return i;
    }

    Material::Layer Material::getLayer(uint32_t layerIdx) const
    {
        finalize();
        Layer layer;
        if(layerIdx < getNumLayers())
        {
            const auto& desc = mData.desc.layers[layerIdx];
            const auto& vals = mData.values.layers[layerIdx];

            layer.albedo = vals.albedo;
            layer.roughness = vals.roughness;
            layer.extraParam = vals.extraParam;
            layer.pTexture = mData.textures.layers[layerIdx];

            layer.type = (Layer::Type)desc.type;
            layer.ndf = (Layer::NDF)desc.ndf;
            layer.blend = (Layer::Blend)desc.blending;
            layer.pmf = vals.pmf;
        }

        return layer;
    }

    bool Material::addLayer(const Layer& layer)
    {
        size_t numLayers = getNumLayers();
        if(numLayers >= MatMaxLayers)
        {
            logError("Exceeded maximum number of layers in a material");
            return false;
        }

        auto& desc = mData.desc.layers[numLayers];
        auto& vals = mData.values.layers[numLayers];
        
        vals.albedo = layer.albedo;
        vals.roughness = layer.roughness;
        vals.extraParam = layer.extraParam;

        mData.textures.layers[numLayers] = layer.pTexture;
        desc.hasTexture = (layer.pTexture != nullptr);

        desc.type = (uint32_t)layer.type;
        desc.ndf = (uint32_t)layer.ndf;
        desc.blending = (uint32_t)layer.blend;
        vals.pmf = layer.pmf;
        mDescDirty = true;

        // Update the index by type
        if(desc.type != MatNone && mData.desc.layerIdByType[desc.type].id == -1)
        {
            mData.desc.layerIdByType[desc.type].id = (int)numLayers;
        }
        
        // For dielectric and conductors, check if we have roughness
        if (layer.type == Layer::Type::Dielectric || layer.type == Layer::Type::Conductor)
        {
            if (desc.hasTexture)
            {
                ResourceFormat texFormat = layer.pTexture->getFormat();
                if (getFormatChannelCount(texFormat) == 4)
                {
                    switch (texFormat)
                    {
                    case ResourceFormat::RGBX8Unorm:
                    case ResourceFormat::RGBX8UnormSrgb:
                    case ResourceFormat::BGRX8Unorm:
                    case ResourceFormat::BGRX8UnormSrgb:
                        break;
                    default:
                        desc.hasTexture |= ROUGHNESS_CHANNEL_BIT;
                    }
                }
            }
        }
        return true;
    }

    void Material::removeLayer(uint32_t layerIdx)
    {
        if(layerIdx >= getNumLayers())
        {
            assert(false);
            return;
        }

        const bool needCompaction = layerIdx + 1 < getNumLayers();
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

        // Update indices by type
        memset(&mData.desc.layerIdByType, -1, sizeof(mData.desc.layerIdByType));
        for(int i = 0; i < MatMaxLayers - 1; ++i)
        {
            if(mData.desc.layers[i].type != MatNone && mData.desc.layerIdByType[mData.desc.layers[i].type].id != -1)
            {
                mData.desc.layerIdByType[mData.desc.layers[i].type].id = i;
            }
        }

        mDescDirty = true;
    }

    void Material::normalize() const
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
            float albedo = luminance(glm::vec3(values.albedo));

            if(desc.blending == BlendAdd || desc.blending == BlendFresnel)
            {
                totalAlbedo += albedo;
            }
            else
            {
                totalAlbedo += glm::mix(totalAlbedo, albedo, values.albedo.w);
            }
        }

        if(totalAlbedo == 0.f)
        {
            logWarning("Material " + mName + " is pitch black");
            totalAlbedo = 1.f;
        }
        else if(totalAlbedo > 1.f)
        {
            logWarning("Material " + mName + " is not energy conserving. Renormalizing...");

            /* Renormalize all albedos assuming linear blending between layers */
            for(size_t i = 0;i < MatMaxLayers;++i)
            {
                MaterialLayerValues& values = mData.values.layers[i];
                const MaterialLayerDesc& desc = mData.desc.layers[i];
                if (desc.type != MatLambert && desc.type != MatConductor && desc.type != MatDielectric)
                {
                    break;
                }

                glm::vec3 newAlbedo = glm::vec3(values.albedo);
                newAlbedo /= totalAlbedo;
                values.albedo = glm::vec4(newAlbedo, values.albedo.w);
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

            float albedo = luminance(glm::vec3(values.albedo));
            /* Embed the expected probability that is based on the constant blending */
            if(desc.blending == BlendConstant)
            {
                albedo *= values.albedo.w;
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

    void Material::updateTextureCount() const
    {
        mTextureCount = 0;
        auto pTextures = (Texture::SharedPtr*)&mData.textures;

        for (uint32_t i = 0; i < kTexCount; i++)
        {
            if (pTextures[i] != nullptr)
            {
                mTextureCount++;
            }
        }
    }

#if _LOG_ENABLED
#define check_offset(_a) assert(pCB->getVariableOffset(std::string(varName) + "." + #_a) == (offsetof(MaterialData, _a) + offset))
#else
#define check_offset(_a)
#endif

    void Material::setIntoProgramVars(ProgramVars* pVars, ConstantBuffer* pCB, const char varName[]) const
    {
        // OPTME:
        // We can specialize this function based on the API we are using. This might be worth the extra maintenance cost:
        // - DX12 - we could create a descriptor-table with all of the SRVs. This will reduce the API overhead to a single call. Pitfall - the textures might be dirty, so we will need to verify it
        // - Bindless GL - just copy a blob with the GPU pointers. This is actually similar to DX12, but instead of SRVs we store uint64_t
        // - DX11 - Single call at a time.
        // Actually, looks like if we will be smart in the way we design ProgramVars::setTextureArray(), we could get away with a unified code

        // First set the desc and the values
        finalize();
        static const size_t dataSize = sizeof(MaterialDesc) + sizeof(MaterialValues);
        static_assert(dataSize % sizeof(glm::vec4) == 0, "Material::MaterialData size should be a multiple of 16");

        size_t offset = pCB->getVariableOffset(std::string(varName) + ".desc.layers[0].type");

        if(offset == ConstantBuffer::kInvalidOffset)
        {
            logError(std::string("Material::setIntoConstantBuffer() - variable \"") + varName + "\"not found in constant buffer\n");
            return;
        }

        check_offset(values.layers[0].albedo);
        check_offset(values.id);
        assert(offset + dataSize <= pCB->getSize());

        pCB->setBlob(&mData, offset, dataSize);

#ifdef FALCOR_GL
#pragma error Fix material texture bindings for OpenGL
#endif

        // Now set the textures
        std::string resourceName = std::string(varName) + ".textures.layers[0]";
        const auto pResourceDesc = pVars->getReflection()->getResourceDesc(resourceName);
        if (pResourceDesc == nullptr)
        {
            logError(std::string("Material::setIntoConstantBuffer() - can't find the first texture object"));
            return;
        }

        auto pTextures = (Texture::SharedPtr*)&mData.textures;

        for (uint32_t i = 0; i < kTexCount; i++)
        {
            if (pTextures[i] != nullptr)
            {
                pVars->setSrv(pResourceDesc->regIndex + i, pTextures[i]->getSRV());
            }
        }

        pVars->setSampler("gMaterial.samplerState", mData.samplerState);
    }

    bool Material::operator==(const Material& other) const
    {
        return memcmp(&mData, &other.mData, sizeof(mData)) == 0 && mData.samplerState == other.mData.samplerState;
    }

    void Material::evictTextures() const
    {
        Texture::SharedPtr* pTextures = (Texture::SharedPtr*)&mData.textures;
        for(uint32_t i = 0; i < kTexCount ; i++)
        {
            if(pTextures[i])
            {
                pTextures[i]->evict(mData.samplerState.get());
            }
        }
    }

    void Material::setLayerTexture(uint32_t layerId, const Texture::SharedPtr& pTexture)
    {
        mData.textures.layers[layerId] = pTexture;
        mData.desc.layers[layerId].hasTexture = (pTexture != nullptr);
        mDescDirty = true;
    }

    void Material::setNormalMap(Texture::SharedPtr& pNormalMap)
    {
        mData.textures.normalMap = pNormalMap; 
        mData.desc.hasNormalMap = (pNormalMap != nullptr);
        mDescDirty = true;
    }

    void Material::setAlphaMap(const Texture::SharedPtr& pAlphaMap)
    { 
        mData.textures.alphaMap = pAlphaMap;
        mData.desc.hasAlphaMap = (pAlphaMap != nullptr);
        mDescDirty = true;
    }

    void Material::setAmbientOcclusionMap(const Texture::SharedPtr& pAoMap)
    {
        mData.textures.ambientMap = pAoMap;
        mData.desc.hasAmbientMap = (pAoMap != nullptr);
        mDescDirty = true;
    }

    void Material::setHeightMap(const Texture::SharedPtr& pHeightMap)
    { 
        mData.textures.heightMap = pHeightMap;
        mData.desc.hasHeightMap = (pHeightMap != nullptr);
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
            normalize();
            updateTextureCount();
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

//         shaderDcl = "{{";
//         for(uint32_t layerId = 0; layerId < arraysize(mData.desc.layers); layerId++)
//         {
//             const MaterialLayerDesc& layer = mData.desc.layers[layerId];
//             shaderDcl += '{' + getLayerTypeStr(layer.type) + ',';
//             shaderDcl += getLayerNdfStr(layer.ndf) + ',';
//             shaderDcl += getLayerBlendStr(layer.blending) + ',';
//             shaderDcl += std::to_string(layer.hasAlbedoTexture) + ',' + std::to_string(layer.hasRoughnessTexture) + ',' + std::to_string(layer.hasExtraParamTexture) + ",{" + std::to_string(layer.pad.x) + "," + std::to_string(layer.pad.y) + '}';
//             shaderDcl += '}';
//             if(layerId != arraysize(mData.desc.layers) - 1)
//             {
//                 shaderDcl += ',';
//             }
//         }
//         shaderDcl += "},";
//        shaderDcl += std::to_string(mData.desc.hasAlphaMap) + ',' + std::to_string(mData.desc.hasNormalMap) + ',' + std::to_string(mData.desc.hasHeightMap) + ',' + std::to_string(mData.desc.hasAmbientMap);
//
//        shaderDcl += "{";
//        for(uint32_t layerType = 0; layerType < MatNumTypes; layerType++)
//        {
//            shaderDcl += "{" + std::to_string(mData.desc.layerIdByType[layerType].id) + "}";
//            if(layerType != MatNumTypes - 1)
//                shaderDcl += ',';
//        }
//        shaderDcl += "},";
//        shaderDcl += "0,0"; // Padding
//         shaderDcl += '}';
    }

    // Spire stuff

	std::string ReplaceStr(std::string templateStr, std::string name, std::string content)
	{
		auto pos = templateStr.find(name);
		return templateStr.substr(0, pos) + content + templateStr.substr(pos + name.length());
	}

    ProgramReflection::ComponentClassReflection::SharedPtr const& Material::getSpireComponentClass() const
    {
        auto spireContext = ShaderRepository::Instance().GetContext();

        if( !mSpireComponentClass )
        {
			if (UseGeneralShader)
				mSpireComponentClass = ShaderRepository::Instance().findComponentClass("GeneralMaterial");
			else
			{
				int layerCount = this->getNumLayers();
				// currently material module source depend on number of layers and type of each layer
				// we encode the material module configuration in the module name
				std::string moduleName = "Material" + std::to_string(layerCount);
				for (int i = 0; i < layerCount; i++)
				{
					moduleName += "_";
					auto layer = getLayer(i);
					if (layer.pTexture)
						moduleName += "M";
					else
						moduleName += "C";
					moduleName += std::to_string((int)layer.type);
					moduleName += std::to_string((int)layer.ndf);
					moduleName += std::to_string((int)layer.blend);
				}
				// check if we have already generated the material module
				SpireModule* spireComponentClass = spFindModule(spireContext, moduleName.c_str());
				if (!spireComponentClass)
				{
					// if module not found, generate it
					std::stringstream sb;
					std::string kernelStr;
					std::string fullPath;
					if (findFileInDataDirectories("MaterialInclude.spire", fullPath))
					{
						readFileToString(fullPath, kernelStr);
						sb << "module " << moduleName << " implements Material\n{\n";
						std::stringstream sbEval, sbUsing;
						for (int i = 0; i < layerCount; i++)
						{
							sbUsing << "using layer" << i << " = MaterialLayer(" << (getLayer(i).pTexture ? "1" : "0") << ", " << (int)getLayer(i).type
								<< ", " << (int)getLayer(i).ndf << ", " << (int)getLayer(i).blend << ");\n";

							sbEval << "layer" << i << ".evalMaterialLayer(shAttr, lAttr, passResult);\n";
						}
						kernelStr = ReplaceStr(kernelStr, "$LAYER_EVAL", sbEval.str());
						kernelStr = ReplaceStr(kernelStr, "$LAYER_USING", sbUsing.str());

						sb << kernelStr;
						sb << "\n}\n";
						auto moduleStr = sb.str();
						spireComponentClass = ShaderRepository::Instance().CreateLibraryModuleFromSource(moduleStr.c_str(), moduleName.c_str());
					}
					else
						logError("Cannot find 'MaterialInclude.spire'.");

				}
				// 
				mSpireComponentClass = ShaderRepository::Instance().findComponentClass(spireComponentClass);
			}
            // TODO: Here is where we'd need to construct an appropriate component
            // class for the material, based on the data in the layers.
            //
            // TODO: the proper logic for invalidating and re-generating this
            // also needs to be worked out...
            /*if( mData.textures.layers[0] )
            {
                mSpireComponentClass = spFindModule(spireContext, "TexturedMaterial");
            }
            else
            {
                mSpireComponentClass = spFindModule(spireContext, "ConstantMaterial");
            }*/
            assert(mSpireComponentClass);
        }

        return mSpireComponentClass;
    }

    ComponentInstance::SharedPtr const& Material::getSpireComponentInstance() const
    {
        if( !mSpireComponentInstance )
        {
            mSpireComponentInstance = ComponentInstance::create(getSpireComponentClass());
            mDescDirty = true;
        }

        // fill in the instance if data is dirty...
        //
        // TODO: this is a bit hacky, because we are using the same dirty
        // flag for two different things...
        if( mDescDirty )
        {
            finalize();
			if (auto normalMap = this->getNormalMap())
			{
				mSpireComponentInstance->setTexture("normalMap", normalMap);
				mSpireComponentInstance->setVariable("hasNormalMap", true);
			}
			if (auto alphaMap = this->getAlphaMap())
			{
				mSpireComponentInstance->setTexture("alphaMap", alphaMap);
				mSpireComponentInstance->setVariable("hasAlphaMap", true);
			}
			if (auto ambientMap = this->getAmbientOcclusionMap())
			{
				mSpireComponentInstance->setTexture("ambientMap", ambientMap);
				mSpireComponentInstance->setVariable("hasAmbientMap", true);
			}
			if (auto heightMap = this->getHeightMap())
			{
				mSpireComponentInstance->setTexture("heightMap", heightMap);
				mSpireComponentInstance->setVariable("hasHeightMap", true);
			}
			mSpireComponentInstance->setSampler("samplerState", mData.samplerState);
			mSpireComponentInstance->setVariable("height", getHeightModifiers());
			mSpireComponentInstance->setVariable("id", getId());
			mSpireComponentInstance->setVariable("alphaThreshold", getAlphaThreshold());
			for (auto i = 0u; i < getNumLayers(); i++)
			{
				auto layer = getLayer(i);
				auto subModule = spModuleGetSubModule(mSpireComponentClass->getSpireComponentClass(), i);
				auto offset = spModuleGetBufferOffset(subModule);
				mSpireComponentInstance->setBlob(&layer.getValues(), (size_t)offset, sizeof(LayerValues));
				SpireBindingIndex index;
				spModuleGetBindingOffset(subModule, &index);
				if (layer.pTexture)
					mSpireComponentInstance->setSrv(index.texture, layer.pTexture->getSRV(), layer.pTexture);
			}
			// if we are using general shader, set additional layer parameters 
			if (UseGeneralShader)
			{
				/*
				param int4 hasTexture;
				param int4 materialType;
				param int4 ndf;
				param int4 blend;
				*/
				int hasTexture[MatMaxLayers];
				int materialType[MatMaxLayers];
				int ndf[MatMaxLayers];
				int blend[MatMaxLayers];
				for (auto i = 0u; i < std::min(getNumLayers(), (uint32)MatMaxLayers); i++)
				{
					auto layer = getLayer(i);
					hasTexture[i] = layer.pTexture != nullptr;
					materialType[i] = (int)layer.type;
					ndf[i] = (int)layer.ndf;
					blend[i] = (int)layer.blend;
				}
				mSpireComponentInstance->setVariableBlob("hasTexture", hasTexture, sizeof(int)*MatMaxLayers);
				mSpireComponentInstance->setVariableBlob("materialType", materialType, sizeof(int)*MatMaxLayers);
				mSpireComponentInstance->setVariableBlob("ndf", ndf, sizeof(int)*MatMaxLayers);
				mSpireComponentInstance->setVariableBlob("blend", blend, sizeof(int)*MatMaxLayers);
				mSpireComponentInstance->setVariable("layerCount", (int)getNumLayers());
			}
			// Fill in the values for the material fields, if anything has changed.
            //
            // TODO: we want to copy in data from our "sub-component instances"
            // (the layers), which we assume will match the "component instance"
            // we created for the material.

            //if( mData.textures.layers[0] )
            //{
            //    // `TexturedMaterial`
                //mSpireComponentInstance->setTexture("diffuseMap", mData.textures.layers[0].get());
            //    mSpireComponentInstance->setSampler("samplerState", mData.samplerState.get());
            //}
            //else
            //{
            //    // `ConstantMaterial`
            //    mSpireComponentInstance->setVariable("diffuseVal", mData.values.layers[0].albedo);
            //}

        }

        return mSpireComponentInstance;
    }

}
