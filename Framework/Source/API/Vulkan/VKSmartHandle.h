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
    class VkBaseApiHandle : public std::enable_shared_from_this<VkBaseApiHandle>
    {
    public:
        using SharedPtr = std::shared_ptr<VkBaseApiHandle>;
        virtual ~VkBaseApiHandle() = default;
        virtual operator bool() { return false; }
    };

    // Enables regular Vulkan handle usage with a ref-counting backend using std::shared_ptr
    template<typename SmartHandle>
    class VkSmartHandle : public std::shared_ptr<SmartHandle>
    {
    public:
        VkSmartHandle() = default;
        VkSmartHandle(SmartHandle* pHandle) : std::shared_ptr<SmartHandle>(pHandle->inherit_shared_from_this::shared_from_this()) {}
        VkSmartHandle(decltype(nullptr)) { reset(); }

        template<typename T>
        operator T() const { return (T)(*get()); }

        template<typename T>
        VkSmartHandle(const T& t) : std::shared_ptr<SmartHandle>(new SmartHandle(t)) { }

        SmartHandle& operator&()
        {
            if (*this == nullptr)
            {
                *this = VkSmartHandle((SmartHandle)VK_NULL_HANDLE);
            }
            return *get();
        }
    };

    template<typename ApiHandle>
    class VkSimpleSmartHandle : public VkBaseApiHandle, public inherit_shared_from_this<VkBaseApiHandle, VkSimpleSmartHandle<typename ApiHandle>>
    {
    public:
        using SharedPtr = VkSmartHandle<VkSimpleSmartHandle<ApiHandle>>;

        VkSimpleSmartHandle() = default;
        VkSimpleSmartHandle(const ApiHandle& apiHandle) : mApiHandle(apiHandle) {}
        ~VkSimpleSmartHandle() { static_assert(false, "VkSimpleSmartHandle missing destructor specialization"); }
        virtual operator bool() override { return mApiHandle != VK_NULL_HANDLE; }
        operator ApiHandle() const { return mApiHandle; }
        operator ApiHandle*() { return &mApiHandle; }
    private:
        ApiHandle mApiHandle = {};
    };

    class VkDeviceData : public VkBaseApiHandle, public inherit_shared_from_this<VkBaseApiHandle, VkDeviceData>
    {
    public:
        using SharedPtr = VkSmartHandle<VkDeviceData>;

        VkDeviceData() = default;
        ~VkDeviceData();
        VkDeviceData& operator=(VkInstance instance) { mInstance = instance; return *this; }
        VkDeviceData& operator=(VkPhysicalDevice physicalDevice) { mPhysicalDevice = physicalDevice; return *this; }
        VkDeviceData& operator=(VkDevice device) { mLogicalDevice = device; return *this; }
        VkDeviceData& operator=(VkSurfaceKHR surface) { mSurface = surface; return *this; }
        operator VkInstance() const { return mInstance; }
        operator VkPhysicalDevice() const { return mPhysicalDevice; }
        operator VkDevice() const { return mLogicalDevice; }
        operator VkSurfaceKHR() const { return mSurface; }
    private:
        VkInstance          mInstance;
        VkPhysicalDevice    mPhysicalDevice;
        VkDevice            mLogicalDevice;
        VkSurfaceKHR        mSurface;

    };

    enum class VkResourceType
    {
        None,
        Image,
        Buffer
    };

    template<typename ImageType, typename BufferType>
    class VkResource : public VkBaseApiHandle, public inherit_shared_from_this<VkBaseApiHandle, VkResource<typename ImageType, typename BufferType>>
    {
    public:
        using SharedPtr = VkSmartHandle<VkResource<ImageType, BufferType>>;

        VkResource() = default;
        VkResource(ImageType image) : mType(VkResourceType::Image), mImage(image) {}
        VkResource(BufferType buffer) : mType(VkResourceType::Buffer), mBuffer(buffer) {}
        ~VkResource() { static_assert(false, "VkResource missing destructor specialization"); }

        VkResourceType getType() const { return mType; }
        virtual operator bool() override;

        operator ImageType() const { assert(mType == VkResourceType::Image); return mImage; }
        operator BufferType() const { assert(mType == VkResourceType::Buffer); return mBuffer; }

        void operator=(ImageType rhs) { assert(mType == VkResourceType::None); mType = VkResourceType::Image; mImage = rhs; }
        void operator=(BufferType rhs) { assert(mType == VkResourceType::None); mType = VkResourceType::Buffer; mBuffer = rhs; }
        void operator=(VkDeviceMemory rhs) { mDeviceMem = rhs; }
        VkResource& operator=(const VkResource& rhs) = default;

        void setDeviceMem(VkDeviceMemory deviceMem) { mDeviceMem = deviceMem; }
        VkDeviceMemory getDeviceMem() const { return mDeviceMem; }

    private:
        VkResourceType mType = VkResourceType::None;
        ImageType mImage = VK_NULL_HANDLE;
        BufferType mBuffer = VK_NULL_HANDLE;
        VkDeviceMemory mDeviceMem = VK_NULL_HANDLE;
    };

    class VkFbo : public VkBaseApiHandle
    {
    public:
        using SharedPtr = VkSmartHandle<VkFbo>;

        ~VkFbo();
        virtual operator bool() override { return mVkRenderPass != VK_NULL_HANDLE && mVkFbo != VK_NULL_HANDLE; }
        VkFbo& operator=(VkRenderPass pass) { mVkRenderPass = pass; return *this; }
        VkFbo& operator=(VkFramebuffer fb) { mVkFbo = fb; return *this; }
        operator VkRenderPass() const { return mVkRenderPass; }
        operator VkFramebuffer() const { return mVkFbo; }
    private:
        VkRenderPass mVkRenderPass = VK_NULL_HANDLE;
        VkFramebuffer mVkFbo = VK_NULL_HANDLE;
    };

    template<> VkResource<VkImage, VkBuffer>::operator bool();
    template<> VkResource<VkImageView, VkBufferView>::operator bool();

    // Destructors
    template<> VkSimpleSmartHandle<VkSwapchainKHR>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkCommandPool>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkSemaphore>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkSampler>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkDescriptorSetLayout>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkPipeline>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkShaderModule>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkPipelineLayout>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkDescriptorPool>::~VkSimpleSmartHandle();

    template<> VkResource<VkImage, VkBuffer>::~VkResource();
    template<> VkResource<VkImageView, VkBufferView>::~VkResource();
}
