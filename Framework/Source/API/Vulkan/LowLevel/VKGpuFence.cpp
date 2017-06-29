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
    VkFence createFence()
    {
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        vkCreateFence(gpDevice->getApiHandle(), &info, nullptr, &fence);
        return fence;
    }

    void destroyFence(VkFence fence)
    {
        vkDestroyFence(gpDevice->getApiHandle(), fence, nullptr);
    }

    void resetFence(VkFence fence)
    {
        vkResetFences(gpDevice->getApiHandle(), 1, &fence);
    }

    struct FenceApiData
    {
        template<typename VkType, decltype(createFence) createFunc, decltype(destroyFence) destroyFunc, decltype(resetFence) resetFunc>
        class SmartQueue
        {
        public:
            ~SmartQueue()
            {
                popAllObjects();
                for (auto& o : freeObjects)
                {
                    destroyFunc(o);
                }
            }

            VkType getObject()
            {
                VkType object;
                if (freeObjects.size())
                {
                    object = freeObjects.back();
                    freeObjects.pop_back();
                    resetFunc(object);
                }
                else
                {
                    object = createFunc();
                }
                activeObjects.push_back(object);
                return object;
            }

            void popAllObjects()
            {
                freeObjects.insert(freeObjects.end(), activeObjects.begin(), activeObjects.end());
                activeObjects.clear();
            }

            void popFront()
            {
                freeObjects.push_back(activeObjects.front());
                activeObjects.pop_front();
            }

            bool hasActiveObjects() const { return activeObjects.size() != 0; }

            std::deque<VkType>& getActiveObjects() { return activeObjects; }
        private:
            std::deque<VkType> activeObjects;
            std::vector<VkType> freeObjects;
        };

        SmartQueue<VkFence, createFence, destroyFence, resetFence> fenceQueue;
//         std::deque<VkSemaphore> semaphoreQueue;
//         std::vector<VkSemaphore> availableSemaphors;

        uint64_t gpuValue = 0;
    };

    GpuFence::~GpuFence()
    {
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
        VkFence fence = mpApiData->fenceQueue.getObject();
        vk_call(vkQueueSubmit(pQueue, 0, nullptr, fence));
        return mCpuValue;
    }

    void GpuFence::syncGpu(CommandQueueHandle pQueue)
    {
    }

    void GpuFence::syncCpu()
    {
        if (mpApiData->fenceQueue.hasActiveObjects() == false) return;

        auto& activeObjects = mpApiData->fenceQueue.getActiveObjects();
        std::vector<VkFence> fenceVec(activeObjects.begin(), activeObjects.end());
        vk_call(vkWaitForFences(gpDevice->getApiHandle(), (uint32_t)fenceVec.size(), fenceVec.data(), true, UINT64_MAX));
        mpApiData->gpuValue += fenceVec.size();
        mpApiData->fenceQueue.popAllObjects();
    }

    uint64_t GpuFence::getGpuValue() const
    {
        auto& activeObjects = mpApiData->fenceQueue.getActiveObjects();

        while (activeObjects.size())
        {
            VkFence fence = activeObjects.front();
            if (vkGetFenceStatus(gpDevice->getApiHandle(), fence) == VK_SUCCESS)
            {
                mpApiData->fenceQueue.popFront();
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
