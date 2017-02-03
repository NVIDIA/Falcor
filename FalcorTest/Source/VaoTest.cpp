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
#include "VaoTest.h"

void VaoTest::addTests()
{
    addTestToList<TestSimpleCreate>();
    addTestToList<TestIndexedCreate>();
    addTestToList<TestMultiBufferCreate>();
    addTestToList<TestStressLayout>();
}

testing_func(VaoTest, TestSimpleCreate)
{
    //Create vertex buffer
    const uint32_t bufferSize = 9u;
    float bufferData[bufferSize] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 2.25f };
    Buffer::SharedPtr pBuffer = Buffer::create(bufferSize, Resource::BindFlags::Vertex, Buffer::CpuAccess::Read, bufferData);
    Vao::BufferVec bufferVec;
    bufferVec.push_back(pBuffer);

    //create vertex layout
    std::string name = VERTEX_POSITION_NAME;
    ResourceFormat format = ResourceFormat::RGB32Float;
    uint32_t shaderLoc = VERTEX_POSITION_LOC;
    VertexBufferLayout::SharedPtr pBufferLayout = VertexBufferLayout::create();
    pBufferLayout->addElement(name, 0u, format, bufferSize, shaderLoc);
    VertexLayout::SharedPtr pLayout = VertexLayout::create();
    pLayout->addBufferLayout(0u, pBufferLayout);

    //Craete and check vao
    Vao::SharedPtr pVao = Vao::create(bufferVec, pLayout, nullptr, ResourceFormat::R32Uint, Vao::Topology::TriangleList);
    //Some of this should be helper function-ized as other tests need it
    if ((pVao->getVertexBuffer(0u)->getBindFlags() & Resource::BindFlags::Vertex) == Resource::BindFlags::None ||
        pVao->getVertexBuffersCount() != 1 ||
        pVao->getIndexBuffer() != nullptr ||
        pVao->getVertexLayout()->getBufferCount() != 1 ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementName(0u) != name ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementFormat(0u) != format ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementArraySize(0u) != bufferSize ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementShaderLocation(0u) != shaderLoc)
    {
        return test_fail("Vao's properties do not match the properties that were used to create it");
    }

    return TEST_PASS;
}

testing_func(VaoTest, TestIndexedCreate)
{
    //do something about this copypaste?

    //Create vertex buffer
    const uint32_t bufferSize = 9u;
    float bufferData[bufferSize] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 2.25f };
    Buffer::SharedPtr pBuffer = Buffer::create(bufferSize, Resource::BindFlags::Vertex, Buffer::CpuAccess::Read, bufferData);
    Vao::BufferVec bufferVec;
    bufferVec.push_back(pBuffer);

    //create veretx layout 
    std::string name = VERTEX_POSITION_NAME;
    ResourceFormat format = ResourceFormat::RGB32Float;
    uint32_t shaderLoc = VERTEX_POSITION_LOC;
    VertexBufferLayout::SharedPtr pBufferLayout = VertexBufferLayout::create();
    pBufferLayout->addElement(name, 0u, format, bufferSize, shaderLoc);
    VertexLayout::SharedPtr pLayout = VertexLayout::create();
    pLayout->addBufferLayout(0u, pBufferLayout);

    //create index buffer
    const uint32_t indexBufferSize = 12;
    uint32_t indexBufferData[indexBufferSize] = { 0, 1, 2, 2, 3, 4, 5, 6, 7, 6, 7, 8 };
    Buffer::SharedPtr pIndexBuffer = Buffer::create(indexBufferSize, Resource::BindFlags::Index, Buffer::CpuAccess::Read, indexBufferData);

    //Create and test vao
    Vao::SharedPtr pVao = Vao::create(bufferVec, pLayout, pIndexBuffer, ResourceFormat::R32Uint, Vao::Topology::TriangleList);
    if ((pVao->getVertexBuffer(0u)->getBindFlags() & Resource::BindFlags::Vertex) == Resource::BindFlags::None ||
        pVao->getVertexBuffersCount() != 1 ||
        pVao->getIndexBuffer()->getSize() != indexBufferSize ||
        (pVao->getIndexBuffer()->getBindFlags() & Resource::BindFlags::Index) == Resource::BindFlags::None ||
        pVao->getVertexLayout()->getBufferCount() != 1 ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementName(0u) != name ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementFormat(0u) != format ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementArraySize(0u) != bufferSize ||
        pVao->getVertexLayout()->getBufferLayout(0u)->getElementShaderLocation(0u) != shaderLoc)
    {
        return test_fail("Vao's properties do not match the properties that were used to create it");
    }

    return TEST_PASS;
}

testing_func(VaoTest, TestMultiBufferCreate)
{
    return TEST_PASS;
}

testing_func(VaoTest, TestStressLayout)
{
    return TEST_PASS;
}

int main()
{
    VaoTest voat;
    voat.init(true);
    voat.run();
    return 0;
}