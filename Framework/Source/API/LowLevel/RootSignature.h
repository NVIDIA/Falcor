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
#include "API/Sampler.h"

namespace Falcor
{
    class ProgramReflection;

    enum class ShaderVisibility
    {
        None = 0,
        Vertex = (1 << (uint32_t)ShaderType::Vertex),
        Pixel = (1 << (uint32_t)ShaderType::Pixel),
        Hull = (1 << (uint32_t)ShaderType::Hull),
        Domain = (1 << (uint32_t)ShaderType::Domain),
        Geometry = (1 << (uint32_t)ShaderType::Geometry),

        All = (1 << (uint32_t)ShaderType::Count) - 1,

    };

    enum_class_operators(ShaderVisibility);

    class RootSignature
    {
    public:
        using SharedPtr = std::shared_ptr<RootSignature>;
        using SharedConstPtr = std::shared_ptr<const RootSignature>;
        using ApiHandle = RootSignatureHandle;
        
        enum class DescType
        {
            SRV,
            UAV,
            CBV,
            Sampler
        };

        class Desc;

        class DescriptorSet
        {
        public:
            struct Range
            {
                DescType type;
                uint32_t firstRegIndex;
                uint32_t descCount;
                uint32_t regSpace;
                uint32_t offsetFromTableStart;
            };

            DescriptorSet(ShaderVisibility visibility = ShaderVisibility::All) : mVisibility(visibility) {}
            static uint32_t const kAppendOffset = uint32_t(-1);
            DescriptorSet& addRange(DescType type, uint32_t firstRegIndex, uint32_t descriptorCount, uint32_t regSpace = 0, uint32_t offsetFromTableStart = kAppendOffset);
            size_t getRangeCount() const { return mRanges.size(); }
            const Range& getRange(size_t index) const { return mRanges[index]; }
            ShaderVisibility getVisibility() const { return mVisibility; }
        private:
            friend class RootSignature::Desc;
            std::vector<Range> mRanges;
            ShaderVisibility mVisibility;
        };

        class Desc
        {
        public:
            Desc& addDescriptorSet(const DescriptorSet& set);
        private:
            friend class RootSignature;
            std::vector<DescriptorSet> mSets;
        };

        ~RootSignature();
        static SharedPtr getEmpty();
        static SharedPtr create(const Desc& desc);
        static SharedPtr create(const ProgramReflection* pReflection);

        ApiHandle getApiHandle() const { return mApiHandle; }

        size_t getDescriptorSetCount() const { return mDesc.mSets.size(); }
        const DescriptorSet& getDescriptorSet(size_t index) const { return mDesc.mSets[index]; }

        uint32_t getSizeInBytes() const { return mSizeInBytes; }
        uint32_t getElementByteOffset(uint32_t elementIndex) { return mElementByteOffset[elementIndex]; }
    private:
        RootSignature(const Desc& desc);
        bool apiInit();
        ApiHandle mApiHandle;
        Desc mDesc;
        std::vector<uint32_t> mDescriptorIndices;
        std::vector<uint32_t> mConstantIndices;
        static SharedPtr spEmptySig;
        static uint64_t sObjCount;

        uint32_t mSizeInBytes;
        std::vector<uint32_t> mElementByteOffset;
    };
}