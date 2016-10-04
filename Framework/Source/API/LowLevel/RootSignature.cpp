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
#include "API/LowLevel/RootSignature.h"
#include "API/ProgramReflection.h"

namespace Falcor
{
    RootSignature::Desc& RootSignature::Desc::addSampler(uint32_t regIndex, Sampler::SharedConstPtr pSampler, ShaderVisibility visiblityMask, BorderColor borderColor, uint32_t regSpace)
    {
        SamplerDesc sd;
        sd.pSampler = pSampler;
        sd.regIndex = regIndex;
        sd.regSpace = regSpace;
        sd.visibility = visiblityMask;
        sd.borderColor = borderColor;
        mSamplers.push_back(sd);
        return *this;
    }

    RootSignature::Desc& RootSignature::Desc::addConstant(uint32_t regIndex, uint32_t dwordCount, ShaderVisibility visiblityMask, uint32_t regSpace)
    {
        ConstantDesc cd;
        cd.dwordCount = dwordCount;
        cd.regIndex = regIndex;
        cd.regSpace = regSpace;
        cd.visibility = visiblityMask;
        mConstants.push_back(cd);
        return *this;
    }

    RootSignature::Desc& RootSignature::Desc::addDescriptor(uint32_t regIndex, DescType type, ShaderVisibility visiblityMask, uint32_t regSpace)
    {
        DescriptorDesc rd;
        rd.regIndex = regIndex;
        rd.regSpace = regSpace;
        rd.visibility = visiblityMask;
        rd.type = type;
        mRootDescriptors.push_back(rd);
        return *this;
    }

    RootSignature::DescriptorTable& RootSignature::DescriptorTable::addRange(DescType type, uint32_t firstRegIndex, uint32_t descriptorCount, uint32_t regSpace, uint32_t offsetFromTableStart)
    {
        Range r;
        r.descCount = descriptorCount;
        r.firstRegIndex = firstRegIndex;
        r.regSpace = regSpace;
        r.type = type;
        r.offsetFromTableStart = offsetFromTableStart;

        mRanges.push_back(r);
        return *this;
    }

    RootSignature::RootSignature(const Desc& desc) : mDesc(desc)
    {

    }

    RootSignature::SharedPtr RootSignature::create(const Desc& desc)
    {
        SharedPtr pSig = SharedPtr(new RootSignature(desc));
        if (pSig->apiInit() == false)
        {
            pSig = nullptr;
        }
        return pSig;
    }


    void initializeBufferDescTable(const ProgramReflection* pReflector, RootSignature::Desc& desc, ProgramReflection::BufferReflection::Type bufferType, RootSignature::DescType descType)
    {
        const auto& bufMap = pReflector->getBufferMap(bufferType);
        RootSignature::DescriptorTable descTable;
        for (const auto& buf : bufMap)
        {
            const ProgramReflection::BufferReflection* pBuffer = buf.second.get();
            descTable.addRange(descType, pBuffer->getRegisterIndex(), 1, pBuffer->getRegisterSpace());
        }
        desc.addDescriptorTable(descTable);
    }

    RootSignature::SharedPtr RootSignature::createFromReflection(const ProgramReflection* pReflector)
    {
        Desc d;
        // There are no static samplers or root constants. Only CBVs, UAVs and SRVs. Need to initialize 2 tables
        // OPTME:
        //      - For now we ignore shader-visibility flags and create everything with full visibility
        //      - We also don't pack ranges. We create an entry for each CBV instead of a range

        initializeBufferDescTable(pReflector, d, ProgramReflection::BufferReflection::Type::Constant, RootSignature::DescType::CBV);
        initializeBufferDescTable(pReflector, d, ProgramReflection::BufferReflection::Type::UnorderedAccess, RootSignature::DescType::UAV);

        const ProgramReflection::ResourceMap& resMap = pReflector->getResourceMap();
        RootSignature::DescriptorTable srvTable;
        RootSignature::DescriptorTable uavTable;
        RootSignature::DescriptorTable samplerTable;
        for (auto& resIt : resMap)
        {
            const ProgramReflection::Resource& resource = resIt.second;
            switch (resource.type)
            {
            case ProgramReflection::Resource::ResourceType::UAV:
                uavTable.addRange(DescType::UAV, resource.regIndex, 1, resource.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::Texture:
                srvTable.addRange(DescType::SRV, resource.regIndex, 1, resource.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::Sampler:
                samplerTable.addRange(DescType::Sampler, resource.regIndex, 1, resource.registerSpace);
                break;
            default:
                should_not_get_here();
            }
        }

        d.addDescriptorTable(srvTable);
        d.addDescriptorTable(uavTable);
        d.addDescriptorTable(samplerTable);

        return create(d);
    }
}