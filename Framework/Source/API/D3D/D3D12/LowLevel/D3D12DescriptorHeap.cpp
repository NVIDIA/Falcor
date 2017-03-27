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
#include "API/LowLevel/DescriptorHeap.h"
#include "API/Device.h"

namespace Falcor
{
    D3D12_DESCRIPTOR_HEAP_TYPE getHeapType(DescriptorHeap::Type type)
    {
        switch(type)
        {
        case DescriptorHeap::Type::SRV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        case DescriptorHeap::Type::Sampler:
            return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        case DescriptorHeap::Type::DSV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        case DescriptorHeap::Type::RTV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        case DescriptorHeap::Type::UAV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        default:
            should_not_get_here();
            return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
        }
    }

    DescriptorHeap::DescriptorHeap(
        DescriptorAllocator*    pDescriptorAllocator, 
        Type                    type,
        uint32_t                descriptorsCount)
        : mpDescriptorAllocator(pDescriptorAllocator)
        , mCount(descriptorsCount)
        , mType (type)
    {
		ID3D12DevicePtr pDevice = gpDevice->getApiHandle();
        mDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(getHeapType(type));
    }

    DescriptorHeap::~DescriptorHeap() = default;

    DescriptorHeap::SharedPtr DescriptorHeap::create(
        DescriptorAllocator*    pDescriptorAllocator, 
        Type                    type,
        uint32_t                descriptorsCount,
        bool                    shaderVisible)
    {
		ID3D12DevicePtr pDevice = gpDevice->getApiHandle();

        DescriptorHeap::SharedPtr pHeap = SharedPtr(new DescriptorHeap(
            pDescriptorAllocator,
            type,
            descriptorsCount));
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};

        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.Type = getHeapType(type);
        desc.NumDescriptors = descriptorsCount;
        if(FAILED(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap->mApiHandle))))
        {
            logError("Can't create descriptor heap");
            return nullptr;
        }

        pHeap->mCpuHeapStart = pHeap->mApiHandle->GetCPUDescriptorHandleForHeapStart();
        pHeap->mGpuHeapStart = pHeap->mApiHandle->GetGPUDescriptorHandleForHeapStart();
        return pHeap;
    }

    template<typename HandleType>
    HandleType getHandleCommon(HandleType base, uint32_t index, uint32_t descSize)
    {
        base.ptr += descSize * index;
        return base;
    }

    DescriptorHeap::CpuHandle DescriptorHeap::getCpuHandle(uint32_t index) const
    {
        assert(index < mCurDesc);
        return getHandleCommon(mCpuHeapStart, index, mDescriptorSize);
    }

    DescriptorHeap::GpuHandle DescriptorHeap::getGpuHandle(uint32_t index) const
    {
        assert(index < mCurDesc);
        return getHandleCommon(mGpuHeapStart, index, mDescriptorSize);
    }

    DescriptorHeapEntry::SharedPtr DescriptorHeap::allocateTable(uint32_t entryCount)
    {
        assert(entryCount);
        return getPool(entryCount)->allocateTable();
    }

    std::shared_ptr<DescriptorPool> DescriptorHeap::getPool(uint32 entryCount)
    {
        assert(entryCount);

        auto iter = mPools.find(entryCount);
        if( iter != mPools.end() )
        {
            auto pool = iter->second.lock();
            if(pool)
                return pool;
        }

        std::shared_ptr<DescriptorPool> pool = std::shared_ptr<DescriptorPool>(
            new DescriptorPool(shared_from_this(), entryCount));

        mPools.insert_or_assign(entryCount, pool);

        return pool;
    }

    DescriptorPool::DescriptorPool(
        DescriptorHeap::SharedPtr   pDescriptorHeap,
        uint32_t                    tableEntryCount)
        : mpDescriptorHeap(pDescriptorHeap)
        , mTableEntryCount(tableEntryCount)
    {}

    DescriptorHeap::Entry DescriptorPool::allocateTable()
    {
        uint32_t entry;
        if (mFreeEntries.empty() == false)
        {
            entry = mFreeEntries.front();
            mFreeEntries.pop();
        }
        else
        {
            auto pDescriptorHeap = mpDescriptorHeap.get();
            uint32_t entryCount = mTableEntryCount;
            uint32_t current = pDescriptorHeap->mCurDesc;
            uint32_t capacity = pDescriptorHeap->mCount;
            assert(entryCount < capacity);
            if ((current + entryCount) > capacity)
            {
                logError("Can't find free CPU handle in descriptor heap");
                return nullptr;
            }
            entry = current;
            pDescriptorHeap->mCurDesc = current + entryCount;
        }

        return DescriptorHeapEntry::create(shared_from_this(), entry);
    }

    DescriptorHeapEntry::~DescriptorHeapEntry()
    {
        getHeap()->getAllocator()->deferredRelease(this);
    }

    void DescriptorPool::releaseTable(uint32_t entry)
    {
        mFreeEntries.push(entry);
    }

    void DescriptorAllocator::init(GpuFence::SharedPtr const& pFence)
    {
        mpFence = pFence;
    }

    void DescriptorAllocator::deferredRelease(DescriptorHeapEntry* entry)
    {
        Entry info;
        info.pPool = entry->mpPool;
        info.heapEntry = entry->mHeapEntry;
        info.fenceValue = mpFence->getCpuValue();
        mDeferredReleases.push(info);
    }

    void DescriptorAllocator::executeDeferredReleases()
    {
        uint64_t gpuVal = mpFence->getGpuValue();
        while (mDeferredReleases.size() && mDeferredReleases.top().fenceValue <= gpuVal)
        {
            const Entry& info = mDeferredReleases.top();

            info.pPool->releaseTable(info.heapEntry);

            mDeferredReleases.pop();
        }
    }

}
