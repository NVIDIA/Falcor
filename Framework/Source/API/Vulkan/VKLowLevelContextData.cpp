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
#include "API/LowLevel/LowLevelContextData.h"
#include "API/Device.h"

namespace Falcor
{
    bool createCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPool *pCommandPool)
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};

        commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags            = 0;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

        if (VK_FAILED(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, pCommandPool)))
        {
            logError("Could not create command pool");
            return false;
        }

        return true;
    }

    bool createCommandBuffer(VkDevice device, uint32_t pCmdBufferCount, VkCommandBuffer *pCmdBuffer, VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = commandPool;
        cmdBufAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Allocate only primary now.
        cmdBufAllocateInfo.commandBufferCount = pCmdBufferCount;

        if (VK_FAILED(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, pCmdBuffer)))
        {
            logError("Could not create command buffer.");
            return false;
        }

        return true;
    }

    LowLevelContextData::SharedPtr LowLevelContextData::create(LowLevelContextData::CommandQueueType type, CommandQueueHandle queue)
    {
        SharedPtr pThis = SharedPtr(new LowLevelContextData);
        pThis->mType = type;
        pThis->mpFence = GpuFence::create();
        pThis->mpQueue = queue;

        DeviceHandle device = gpDevice->getApiHandle();
        createCommandPool(device, gpDevice->getApiCommandQueueType(type), &pThis->mpAllocator);

        // #VKTODO Generally, we'd have one per swapchain image. Not sure if that needs to be handled here
        createCommandBuffer(device, 1, &pThis->mpList, pThis->mpAllocator);

        // #VKTODO Check how the allocator maps in Vulkan.
        // pThis->mpAllocator = pThis->mpAllocatorPool->newObject();

        return pThis;
    }

    void LowLevelContextData::reset()
    {
        if (VK_FAILED(vkResetCommandBuffer(mpList, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)))
        {
            logError("Could not reset command buffer.");
        }
    }

    // Submit the recorded command buffers here. 
    void LowLevelContextData::flush()
    {
        VkSubmitInfo submitInfo = {};

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mpList;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;

        // #VKTODO The fence has to be reset. This has to be ideally part of the GPUFence class
        // Add a new interface there? 
        // vkResetFences(gpDevice->getApiHandle(), 1, &(mpFence->getApiHandle()));

        if (VK_FAILED(vkQueueSubmit(mpQueue, 1, &submitInfo, mpFence->getApiHandle())))
        {
            logError("Could not submit command queue.");
        }
    }
}