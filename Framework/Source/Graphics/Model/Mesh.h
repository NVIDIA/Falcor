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
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "API/VAO.h"
#include "API/RenderContext.h"
#include "utils/AABB.h"
#include "Graphics/Material/Material.h"
#include "Graphics/Paths/MovableObject.h"

namespace Falcor
{
    class Model;
    class Buffer;
    class Vao;
    class VertexBufferLayout;
    class Camera;

    class AssimpModelImporter;
    class BinaryModelImporter;
    class SimpleModelImporter;

    /** Class representing a single mesh
    */
    class Mesh : public IMovableObject
    {
    public:
        using SharedPtr = std::shared_ptr<Mesh>;
        using SharedConstPtr = std::shared_ptr<const Mesh>;

        /** create a new mesh
            \param[in] VertexBuffers Vector of vertex buffer descriptors
            \param[in] VertexCount Number of vertices in the vertex buffer
            \param[in] pIndexBuffer Pointer to the index buffer
            \param[in] IndexCount Number of indices in the index buffer
            \param[in] Topology The primitive topology of the mesh
            \param[in] pMaterial The material of the mesh
            \param[in] BoundingBox The mesh's axis-aligned bounding-box
            \param[in] bHasBones Indicates the the mesh uses bones for animation
        */
        static SharedPtr create(const Vao::BufferVec& vertexBuffers,
            uint32_t vertexCount,
            const Buffer::SharedPtr& pIndexBuffer,
            uint32_t indexCount,
            const VertexLayout::SharedPtr& pLayout,
            RenderContext::Topology topology,
            const Material::SharedPtr& pMaterial,
            const BoundingBox& boundingBox,
            bool hasBones);

        /** Destructor
        */
        ~Mesh();

        /** Permanently transform the mesh by the given transform
            \param[in] Transform The transform to apply
        */
        void Mesh::applyTransform(const glm::mat4& transform);

        /** Get the mesh's axis-aligned bounding-box in object space
        */
        const BoundingBox& getObjectSpaceBoundingBox() const { return mBoundingBox; }
        /** Get the number of vertices in the vertex buffer. If you want to draw, use GetIndexCount() instead.
        */
        uint32_t getVertexCount() const { return mVertexCount; }
        /** Get the number of primitives.
        */
        uint32_t getPrimitiveCount() const { return mPrimitiveCount; }
        /** Get the number of indices in the index buffer. Use this value when drawing the mesh.
        */
        uint32_t getIndexCount() const { return mIndexCount; }

        /** Get a pointer to the mesh's material
        */
        const Material::SharedPtr& getMaterial() const { return mpMaterial; }
        
        /** Get a the mesh's topology
        */
        const RenderContext::Topology getTopology() const { return mTopology; }

        /** Does the mesh have bones?
        */
        bool hasBones() const { return mHasBones; }

        /** Set the mesh's material. Can be used to override the material loaded with the model.
        */
        void setMaterial(const Material::SharedPtr& pMaterial) { mpMaterial = pMaterial; }

        /** Get the vertex array object matching the mesh
        */
        const Vao::SharedPtr getVao() const { return mpVao; }

        /** Get the number of instance
        */
        uint32_t getInstanceCount() const { return (uint32_t)mInstanceMatrices.size(); }

        /** Get an instance matrix
        */
        const glm::mat4& getInstanceMatrix(uint32_t instanceID) const { return mInstanceMatrices[instanceID]; }
        
        /** Set an instance matrix
        */
        void setInstanceMatrix(uint32_t instanceID, const glm::mat4& mx) { mInstanceMatrices[instanceID] = mx; }

        /** Get an instance bounding-box in world space
        */
        const BoundingBox& getInstanceBoundingBox(uint32_t instanceID) const { return mInstanceBoundingBox[instanceID]; }

        /** Get a pointer to the instance matrices array. Can be used to set a batch of instances at ones.
        */
        const glm::mat4* getInstanceMatrices() const {  return mInstanceMatrices.data(); }

        /** Delete culled instances from the mesh based on the camera frustum cull test.
        */
        void deleteCulledInstances(const Camera* pCamera);

		const uint32_t getId() const { return mId; }

        /** Reset all global id counter of model, mesh and material
        */
        static void resetGlobalIdCounter();

        /** Move the mesh. Moves all of the mesh instances
        */
        void move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) override;
    protected:
        friend AssimpModelImporter;
        friend BinaryModelImporter;
        friend SimpleModelImporter;
        void addInstance(const glm::mat4& transform);
        static const uint32_t kMaxBonesPerVertex = 4;              ///> Max supported bones per vertex

    private:
        Mesh(const Vao::BufferVec& vertexBuffers,
            uint32_t vertexCount,
            const Buffer::SharedPtr& pIndexBuffer,
            uint32_t indexCount,
            const VertexLayout::SharedPtr& pLayout,
            RenderContext::Topology topology,
            const Material::SharedPtr& pMaterial,
            const BoundingBox& boundingBox,
            bool hasBones);

		static uint32_t sMeshCounter;

		uint32_t mId;
        uint32_t mIndexCount = 0;
        uint32_t mVertexCount = 0;
        uint32_t mPrimitiveCount = 0;
        bool mHasBones = false;
        Material::SharedPtr mpMaterial;
        RenderContext::Topology mTopology;
        BoundingBox mBoundingBox;

        Vao::SharedPtr mpVao;
        std::vector<glm::mat4> mInstanceMatrices;
        std::vector<glm::mat4> mOriginalInstanceMatrices;
        bool mDirty = true;
        std::vector<BoundingBox> mInstanceBoundingBox;
    };
}