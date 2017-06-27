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
#include "API/Texture.h"

namespace Falcor
{
    static VkImageLayout getImageLayout(Resource::State state)
    {
        switch (state)
        {
        case Resource::State::Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case Resource::State::Common:
        case Resource::State::UnorderedAccess:
            return VK_IMAGE_LAYOUT_GENERAL;
        case Resource::State::RenderTarget:
        case Resource::State::ResolveDest:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;            
        case Resource::State::DepthStencil:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case Resource::State::ShaderResource:
        case Resource::State::ResolveSource:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case Resource::State::CopyDest:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case Resource::State::CopySource:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            break;
        case Resource::State::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default:
            should_not_get_here();
            return VkImageLayout (-1);
        }
    }

    static VkAccessFlagBits getImageAccessMask(Resource::State state)
    {
        switch (state)
        {
        case Resource::State::Undefined:
        case Resource::State::Present:
        case Resource::State::Common:
            return VkAccessFlagBits(0);
        case Resource::State::VertexBuffer:
            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        case Resource::State::ConstantBuffer:
            return VK_ACCESS_UNIFORM_READ_BIT;
        case Resource::State::IndexBuffer:
            return VK_ACCESS_INDEX_READ_BIT;
        case Resource::State::RenderTarget:
            return VkAccessFlagBits(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
        case Resource::State::UnorderedAccess:
            return VK_ACCESS_SHADER_WRITE_BIT;
        case Resource::State::DepthStencil:
            return VkAccessFlagBits(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        case Resource::State::ShaderResource:
            return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case Resource::State::IndirectArg:
            return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case Resource::State::CopyDest:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case Resource::State::CopySource:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case Resource::State::ResolveDest:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case Resource::State::ResolveSource:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        default:
            should_not_get_here();
            return VkAccessFlagBits(-1);
        }
    }

    void CopyContext::bindDescriptorHeaps()
    {
    }

    void copySubresourceData(/*const D3D12_SUBRESOURCE_DATA& srcData, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& dstFootprint, */uint8_t* pDstStart, uint64_t rowSize, uint64_t rowsToCopy)
    {

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

    void CopyContext::resourceBarrier(const Resource* pResource, Resource::State newState)
    {
        if (pResource->getState() != newState)
        {
            if(pResource->getApiHandle().getType() == VkResourceType::Image)
            {
                const Texture* pTexture = dynamic_cast<const Texture*>(pResource);
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.newLayout = getImageLayout(newState);
                barrier.oldLayout = getImageLayout(pResource->mState);
                barrier.image = pResource->getApiHandle().getImage();
                barrier.subresourceRange.aspectMask = isDepthStencilFormat(pTexture->getFormat()) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.layerCount = pTexture->getArraySize();
                barrier.subresourceRange.levelCount = pTexture->getMipCount();
                barrier.srcAccessMask = getImageAccessMask(pResource->mState);
                barrier.dstAccessMask = getImageAccessMask(newState);

                // #OPTME: Group barriers into a single call. Do we always need to use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT?
                vkCmdPipelineBarrier(mpLowLevelData->getCommandList(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            }
            else
            {
                assert(pResource->getApiHandle().getType() == VkResourceType::Buffer);
                should_not_get_here();
            }


            pResource->mState = newState;
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
