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
#include "API/ResourceViews.h"
#include "API/Resource.h"
#include "API/D3D/D3DViews.h"
#include "API/Device.h"

namespace Falcor
{
    DepthStencilView::SharedPtr DepthStencilView::sNullView;
    RenderTargetView::SharedPtr RenderTargetView::sNullView;
    UnorderedAccessView::SharedPtr UnorderedAccessView::sNullView;
    ShaderResourceView::SharedPtr ShaderResourceView::sNullView;

    VkImageViewType getViewType(Resource::Type type, bool isArray)
    {
        switch (type)
        {
        case Resource::Type::Texture1D:
            return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case Resource::Type::Texture2D:
        case Resource::Type::Texture2DMultisample:
            return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case Resource::Type::Texture3D:
            assert(isArray == false);
            return VK_IMAGE_VIEW_TYPE_3D;
        case Resource::Type::TextureCube:
            return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        default:
            should_not_get_here();
            return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    void initializeImageViewInfo(const Texture::SharedConstPtr& pTexture, uint32_t mostDetailedMip, uint32_t mipCount, uint32_t firstArraySlice, uint32_t arraySize, VkImageViewCreateInfo& outInfo)
    {
        outInfo = {};

        ResourceFormat colorFormat = depthToColorFormat(pTexture->getFormat());
        bool isDepth = isDepthFormat(colorFormat);

        outInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        outInfo.image = pTexture->getApiHandle().getImage();
        outInfo.viewType = getViewType(pTexture->getType(), pTexture->getArraySize() > 1);
        outInfo.format = getVkFormat(colorFormat);
        outInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        outInfo.subresourceRange.baseMipLevel = mostDetailedMip;
        outInfo.subresourceRange.levelCount = mipCount;
        outInfo.subresourceRange.baseArrayLayer = firstArraySlice;
        outInfo.subresourceRange.layerCount = arraySize;
    }

    void initializeBufferViewInfo(const Buffer::SharedConstPtr& pBuffer, VkBufferViewCreateInfo& outInfo)
    {
        outInfo = {};
        outInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        outInfo.buffer = pBuffer->getApiHandle().getBuffer();
        outInfo.offset = 0;
        outInfo.range = VK_WHOLE_SIZE;

        TypedBufferBase::SharedConstPtr pTypedBuffer = std::dynamic_pointer_cast<const TypedBufferBase>(pBuffer);
        StructuredBuffer::SharedConstPtr pStructuredBuffer = std::dynamic_pointer_cast<const StructuredBuffer>(pBuffer);

        if (pTypedBuffer)
        {
            outInfo.format = getVkFormat(pTypedBuffer->getResourceFormat());
        }
        else if (pStructuredBuffer)
        {
            // #VKTODO what format for structured buffer?
        }
        else
        {
            // #VKTODO what format for raw buffer?
        }
    }

    VkResource<VkImageView, VkBufferView> createViewCommon(const Resource::SharedConstPtr& pSharedPtr, uint32_t mostDetailedMip, uint32_t mipCount, uint32_t firstArraySlice, uint32_t arraySize)
    {
        switch (pSharedPtr->getApiHandle().getType())
        {
        case VkResourceType::Image:
        {
            VkImageViewCreateInfo info;
            VkImageView imageView;
            initializeImageViewInfo(std::static_pointer_cast<const Texture>(pSharedPtr), firstArraySlice, arraySize, mostDetailedMip, mipCount, info);
            vkCreateImageView(gpDevice->getApiHandle(), &info, nullptr, &imageView);
            return imageView;
            //return SharedPtr(new View(pResource, view, mostDetailedMip, mipCount, firstArraySlice, arraySize));
        }

        case VkResourceType::Buffer:
        {
            VkBufferViewCreateInfo info;
            VkBufferView bufferView;
            initializeBufferViewInfo(std::static_pointer_cast<const Buffer>(pSharedPtr), info);
            vkCreateBufferView(gpDevice->getApiHandle(), &info, nullptr, &bufferView);
            return bufferView;
            //return SharedPtr(new View(pResource, view, mostDetailedMip, mipCount, firstArraySlice, arraySize));
        }

        default:
            should_not_get_here();
            return VkResource<VkImageView, VkBufferView>();
        }
    }

    ShaderResourceView::SharedPtr ShaderResourceView::create(ResourceWeakPtr pResource, uint32_t mostDetailedMip, uint32_t mipCount, uint32_t firstArraySlice, uint32_t arraySize)
    {
        Resource::SharedConstPtr pSharedPtr = pResource.lock();
        if (!pSharedPtr && sNullView)
        {
            return sNullView;
        }

        auto view = createViewCommon(pSharedPtr, mostDetailedMip, mipCount, firstArraySlice, arraySize);
        return SharedPtr(new ShaderResourceView(pResource, view, mostDetailedMip, mipCount, firstArraySlice, arraySize));
    }

    ShaderResourceView::SharedPtr ShaderResourceView::getNullView()
    {
        return create(ResourceWeakPtr(), 0, 0, 0, 0);
    }

    DepthStencilView::SharedPtr DepthStencilView::create(ResourceWeakPtr pResource, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        Resource::SharedConstPtr pSharedPtr = pResource.lock();
        if (!pSharedPtr && sNullView)
        {
            return sNullView;
        }

        if (pSharedPtr->getApiHandle().getType() == VkResourceType::Buffer)
        {
            logWarning("Cannot create DepthStencilView from a buffer!");
            return sNullView;
        }

        auto view = createViewCommon(pSharedPtr, mipLevel, 1, firstArraySlice, arraySize);
        return SharedPtr(new DepthStencilView(pResource, view, mipLevel, firstArraySlice, arraySize));
    }

    DepthStencilView::SharedPtr DepthStencilView::getNullView()
    {
        return create(ResourceWeakPtr(), 0, 0, 0);
    }

    UnorderedAccessView::SharedPtr UnorderedAccessView::create(ResourceWeakPtr pResource, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        Resource::SharedConstPtr pSharedPtr = pResource.lock();

        if (!pSharedPtr && sNullView)
        {
            return sNullView;
        }

        auto view = createViewCommon(pSharedPtr, mipLevel, 1, firstArraySlice, arraySize);
        return SharedPtr(new UnorderedAccessView(pResource, view, mipLevel, firstArraySlice, arraySize));
    }

    UnorderedAccessView::SharedPtr UnorderedAccessView::getNullView()
    {
        return create(ResourceWeakPtr(), 0, 0, 0);
    }


    RenderTargetView::SharedPtr RenderTargetView::create(ResourceWeakPtr pResource, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        Resource::SharedConstPtr pSharedPtr = pResource.lock();
        if (!pSharedPtr && sNullView)
        {
            return sNullView;
        }

        if (pSharedPtr->getApiHandle().getType() == VkResourceType::Buffer)
        {
            logWarning("Cannot create RenderTargetView from a buffer!");
            return sNullView;
        }

        auto view = createViewCommon(pSharedPtr, mipLevel, 1, firstArraySlice, arraySize);
        return SharedPtr(new RenderTargetView(pResource, view, mipLevel, firstArraySlice, arraySize));
    }

    RenderTargetView::SharedPtr RenderTargetView::getNullView()
    {
        return create(ResourceWeakPtr(), 0, 0, 0);
    }
}

