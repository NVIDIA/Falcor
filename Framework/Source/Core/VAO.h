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
#include <vector>
#include "VertexLayout.h"
#include "Buffer.h"

namespace Falcor
{
    /** Abstracts vertex array objects
    */
    class Vao : public std::enable_shared_from_this<Vao>
    {
    public:
        using SharedPtr = std::shared_ptr<Vao>;
        using SharedConstPtr = std::shared_ptr<const Vao>;

        /** Vertex buffer descriptor
        */
        struct VertexBufferDesc
        {
          Buffer::SharedPtr pBuffer;           ///< The buffer object
          uint32_t stride        = 0;           ///< Vertex stride
          VertexBufferLayout::SharedPtr pLayout;     ///< Vertex layout object describing the vertex elements in the buffer

          VertexBufferDesc() { pLayout = VertexBufferLayout::create(); }
        };

        struct ElementDesc
        {
            static const uint32_t kInvalidIndex = -1;
            uint32_t vbIndex = kInvalidIndex;
            uint32_t elementIndex = kInvalidIndex;
        };

        using VertexBufferDescVector = std::vector<VertexBufferDesc>;
        /** create a new object
            \param vbDesc Array of pointers to vertex buffer descriptor. Must have at least 1 element
            \param pIB Pointer to the index-buffer. Can be nullptr, in which case no index-buffer will be bound.
        */
        static SharedPtr create(const VertexBufferDescVector& vbDesc, const Buffer::SharedPtr& pIB);
        ~Vao();

        /** Get the API handle
        */
        VaoHandle getApiHandle() const;

        /** Get the vertex buffer count
        */
        uint32_t getVertexBuffersCount() const { return (uint32_t)mpVBs.size(); }

        /** Get a vertex buffer
        */
        Buffer::SharedConstPtr getVertexBuffer(uint32_t index) const { return mpVBs[index].pBuffer; }

        /** Get a vertex buffer layout
        */
        VertexBufferLayout::SharedConstPtr getVertexBufferLayout(uint32_t index) const { return mpVBs[index].pLayout; }

        /** Return the vertex buffer index and the element index by its location.
            If the element is not found, returns the default ElementDesc
        */
        ElementDesc getElementIndexByLocation(uint32_t elementLocation) const;

        /** Get a vertex buffer layout
        */
        uint32_t getVertexBufferStride(uint32_t index) const { return mpVBs[index].stride; }

        /** Get the index buffer
        */
        Buffer::SharedConstPtr getIndexBuffer() const { return mpIB; }

    protected:
        friend class RenderContext;
#ifdef FALCOR_D3D11
        ID3D11InputLayoutPtr getInputLayout(ID3DBlob* pVsBlob) const;
#endif
    private:
        Vao(const VertexBufferDescVector& vbDesc, const Buffer::SharedPtr& pIB);
        bool initialize();
        VaoHandle mApiHandle;
        VertexBufferDescVector mpVBs;
        Buffer::SharedConstPtr mpIB = nullptr;
        void* mpPrivateData = nullptr;
    };
}