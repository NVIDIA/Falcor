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
    template<> VkSmartHandle<VkDevice>::~VkSmartHandle() { vkDestroyDevice(mHandle, nullptr); }
    template<> VkSmartHandle<VkCommandBuffer>::~VkSmartHandle() { /* Need to track pool to free */ }
    template<> VkSmartHandle<VkCommandPool>::~VkSmartHandle() { vkDestroyCommandPool(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkSemaphore>::~VkSmartHandle() { vkDestroySemaphore(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkImage>::~VkSmartHandle() { vkDestroyImage(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkBuffer>::~VkSmartHandle() { vkDestroyBuffer(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkImageView>::~VkSmartHandle() { vkDestroyImageView(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkBufferView>::~VkSmartHandle() { vkDestroyBufferView(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkRenderPass>::~VkSmartHandle() { vkDestroyRenderPass(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkFramebuffer>::~VkSmartHandle() { vkDestroyFramebuffer(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkSampler>::~VkSmartHandle() { vkDestroySampler(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkDescriptorSetLayout>::~VkSmartHandle() { vkDestroyDescriptorSetLayout(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkDescriptorSet>::~VkSmartHandle() { /* Need to track pool to free */ }
    template<> VkSmartHandle<VkPipeline>::~VkSmartHandle() { vkDestroyPipeline(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkShaderModule>::~VkSmartHandle() { vkDestroyShaderModule(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkPipelineLayout>::~VkSmartHandle() { vkDestroyPipelineLayout(gpDevice->getApiHandle(), mHandle, nullptr); }
    template<> VkSmartHandle<VkDescriptorPool>::~VkSmartHandle() { vkDestroyDescriptorPool(gpDevice->getApiHandle(), mHandle, nullptr); }
}

