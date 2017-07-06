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
#pragma once
#define NOMINMAX
#include "API/Formats.h"

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <Vulkan/vulkan.h>

#ifdef _WIN32
    #pragma comment(lib, "vulkan-1.lib")
#endif

#include "API/Vulkan/VKResourceHelper.h"

__forceinline BOOL vkBool(bool b) { return b ? VK_TRUE : VK_FALSE; }

namespace Falcor
{
    struct VkFormatDesc
    {
        ResourceFormat falcorFormat;
        VkFormat vkFormat;
    };

    extern const VkFormatDesc kVkFormatDesc[];

    inline VkFormat getVkFormat(ResourceFormat format)
    {
        assert(kVkFormatDesc[(uint32_t)format].falcorFormat == format);
        assert(kVkFormatDesc[(uint32_t)format].vkFormat != VK_FORMAT_UNDEFINED);
        return kVkFormatDesc[(uint32_t)format].vkFormat;
    }

    class VkFboHandle
    {
    public:
        VkFboHandle& operator=(VkRenderPass pass) { mVkRenderPass = pass; return *this; }
        VkFboHandle& operator=(VkFramebuffer fb) { mVkFbo = fb; return *this; }
        operator VkRenderPass() const { return mVkRenderPass; }
        operator VkFramebuffer() const { return mVkFbo; }
    private:
        VkRenderPass mVkRenderPass = VK_NULL_HANDLE;
        VkFramebuffer mVkFbo = VK_NULL_HANDLE;
    };

    using HeapCpuHandle = void*;
    using HeapGpuHandle = void*;

    class DescriptorHeapEntry;

#ifdef _WIN32
    using WindowHandle = HWND;
#else
    using WindowHandle = void*;
#endif

    using DeviceHandle = VkDevice;
    using CommandListHandle = VkCommandBuffer;
    using CommandQueueHandle = VkQueue;
    using ApiCommandQueueType = uint32_t;
    using CommandAllocatorHandle = VkCommandPool;
    using CommandSignatureHandle = void*;
    using FenceHandle = VkSemaphore;
    using ResourceHandle = VkResource<VkImage, VkBuffer>;
    using RtvHandle = VkImageView;
    using DsvHandle = VkImageView;
    using SrvHandle = VkResource<VkImageView, VkBufferView>;
    using UavHandle = VkResource<VkImageView, VkBufferView>;
    using CbvHandle = VkBufferView;
    using FboHandle = VkFboHandle;
    using SamplerHandle = VkSampler;
    using GpuAddress = size_t;
    using DescriptorSetApiHandle = VkDescriptorSet;

    using PsoHandle = VkPipeline;
    using ComputeStateHandle = VkPipeline;
    using ShaderHandle = VkShaderModule;
    using ShaderReflectionHandle = void*;
    using RootSignatureHandle = VkPipelineLayout;
    using DescriptorHeapHandle = VkDescriptorPool;

    using VaoHandle = void*;
    using VertexShaderHandle = void*;
    using FragmentShaderHandle = void*;
    using DomainShaderHandle = void*;
    using HullShaderHandle = void*;
    using GeometryShaderHandle = void*;
    using ComputeShaderHandle = void*;
    using ProgramHandle = void*;
    using DepthStencilStateHandle = void*;
    using RasterizerStateHandle = void*;
    using BlendStateHandle = void*;

    static const uint32_t kSwapChainBuffers = 3;

    using ApiObjectHandle = ResourceHandle;

    inline uint32_t getMaxViewportCount()
    {
        // #VKTODO we need to get this from querying PhysicalDeviceProperties
        return 8;
    }
}

#define DEFAULT_API_MAJOR_VERSION 1
#define DEFAULT_API_MINOR_VERSION 0

#define VK_FAILED(res) (res != VK_SUCCESS)

#define VK_DISABLE_UNIMPLEMENTED

#ifdef _LOG_ENABLED
#define vk_call(a) {auto r = a; if(VK_FAILED(r)) { logError("Vulkan call failed.\n"#a); }}
#else
#define vk_call(a) a
#endif

#define UNSUPPORTED_IN_VULKAN(msg_) {logWarning(msg_ + std::string(" is not supported in Vulkan. Ignoring call."));}
