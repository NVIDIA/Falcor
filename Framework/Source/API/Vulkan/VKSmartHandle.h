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
        virtual operator bool() override { return ((mType == VkResourceType::Image && mImage != VK_NULL_HANDLE) || (mType == VkResourceType::Buffer && mBuffer != VK_NULL_HANDLE)) && (mDeviceMem != VK_NULL_HANDLE); }

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

    class VkCommandList : public VkBaseApiHandle
    {
    public:
        using SharedPtr = VkSmartHandle<VkCommandList>;

        ~VkCommandList();
        virtual operator bool() override { return mCommandBuffer != VK_NULL_HANDLE && mCommandPool != VK_NULL_HANDLE; }
        VkCommandList& operator=(VkCommandBuffer buffer) { mCommandBuffer = buffer; return *this; }
        VkCommandList& operator=(VkCommandPool pool) { mCommandPool = pool; return *this; }
        operator VkCommandBuffer() const { return mCommandBuffer; }
        operator VkCommandPool() const { return mCommandPool; }

    private:
        VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
        VkCommandPool mCommandPool = VK_NULL_HANDLE; // Only used to destroy mCommandBuffer
    };


    template<> VkSimpleSmartHandle<VkInstance>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkSwapchainKHR>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkDevice>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkCommandPool>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkSemaphore>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkImage>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkBuffer>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkImageView>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkBufferView>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkRenderPass>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkFramebuffer>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkSampler>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkDescriptorSetLayout>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkPipeline>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkShaderModule>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkPipelineLayout>::~VkSimpleSmartHandle();
    template<> VkSimpleSmartHandle<VkDescriptorPool>::~VkSimpleSmartHandle();

    template<> VkResource<VkImage, VkBuffer>::~VkResource();
    template<> VkResource<VkImageView, VkBufferView>::~VkResource();
}
