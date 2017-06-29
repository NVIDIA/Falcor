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
#include "API/LowLevel/GpuFence.h"
#include "API/Device.h"
#include "API/Vulkan/FalcorVK.h"

namespace Falcor
{
    struct FenceApiData
    {
        VkFence fence = nullptr;
    };

    static VkFence createFence(bool signaled)
    {
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (signaled) info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence;
        vkCreateFence(gpDevice->getApiHandle(), &info, nullptr, &fence);
        return fence;
    }

    static VkFence getFence(FenceApiData* pApiData)
    {
        Device::ApiHandle device = gpDevice->getApiHandle();
        if (vkGetFenceStatus(device, pApiData->fence) == VK_SUCCESS)
        {
            // Fence is signaled, just reset it
            vkResetFences(device, 1, &pApiData->fence);
        }
        else
        {
            // Fence is not ready, create a new one
            vkDestroyFence(device, pApiData->fence, nullptr);
            pApiData->fence = createFence(false);
        }
        return pApiData->fence;
    }

    GpuFence::~GpuFence()
    {
        Device::ApiHandle device = gpDevice->getApiHandle();
        if (mpApiData->fence) vkDestroyFence(device, mpApiData->fence, nullptr);

        safe_delete(mpApiData);
    }

    GpuFence::SharedPtr GpuFence::create()
    {
        SharedPtr pFence = SharedPtr(new GpuFence());
        pFence->mpApiData = new FenceApiData;
        pFence->mpApiData->fence = createFence(true);
        return pFence;
    }

    uint64_t GpuFence::gpuSignal(CommandQueueHandle pQueue)
    {
        mCpuValue++;
        VkFence fence = getFence(mpApiData);
        vk_call(vkQueueSubmit(pQueue, 0, nullptr, fence));
        return mCpuValue;
    }

    uint64_t GpuFence::cpuSignal()
    {
        return 0;
    }

    void GpuFence::syncGpu(CommandQueueHandle pQueue)
    {
    }

    void GpuFence::syncCpu()
    {
        vk_call(vkWaitForFences(gpDevice->getApiHandle(), 1, &mpApiData->fence, false, UINT64_MAX));
    }

    uint64_t GpuFence::getGpuValue() const
    {
        return 0;
    }
}
