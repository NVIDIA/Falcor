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
    // #VKTODO This entire class seems overly complicated. Need to make sure that there are no performance issues
    struct FenceApiData
    {
        std::queue<VkFence> fenceQueue;
        std::vector<VkFence> availableFences;
        uint64_t gpuValue = 0;
    };

    static VkFence getFence(FenceApiData* pApiData)
    {
        if (pApiData->availableFences.size())
        {
            VkFence fence = pApiData->availableFences.back();
            pApiData->availableFences.pop_back();
            vkResetFences(gpDevice->getApiHandle(), 1, &fence);
            return fence;
        }
        else
        {
            VkFenceCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VkFence fence;
            vkCreateFence(gpDevice->getApiHandle(), &info, nullptr, &fence);
            return fence;
        }
    }

    static void moveFencesToVec(std::queue<VkFence>& queue, std::vector<VkFence>& vec)
    {
        while (queue.size())
        {
            vec.push_back(queue.front());
            queue.pop();
        }
    }

    GpuFence::~GpuFence()
    {
        Device::ApiHandle device = gpDevice->getApiHandle();
        while (mpApiData->fenceQueue.size())
        {
            vkDestroyFence(device, mpApiData->fenceQueue.front(), nullptr);
            mpApiData->fenceQueue.pop();
        }
        safe_delete(mpApiData);
    }

    GpuFence::SharedPtr GpuFence::create()
    {
        SharedPtr pFence = SharedPtr(new GpuFence());
        pFence->mpApiData = new FenceApiData;
        return pFence;
    }

    uint64_t GpuFence::gpuSignal(CommandQueueHandle pQueue)
    {
        mCpuValue++;
        VkFence fence = getFence(mpApiData);
        mpApiData->fenceQueue.push(fence);
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
        if (mpApiData->fenceQueue.empty()) return;
        std::vector<VkFence> fenceVec;
        fenceVec.reserve((mpApiData->fenceQueue.size()));
        moveFencesToVec(mpApiData->fenceQueue, fenceVec);

        vk_call(vkWaitForFences(gpDevice->getApiHandle(), (uint32_t)fenceVec.size(), fenceVec.data(), true, UINT64_MAX));
        mpApiData->gpuValue += fenceVec.size();
        mpApiData->availableFences.insert(mpApiData->availableFences.end(), fenceVec.begin(), fenceVec.end());
    }

    uint64_t GpuFence::getGpuValue() const
    {
        while (mpApiData->fenceQueue.size())
        {
            VkFence fence = mpApiData->fenceQueue.front();
            if (vkGetFenceStatus(gpDevice->getApiHandle(), fence) == VK_SUCCESS)
            {
                mpApiData->availableFences.push_back(fence);
                mpApiData->fenceQueue.pop();
                mpApiData->gpuValue++;
            }
            else
            {
                break;
            }
        }

        return mpApiData->gpuValue;
    }
}
