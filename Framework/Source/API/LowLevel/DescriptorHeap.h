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
#include "Framework.h"
#include <queue>

namespace Falcor
{
    class DescriptorHeapEntry;

    class DescriptorHeap : public std::enable_shared_from_this<DescriptorHeap>
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorHeap>;
        using SharedConstPtr = std::shared_ptr<const DescriptorHeap>;
        using ApiHandle = DescriptorHeapHandle;
        using Entry = std::shared_ptr<DescriptorHeapEntry>;
        using CpuHandle = HeapCpuHandle;
        using GpuHandle = HeapGpuHandle;

        ~DescriptorHeap();

        enum class Type
        {
            ShaderResource,
            Sampler,
            RenderTargetView,
            DepthStencilView,
            UnorderedAccessView
        };
        static SharedPtr create(Type type, uint32_t descriptorsCount, bool shaderVisible = true);

        Entry allocateEntry();
        ApiHandle getApiHandle() const { return mApiHandle; }
        Type getType() const { return mType; }
    private:
        friend DescriptorHeapEntry;
        DescriptorHeap(Type type, uint32_t descriptorsCount);

        CpuHandle getCpuHandle(uint32_t index) const;
        GpuHandle getGpuHandle(uint32_t index) const;
        void releaseEntry(uint32_t handle)
        {
            mFreeEntries.push(handle);
        }

        CpuHandle mCpuHeapStart = {};
        GpuHandle mGpuHeapStart = {};
        uint32_t mDescriptorSize;
        uint32_t mCount;
        uint32_t mCurDesc = 0;
        ApiHandle mApiHandle;
        Type mType;

        std::queue<uint32_t> mFreeEntries;
    };

    // Ideally this would be nested inside the Descriptor heap. Unfortunately, we need to forward declare it in FalcorD3D12.h, which is impossible with nesting
    class DescriptorHeapEntry
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorHeapEntry>;
        using WeakPtr = std::weak_ptr<DescriptorHeapEntry>;
        using CpuHandle = DescriptorHeap::CpuHandle;
        using GpuHandle = DescriptorHeap::GpuHandle;

        static SharedPtr create(DescriptorHeap::SharedPtr pHeap, uint32_t heapEntry)
        {
            SharedPtr pEntry = SharedPtr(new DescriptorHeapEntry(pHeap));
            pEntry->mHeapEntry = heapEntry;
            return pEntry;
        }

        ~DescriptorHeapEntry()
        {
            mpHeap->releaseEntry(mHeapEntry);
        }

        // OPTME we could store the handles in the class to avoid the additional indirection at the expense of memory
        CpuHandle getCpuHandle() const { return mpHeap->getCpuHandle(mHeapEntry); }
        GpuHandle getGpuHandle() const { return mpHeap->getGpuHandle(mHeapEntry); }
        DescriptorHeap::SharedPtr getHeap() const { return mpHeap; }
    private:
        DescriptorHeapEntry(DescriptorHeap::SharedPtr pHeap) : mpHeap(pHeap) {}
        uint32_t mHeapEntry = -1;
        DescriptorHeap::SharedPtr mpHeap;
    };
}
