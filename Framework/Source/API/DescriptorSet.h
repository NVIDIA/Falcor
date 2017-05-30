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
#include "LowLevel/DescriptorPool.h"

namespace Falcor
{
    struct DescriptorSetApiData;
    class DescriptorSet
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorSet>;
        using Type = DescriptorPool::Type;
        using CpuHandle = DescriptorPool::CpuHandle;
        using GpuHandle = DescriptorPool::GpuHandle;

        ~DescriptorSet();

        struct Layout
        {
        public:
            Layout& addRange(Type type, uint32_t count) { mRanges.push_back({ type, count }); return *this; }
        private:
            friend class DescriptorSet;
            struct Range
            {
                Type type;
                uint32_t count;
            };
            std::vector<Range> mRanges;
        };

        static SharedPtr create(DescriptorPool::SharedPtr pPool, const Layout& layout);

        size_t getRangeCount() const { return mLayout.mRanges.size(); }
        Type getRangeType(uint32_t range) const { return mLayout.mRanges[range].type; }
        uint32_t getRangeDescCount(uint32_t range) const { return mLayout.mRanges[range].count; }

        CpuHandle getCpuHandle(uint32_t rangeIndex, uint32_t descInRange = 0) const;
        GpuHandle getGpuHandle(uint32_t rangeIndex, uint32_t descInRange = 0) const;

        void copy(const DescriptorSet* pSrc, uint32_t rangeIndex, uint32_t descCount, uint32_t descOffset);
    private:
        using ApiData = DescriptorSetApiData;
        DescriptorSet(DescriptorPool::SharedPtr pPool, const Layout& layout) : mpPool(pPool), mLayout(layout) {}

        bool apiInit();
        Layout mLayout;
        std::shared_ptr<ApiData> mpApiData;
        DescriptorPool::SharedPtr mpPool;
    };
}