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
    class DescriptorHeapRange;

    class DescriptorHeap : public std::enable_shared_from_this<DescriptorHeap>
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorHeap>;
        using SharedConstPtr = std::shared_ptr<const DescriptorHeap>;
        using ApiHandle = DescriptorHeapHandle;
        using Entry = std::shared_ptr<DescriptorHeapRange>;
        using CpuHandle = HeapCpuHandle;
        using GpuHandle = HeapGpuHandle;

        ~DescriptorHeap();

        enum class Type
        {
            SRV,
            Sampler,
            RTV,
            DSV,
            UAV
        };
        static SharedPtr create(Type type, uint32_t descriptorsCount, bool shaderVisible = true);

        Entry allocateEntries(uint32_t count);
        ApiHandle getApiHandle() const { return mApiHandle; }
        Type getType() const { return mType; }

        uint32_t getReservedDescCount() const { return mCount; }
        GpuHandle getGpuBaseHandle() const { return mGpuHeapStart; }
        CpuHandle getCpuBaseHandle() const { return mCpuHeapStart; }
        uint32_t getDescriptorSize() const { return mDescriptorSize; }
    private:
        friend DescriptorHeapRange;
        DescriptorHeap(Type type, uint32_t descriptorsCount);

        CpuHandle getCpuHandle(uint32_t index) const;
        GpuHandle getGpuHandle(uint32_t index) const;
        void releaseEntry(uint32_t handle, uint32_t count)
        {
            mFreeEntries.push({ handle, count });
        }

        CpuHandle mCpuHeapStart = {};
        GpuHandle mGpuHeapStart = {};
        uint32_t mDescriptorSize;
        const uint32_t mCount;
        uint32_t mCurDesc = 0;
        ApiHandle mApiHandle;
        Type mType;

        struct EntryRange
        {
            uint32_t base;
            uint32_t count;
        };
        std::queue<EntryRange> mFreeEntries;
    };

    // Ideally this would be nested inside the Descriptor heap. Unfortunately, we need to forward declare it in FalcorD3D12.h, which is impossible with nesting
    class DescriptorHeapRange
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorHeapRange>;
        using WeakPtr = std::weak_ptr<DescriptorHeapRange>;
        using CpuHandle = DescriptorHeap::CpuHandle;
        using GpuHandle = DescriptorHeap::GpuHandle;

        static SharedPtr create(DescriptorHeap::SharedPtr pHeap, uint32_t baseEntry, uint32_t count)
        {
            SharedPtr pEntry = SharedPtr(new DescriptorHeapRange(pHeap, baseEntry, count));
            return pEntry;
        }

        ~DescriptorHeapRange()
        {
            mpHeap->releaseEntry(mBaseEntry, mCount);
        }

        // OPTME we could store the handles in the class to avoid the additional indirection at the expense of memory
        CpuHandle getCpuHandle(uint32_t entryIndex) const { assert(entryIndex < mCount); return mpHeap->getCpuHandle(mBaseEntry + entryIndex); }
        GpuHandle getGpuHandle(uint32_t entryIndex) const { assert(entryIndex < mCount); return mpHeap->getGpuHandle(mBaseEntry + entryIndex); }
        uint32_t getBaseEntryIndex() const { return mBaseEntry; }
        uint32_t getEntriesCount() const { return mCount; }
        DescriptorHeap::SharedPtr getHeap() const { return mpHeap; }
    private:
        DescriptorHeapRange(DescriptorHeap::SharedPtr pHeap, uint32_t baseEntry, uint32_t count) : mpHeap(pHeap), mBaseEntry(baseEntry), mCount(count) {}
        const uint32_t mBaseEntry = -1;
        const uint32_t mCount = 0;
        DescriptorHeap::SharedPtr mpHeap;
    };
}
