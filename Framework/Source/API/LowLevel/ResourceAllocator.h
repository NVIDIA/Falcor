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
#ifdef FALCOR_LOW_LEVEL_API
#include <unordered_map>
#include <queue>

namespace Falcor
{
    class ResourceAllocator
    {
    public:
        using SharedPtr = std::shared_ptr<ResourceAllocator>;
        using SharedConstPtr = std::shared_ptr<const ResourceAllocator>;

        static SharedPtr create(size_t pageSize);
        struct AllocationData
        {
            BufferHandle pResourceHandle;
            GpuAddress gpuAddress;
            uint8_t* pData;
            uint64_t allocationID;
        };

        AllocationData allocate(size_t size, size_t alignment = 1);
        void release(AllocationData& data);
        size_t getPageSize() const { return mPageSize; }

    private:
        ResourceAllocator(size_t pageSize) : mPageSize(pageSize) {}
        struct PageData
        {
            uint32_t allocationsCount = 0;
            size_t currentOffset = 0;
            BufferHandle pResourceHandle = nullptr;
            GpuAddress gpuAddress = 0;
            uint8_t* pData = nullptr;

            using UniquePtr = std::unique_ptr<PageData>;
        };
        
        size_t mPageSize = 0;
        size_t mCurrentAllocationId = 0;
        PageData::UniquePtr mpActivePage;
        std::unordered_map<size_t, PageData::UniquePtr> mUsedPages;
        std::queue<PageData::UniquePtr> mAvailablePages;

        void allocateNewPage();
    };
}
#endif // FALCOR_LOW_LEVEL_API