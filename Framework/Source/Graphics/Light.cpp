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
#include "light.h"
#include "Utils/Gui.h"
#include "API/UniformBuffer.h"
#include "API/Buffer.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "Data/VertexAttrib.h"
#include "Graphics/Model/Model.h"

using glm::ivec3;

namespace Falcor
{
#if _LOG_ENABLED
#define check_offset(_a) assert(pBuffer->getVariableOffset(varName + "." + #_a) == (offsetof(LightData, _a) + offset))
#else
#define check_offset(_a)
#endif

    uint32_t Light::sCount = 0;

    Light::Light()
    {
        mIndex = sCount;
        sCount++;
    }

	void Light::setIntoUniformBuffer(UniformBuffer* pBuffer, const std::string& varName)
	{
		static const size_t dataSize = sizeof(LightData);
		static_assert(dataSize % sizeof(float) * 4 == 0, "LightData size should be a multiple of 16");

		size_t offset = pBuffer->getVariableOffset(varName + ".worldPos");
		if (offset == UniformBuffer::kInvalidUniformOffset)
		{
			Logger::log(Logger::Level::Warning, "AreaLight::setIntoUniformBuffer() - variable \"" + varName + "\"not found in uniform buffer\n");
			return;
		}

		check_offset(worldDir);
		check_offset(intensity);
		check_offset(aabbMin);
		check_offset(aabbMax);
		check_offset(transMat);
        check_offset(material.desc.layers[0].type);
        check_offset(material.values.layers[0].albedo.texture.ptr);
        check_offset(material.values.ambientMap.texture.ptr);
        check_offset(material.values.id);
		check_offset(numIndices);
		assert(offset + dataSize <= pBuffer->getBuffer()->getSize());

		pBuffer->setBlob(&mData, offset, dataSize);
	}

    void Light::resetGlobalIdCounter()
    {
        sCount = 0;
    }

    void GUI_CALL Light::GetColorCB(void* pVal, void* pUserData)
    {
        Light* light = (Light*)pUserData;

        if ((light->mUiLightIntensityColor * light->mUiLightIntensityScale) != light->mData.intensity)
        {
            float mag = max(light->mData.intensity.x, max(light->mData.intensity.y, light->mData.intensity.z));
            if (mag <= 1.f)
            {
                light->mUiLightIntensityColor = light->mData.intensity;
                light->mUiLightIntensityScale = 1.0f;
            }
            else
            {
                light->mUiLightIntensityColor = light->mData.intensity / mag;
                light->mUiLightIntensityScale = mag;
            }
        }

        *(glm::vec3*)pVal = light->mUiLightIntensityColor;
    }

    void GUI_CALL Light::SetColorCB(const void* pVal, void* pUserData)
    {
        Light* light = (Light*)pUserData; 
        glm::vec3 color = *(glm::vec3*)pVal;

        light->mUiLightIntensityColor = color;
        light->mData.intensity = (light->mUiLightIntensityColor * light->mUiLightIntensityScale);
    }


    void GUI_CALL Light::GetIntensityCB(void* pVal, void* pUserData)
    {
        Light* light = (Light*)pUserData; 

        if ((light->mUiLightIntensityColor * light->mUiLightIntensityScale) != light->mData.intensity)
        {
            float mag = max(light->mData.intensity.x, max(light->mData.intensity.y, light->mData.intensity.z));
            if (mag <= 1.f)
            {
                light->mUiLightIntensityColor = light->mData.intensity;
                light->mUiLightIntensityScale = 1.0f;
            }
            else
            {
                light->mUiLightIntensityColor = light->mData.intensity / mag;
                light->mUiLightIntensityScale = mag;
            }
        }

        *(float*)pVal = light->mUiLightIntensityScale;
    }

    void GUI_CALL Light::SetIntensityCB(const void* pVal, void* pUserData)
    {
        Light* light = (Light*)pUserData;
        float intensityScale = *(float*)pVal;

        light->mUiLightIntensityScale = intensityScale;
        light->mData.intensity = (light->mUiLightIntensityColor * light->mUiLightIntensityScale);
    }

	DirectionalLight::DirectionalLight()
	{
		mData.type = LightDirectional;
		mName = "dirLight" + std::to_string(mIndex);
	}

    DirectionalLight::SharedPtr DirectionalLight::create()
    {
        DirectionalLight* pLight = new DirectionalLight;
        return SharedPtr(pLight);
    }

    DirectionalLight::~DirectionalLight() = default;

    void DirectionalLight::setUiElements(Gui* pGui, const std::string& uiGroup)
    {
        pGui->addDir3FVarWithCallback("Direction", &DirectionalLight::setDirectionCB, &DirectionalLight::getDirectionCB, this, uiGroup);
        pGui->addRgbColorWithCallback("Color",     SetColorCB,     GetColorCB,     this, uiGroup);
        pGui->addFloatVarWithCallback("Intensity", SetIntensityCB, GetIntensityCB, this, uiGroup, 0.0f, 1e15f, 0.1f);
    }

    void DirectionalLight::setWorldDirection(const glm::vec3& dir)
    {
        mData.worldDir = normalize(dir);
        mData.worldPos = -mData.worldDir * 1e8f; // Move light's position sufficiently far away
    }

	void DirectionalLight::prepareGPUData()
	{		
	}

	void DirectionalLight::unloadGPUData()
	{		
	}

    void DirectionalLight::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
	{
		Logger::log(Logger::Level::Error, "DirectionalLight::move() is not used and thus not implemented for now.");
	}


    void GUI_CALL DirectionalLight::setDirectionCB(const void* pVal, void* pUserData)
    {
        DirectionalLight* pLight = (DirectionalLight*)pUserData;
        pLight->setWorldDirection(*(glm::vec3*)pVal);
    }

    void GUI_CALL DirectionalLight::getDirectionCB(void* pVal, void* pUserData)
    {
        const DirectionalLight* pLight = (DirectionalLight*)pUserData;
        *(glm::vec3*)pVal = pLight->mData.worldDir;
    }

    PointLight::SharedPtr PointLight::create()
    {
        PointLight* pLight = new PointLight;
        return SharedPtr(pLight);
    }

	PointLight::PointLight()
	{
		mData.type = LightPoint;
		mName = "pointLight" + std::to_string(mIndex);
	}

    PointLight::~PointLight() = default;

	void GUI_CALL PointLight::setOpeningAngleCB(const void* pVal, void* pUserData)
	{
		PointLight* pLight = (PointLight*)pUserData;
		pLight->setOpeningAngle(*(float*)pVal);
	}

	void GUI_CALL PointLight::getOpeningAngleCB(void* pVal, void* pUserData)
	{
		const PointLight* pLight = (PointLight*)pUserData;
		*(float*)pVal = pLight->getOpeningAngle();
	}

	void GUI_CALL PointLight::setPenumbraAngleCB(const void* pVal, void* pUserData)
	{
		PointLight* pLight = (PointLight*)pUserData;
        pLight->setPenumbraAngle(*(float*)pVal);
	}

	void GUI_CALL PointLight::getPenumbraAngleCB(void* pVal, void* pUserData)
	{
		const PointLight* pLight = (PointLight*)pUserData;
		*(float*)pVal = pLight->getPenumbraAngle();
	}

    void PointLight::setUiElements(Gui* pGui, const std::string& uiGroup)
    {
        std::string posGroup = "worldPos" + std::to_string(mIndex);
        pGui->addFloatVar("x", &mData.worldPos.x, posGroup, -FLT_MAX, FLT_MAX);
        pGui->addFloatVar("y", &mData.worldPos.y, posGroup, -FLT_MAX, FLT_MAX);
        pGui->addFloatVar("z", &mData.worldPos.z, posGroup, -FLT_MAX, FLT_MAX);
        pGui->nestGroups(uiGroup, posGroup);
        pGui->setVarTitle(posGroup, "World Position");

		pGui->addDir3FVar("Direction", &mData.worldDir, uiGroup);
		pGui->addFloatVarWithCallback("Opening Angle", &PointLight::setOpeningAngleCB, &PointLight::getOpeningAngleCB, this, uiGroup, 0.f, (float)M_PI);
		pGui->addFloatVarWithCallback("Penumbra Width", &PointLight::setPenumbraAngleCB, &PointLight::getPenumbraAngleCB, this, uiGroup, 0.f, (float)M_PI);
        pGui->addRgbColorWithCallback("Color", SetColorCB, GetColorCB, this, uiGroup);
        pGui->addFloatVarWithCallback("Intensity", SetIntensityCB, GetIntensityCB, this, uiGroup, 0.0f, 1000000.0f, 0.1f);
	}

    void PointLight::setOpeningAngle(float openingAngle)
	{
		openingAngle = glm::clamp(openingAngle, 0.f, (float)M_PI);
		mData.openingAngle = openingAngle;
		/* Prepare an auxiliary cosine of the opening angle to quickly check whether we're within the cone of a spot light */
		mData.cosOpeningAngle = cos(openingAngle);
	}

	void PointLight::prepareGPUData()
	{
	}

	void PointLight::unloadGPUData()
	{
	}

    void PointLight::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
	{
		mData.worldPos = position;
        mData.worldDir = target - position;
	}

	AreaLight::SharedPtr AreaLight::create()
	{
		AreaLight* pLight = new AreaLight;
		return SharedPtr(pLight);
	}

	AreaLight::AreaLight()
	{
		mData.type = LightArea;
		mName = std::string("areaLight") + std::to_string(mIndex);
	}

	AreaLight::~AreaLight() = default;

	void AreaLight::setUiElements(Gui* pGui, const std::string& uiGroup)
	{
		Logger::log(Logger::Level::Error, "AreaLight::setUiElements() is not used and thus not implemented for now.");
	}

	void AreaLight::setIntoUniformBuffer(UniformBuffer* pBuffer, const std::string& varName)
	{
		// Upload data to GPU
		prepareGPUData();

		// Call base class method;
		Light::setIntoUniformBuffer(pBuffer, varName);
	}

	void AreaLight::prepareGPUData()
	{
		// Set OGL buffer pointers for indices, vertices, and texcoord
		if (mData.indexPtr.ptr == 0ull)
		{
			mData.indexPtr.ptr = mIndexBuf->makeResident();
			mData.vertexPtr.ptr = mVertexBuf->makeResident();
			if (mTexCoordBuf)
				mData.texCoordPtr.ptr = mTexCoordBuf->makeResident();
			// Store the mesh CDF buffer id
			mData.meshCDFPtr.ptr = mMeshCDFBuf->makeResident();
		}
		mData.numIndices = uint32_t(mIndexBuf->getSize() / sizeof(glm::ivec3));

		// Get the surface area of the geometry mesh
		mData.surfaceArea = mSurfaceArea;

		// Fetch the mesh instance transformation
		mData.transMat = mMeshData.pMesh->getInstanceMatrix(mMeshData.instanceId);

		// Copy the material data
		const Material::SharedPtr& pMaterial = mMeshData.pMesh->getMaterial();
		if (pMaterial)
			memcpy(&mData.material, &pMaterial->getData(), sizeof(MaterialData));
	}

	void AreaLight::unloadGPUData()
	{
	    // Unload GPU data by calling evict()
		mIndexBuf->evict();
		mVertexBuf->evict();
		if (mTexCoordBuf)
			mTexCoordBuf->evict();
		mMeshCDFBuf->evict();
	}

	void AreaLight::setMeshData(const Mesh::SharedPtr& pMesh, uint32_t instanceId)
	{
		if (pMesh)
		{
			mMeshData.pMesh = pMesh;
			mMeshData.instanceId = instanceId;

			auto& vao = mMeshData.pMesh->getVao();
		
            setIndexBuffer(vao->getIndexBuffer());

			int32_t posIdx = vao->getElementIndexByLocation(VERTEX_POSITION_LOC).vbIndex;
			assert(posIdx != Vao::ElementDesc::kInvalidIndex);
            setPositionsBuffer(vao->getVertexBuffer(posIdx));

			const int32_t uvIdx = vao->getElementIndexByLocation(VERTEX_TEXCOORD_LOC).vbIndex;
			bool hasUv = uvIdx != Vao::ElementDesc::kInvalidIndex;
			if (hasUv)
			{
                setTexCoordBuffer(vao->getVertexBuffer(VERTEX_TEXCOORD_LOC));
			}

			// Compute surface area of the mesh and generate probability
			// densities for importance sampling a triangle mesh
			computeSurfaceArea();

            // Check if this mesh has a material
            const Material::SharedPtr& pMaterial = pMesh->getMaterial();
            if(pMaterial)
            {
                for(uint32_t layerId = 0; layerId < pMaterial->getNumActiveLayers(); ++layerId)
                {
                    if(pMaterial->getLayerDesc(layerId)->type == MatEmissive)
                    {
                        mData.intensity = v3(pMaterial->getLayerValues(layerId)->albedo.constantColor);
                        break;
                    }
                }
            }
		}
	}

	void AreaLight::computeSurfaceArea()
	{
		if(mVertexBuf && mIndexBuf)
        {
			// Read data from the buffers
			std::vector<ivec3> indices(mMeshData.pMesh->getPrimitiveCount());
			mIndexBuf->readData(indices.data(), 0, mIndexBuf->getSize());
			std::vector<vec3> vertices(mVertexBuf->getSize() / sizeof(vec3));
			mVertexBuf->readData(vertices.data(), 0, mVertexBuf->getSize());

			// Calculate surface area of the mesh
			mSurfaceArea = 0.f;
			mMeshCDF.push_back(0.f);
			for (uint32_t i = 0; i < mMeshData.pMesh->getPrimitiveCount(); ++i)
			{
				ivec3 pId = indices[i];
				const vec3 p0(vertices[pId.x]), p1(vertices[pId.y]),  p2(vertices[pId.z]);

				mSurfaceArea += 0.5f * glm::length(glm::cross(p1 - p0, p2 - p0));

				// Add an entry using surface area measure as the discrete probability
				mMeshCDF.push_back(mMeshCDF[mMeshCDF.size() - 1] + mSurfaceArea);
			}

			// Normalize the probability densities
			if (mSurfaceArea > 0.f)
			{
				float invSurfaceArea = 1.f / mSurfaceArea;
				for (uint32_t i = 1; i < mMeshCDF.size(); ++i)
				{
					mMeshCDF[i] *= invSurfaceArea;
				}
						
				mMeshCDF[mMeshCDF.size() - 1] = 1.f;
			}

            // Create a CDF buffer
            auto pCDFBuffer = Buffer::create(sizeof(mMeshCDF[0])*mMeshCDF.size(), Buffer::BindFlags::ShaderResource, Buffer::AccessFlags::None, mMeshCDF.data());
            setMeshCDFBuffer(pCDFBuffer);

			// Set the world position and world direction of this light
			if (!vertices.empty() && !indices.empty())
			{
				glm::vec3 boxMin = vertices[0];
				glm::vec3 boxMax = vertices[0];
				for (uint32_t id = 1; id < vertices.size(); ++id)
				{
					boxMin = glm::min(boxMin, vertices[id]);
					boxMax = glm::max(boxMax, vertices[id]);
				}

				mData.worldPos = BoundingBox::fromMinMax(boxMin, boxMax).center;

				// This holds only for planar light sources
				const glm::vec3& p0 = vertices[indices[0].x];
				const glm::vec3& p1 = vertices[indices[0].y];
				const glm::vec3& p2 = vertices[indices[0].z];

                // Take the normal of the first triangle as a light normal
				mData.worldDir = normalize(cross(p1 - p0, p2 - p0));

                // Save the axis-aligned bounding box
                mData.aabbMin = boxMin;
                mData.aabbMax = boxMax;
			}
		}
	}

	Light::SharedPtr createAreaLight(const Mesh::SharedPtr& pMesh, uint32_t instanceId)
	{
		// Create an area light
		AreaLight::SharedPtr pAreaLight = AreaLight::create();
		if (pAreaLight)
		{
			// Set the geometry mesh
			pAreaLight->setMeshData(pMesh, instanceId);
		}

		return pAreaLight;
	}

	void createAreaLightsForModel(const Model* pModel, std::vector<Light::SharedPtr>& areaLights)
	{
		assert(pModel);

		// Get meshes for this model
		for (uint32_t meshId = 0; meshId < pModel->getMeshCount(); ++meshId)
		{
			const Mesh::SharedPtr& pMesh = pModel->getMesh(meshId);

			// Obtain mesh instances for this mesh
			for (uint32_t meshInstanceId = 0; meshInstanceId < pMesh->getInstanceCount(); ++meshInstanceId)
			{
				// Check if this mesh has a material
				const Material::SharedPtr& pMaterial = pMesh->getMaterial();
				if (pMaterial)
				{
					size_t numLayers = pMaterial->getNumActiveLayers();
					const MaterialLayerDesc* pLayerDesc = nullptr;

					for (uint32_t layerId = 0; layerId < numLayers; ++layerId)
					{
						pLayerDesc = pMaterial->getLayerDesc(layerId);
						if (pLayerDesc->type == MatEmissive)
						{
							// Create an area light for an emissive material
							areaLights.push_back(createAreaLight(pMesh, meshInstanceId));
							break;
						}
					}
				}
			}			
		}			
	}

	void AreaLight::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
	{
		if(mMeshData.pMesh)
		{
			mat4 mx = mMeshData.pMesh->getInstanceMatrix(mMeshData.instanceId);
			mx[3] = v4(position, 1.f);
			mMeshData.pMesh->setInstanceMatrix(mMeshData.instanceId, mx);
		}
	}
}