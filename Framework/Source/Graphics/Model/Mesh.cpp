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
		bool hasBones) : mId(sMeshCounter++)
    {
        mVertexCount = vertexCount;
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

        mIndexCount = indexCount;
        mPrimitiveCount = mIndexCount / VertsPerPrim;
        mpMaterial = pMaterial;
        mBoundingBox = boundingBox;
        mHasBones = hasBones;

        mpVao = Vao::create(vertexBuffers, pLayout, pIndexBuffer, ResourceFormat::R32Uint, topology);
    }

    void Mesh::applyTransform(const glm::mat4& Transform) 
    {
        // Transform geometry, keeping track of min/max
        glm::vec3 posMin(std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max());
        glm::vec3 posMax(std::numeric_limits<float>::min(),std::numeric_limits<float>::min(),std::numeric_limits<float>::min());
        for(uint32_t i=0u; i<mpVao->getVertexBuffersCount(); ++i)
        {
            const auto& pLayout = mpVao->getVertexLayout()->getBufferLayout(i);
            const uint32_t stride = pLayout->getStride();
            Buffer* pBuffer = const_cast<Buffer*>(mpVao->getVertexBuffer(i).get());
            size_t numVerts = pBuffer->getSize() / stride;

            float* tempData = new float[(pBuffer->getSize()+sizeof(float)-1)/sizeof(float)];
            pBuffer->readData(tempData, 0,pBuffer->getSize());

            for(uint32_t j = 0u; j<pLayout->getElementCount(); ++j)
            {
                glm::mat4 matrix;
                bool doMinMax;
                if(pLayout->getElementShaderLocation(j) == VERTEX_POSITION_LOC)
                {
                    matrix = Transform;
                    doMinMax = true;
                }
                else if(pLayout->getElementShaderLocation(j) == VERTEX_NORMAL_LOC)
                {
                    matrix = glm::transpose(glm::inverse(Transform));
                    doMinMax = false;
                }
                else continue;

                const uint32_t channels = getFormatChannelCount(pLayout->getElementFormat(j));

                for(size_t k = 0 ; k < numVerts ; ++k)
                {   // TODO: optimize
                    float* pElement = tempData + k*stride / sizeof(float) + pLayout->getElementOffset(j) / sizeof(float);
                    glm::vec4 vert;
                    switch (channels) 
                    {
                        case 1: 
                            vert=glm::vec4(pElement[0],  0.0f,  0.0f,  1.0f); 
                            break;
                        case 2: 
                            vert=glm::vec4(pElement[0],pElement[1],  0.0f,  1.0f); 
                            break;
                        case 3: 
                            vert=glm::vec4(pElement[0],pElement[1],pElement[2],  1.0f); 
                            break;
                        case 4: 
                            vert=glm::vec4(pElement[0],pElement[1],pElement[2],pElement[3]); 
                            break;
                        default: 
                            should_not_get_here();
                    }

                    vert = matrix * vert;
                    switch (channels) 
                    {
                        case 4: 
                            pElement[3]=vert.w; // fallthrough
                        case 3: 
                            pElement[2]=vert.z; // fallthrough
                        case 2: 
                            pElement[1]=vert.y; // fallthrough
                        case 1: 
                            pElement[0]=vert.x; 
                            break;
                        default: 
                            should_not_get_here();
                    }

                    if (doMinMax) 
                    {
                        //glm::vec3 xyz(Vert.x,Vert.y,Vert.z);
                        posMin = glm::vec3(std::min(posMin.x,vert.x),std::min(posMin.y,vert.y),std::min(posMin.z,vert.z)); //glm::min(PosMin,xyz);
                        posMax = glm::vec3(std::max(posMax.x,vert.x),std::max(posMax.y,vert.y),std::max(posMax.z,vert.z)); //glm::max(PosMax,xyz);
                    }
                }
            }

            pBuffer->updateData(tempData, 0, pBuffer->getSize());
            delete [] tempData;
        }

        // Update bounding box
        mBoundingBox = BoundingBox::fromMinMax(posMin,posMax);

        // Update instances
        mInstanceBoundingBox.clear();
        for(auto const& matrix : mInstanceMatrices) 
        {
            BoundingBox box = mBoundingBox.transform(matrix);
            mInstanceBoundingBox.push_back(box);
        }
    }

    void Mesh::addInstance(const glm::mat4& transform)
    {
        assert(mInstanceMatrices.size() == mInstanceBoundingBox.size());
        mOriginalInstanceMatrices.push_back(transform);
        mInstanceMatrices.push_back(transform);
        BoundingBox Bbox = mBoundingBox.transform(transform);
        mInstanceBoundingBox.push_back(Bbox);
    }

    void Mesh::deleteCulledInstances(const Camera* pCamera)
    {
        const glm::mat4 zeroMat(0);
        const BoundingBox zeroBbox = {glm::vec3(0, 0, 0), glm::vec3(0,0,0)};

        // Mark culled instances
        for(uint32_t i = 0; i < mOriginalInstanceMatrices.size(); i++)
        {
            if(pCamera->isObjectCulled(mInstanceBoundingBox[i]))
            {
                mInstanceBoundingBox[i] = zeroBbox;
                mOriginalInstanceMatrices[i] = zeroMat;
            }
        }

        // Remove culled instances
        auto matPred = [&zeroMat](glm::mat4& m) {return m == zeroMat; };
        auto& matEnd = std::remove_if(mOriginalInstanceMatrices.begin(), mOriginalInstanceMatrices.end(), matPred);
        mOriginalInstanceMatrices.erase(matEnd, mOriginalInstanceMatrices.end());
        mInstanceMatrices = mOriginalInstanceMatrices;

        auto boxPred = [&zeroBbox](BoundingBox& b) {return b == zeroBbox; };
        auto& boxEnd = std::remove_if(mInstanceBoundingBox.begin(), mInstanceBoundingBox.end(), boxPred);
        mInstanceBoundingBox.erase(boxEnd, mInstanceBoundingBox.end());

        assert(mInstanceBoundingBox.size() == mInstanceMatrices.size());
    }

    void Mesh::resetGlobalIdCounter()
    {
        sMeshCounter = 0;
        Material::resetGlobalIdCounter();
    }

    void Mesh::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        // Only changing position for now
        for(uint32_t i = 0; i < mOriginalInstanceMatrices.size(); i++)
        {
            mInstanceMatrices[i][3] = mOriginalInstanceMatrices[i][3] + v4(position, 0.f);
        }
    }
}
