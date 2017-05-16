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
#include "API/Buffer.h"
#include "API/Device.h"
#include "Api/LowLevel/ResourceAllocator.h"
#include "API/vulkan/FalcorVK.h"

namespace Falcor
{
    // TODO: 
    // These classes need access to the DeviceData struct from VKDevice.cpp
    // D3D seems to do it differently. The below is just temporary. The actual code
    // will be to replace this with the pLogicalDevice from DeviceData and other
    // meaningful variables.
    VkDevice      dev;
    VkBuffer      buf;
    VkFlags       memProps;
    VkSharingMode sMode;


    struct BufferData
    {
        ResourceAllocator::AllocationData dynamicData;
        Buffer::SharedPtr                 pStagingResource; // For buffers that have both CPU read flag and can be used by the GPU
        VkDeviceMemory                    bufferMemory;     // This is the actual backing store for the buffer.
    };

    inline uint32_t getMemoryType(uint32_t type, VkFlags memoryProperties)
    {
        for (uint32_t i = 0; i < 32; i++)
        {
            if ((type & 1) == 1)
            {
                // TODO: This has to be again obtained from the deviceData
                // if (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties)
                {
                    return i;
                }
            }
            type >>= 1;
        }

        assert(0);
        return 0;
    }

    VkBufferUsageFlags getBufferUSageFlag(Buffer::BindFlags bindFlags)
    {       
        switch (bindFlags)
        {
            case  Buffer::BindFlags::Vertex:          return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            case  Buffer::BindFlags::Index:           return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            case  Buffer::BindFlags::UnorderedAccess: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            case  Buffer::BindFlags::ShaderResource:  return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            case  Buffer::BindFlags::IndirectArg:     return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

            // TODO: The below ones need to be assigned the right flags.
            case  Buffer::BindFlags::StreamOutput:    return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            case  Buffer::BindFlags::RenderTarget:    return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            case  Buffer::BindFlags::DepthStencil:    return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            default:                                  return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    bool createBuffer(VkBuffer& buffer, Buffer::State initState, size_t size, VkFlags memoryProperties, Buffer::BindFlags bindFlags, VkSharingMode sharingMode)
    {
        static const VkBufferCreateInfo bufferCreateInfo =
        {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            nullptr,
            0,
            size,
            getBufferUSageFlag(bindFlags),
            sharingMode,
            0,
            nullptr
        };

        vkCreateBuffer(dev, &bufferCreateInfo, nullptr, &buffer);
        assert(buffer != VK_NULL_HANDLE);

        // The allocation of memory is not handled here. Commenting out for now.
        //VkMemoryAllocateInfo memAlloc = {};
        //VkMemoryRequirements memReqs;
        //vkGetBufferMemoryRequirements(dev, buffer, &memReqs);
        //memAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        //memAlloc.allocationSize  = memReqs.size;
        //memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryProperties);        
        //auto result = vkAllocateMemory(vkTutDevice, &memAlloc, nullptr, &bufferMemory);
        //assert(result == VK_SUCCESS);
        //result = vkBindBufferMemory(vkTutDevice, buffer, bufferMemory, 0);

        return buffer != VK_NULL_HANDLE;
    }

    Buffer::~Buffer()
    {
        BufferData* pApiData = (BufferData*)mpApiData;
        gpDevice->getResourceAllocator()->release(pApiData->dynamicData);
        safe_delete(pApiData);
        gpDevice->releaseResource(mApiHandle);
    }


    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, CpuAccess cpuAccess, const void* pInitData)
    {
        Buffer::SharedPtr pBuffer = SharedPtr(new Buffer(size, usage, cpuAccess));
        return pBuffer->init(pInitData) ? pBuffer : nullptr;
    }

    bool Buffer::init(const void* pInitData)
    {
        BufferData* pApiData = new BufferData;
        mpApiData = pApiData;

        if (mCpuAccess == CpuAccess::Write)
        {
            mState = Resource::State::GenericRead;
            //pApiData->dynamicData = gpDevice->getResourceAllocator()->allocate(mSize, getDataAlignmentFromUsage(mBindFlags));
            //mApiHandle = pApiData->dynamicData.pResourceHandle;
        }
        else if (mCpuAccess == CpuAccess::Read && mBindFlags == BindFlags::None)
        {
            mState = Resource::State::CopyDest;
            //mApiHandle = createBuffer(buf, mState, mSize, memProps, mBindFlags, sMode);
        }
        else
        {
            mState = Resource::State::Common;
            //mApiHandle = createBuffer(buf, mState, mSize, memProps, mBindFlags, sMode);
        }
    
        return true;
    }

    void Buffer::updateData(const void* pData, size_t offset, size_t size) const
    {
        if (mCpuAccess == CpuAccess::Write)
        {
            uint8_t* pDst = (uint8_t*)map(MapType::WriteDiscard) + offset;
            memcpy(pDst, pData, size);
        }
        else
        {
            gpDevice->getRenderContext()->updateBuffer(this, pData, offset, size);
        }
    }

    void Buffer::readData(void* pData, size_t offset, size_t size) const
    {
    }

    void* Buffer::map(MapType type) const
    {
        BufferData* pApiData = (BufferData*)mpApiData;
        void*       pData = nullptr;
        const unsigned int offset = 0;

        auto res = vkMapMemory(dev, pApiData->bufferMemory, offset, VK_WHOLE_SIZE, 0/*flags*/, &pData);
        assert(res == VK_SUCCESS);

        // TODO: These can be used when the flags get updated. For now, the mapping is the same.
        if (type == MapType::WriteDiscard)
        {
        }
        else
        {

        }

        return pData;
    }

    uint64_t Buffer::getGpuAddress() const
    {
        return 0;
    }

    void Buffer::unmap() const
    {
        // Only unmap read buffers
        BufferData* pApiData = (BufferData*)mpApiData;
        
        vkUnmapMemory(dev, pApiData->bufferMemory);
    }

    uint64_t Buffer::makeResident(Buffer::GpuAccessFlags flags) const
    {
        return 0;
    }

    void Buffer::evict() const
    {
    }
}
