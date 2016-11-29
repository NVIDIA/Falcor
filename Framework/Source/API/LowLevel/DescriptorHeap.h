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
#include "Framework.h"

namespace Falcor
{
    class DescriptorHeap : public std::enable_shared_from_this<DescriptorHeap>
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorHeap>;
        using SharedConstPtr = std::shared_ptr<const DescriptorHeap>;
        using ApiHandle = DescriptorHeapHandle;
        ~DescriptorHeap();

        using CpuHandle = HeapCpuHandle;
        using GpuHandle = HeapGpuHandle;

        enum class Type
        {
            ShaderResource,
            Sampler,
            RenderTargetView,
            DepthStencilView,
            UnorderedAccessView
        };
        static SharedPtr create(Type type, uint32_t descriptorsCount, bool shaderVisible = true);

        CpuHandle getCpuHandle(uint32_t index) const;
        GpuHandle getGpuHandle(uint32_t index) const;
        uint32_t allocateHandle();
        ApiHandle getApiHandle() const { return mApiHandle; }

        static const uint32_t kInvalidHandleIndex = -1;
    private:
        DescriptorHeap(Type type, uint32_t descriptorsCount);

        CpuHandle mCpuHeapStart = {};
        GpuHandle mGpuHeapStart = {};
        uint32_t mDescriptorSize;
        uint32_t mCount;
        uint32_t mCurDesc = 0;
        ApiHandle mApiHandle;
        Type mType;
    };
}
