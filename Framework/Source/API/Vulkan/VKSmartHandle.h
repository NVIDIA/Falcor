#pragma once

namespace Falcor
{
    class VkHandleBase : public std::shared_ptr<VkHandleBase>
    {
    public:
        VkHandleBase() = default;
        virtual ~VkHandleBase() = default;
    };

    template<typename ApiHandle>
    class VkSmartHandle : public VkHandleBase
    {
    public:
        VkSmartHandle() : mHandle(VK_NULL_HANDLE) {}
        VkSmartHandle(ApiHandle vkHandle) : mHandle(vkHandle) {}
        virtual ~VkSmartHandle() { static_assert(false, "Unspecialized destructor for VkSmartHandle."); }
        operator ApiHandle() const { return mHandle; }
        ApiHandle* operator &() { return &mHandle; }
    private:
        ApiHandle mHandle;
    };

    template<> VkSmartHandle<VkDevice>::~VkSmartHandle();
    template<> VkSmartHandle<VkCommandBuffer>::~VkSmartHandle();
    template<> VkSmartHandle<VkCommandPool>::~VkSmartHandle();
    template<> VkSmartHandle<VkSemaphore>::~VkSmartHandle();
    template<> VkSmartHandle<VkImage>::~VkSmartHandle();
    template<> VkSmartHandle<VkBuffer>::~VkSmartHandle();
    template<> VkSmartHandle<VkImageView>::~VkSmartHandle();
    template<> VkSmartHandle<VkBufferView>::~VkSmartHandle();
    template<> VkSmartHandle<VkRenderPass>::~VkSmartHandle();
    template<> VkSmartHandle<VkFramebuffer>::~VkSmartHandle();
    template<> VkSmartHandle<VkSampler>::~VkSmartHandle();
    template<> VkSmartHandle<VkDescriptorSetLayout>::~VkSmartHandle();
    template<> VkSmartHandle<VkDescriptorSet>::~VkSmartHandle();
    template<> VkSmartHandle<VkPipeline>::~VkSmartHandle();
    template<> VkSmartHandle<VkShaderModule>::~VkSmartHandle();
    template<> VkSmartHandle<VkPipelineLayout>::~VkSmartHandle();
    template<> VkSmartHandle<VkDescriptorPool>::~VkSmartHandle();

    using VkDevicePtr = VkSmartHandle<VkDevice>;
    using VkCommandBufferPtr = VkSmartHandle<VkCommandBuffer>;
    using VkCommandPoolPtr = VkSmartHandle<VkCommandPool>;
    using VkSemaphorePtr = VkSmartHandle<VkSemaphore>;
    using VkImagePtr = VkSmartHandle<VkImage>;
    using VkBufferPtr = VkSmartHandle<VkBuffer>;
    using VkImageViewPtr = VkSmartHandle<VkImageView>;
    using VkBufferViewPtr = VkSmartHandle<VkBufferView>;
    using VkRenderPassPtr = VkSmartHandle<VkRenderPass>;
    using VkFramebufferPtr = VkSmartHandle<VkFramebuffer>;
    using VkSamplerPtr = VkSmartHandle<VkSampler>;
    using VkDescriptorSetLayoutPtr = VkSmartHandle<VkDescriptorSetLayout>;
    using VkDescriptorSetPtr = VkSmartHandle<VkDescriptorSet>;
    using VkPipelinePtr = VkSmartHandle<VkPipeline>;
    using VkShaderModulePtr = VkSmartHandle<VkShaderModule>;
    using VkPipelineLayoutPtr = VkSmartHandle<VkPipelineLayout>;
    using VkDescriptorPoolPtr = VkSmartHandle<VkDescriptorPool>;
}