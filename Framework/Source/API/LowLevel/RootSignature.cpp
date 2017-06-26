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
    RootSignature::SharedPtr RootSignature::spEmptySig;
    uint64_t RootSignature::sObjCount = 0;

    RootSignature::Desc& RootSignature::Desc::addDescriptorSet(const DescriptorSetLayout& setLayout)
    {
        assert(setLayout.getRangeCount());
        mSets.push_back(setLayout);
        return *this; 
    }

    RootSignature::RootSignature(const Desc& desc) : mDesc(desc)
    {
        sObjCount++;
    }

    RootSignature::~RootSignature()
    {
        sObjCount--;        
        if (spEmptySig && sObjCount == 1) // That's right, 1. It means spEmptySig is the only object
        {
            spEmptySig = nullptr;
        }
    }

    RootSignature::SharedPtr RootSignature::getEmpty()
    {
        if (spEmptySig == nullptr)
        {
            spEmptySig = create(Desc());
        }
        return spEmptySig;
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

    static ProgramReflection::ShaderAccess getRequiredShaderAccess(RootSignature::DescType type)
    {
        switch (type)
        {
        case Falcor::RootSignature::DescType::Srv:
        case Falcor::RootSignature::DescType::Cbv:
        case Falcor::RootSignature::DescType::Sampler:
            return ProgramReflection::ShaderAccess::Read;
        case Falcor::RootSignature::DescType::Uav:
            return ProgramReflection::ShaderAccess::ReadWrite;
        default:
            should_not_get_here();
            return ProgramReflection::ShaderAccess(-1);
        }
    }

    uint32_t initializeBufferDescriptors(const ProgramReflection* pReflector, RootSignature::Desc& desc, ProgramReflection::BufferReflection::Type bufferType, RootSignature::DescType descType)
    {
        uint32_t cost = 0;
        const auto& bufMap = pReflector->getBufferMap(bufferType);
        for (const auto& buf : bufMap)
        {
            const ProgramReflection::BufferReflection* pBuffer = buf.second.get();
            if (pBuffer->getShaderAccess() == getRequiredShaderAccess(descType))
            {
                RootSignature::DescriptorSetLayout descTable;
                descTable.addRange(descType, pBuffer->getRegisterIndex(), 1, pBuffer->getRegisterSpace());
                cost += 1;
                desc.addDescriptorSet(descTable);
            }
        }
        return cost;
    }

    RootSignature::SharedPtr RootSignature::create(const ProgramReflection* pReflector)
    {
        uint32_t cost = 0;
        RootSignature::Desc d;

        cost += initializeBufferDescriptors(pReflector, d, ProgramReflection::BufferReflection::Type::Constant, RootSignature::DescType::Cbv);
        cost += initializeBufferDescriptors(pReflector, d, ProgramReflection::BufferReflection::Type::Structured, RootSignature::DescType::Srv);
        cost += initializeBufferDescriptors(pReflector, d, ProgramReflection::BufferReflection::Type::Structured, RootSignature::DescType::Uav);

        const ProgramReflection::ResourceMap& resMap = pReflector->getResourceMap();
        for (auto& resIt : resMap)
        {
            const ProgramReflection::Resource& resource = resIt.second;
            RootSignature::DescType descType;
            if (resource.type == ProgramReflection::Resource::ResourceType::Sampler)
            {
                descType = RootSignature::DescType::Sampler;
            }
            else
            {
                switch (resource.shaderAccess)
                {
                case ProgramReflection::ShaderAccess::ReadWrite:
                    descType = RootSignature::DescType::Uav;
                    break;
                case ProgramReflection::ShaderAccess::Read:
                    descType = RootSignature::DescType::Srv;
                    break;
                default:
                    should_not_get_here();
                }
            }

            uint32_t count = resource.arraySize ? resource.arraySize : 1;
            RootSignature::DescriptorSetLayout descTable;
            descTable.addRange(descType, resource.regIndex, count, resource.registerSpace);
            d.addDescriptorSet(descTable);
            cost += 1;
        }

        if (cost > 64)
        {
            logError("RootSignature::create(): The required storage cost is " + std::to_string(cost) + " DWORDS, which is larger then the max allowed cost of 64 DWORDS");
            return nullptr;
        }
        return (cost != 0) ? RootSignature::create(d) : RootSignature::getEmpty();
    }
}