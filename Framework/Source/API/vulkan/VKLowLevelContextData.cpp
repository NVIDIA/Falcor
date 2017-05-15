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
#include "API/vulkan/FalcorVK.h"

// @Kai-Hwa: This is used as a hack for now. There are some build errors I see inside GLM when I define
// the WIN32 macro.
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>

namespace Falcor
{
    // TODO: These need to go to a common struct somewhere.
    static VkCommandPool	 commandPool;

    static bool createCommandPool()
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};

        commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags            = 0;
        commandPoolCreateInfo.queueFamilyIndex = 0;//dd->queueNodeIndex;

        auto res = vkCreateCommandPool(VK_NULL_HANDLE, &commandPoolCreateInfo, nullptr, &commandPool);
        return (res == VK_SUCCESS);
    }

    LowLevelContextData::SharedPtr LowLevelContextData::create(CommandListType type)
    {
        SharedPtr pThis = SharedPtr(new LowLevelContextData);
        pThis->mpFence = GpuFence::create();
        
        //pThis->mpQueue = &pVKData->deviceQueue; // TODO: Actually not even needed. Device has it!

        createCommandPool();

        // TODO: Check how the allocator maps in Vulkan.
        // pThis->mpAllocator = pThis->mpAllocatorPool->newObject();

        // TODO: Create a command list
        // TODO: Is this a command buffer?
        // if (FAILED(pDevice->CreateCommandList(0, cqDesc.Type, pThis->mpAllocator, nullptr, IID_PPV_ARGS(&pThis->mpList))))

        return pThis;
    }

    void LowLevelContextData::reset()
    {
        return;
    }

    void LowLevelContextData::flush()
    {
        return;
    }
}