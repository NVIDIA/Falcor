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
#include "API/Texture.h"
#include "API/Device.h"
#include "API/Resource.h"

namespace Falcor
{
    void Texture::apiInit()
    {
    }

    // Like getD3D12ResourceFlags but for Images specifically
    VkImageUsageFlags getVkImageUsageFlags(Resource::BindFlags bindFlags)
    {
        VkImageUsageFlags vkFlags;

        //if (is_set(bindFlags, Resource::BindFlags::UnorderedAccess))
        //{
        //    vkFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        //}

        if (is_set(bindFlags, Resource::BindFlags::DepthStencil))
        {

            vkFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (is_set(bindFlags, Resource::BindFlags::ShaderResource) == false)
        {
            // #VKTODO what does VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT mean?
            vkFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (is_set(bindFlags, Resource::BindFlags::RenderTarget))
        {
            vkFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        // According to spec, must not be 0
        assert(vkFlags != 0);

        return vkFlags;
    }

    uint32_t getMaxMipCount(const VkExtent3D& size)
    {
        return 1 + uint32_t(glm::log2(float(glm::max(glm::max(size.width, size.height), size.depth))));
    }

    void createTextureCommon(const Texture* pTexture, Texture::ApiHandle& apiHandle, const void* pData, VkImageType imageType, bool autoGenMips, Texture::BindFlags bindFlags)
    {
        ResourceFormat texFormat = pTexture->getFormat();

        VkImageCreateInfo imageInfo = {};

        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = getVkFormat(texFormat);
        imageInfo.flags = pTexture->getType() == Texture::Type::TextureCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        imageInfo.extent.width = align_to(getFormatWidthCompressionRatio(texFormat), pTexture->getWidth());
        imageInfo.extent.height = align_to(getFormatHeightCompressionRatio(texFormat), pTexture->getHeight());
        imageInfo.extent.depth = pTexture->getDepth();
        imageInfo.mipLevels = glm::min(pTexture->getMipCount(), getMaxMipCount(imageInfo.extent));
        imageInfo.samples = (VkSampleCountFlagBits)pTexture->getSampleCount();
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
        imageInfo.usage = getVkImageUsageFlags(bindFlags);
        imageInfo.initialLayout = pData != nullptr ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = 0;
        imageInfo.pQueueFamilyIndices = nullptr;

        if (VK_FAILED(vkCreateImage(gpDevice->getApiHandle(), &imageInfo, nullptr, &apiHandle)))
        {
            logError("Failed to create texture.");
            return;
        }

        if (pData != nullptr)
        {
            
        }
    }

    // #VKTODO Same as in D3D12Texture, maybe move to Texture.cpp?
    Texture::BindFlags updateBindFlags(Texture::BindFlags flags, bool hasInitData, uint32_t mipLevels)
    {
        if ((mipLevels != Texture::kMaxPossible) || (hasInitData == false))
        {
            return flags;
        }

        flags |= Texture::BindFlags::RenderTarget;
        return flags;
    }

    Texture::SharedPtr Texture::create2D(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData, BindFlags bindFlags)
    {
        bindFlags = updateBindFlags(bindFlags, pData != nullptr, mipLevels);
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, height, 1, arraySize, mipLevels, 1, format, Type::Texture2D, bindFlags));
        createTextureCommon(pTexture.get(), pTexture->mApiHandle, pData, VK_IMAGE_TYPE_2D, (mipLevels == kMaxPossible), bindFlags);
        return pTexture->mApiHandle ? pTexture : nullptr;
    }

}