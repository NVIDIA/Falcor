/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
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
#include "API/LowLevel/ResourceAllocator.h"
#include "API/Buffer.h"
#include "API/Device.h"

namespace Falcor
{
    VkBuffer createBuffer(size_t size, Buffer::BindFlags bindFlags);

    VkDeviceMemory allocateDeviceMemory(Device::MemoryType memType, size_t size)
    {
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = size;
        allocInfo.memoryTypeIndex = gpDevice->getVkMemoryType(memType);

        VkDeviceMemory deviceMem;
        vk_call(vkAllocateMemory(gpDevice->getApiHandle(), &allocInfo, nullptr, &deviceMem));
        return deviceMem;
    }

    void ResourceAllocator::initCommonPageData(CommonData& data, size_t size)
    {
        // Create a buffer
        data.pResourceHandle = createBuffer(size, Buffer::BindFlags::Constant | Buffer::BindFlags::Vertex);
        data.pResourceHandle.setDeviceMem(allocateDeviceMemory(Device::MemoryType::Upload, size));
        data.gpuAddress = 0;
        vk_call(vkMapMemory(gpDevice->getApiHandle(), data.pResourceHandle.getDeviceMem(), 0, VK_WHOLE_SIZE, 0, (void**)&data.pData));
        vk_call(vkBindBufferMemory(gpDevice->getApiHandle(), data.pResourceHandle.getBuffer(), data.pResourceHandle.getDeviceMem(), 0));
    }
}
