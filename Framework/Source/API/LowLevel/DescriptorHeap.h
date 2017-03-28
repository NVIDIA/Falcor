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

// SPIRE:
#include "GpuFence.h"

namespace Falcor
{
    class DescriptorAllocator;
    class DescriptorHeapEntry;
    class DescriptorPool;

    // A `DescriptorHeap` manages a single contiguous range of descriptor memory,
    // of a single type. It allows contiguous blocks (tables) to be allocated
    // from that memory, but blocks are never freed back to the heap.
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
            SRV,
            Sampler,
            RTV,
            DSV,
            UAV
        };
        static SharedPtr create(
            DescriptorAllocator*    pAllocator,
            Type                    type,
            uint32_t                descriptorsCount,
            bool                    shaderVisible = true);

        std::shared_ptr<DescriptorPool> getPool(uint32 entryCount);

        DescriptorHeap::Entry allocateTable(uint32_t entryCount);
        DescriptorHeap::Entry allocateEntry()
        {
            return allocateTable(1);
        }

        ApiHandle getApiHandle() const { return mApiHandle; }
        Type getType() const { return mType; }

        DescriptorAllocator* getAllocator() const { return mpDescriptorAllocator; }

    private:
        friend DescriptorHeapEntry;
        friend class DescriptorPool;
        DescriptorHeap(
            DescriptorAllocator*    pAllocator,
            Type type, uint32_t descriptorsCount);

        CpuHandle getCpuHandle(uint32_t index) const;
        GpuHandle getGpuHandle(uint32_t index) const;

        DescriptorAllocator* mpDescriptorAllocator;
        CpuHandle mCpuHeapStart = {};
        GpuHandle mGpuHeapStart = {};
        uint32_t mDescriptorSize;
        uint32_t mCount;
        uint32_t mCurDesc = 0;
        ApiHandle mApiHandle;
        Type mType;

        std::map<uint32_t, std::weak_ptr<DescriptorPool>> mPools;
    };

    // A `DescriptorPool` manages allocation of fixed-size descriptor tables
    // (the number of tables is baked into the pool object), and allows allocated
    // tables to be released and put back into a free list on the pool.
    class DescriptorPool : public std::enable_shared_from_this<DescriptorPool>
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorPool>;

        DescriptorHeap::SharedPtr getDescriptorHeap() { return mpDescriptorHeap; }
        uint32_t getTableEntryCount() { return mTableEntryCount; }

        DescriptorHeap::Entry allocateTable();
        void releaseTable(uint32_t entry);

    private:
        friend class DescriptorHeap;
        DescriptorPool(
            DescriptorHeap::SharedPtr   pDescriptorHeap,
            uint32_t                    tableEntryCount);

        DescriptorHeap::SharedPtr   mpDescriptorHeap;
        uint32_t                    mTableEntryCount;

        std::queue<uint32_t>        mFreeEntries;
    };

    // Ideally this would be nested inside the Descriptor heap. Unfortunately, we need to forward declare it in FalcorD3D12.h, which is impossible with nesting
    class DescriptorHeapEntry
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorHeapEntry>;
        using WeakPtr = std::weak_ptr<DescriptorHeapEntry>;
        using CpuHandle = DescriptorHeap::CpuHandle;
        using GpuHandle = DescriptorHeap::GpuHandle;

        static SharedPtr create(DescriptorPool::SharedPtr pHeap, uint32_t heapEntry)
        {
            SharedPtr pEntry = SharedPtr(new DescriptorHeapEntry(pHeap));
            pEntry->mHeapEntry = heapEntry;
            return pEntry;
        }

        ~DescriptorHeapEntry();
#if 0
        {
            mpPool->releaseTable(mHeapEntry);
        }
#endif

        // OPTME we could store the handles in the class to avoid the additional indirection at the expense of memory
        CpuHandle getCpuHandle(uint32_t offset = 0) const { return mpPool->getDescriptorHeap()->getCpuHandle(mHeapEntry + offset); }
        GpuHandle getGpuHandle(uint32_t offset = 0) const { return mpPool->getDescriptorHeap()->getGpuHandle(mHeapEntry + offset); }
        DescriptorHeap::SharedPtr getHeap() const { return mpPool->getDescriptorHeap(); }



    private:
        friend class DescriptorAllocator;
        DescriptorHeapEntry(DescriptorPool::SharedPtr pPool) : mpPool(pPool) {}
        DescriptorPool::SharedPtr mpPool;
        uint32_t mHeapEntry = -1;
    };

    class DescriptorAllocator
    {
    public:
        void init(GpuFence::SharedPtr const& pFence);

        void deferredRelease(DescriptorHeapEntry* entry);
        void executeDeferredReleases();


    private:
        GpuFence::SharedPtr mpFence;

        struct Entry
        {
            DescriptorPool::SharedPtr pPool;
            uint32_t heapEntry;
            uint64_t fenceValue = 0;

            bool operator<(const Entry& other)  const { return fenceValue > other.fenceValue; }
        };
        std::priority_queue<Entry> mDeferredReleases;

    };
}
