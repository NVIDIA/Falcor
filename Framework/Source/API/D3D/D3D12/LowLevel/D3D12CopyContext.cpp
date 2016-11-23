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
#include "API/LowLevel/CopyContext.h"
#include "API/Device.h"
#include "D3D12FencedPool.h"
#include "API/Buffer.h"
#include <queue>

namespace Falcor
{
    struct CopyContextData
    {
        CopyCommandAllocatorPool::SharedPtr pAllocatorPool;
        ID3D12GraphicsCommandListPtr pCmdList;
        ID3D12CommandQueuePtr pQueue;
        ID3D12CommandAllocatorPtr pAllocator;
        struct UploadDesc
        {
            uint64_t id;
            Buffer::SharedPtr pBuffer;
        };

        std::queue<UploadDesc> uploadQueue;
    };

    CopyContext::~CopyContext()
    {
        delete (CopyContextData*)mpApiData;
        mpApiData = nullptr;
    }

    void releaseUnusedResources(std::queue<CopyContextData::UploadDesc>& pQueue, uint64_t fenceValue)
    {
        while (pQueue.empty() == false && pQueue.front().id < fenceValue)
        {
            pQueue.pop();
        }
    }

    bool CopyContext::initApiData()
    {
        CopyContextData* pApiData = new CopyContextData;
        mpApiData = pApiData;
        pApiData->pAllocatorPool = CopyCommandAllocatorPool::create(mpFence);
        pApiData->pAllocator = pApiData->pAllocatorPool->newObject();

        // Create a command list
        if (FAILED(gpDevice->getApiHandle()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, pApiData->pAllocator, nullptr, IID_PPV_ARGS(&pApiData->pCmdList))))
        {
            Logger::log(Logger::Level::Error, "Failed to create command list for CopyContext");
            return false;
        }

        // Create the command queue
        D3D12_COMMAND_QUEUE_DESC cqDesc = {};
        cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cqDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

        if (FAILED(gpDevice->getApiHandle()->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pApiData->pQueue))))
        {
            Logger::log(Logger::Level::Error, "Failed to create command queue for CopyContext");
            return false;
        }
        return true;
    }

    void CopyContext::reset()
    {
        assert(mDirty == false);
        CopyContextData* pApiData = (CopyContextData*)mpApiData;
        releaseUnusedResources(pApiData->uploadQueue, mpFence->getGpuValue());
        pApiData->pAllocator = pApiData->pAllocatorPool->newObject();
        d3d_call(pApiData->pCmdList->Close());
        d3d_call(pApiData->pAllocator->Reset());
        d3d_call(pApiData->pCmdList->Reset(pApiData->pAllocator, nullptr));
    }

    void copySubresourceData(const D3D12_SUBRESOURCE_DATA& srcData, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& dstFootprint, uint8_t* pDstStart, uint64_t rowSize, uint64_t rowsToCopy)
    {
        const uint8_t* pSrc = (uint8_t*)srcData.pData;
        uint8_t* pDst = pDstStart + dstFootprint.Offset;
        const D3D12_SUBRESOURCE_FOOTPRINT& dstData = dstFootprint.Footprint;

        for (uint32_t z = 0; z < dstData.Depth; z++)
        {
            uint8_t* pDstSlice = pDst + rowsToCopy * dstData.RowPitch * z;
            const uint8_t* pSrcSlice = pSrc + srcData.SlicePitch * z;

            for (uint32_t y = 0; y < rowsToCopy; y++)
            {
                const uint8_t* pSrcRow = pSrcSlice + srcData.RowPitch * y;
                uint8_t* pDstRow = pDstSlice + dstData.RowPitch* y;
                memcpy(pDstRow, pSrcRow, rowSize);
            }
        }
    }

    void CopyContext::flush(GpuFence* pFence)
    {
        if (mDirty)
        {
            CopyContextData* pApiData = (CopyContextData*)mpApiData;

            if (pFence == nullptr)
            {
                pFence = mpFence.get();
            }
            else
            {
                // Need to signal the internal fence. The command allocator uses it to check if it can reuse an allocator or create a new one
                mpFence->gpuSignal(pApiData->pQueue);
            }

            // Execute the list
            pApiData->pCmdList->Close();
            ID3D12CommandList* pList = pApiData->pCmdList;
            pApiData->pQueue->ExecuteCommandLists(1, &pList);
            pFence->gpuSignal(pApiData->pQueue);

            // Reset the list
            d3d_call(pApiData->pCmdList->Reset(pApiData->pAllocator, nullptr));

            mDirty = false;
        }
    }

    void CopyContext::updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset, size_t size)
    {
        CopyContextData* pApiData = (CopyContextData*)mpApiData;
        if (size == 0)
        {
            size = pBuffer->getSize() - offset;
        }

        if (pBuffer->adjustSizeOffsetParams(size, offset) == false)
        {
            logWarning("CopyContext::updateBuffer() - size and offset are invalid. Nothing to update.");
            return;
        }

        mDirty = true;
        // Allocate a buffer on the upload heap
        CopyContextData::UploadDesc uploadDesc;
        uploadDesc.pBuffer = Buffer::create(size, Buffer::BindFlags::Staging, Buffer::CpuAccess::Write, nullptr);
        uploadDesc.pBuffer->updateData(pData, offset, size);
        ID3D12Resource* pResource = uploadDesc.pBuffer->getApiHandle();

        // Dispatch a command
        uploadDesc.id = mpFence->getCpuValue();
        pApiData->uploadQueue.push(uploadDesc);

        offset = uploadDesc.pBuffer->getGpuAddress() - pResource->GetGPUVirtualAddress();
        pApiData->pCmdList->CopyBufferRegion(pBuffer->getApiHandle(), 0, pResource, offset, size);
    }

    void CopyContext::updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData)
    {
        mDirty = true;

        uint32_t arraySize = (pTexture->getType() == Texture::Type::TextureCube) ? pTexture->getArraySize() * 6 : pTexture->getArraySize();
        assert(firstSubresource + subresourceCount <= arraySize * pTexture->getMipCount());
        CopyContextData* pApiData = (CopyContextData*)mpApiData;

        ID3D12Device* pDevice = gpDevice->getApiHandle();

        // Get the footprint
        D3D12_RESOURCE_DESC texDesc = pTexture->getApiHandle()->GetDesc();
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprint(subresourceCount);
        std::vector<uint32_t> rowCount(subresourceCount);
        std::vector<uint64_t> rowSize(subresourceCount);
        uint64_t size;
        pDevice->GetCopyableFootprints(&texDesc, firstSubresource, subresourceCount, 0, footprint.data(), rowCount.data(), rowSize.data(), &size);

        // Allocate a buffer on the upload heap
        CopyContextData::UploadDesc uploadDesc;
        uploadDesc.pBuffer = Buffer::create(size, Buffer::BindFlags::Staging, Buffer::CpuAccess::Write, nullptr);
        // Map the buffer
        uint8_t* pDst = (uint8_t*)uploadDesc.pBuffer->map(Buffer::MapType::WriteDiscard);
        ID3D12Resource* pResource = uploadDesc.pBuffer->getApiHandle();

        // Get the offset from the beginning of the resource
        uint64_t offset = uploadDesc.pBuffer->getGpuAddress() - pResource->GetGPUVirtualAddress();

        const uint8_t* pSrc = (uint8_t*)pData;
        for (uint32_t s = 0; s < subresourceCount; s++)
        {
            uint32_t subresource = s + firstSubresource;
            uint32_t physicalWidth = footprint[subresource].Footprint.Width / getFormatWidthCompressionRatio(pTexture->getFormat());
            uint32_t physicalHeight = footprint[subresource].Footprint.Height / getFormatHeightCompressionRatio(pTexture->getFormat());

            D3D12_SUBRESOURCE_DATA src;
            src.pData = pSrc;
            src.RowPitch = physicalWidth * getFormatBytesPerBlock(pTexture->getFormat());
            src.SlicePitch = src.RowPitch * physicalHeight;
            copySubresourceData(src, footprint[subresource], pDst, rowSize[subresource], rowCount[subresource]);
            pSrc = (uint8_t*)pSrc + footprint[subresource].Footprint.Depth * src.SlicePitch;

            // Dispatch a command
            footprint[subresource].Offset += offset;
            D3D12_TEXTURE_COPY_LOCATION dstLoc = { pTexture->getApiHandle(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, subresource };
            D3D12_TEXTURE_COPY_LOCATION srcLoc = { pResource, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, footprint[subresource] };
            pApiData->pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
        }

        uploadDesc.pBuffer->unmap();
        uploadDesc.id = mpFence->getCpuValue();
        pApiData->uploadQueue.push(uploadDesc);
    }

    void CopyContext::updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData)
    {
        mDirty = true;
        updateTextureSubresources(pTexture, subresourceIndex, 1, pData);
    }

    void CopyContext::updateTexture(const Texture* pTexture, const void* pData)
    {
        mDirty = true;
        uint32_t subresourceCount = pTexture->getArraySize() * pTexture->getMipCount();
        if (pTexture->getType() == Texture::Type::TextureCube)
        {
            subresourceCount *= 6;
        }
        updateTextureSubresources(pTexture, 0, subresourceCount, pData);
    }
}