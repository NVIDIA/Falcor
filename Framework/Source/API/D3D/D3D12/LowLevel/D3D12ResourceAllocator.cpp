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
#ifdef FALCOR_D3D12
#include "Framework.h"
#include "API/LowLevel/ResourceAllocator.h"

namespace Falcor
{
    ID3D12ResourcePtr createBuffer(size_t size, const D3D12_HEAP_PROPERTIES& heapProps);

    static D3D12_HEAP_PROPERTIES kUploadHeapProps =
    {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0,
    };

    ResourceAllocator::SharedPtr ResourceAllocator::create(size_t pageSize)
    {
        SharedPtr pAllocator = SharedPtr(new ResourceAllocator(pageSize));
        pAllocator->allocateNewPage();
        return pAllocator;
    }

    void ResourceAllocator::allocateNewPage()
    {
        if (mpActivePage)
        {
            mUsedPages[mCurrentAllocationId] = std::move(mpActivePage);
        }

//         if (mAvailablePages.size())
//         {
//             mpActivePage = std::move(mAvailablePages.front());
//             mAvailablePages.pop();
//             mpActivePage->allocationsCount = 0;
//             mpActivePage->currentOffset = 0;
//         }
//         else
        {
            mpActivePage = std::make_unique<PageData>();
            mpActivePage->pResourceHandle = createBuffer(mPageSize, kUploadHeapProps);
            mpActivePage->gpuAddress = mpActivePage->pResourceHandle->GetGPUVirtualAddress();
            D3D12_RANGE readRange = {};
            d3d_call(mpActivePage->pResourceHandle->Map(0, &readRange, (void**)&mpActivePage->pData));
        }

        mpActivePage->currentOffset = 0;
        mCurrentAllocationId++;
    }

    ResourceAllocator::AllocationData ResourceAllocator::allocate(size_t size)
    {
        allocateNewPage();
        AllocationData data;
        data.allocationID = mCurrentAllocationId;
        data.gpuAddress = mpActivePage->gpuAddress;// +mpActivePage->currentOffset;
        data.pData = mpActivePage->pData;// +mpActivePage->currentOffset;
        data.pResourceHandle = mpActivePage->pResourceHandle;
        return data;

        if (mpActivePage->currentOffset + size > mPageSize)
        {
            allocateNewPage();
        }

//        AllocationData data;
        data.allocationID = mCurrentAllocationId;
        data.gpuAddress = mpActivePage->gpuAddress + mpActivePage->currentOffset;
        data.pData = mpActivePage->pData + mpActivePage->currentOffset;
        data.pResourceHandle = mpActivePage->pResourceHandle;
        mpActivePage->currentOffset += size;

        return data;
    }

    void ResourceAllocator::release(const AllocationData& data)
    {
        return;
        if (data.allocationID == mCurrentAllocationId)
        {
            mpActivePage->allocationsCount--;
            if (mpActivePage->allocationsCount == 0)
            {
                mpActivePage->currentOffset = 0;
            }
        }
        else
        {
            auto& pData = mUsedPages[data.allocationID];
            pData->allocationsCount--;
            if (pData->allocationsCount == 0)
            {
                mAvailablePages.push(std::move(pData));
                mUsedPages.erase(data.allocationID);
            }
        }
    }
}
#endif