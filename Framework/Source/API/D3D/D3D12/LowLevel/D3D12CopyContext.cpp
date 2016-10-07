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

        struct UploadDesc
        {
            uint64_t id;
            ID3D12ResourcePtr pResource;
        };

        std::queue<UploadDesc> uploadQueue;
    };

    void releaseUnusedResources(std::queue<CopyContextData::UploadDesc>& uploadQueue, uint64_t id)
    {
        while (uploadQueue.size() && uploadQueue.front().id <= id)
        {
            uploadQueue.pop();
        }
    }

    CopyContext::~CopyContext()
    {
        delete (CopyContextData*)mpApiData;
        mpApiData = nullptr;
    }

    bool CopyContext::initApiData()
    {
        CopyContextData* pApiData = new CopyContextData;
        mpApiData = pApiData;
        pApiData->pAllocatorPool = CopyCommandAllocatorPool::create(mpFence);

        // Create a command list
        if (FAILED(gpDevice->getApiHandle()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, pApiData->pAllocatorPool->getObject(), nullptr, IID_PPV_ARGS(&pApiData->pCmdList))))
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

    void CopyContext::flush(uint64_t copyId)
    {
        CopyContextData* pApiData = (CopyContextData*)mpApiData;
        copyId = (copyId == 0) ? mpFence->getCpuValue() : copyId;
        assert(copyId <= mpFence->getCpuValue());
        mpFence->wait(copyId);
        releaseUnusedResources(pApiData->uploadQueue, copyId);
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

    void CopyContext::submit(bool flush)
    {
        CopyContextData* pApiData = (CopyContextData*)mpApiData;

        // Execute the list
        pApiData->pCmdList->Close();
        ID3D12CommandList* pList = pApiData->pCmdList;
        pApiData->pQueue->ExecuteCommandLists(1, &pList);
        mpFence->signal(pApiData->pQueue);

        // Reset the allocator and list
        ID3D12CommandAllocator* pAllocator = pApiData->pAllocatorPool->getObject();
        d3d_call(pAllocator->Reset());
        d3d_call(pApiData->pCmdList->Reset(pAllocator, nullptr));

        if (flush)
        {
            this->flush();
        }
    }

    uint64_t CopyContext::updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset, size_t size, bool submit)
    {
        CopyContextData* pApiData = (CopyContextData*)mpApiData;
        ID3D12Resource* pResource = pBuffer->getApiHandle();
        if (size == 0)
        {
            size = pBuffer->getSize() - offset;
        }

        if (pBuffer->adjustSizeOffsetParams(size, offset) == false)
        {
            logWarning("CopyContext::updateBuffer() - size and offset are invalid. Nothing to update.");
            return mpFence->getCpuValue();
        }

        // Allocate a buffer on the upload heap
        Buffer::SharedPtr pUploadBuffer = Buffer::create(size, Buffer::BindFlags::None, Buffer::CpuAccess::Write, nullptr);
        pUploadBuffer->updateData(pData, offset, size);

        // Dispatch a command
        CopyContextData::UploadDesc uploadDesc;
        uploadDesc.pResource = pBuffer->getApiHandle();
        uploadDesc.id = mpFence->inc();
        pApiData->uploadQueue.push(uploadDesc);

        D3D12_TEXTURE_COPY_LOCATION dstLoc = { pBuffer->getApiHandle(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
        D3D12_TEXTURE_COPY_LOCATION srcLoc = { pBuffer->getApiHandle(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
        pApiData->pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        if (submit)
        {
            this->submit();
        }

        return uploadDesc.id;
    }

    uint64_t CopyContext::updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData, bool submit)
    {
        assert(firstSubresource + subresourceCount <= pTexture->getArraySize() * pTexture->getMipCount());
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
        Buffer::SharedPtr pBuffer = Buffer::create(size, Buffer::BindFlags::None, Buffer::CpuAccess::Write, nullptr);

        CopyContextData::UploadDesc uploadDesc;

        // Map the buffer
        uint8_t* pDst = (uint8_t*)pBuffer->map(Buffer::MapType::WriteDiscard);

        uploadDesc.pResource = pBuffer->getApiHandle();
        uploadDesc.id = mpFence->inc();
        pApiData->uploadQueue.push(uploadDesc);

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
            D3D12_TEXTURE_COPY_LOCATION dstLoc = { pTexture->getApiHandle(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, subresource };
            D3D12_TEXTURE_COPY_LOCATION srcLoc = { uploadDesc.pResource, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, footprint[subresource] };
            pApiData->pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
        }

        uploadDesc.pResource->Unmap(0, nullptr);

        if (submit)
        {
            this->submit();
        }

        return uploadDesc.id;
    }

    uint64_t CopyContext::updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData, bool submit)
    {
        return updateTextureSubresources(pTexture, subresourceIndex, 1, pData, submit);
    }

    uint64_t CopyContext::updateTexture(const Texture* pTexture, const void* pData, bool submit)
    {
        uint32_t subresourceCount = pTexture->getArraySize() * pTexture->getMipCount();
        return updateTextureSubresources(pTexture, 0, subresourceCount, pData, submit);
    }
}