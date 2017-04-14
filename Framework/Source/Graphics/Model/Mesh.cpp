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
#include "Mesh.h"
#include "Model.h"
#include "Externals/Assimp/Include/mesh.h"
#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "API/Buffer.h"
#include "AnimationController.h"
#include "API/VertexLayout.h"
#include "Graphics/Camera/Camera.h"
#include "Data/VertexAttrib.h"

namespace Falcor
{ 
    uint32_t Mesh::sMeshCounter = 0;
    Mesh::~Mesh() = default;

    Mesh::SharedPtr Mesh::create(const Vao::BufferVec& vertexBuffers,
        uint32_t vertexCount,
        const Buffer::SharedPtr& pIndexBuffer,
        uint32_t indexCount,
        const VertexLayout::SharedPtr& pLayout,
        Vao::Topology topology,
        const Material::SharedPtr& pMaterial,
        const BoundingBox& boundingBox,
        bool hasBones)
    {
        return SharedPtr(new Mesh(vertexBuffers, vertexCount, pIndexBuffer, indexCount, pLayout, topology, pMaterial, boundingBox, hasBones));
    }

    Mesh::Mesh(const Vao::BufferVec& vertexBuffers,
        uint32_t vertexCount,
        const Buffer::SharedPtr& pIndexBuffer,
        uint32_t indexCount,
        const VertexLayout::SharedPtr& pLayout,
        Vao::Topology topology,
        const Material::SharedPtr& pMaterial,
        const BoundingBox& boundingBox,
        bool hasBones) 
        : mId(sMeshCounter++)
        , mIndexCount(indexCount)
        , mVertexCount(vertexCount)
        , mpMaterial(pMaterial)
        , mBoundingBox(boundingBox)
        , mHasBones(hasBones)
    {
        uint32_t VertsPerPrim;
        switch(topology)
        {
        case Vao::Topology::PointList:
            VertsPerPrim = 1;
            break;
        case Vao::Topology::LineList:
            VertsPerPrim = 2;
            break;
        case Vao::Topology::TriangleList:
            VertsPerPrim = 3;
            break;
        default:
            should_not_get_here();
        }

        mPrimitiveCount = mIndexCount / VertsPerPrim;

        mpVao = Vao::create(vertexBuffers, pLayout, pIndexBuffer, ResourceFormat::R32Uint, topology);

		// create spire vertex component instance from VertexLayout
		mpVertexModule = ShaderRepository::Instance().GetVertexModule(pLayout.get());
        auto componentClass = ShaderRepository::Instance().findComponentClass(mpVertexModule);
		mpVertexComponentInstance = ComponentInstance::create(componentClass);

        // Find the correct component class to use for transformation:
        ProgramReflection::ComponentClassReflection::SharedPtr pTransformClass;
        if( mHasBones )
        {
            pTransformClass = ShaderRepository::Instance().findComponentClass("SkeletalMeshGeometry");
        }
        else
        {
            pTransformClass = ShaderRepository::Instance().findComponentClass("StaticMeshGeometry");
        }

        mWorldMatOffset = pTransformClass->getVariableData("gWorldMat", true)->location;
        mMeshIdOffset = pTransformClass->getVariableData("gMeshId")->location;
        mpTransformComponentInstance = ComponentInstance::create(pTransformClass);
    }

    void Mesh::resetGlobalIdCounter()
    {
        sMeshCounter = 0;
        Material::resetGlobalIdCounter();
    }

    const ComponentInstance::SharedPtr& Mesh::getTransformComponent(
        glm::mat4 const&    inWorldMat,
        uint32_t            drawInstanceID) const
    {
        glm::mat4 worldMat = inWorldMat;
        if( hasBones() )
        {
            worldMat = glm::mat4();
        }

        mpTransformComponentInstance->setVariable(
            mWorldMatOffset + drawInstanceID * sizeof(glm::mat4),
            worldMat);

        // TODO: why do this in the inner loop?
        mpTransformComponentInstance->setVariable(
            mMeshIdOffset,
            getId());

        return mpTransformComponentInstance;
    }

    void Mesh::enableTessellation()
    {
        // We are basically re-doing construction here... :(

        Vao::Topology patchTopology;
        switch(mpVao->getPrimitiveTopology())
        {
        case Vao::Topology::PointList:
            patchTopology = Vao::Topology::Patch1;
            break;
        case Vao::Topology::LineList:
            patchTopology = Vao::Topology::Patch3;
            break;
        case Vao::Topology::TriangleList:
            patchTopology = Vao::Topology::Patch3;
            break;
        default:
            should_not_get_here();
        }

        mpVao->setPrimitiveTopology(patchTopology);

        ProgramReflection::ComponentClassReflection::SharedPtr pTransformClass =
            ShaderRepository::Instance().findComponentClass("TessellatedMeshGeometry");

        mWorldMatOffset = pTransformClass->getVariableData("gWorldMat", true)->location;
        mMeshIdOffset = pTransformClass->getVariableData("gMeshId")->location;
        mpTransformComponentInstance = ComponentInstance::create(pTransformClass);
    }
}
