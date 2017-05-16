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

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_PROTOTYPES

#include <Vulkan/vulkan.h>

#pragma comment(lib, "vulkan-1.lib")

namespace Falcor
{
    using ApiObjectHandle = void*;

    using HeapCpuHandle = void*;
    using HeapGpuHandle = void*;

    class DescriptorHeapEntry;

    using WindowHandle = HWND;
    using DeviceHandle = VkDevice;
    using CommandListHandle = void*;
    using CommandQueueHandle = void*;
    using CommandAllocatorHandle = void*;
    using CommandSignatureHandle = void*;
    using FenceHandle = void*;
    using ResourceHandle = void*;
    using RtvHandle = void*;
    using DsvHandle = void*;
    using SrvHandle = void*;
    using SamplerHandle = void*;
    using UavHandle = void*;
    using GpuAddress = void*;

    using PsoHandle = void*;
    using ComputeStateHandle = void*;
    using ShaderHandle = void*;
    using ShaderReflectionHandle = void*;
    using RootSignatureHandle = void*;
    using DescriptorHeapHandle = void*;

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
