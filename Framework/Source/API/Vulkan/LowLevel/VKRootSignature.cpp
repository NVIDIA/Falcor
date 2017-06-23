/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
#include "Framework.h"
#include "API/LowLevel/RootSignature.h"
#include "API/Device.h"

namespace Falcor
{
    VkShaderStageFlags getShaderVisibility(ShaderVisibility visibility)
    {
        VkShaderStageFlags flags = 0;

        if ((visibility & ShaderVisibility::Vertex) != ShaderVisibility::None)
        {
            flags |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if ((visibility & ShaderVisibility::Pixel) != ShaderVisibility::None)
        {
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if ((visibility & ShaderVisibility::Geometry) != ShaderVisibility::None)
        {
            flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        if ((visibility & ShaderVisibility::Domain) != ShaderVisibility::None)
        {
            flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;;
        }
        if ((visibility & ShaderVisibility::Hull) != ShaderVisibility::None)
        {
            flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }

        return flags;
    }

    //void createSamplerSetLayout(const std::vector<RootSignature::SamplerDesc>& falcorDesc, VkDescriptorSetLayout& setLayoutOut)
    //{
    //    // #VKTODO Fill in when descriptors and sampler creation is implemented
    //}

    //void convertRootConstant(const RootSignature::ConstantDesc& falcorDesc, VkPushConstantRange& desc)
    //{
    //    desc.stageFlags = getShaderVisibility(falcorDesc.visibility);
    //    desc.offset = 0;
    //    desc.size = 4;
    //}

    VkDescriptorType getDescriptorType(RootSignature::DescType type)
    {
        //switch (type)
        //{
        //case RootSignature::DescType::CBV:
        //    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // #VKTODO what about BUFFER_DYNAMIC?
        //case RootSignature::DescType::SRV:
        //    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        //case RootSignature::DescType::UAV:
        //    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        //default:
        //    should_not_get_here();
        //    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        //}

        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    //void convertRootDescriptor(const RootSignature::DescriptorDesc& falcorDesc, VkDescriptorSetLayout& setLayoutOut)
    //{
    //    VkDescriptorSetLayoutBinding info = {};
    //    info.descriptorType = getDescriptorType(falcorDesc.type);
    //    info.stageFlags = getShaderVisibility(falcorDesc.visibility);
    //    info.binding = falcorDesc.regIndex;
    //    info.descriptorCount = 1;
    //    info.pImmutableSamplers = nullptr;

    //    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    //    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //    setLayoutInfo.bindingCount = 1;
    //    setLayoutInfo.pBindings = &info;

    //    vkCreateDescriptorSetLayout(gpDevice->getApiHandle(), &setLayoutInfo, nullptr, &setLayoutOut);
    //}

    //void convertDescriptorTable(const RootSignature::DescriptorTable& falcorTable, VkDescriptorSetLayout& setLayoutOut)
    //{
    //    std::vector<VkDescriptorSetLayoutBinding> bindingsInfo(falcorTable.getRangeCount());

    //    for (size_t i = 0; i < falcorTable.getRangeCount(); i++)
    //    {
    //        const auto& falcorRange = falcorTable.getRange(i);

    //        VkDescriptorSetLayoutBinding& info = bindingsInfo[i];
    //        info.descriptorType = getDescriptorType(falcorRange.type);
    //        info.stageFlags = getShaderVisibility(falcorTable.getVisibility());
    //        info.binding = falcorRange.firstRegIndex;
    //        info.descriptorCount = falcorRange.descCount;
    //        info.pImmutableSamplers = nullptr;
    //    }

    //    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    //    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //    setLayoutInfo.bindingCount = (uint32_t)bindingsInfo.size();
    //    setLayoutInfo.pBindings = bindingsInfo.data();

    //    vkCreateDescriptorSetLayout(gpDevice->getApiHandle(), &setLayoutInfo, nullptr, &setLayoutOut);
    //}

    bool RootSignature::apiInit()
    {
        //// Push Constants
        //std::vector<VkPushConstantRange> rootConstants(mDesc.mConstants.size());
        //for (size_t i = 0; i < mDesc.mConstants.size(); i++)
        //{
        //    convertRootConstant(mDesc.mConstants[i], rootConstants[i]);
        //}

        //uint32_t elementIndex = 0;
        //std::vector<VkDescriptorSetLayout> rootParams(mDesc.mRootDescriptors.size() + mDesc.mDescriptorTables.size() + mDesc.mSamplers.size());

        //// Samplers
        //createSamplerSetLayout(mDesc.mSamplers, rootParams[elementIndex++]);

        //// Root descriptors
        //for (size_t i = 0; i < mDesc.mRootDescriptors.size(); i++, elementIndex++)
        //{
        //    convertRootDescriptor(mDesc.mRootDescriptors[i], rootParams[i]);
        //}

        //// Desc Table
        //for (size_t i = 0; i < mDesc.mDescriptorTables.size(); i++, elementIndex++)
        //{
        //    convertDescriptorTable(mDesc.mDescriptorTables[i], rootParams[elementIndex]);
        //}

        //VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        //pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        //pipelineLayoutInfo.setLayoutCount = (uint32_t)rootParams.size();
        //pipelineLayoutInfo.pSetLayouts = rootParams.data();
        //pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)rootConstants.size();
        //pipelineLayoutInfo.pPushConstantRanges = rootConstants.data();

        //vkCreatePipelineLayout(gpDevice->getApiHandle(), &pipelineLayoutInfo, nullptr, &mApiHandle);

        return true;
    }
}
