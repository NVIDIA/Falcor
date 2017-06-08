/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
#include "API/CopyContext.h"
#include "API/Buffer.h"

namespace Falcor
{
    CopyContext::~CopyContext() = default;

    CopyContext::SharedPtr CopyContext::create(CommandQueueHandle queue)
    {
        SharedPtr pCtx = SharedPtr(new CopyContext());
        pCtx->mpLowLevelData = LowLevelContextData::create(LowLevelContextData::CommandQueueType::Copy, queue);
        return pCtx->mpLowLevelData ? pCtx : nullptr;
    }

    void CopyContext::bindDescriptorHeaps()
    {
    }

    void CopyContext::reset()
    {
        flush();
        mpLowLevelData->reset();
        bindDescriptorHeaps();
    }

    void copySubresourceData(/*const D3D12_SUBRESOURCE_DATA& srcData, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& dstFootprint, */uint8_t* pDstStart, uint64_t rowSize, uint64_t rowsToCopy)
    {

    }

    void CopyContext::flush(bool wait)
    {
        if (mCommandsPending)
        {
            mpLowLevelData->flush();
            mCommandsPending = false;
            bindDescriptorHeaps();
        }

        if (wait)
        {
            mpLowLevelData->getFence()->syncCpu();
        }
    }

    void CopyContext::updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset, size_t size)
    {
        if (size == 0)
        {
            size = pBuffer->getSize() - offset;
        }

        if (pBuffer->adjustSizeOffsetParams(size, offset) == false)
        {
            logWarning("CopyContext::updateBuffer() - size and offset are invalid. Nothing to update.");
            return;
        }

        mCommandsPending = true;


    }

    void CopyContext::updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData)
    {
        mCommandsPending = true;

    }

    void CopyContext::updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData)
    {
        mCommandsPending = true;
        updateTextureSubresources(pTexture, subresourceIndex, 1, pData);
    }

    std::vector<uint8> CopyContext::readTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex)
    {
        //Get buffer data
        std::vector<uint8> result;
        return result;
    }

    void CopyContext::updateTexture(const Texture* pTexture, const void* pData)
    {
        mCommandsPending = true;


        //updateTextureSubresources(pTexture, 0, subresourceCount, pData);
    }

    void CopyContext::resourceBarrier(const Resource* pResource, Resource::State newState)
    {
        if (pResource->getState() != newState)
        {

            mCommandsPending = true;
        }
    }

    void CopyContext::copyResource(const Resource* pDst, const Resource* pSrc)
    {
        resourceBarrier(pDst, Resource::State::CopyDest);
        resourceBarrier(pSrc, Resource::State::CopySource);
        //mpLowLevelData->getCommandList()->CopyResource(pDst->getApiHandle(), pSrc->getApiHandle());
        mCommandsPending = true;
    }

    void CopyContext::copySubresource(const Resource* pDst, uint32_t dstSubresourceIdx, const Resource* pSrc, uint32_t srcSubresourceIdx)
    {
        resourceBarrier(pDst, Resource::State::CopyDest);
        resourceBarrier(pSrc, Resource::State::CopySource);


        mCommandsPending = true;
    }

    void CopyContext::copyBufferRegion(const Resource* pDst, uint64_t dstOffset, const Resource* pSrc, uint64_t srcOffset, uint64_t numBytes)
    {
        resourceBarrier(pDst, Resource::State::CopyDest);
        resourceBarrier(pSrc, Resource::State::CopySource);
        //mpLowLevelData->getCommandList()->CopyBufferRegion(pDst->getApiHandle(), dstOffset, pSrc->getApiHandle(), srcOffset, numBytes);
        mCommandsPending = true;
    }
}
