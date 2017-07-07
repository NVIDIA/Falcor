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
#include "API/Vulkan/VKSmartHandle.h"
#include "API/Device.h"

namespace Falcor
{
    template<> VkSimpleSmartHandle<VkInstance>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyInstance(mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkSwapchainKHR>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroySwapchainKHR(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkDevice>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyDevice(mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkCommandPool>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyCommandPool(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkSemaphore>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroySemaphore(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkImage>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyImage(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkBuffer>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyBuffer(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkImageView>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyImageView(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkBufferView>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyBufferView(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkRenderPass>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyRenderPass(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkFramebuffer>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyFramebuffer(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkSampler>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroySampler(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkDescriptorSetLayout>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkPipeline>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyPipeline(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkShaderModule>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyShaderModule(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkPipelineLayout>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyPipelineLayout(gpDevice->getApiHandle(), mApiHandle, nullptr); }
    template<> VkSimpleSmartHandle<VkDescriptorPool>::~VkSimpleSmartHandle() { if(mApiHandle != VK_NULL_HANDLE) vkDestroyDescriptorPool(gpDevice->getApiHandle(), mApiHandle, nullptr); }

    template<>
    VkResource<VkImage, VkBuffer>::~VkResource()
    {
        if (*this)
        {
            switch (mType)
            {
            case VkResourceType::Image:
                vkDestroyImage(gpDevice->getApiHandle(), mImage, nullptr);
                break;
            case VkResourceType::Buffer:
                vkDestroyBuffer(gpDevice->getApiHandle(), mBuffer, nullptr);
                break;
            default:
                should_not_get_here();
            }
            vkFreeMemory(gpDevice->getApiHandle(), mDeviceMem, nullptr);
        }
    }

    template<>
    VkResource<VkImageView, VkBufferView>::~VkResource()
    {
        if (*this)
        {
            switch (mType)
            {
            case VkResourceType::Image:
                vkDestroyImageView(gpDevice->getApiHandle(), mImage, nullptr);
                break;
            case VkResourceType::Buffer:
                vkDestroyBufferView(gpDevice->getApiHandle(), mBuffer, nullptr);
                break;
            default:
                should_not_get_here();
            }
        }
    }

    VkFbo::~VkFbo()
    {
        vkDestroyRenderPass(gpDevice->getApiHandle(), mVkRenderPass, nullptr);
        vkDestroyFramebuffer(gpDevice->getApiHandle(), mVkFbo, nullptr);
    }

    VkCommandList::~VkCommandList()
    {
        vkFreeCommandBuffers(gpDevice->getApiHandle(), mCommandPool, 1, &mCommandBuffer);
    }

    // Force template instantiation
    template VkSimpleSmartHandle<VkInstance>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkSwapchainKHR>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkDevice>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkCommandPool>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkSemaphore>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkImage>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkBuffer>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkImageView>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkBufferView>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkRenderPass>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkFramebuffer>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkSampler>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkDescriptorSetLayout>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkPipeline>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkShaderModule>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkPipelineLayout>::~VkSimpleSmartHandle();
    template VkSimpleSmartHandle<VkDescriptorPool>::~VkSimpleSmartHandle();

    template VkResource<VkImage, VkBuffer>::~VkResource();
    template VkResource<VkImageView, VkBufferView>::~VkResource();

}

