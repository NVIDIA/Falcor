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
#include "Core/Sampler.h"

namespace Falcor
{
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

    gen_bitwise_for_enum_class(ShaderVisibility);

    class RootSignature
    {
    public:
        using SharedPtr = std::shared_ptr<RootSignature>;
        using SharedConstPtr = std::shared_ptr<const RootSignature>;
        using ApiHandle = RootSignatureHandle;
        
        enum class BorderColor
        {
            TransparentBlack,
            OpaqueBlack,
            OpaqueWhite
        };

        struct RegisterDesc
        {
            uint32_t regIndex = 0;
            uint32_t regSpace = 0;
        };

        struct ConstantDesc : public RegisterDesc
        {
            uint32_t dwordCount = 0;
        };

        struct SamplerDesc : public RegisterDesc
        {
            ShaderVisibility visibility;
            BorderColor borderColor;
            Sampler::SharedConstPtr pSampler;
        };

        class DescriptorTable
        {
            enum class DescType
            {
                SRV,
                UAV,
                CBV,
                Sampler
            };

            struct Range
            {
                DescType type;
                uint32_t firstRegIndex;
                uint32_t descCount;
                uint32_t regSpace;
                uint32_t offsetFromTableStart;
            };

            static uint32_t const kAppendOffset = uint32_t(-1);
            DescriptorTable& addRange(DescType type, uint32_t firstRegIndex, uint32_t descriptorCount, uint32_t regSpace = 0, uint32_t offsetFromTableStart = kAppendOffset);
        public:
            friend class Desc;
            DescriptorTable() = default;
            std::vector<Range> mRanges;
        };

        class Desc
        {
        public:
            Desc& addConstant(uint32_t regIndex, uint32_t dwordCount, uint32_t regSpace = 0);
            Desc& addSampler(uint32_t regIndex, Sampler::SharedConstPtr pSampler, ShaderVisibility shaderMask, BorderColor borderColor = BorderColor::OpaqueBlack, uint32_t regSpace = 0);
            Desc& addDescriptor(uint32_t regIndex, uint32_t regSpace = 0);
            DescriptorTable& addDescriptorTable() { mDescriptorTables.push_back(DescriptorTable()); return mDescriptorTables.back(); }
        private:
            friend class RootSignature;
            std::vector<ConstantDesc> mConstants;
            std::vector<RegisterDesc> mRootDescriptors;
            std::vector<DescriptorTable> mDescriptorTables;
            std::vector<SamplerDesc> mSamplers;
        };

        static SharedPtr create(const Desc& desc);
        ApiHandle getApiHandle() const { return mApiHandle; }
    private:
        RootSignature(const Desc& desc);
        bool apiInit();
        ApiHandle mApiHandle;
        Desc mDesc;
    };
}