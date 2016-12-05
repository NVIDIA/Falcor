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
#include <typeindex>

namespace Falcor
{
    class TypedBufferBase : public Buffer
    {
    public:
        using SharedPtr = std::shared_ptr<TypedBufferBase>;
        using SharedConstPtr = std::shared_ptr<const TypedBufferBase>;

        void uploadToGPU() const;
        uint32_t getElementCount() const { return mElementCount; }
        void setGpuDirty() const { mGpuDirty = true; }
        ResourceFormat getResourceFormat() const { return mFormat; }
    protected:
        TypedBufferBase(uint32_t elementCount, ResourceFormat format);
        void readFromGpu();
        uint32_t mElementCount = 0;
        std::vector<uint8_t> mData;
        mutable ShaderResourceView::SharedPtr mpSrv;
        mutable bool mCpuDirty = false;
        mutable bool mGpuDirty = false;
        ResourceFormat mFormat;
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

                operator BufferType() const { return mpBuffer->getElement(mElemIdx); }
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
            mCpuDirty = true;
        }

        const BufferType& getElement(uint32_t index)
        {
            readFromGpu();
            const BufferType* pData = (BufferType*)mData.data();
            return pData[index];
        }

        static ResourceFormat type2format()
        {
#define t2f(_type, _format) if(typeid(BufferType) == typeid(_type)) return ResourceFormat::_format;
            t2f(float,      R32Float);
            t2f(vec2,       RG32Float);
            t2f(vec3,       RGB32Float);
            t2f(vec4,       RGBA32Float);

            t2f(uint32_t,   R32Uint);
            t2f(uvec2,      RG32Uint);
            t2f(uvec3,      RGB32Uint);
            t2f(uvec3,      RGBA32Uint);

            t2f(int32_t,    R32Int);
            t2f(ivec2,      RG32Int);
            t2f(ivec3,      RGB32Int);
            t2f(ivec3,      RGBA32Int);

            should_not_get_here();
            return ResourceFormat::Unknown;
#undef t2f
        }
    private:
        friend SharedPtr;
        TypedBuffer(uint32_t elementCount) : TypedBufferBase(elementCount, type2format()) {}
    };
}