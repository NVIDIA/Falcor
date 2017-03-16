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
#include <map>
#include <vector>
#include "Graphics/Model/Loaders/ModelImporter.h"
#include "../AnimationController.h"
#include "../Mesh.h"
#include "../Model.h"

struct aiScene;
struct aiNode;
struct aiAnimation;
struct aiMesh;
struct aiMaterial;

namespace Falcor
{
    class Animation;
    class Buffer;
    class VertexBufferLayout;
    class Texture;

    class AssimpModelImporter : public ModelImporter
    {
    public:
        /** create a new model using ASSIMP
            \param[in] filename Model's filename. Loader will look for it in the data directories.
            \param[in] flags Flags controlling model creation
            returns nullptr if loading failed, otherwise a new Model object
        */
        static Model::SharedPtr createFromFile(const std::string& filename, uint32_t flags);

    private:

        using IdToMesh = std::unordered_map<uint32_t, Mesh::SharedPtr>;

        AssimpModelImporter(uint32_t flags);
        AssimpModelImporter(const AssimpModelImporter&) = delete;
        void operator=(const AssimpModelImporter&) = delete;

        bool initModel(const std::string& filename);
        bool createDrawList(const aiScene* pScene);
        bool parseAiSceneNode(const aiNode* pCurrent, const aiScene* pScene, IdToMesh& aiToFalcorMesh);
        bool createAllMaterials(const aiScene* pScene, const std::string& modelFolder, bool isObjFile, bool useSrgb);

        void createAnimationController(const aiScene* pScene);
        void initializeBones(const aiScene* pScene);
        uint32_t initBone(const aiNode* pNode, uint32_t parentID, uint32_t boneID);
        void initializeBonesOffsetMatrices(const aiScene* pScene);

        Animation::UniquePtr createAnimation(const aiAnimation* pAiAnim);

        Mesh::SharedPtr createMesh(const aiMesh* pAiMesh);
        VertexLayout::SharedPtr createVertexLayout(const aiMesh* pAiMesh);
        Buffer::SharedPtr createIndexBuffer(const aiMesh* pAiMesh);
        Buffer::SharedPtr createVertexBuffer(const aiMesh* pAiMesh, const VertexBufferLayout* pLayout, const uint8_t* pBoneIds, const vec4* pBoneWeights);
        void loadTextures(const aiMaterial* pAiMaterial, const std::string& folder, BasicMaterial* pMaterial, bool isObjFile, bool useSrgb);
        Material::SharedPtr createMaterial(const aiMaterial* pAiMaterial, const std::string& folder, bool isObjFile, bool useSrgb);

        std::map<std::string, uint32_t> mBoneNameToIdMap;
        std::map<uint32_t, Material::SharedPtr> mAiMaterialToFalcor;

        Model::SharedPtr mpModel;

        std::vector<Bone> mBones;
        uint32_t mFlags;
        std::map<const std::string, Texture::SharedPtr> mTextureCache;
    };
}
