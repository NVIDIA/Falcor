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
#pragma once

namespace Falcor
{
    enum class VkResourceType
    {
        None,
        Image,
        Buffer
    };

    template<typename ImageType, typename BufferType>
    class VkResource
    {
    public:

        VkResource() = default;

        ~VkResource()
        {
            switch (mType)
            {
            case Falcor::VkResourceType::None:
                break;
            case Falcor::VkResourceType::Image:
                break;
            case Falcor::VkResourceType::Buffer:
                break;
            default:
                break;
            }
        }

        VkResource(ImageType image) : mType(VkResourceType::Image), mImage(image) {}

        VkResource(BufferType buffer) : mType(VkResourceType::Buffer), mBuffer(buffer) {}

        VkResourceType getType() const { return mType; }

        explicit operator bool() { return (mType == VkResourceType::Image && mImage != nullptr) || (mType == VkResourceType::Buffer && mBuffer != nullptr); }

        ImageType getImage() const { assert(mType == VkResourceType::Image); return mImage; }

        BufferType getBuffer() const { assert(mType == VkResourceType::Buffer); return mBuffer; }

        void operator=(ImageType rhs) { set(rhs); }

        void operator=(BufferType rhs) { set(rhs); }

        void set(ImageType image) { assert(mType == VkResourceType::None); mType = VkResourceType::Image; mImage = image; }

        void set(BufferType buffer) { assert(mType == VkResourceType::None); mType = VkResourceType::Buffer; mBuffer = buffer; }

        void reset()
        {
            mType = VkResourceType::None;
            mImage = VK_NULL_HANDLE;
            mBuffer = VK_NULL_HANDLE;
        }

        void setDeviceMem(VkDeviceMemory deviceMem) { mDeviceMem = deviceMem; }

        VkDeviceMemory getDeviceMem() const { return mDeviceMem; }

        VkResource& operator=(const VkResource& rhs) = default;

        void release()
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

        operator bool() const { return mDeviceMem != VK_NULL_HANDLE; }
        operator ImageType() const { return getImage(); }
        operator BufferType() const { return getBuffer(); }
    private:
        VkDeviceMemory mDeviceMem;
        VkResourceType mType = VkResourceType::None;
        ImageType mImage = VK_NULL_HANDLE;
        BufferType mBuffer = VK_NULL_HANDLE;
    };
}
