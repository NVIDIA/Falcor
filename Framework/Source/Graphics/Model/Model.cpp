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
#include "Model.h"
#include "Loaders/AssimpModelImporter.h"
#include "Loaders/BinaryModelImporter.h"
#include "Loaders/BinaryModelExporter.h"
#include "Utils/OS.h"
#include "mesh.h"
#include "glm/geometric.hpp"
#include "AnimationController.h"
#include "Animation.h"
#include "Core/Buffer.h"
#include "Core/Texture.h"
#include "Graphics/TextureHelper.h"
#include "Utils/StringUtils.h"
#include "Graphics/Camera/Camera.h"
#include "core/VAO.h"

namespace Falcor
{
	uint32_t Model::sModelCounter = 0;
    const char* Model::kSupportedFileFormatsStr = "Supported Formats\0*.obj;*.bin;*.dae;*.x;*.md5mesh;*.ply;*.fbx;*.3ds;*.blend;*.ase;*.ifc;*.xgl;*.zgl;*.dxf;*.lwo;*.lws;*.lxo;*.stl;*.x;*.ac;*.ms3d;*.cob;*.scn;*.3d;*.mdl;*.mdl2;*.pk3;*.smd;*.vta;*.raw;*.ter\0\0";

    // Method to sort meshes
    bool compareMeshes(const Mesh::SharedPtr& p1, const Mesh::SharedPtr& p2)
    {
        // This relies on the fact that Model keeps only unique copies of materials, so same material == same address.
        // See GetOrAddMaterial() for more info
        return p1->getMaterial() < p2->getMaterial();
    }

	Model::Model() : mId(sModelCounter++)
    {

    }

    Model::~Model() = default;

    /** Permanently transform all meshes of the object by the given transform
    */
    void Model::applyTransform(const glm::mat4& transform) 
	{
        for(auto& pMesh : mpMeshes)
        {
            pMesh->applyTransform(transform);
        }

        calculateModelProperties();
    }

    Model::SharedPtr Model::createFromFile(const std::string& filename, uint32_t flags)
    {
        Model::SharedPtr pModel;

        if(hasSuffix(filename, ".bin", false))
        {
            pModel = BinaryModelImporter::createFromFile(filename, flags);
        }
        else
        {
            pModel = AssimpModelImporter::createFromFile(filename, flags);
        }

        if(pModel)
        {
            if(flags & CompressTextures)
            {
                pModel->compressAllTextures();
            }

            pModel->calculateModelProperties();
        }

        return pModel;
    }

    void Model::exportToBinaryFile(const std::string& filename)
    {
        if(hasSuffix(filename, ".bin", false) == false)
        {
            Logger::log(Logger::Level::Warning, "Exporting model to binary file, but extension is not '.bin'. This will cause error when loading the file");
        }

        BinaryModelExporter::exportToFile(filename, this);
    }

    void Model::calculateModelProperties()
    {
        mVertexCount = 0;
        mPrimitiveCount = 0;
        mInstanceCount = 0;

        // Sort the meshes
        //std::sort(mpMeshes.begin(), mpMeshes.end(), compareMeshes);

        vec3 modelMin = vec3(1e25f), modelMax = vec3(-1e25f);
        std::map<const Buffer*, bool> vbFound;

        for(const auto& pMesh : mpMeshes)
        {
            for(uint32_t i = 0 ; i < pMesh->getInstanceCount() ; i++)
            {
                const BoundingBox& meshBox = pMesh->getInstanceBoundingBox(i);

                vec3 meshMin = meshBox.center - meshBox.extent * 0.5f;
                vec3 meshMax = meshBox.center + meshBox.extent * 0.5f;

                modelMin = min(modelMin, meshMin);
                modelMax = max(modelMax, meshMax);

                if(vbFound.find(pMesh->getVao()->getVertexBuffer(0).get()) == vbFound.end())
                {
                    mVertexCount += pMesh->getVertexCount();
                    vbFound[pMesh->getVao()->getVertexBuffer(0).get()] = true;
                }

                mPrimitiveCount += pMesh->getPrimitiveCount();
            }
            mInstanceCount += pMesh->getInstanceCount();
        }
        mCenter = (modelMin + modelMax) * 0.5f;
        mRadius = glm::length(modelMin - modelMax) * 0.5f;
    }

    void Model::animate(double currentTime)
    {
        if(mpAnimationController)
        {
            mpAnimationController->animate(currentTime);
        }
    }

    bool Model::hasAnimations() const
    {
        return (getAnimationsCount() != 0);
    }

    uint32_t Model::getAnimationsCount() const
    {
        return mpAnimationController ? (mpAnimationController->getAnimationCount()) : 0;
    }

    uint32_t Model::getActiveAnimation() const
    {
        return mpAnimationController->getActiveAnimation();
    }

    const std::string& Model::getAnimationName(uint32_t animationID) const
    {
        assert(mpAnimationController);
        assert(animationID < getAnimationsCount());
        return mpAnimationController->getAnimationName(animationID);
    }

    void Model::setBindPose()
    {
        if(mpAnimationController)
        {
            mpAnimationController->setActiveAnimation(BIND_POSE_ANIMATION_ID);
        }
    }

    void Model::setActiveAnimation(uint32_t animationID)
    {
        assert(animationID < getAnimationsCount() || animationID == BIND_POSE_ANIMATION_ID);
        mpAnimationController->setActiveAnimation(animationID);
    }

    bool Model::hasBones() const
    {
        return (getBonesCount() != 0);
    }

    uint32_t Model::getBonesCount() const
    {
        return mpAnimationController ? mpAnimationController->getBoneCount() : 0;
    }

    const glm::mat4* Model::getBonesMatrices() const
    {
        assert(mpAnimationController);
        return mpAnimationController->getBoneMatrices();
    }

	void Model::bindSamplerToMaterials(const Sampler::SharedPtr& pSampler)
	{
		// Go over all materials and bind the sampler
		for(auto& m : mpMaterials)
		{
			m->overrideAllSamplers(pSampler);
		}
	}

    void Model::setAnimationController(AnimationController::UniquePtr pAnimController)
    {
        mpAnimationController = std::move(pAnimController);
    }

    void Model::addMesh(Mesh::SharedPtr pMesh)
    {
        mpMeshes.push_back(std::move(pMesh));
    }

    Material::SharedPtr Model::getOrAddMaterial(const Material::SharedPtr& pMaterial)
    {
        // Check if the material already exists
        for(const auto& a : mpMaterials)
        {
            if(*pMaterial == *a)
            {
                return a;
            }
        }

        // New material
        mpMaterials.push_back(pMaterial);
        return pMaterial;
    }

    void Model::addBuffer(const Buffer::SharedConstPtr& pBuffer)
    {
        mpBuffers.push_back(pBuffer);
    }

    void Model::addTexture(const Texture::SharedConstPtr& pTexture)
    {
        mpTextures.push_back(pTexture);
    }
    
    template<typename T>
    void removeNullElements(std::vector<T>& Vec)
    {
        auto Pred = [](T& t) {return t == nullptr; };
        auto& NewEnd = std::remove_if(Vec.begin(), Vec.end(), Pred);
        Vec.erase(NewEnd, Vec.end());
    }

    void Model::deleteCulledMeshes(const Camera* pCamera)
    {
        std::map<const Material*, bool> usedMaterials;
        std::map<const Buffer*, bool> usedBuffers;

        // Loop over all the meshes and remove its instances
        for(auto& pMesh : mpMeshes)
        {
            pMesh->deleteCulledInstances(pCamera);

            if(pMesh->getInstanceCount() == 0)
            {
                pMesh = nullptr;
            }
            else
            {
                // Mark the mesh's objects as used
                usedMaterials[pMesh->getMaterial().get()] = true;
                const auto pVao = pMesh->getVao();
                usedBuffers[pVao->getIndexBuffer().get()] = true;
                for(uint32_t i = 0 ; i < pVao->getVertexBuffersCount() ; i++)
                {
                    usedBuffers[pVao->getVertexBuffer(i).get()] = true;
                }
            }
        }

        // Remove unused meshes from the vector
        removeNullElements(mpMeshes);

        deleteUnusedBuffers(usedBuffers);
        deleteUnusedMaterials(usedMaterials);
        calculateModelProperties();
    }


    void Model::resetGlobalIdCounter()
    {
        sModelCounter = 0;
        Mesh::resetGlobalIdCounter();
    }

    void Model::deleteUnusedBuffers(std::map<const Buffer*, bool> usedBuffers)
    {
        for(auto& Buffer : mpBuffers)
        {
            if(usedBuffers.find(Buffer.get()) == usedBuffers.end())
            {
                Buffer = nullptr;
            }            
        }
        removeNullElements(mpBuffers);
    }

    void Model::deleteUnusedMaterials(std::map<const Material*, bool> usedMaterials)
    {
        std::map<const Texture*, bool> usedTextures;
        for(auto& material : mpMaterials)
        {
            if(usedMaterials.find(material.get()) == usedMaterials.end())
            {
                // Material is not used. Remove it.
                material = nullptr;
            }
            else
            {
                // Material is used. Mark its textures
                std::vector<Texture::SharedConstPtr> activeTextures;
                material->getActiveTextures(activeTextures);
                for(const auto& tex : activeTextures)
                {
                    usedTextures[tex.get()] = true;
                }
            }
        }
        removeNullElements(mpMaterials);

        // Now remove unused textures
        for(auto& texture : mpTextures)
        {
            if(usedTextures.find(texture.get()) == usedTextures.end())
            {
                texture = nullptr;
            }
        }
        removeNullElements(mpTextures);
    }

    void Model::compressAllTextures()
    {
        std::map<const Texture*, uint32_t> texturesIndex;
        std::vector<bool> isNormalMap(mpTextures.size(), false);

        // Find all normal maps. We don't compress them.
        for(uint32_t i = 0; i < mpTextures.size(); i++)
        {
            texturesIndex[mpTextures[i].get()] = i;
        }
        
        for(const auto& m : mpMaterials)
        {
            const auto& pNormalMap = m->getNormalValue().texture.pTexture;
            if(pNormalMap)
            {
                uint32_t Id = texturesIndex[pNormalMap.get()];
                isNormalMap[Id] = true;
            }
        }

        std::vector<uint8_t> texData;

        // Now create the textures
        for(uint32_t i = 0; i < mpTextures.size(); i++)
        {
            if(isNormalMap[i] == true)
            {
                // Not compressing normal map
                continue;
            }

            Texture* pTexture = const_cast<Texture*>(mpTextures[i].get());
            assert(pTexture->getType() == Texture::Type::Texture2D);
            pTexture->compress2DTexture();            
        }
    }
}
