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
#include "Buffer.h"
#include "ResourceViews.h"

namespace Falcor
{
    class TypedBufferBase : public Buffer
    {
    public:
        using SharedPtr = std::shared_ptr<TypedBufferBase>;
        using SharedConstPtr = std::shared_ptr<const TypedBufferBase>;

        void uploadToGPU() const;
        uint32_t getElementCount() const { return mElementCount; }
    protected:
        uint32_t mElementCount = 0;
        TypedBufferBase(uint32_t size, uint32_t elementCount);
        std::vector<uint8_t> mData;
        mutable ShaderResourceView::SharedPtr mpSrv;
        mutable bool mDirty = false;
    };

    template<typename BufferType>
    class TypedBuffer : public TypedBufferBase
    {
    public:
        class SharedPtr : public std::shared_ptr<TypedBuffer>
        {
        public:
            SharedPtr() : std::shared_ptr<TypedBuffer>() {}
            SharedPtr(TypedBuffer* pBuffer) : std::shared_ptr<TypedBuffer>(pBuffer) {}

            class TypedElement
            {
            public:
                TypedElement(TypedBuffer* pBuf, uint32_t elemIdx) : mpBuffer(pBuf), mElemIdx(elemIdx)  {}
                void operator=(const BufferType& val) { mpBuffer->setElement(mElemIdx, val); }
            private:
                uint32_t mElemIdx;
                TypedBuffer* mpBuffer;
            };

            TypedElement operator[](uint32_t index) { return TypedElement(get(), index); }
        };

        using SharedConstPtr = std::shared_ptr<const TypedBuffer>;
        static SharedPtr create(uint32_t elementCount)
        {
            return SharedPtr(new TypedBuffer(elementCount));
        }

        void setElement(uint32_t index, const BufferType& value)
        {
            assert(index < mElementCount);
            BufferType* pVar = (BufferType*)(mData.data() + (index * sizeof(BufferType)));
            *pVar = value;
            mDirty = true;
        }

    private:
        friend SharedPtr;
        TypedBuffer(uint32_t elementCount) : TypedBufferBase(sizeof(BufferType) * elementCount, elementCount) {}
    };
}