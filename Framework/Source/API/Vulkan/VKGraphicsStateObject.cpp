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
#pragma once
#include "Framework.h"
#include "API/GraphicsStateObject.h"
#include "API/FBO.h"
#include "API/Texture.h"
#include "API/Device.h"
#include "API/Vulkan/VKState.h"

namespace Falcor
{
    bool createGraphicsPipeline(VkPipeline& graphicsPipeline, GraphicsStateObject::Desc& desc)
    {
        VkPipelineVertexInputStateCreateInfo vertexInputInfo;
        std::vector<VkVertexInputBindingDescription> bindingDescs;
        std::vector<VkVertexInputAttributeDescription> attribDescs;
        initVkVertexLayoutInfo(desc.getVertexLayout().get(), bindingDescs, attribDescs, vertexInputInfo);

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;


        VkPipelineTessellationStateCreateInfo tessellationInfo;


        VkPipelineViewportStateCreateInfo viewportInfo;


        VkPipelineRasterizationStateCreateInfo rasterizerInfo;
        initVkRasterizerInfo(desc.getRasterizerState().get(), rasterizerInfo);

        VkPipelineMultisampleStateCreateInfo multisampleInfo;


        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        initVkDepthStencilInfo(desc.getDepthStencilState().get(), depthStencilInfo);

        VkPipelineColorBlendStateCreateInfo blendInfo;
        std::vector<VkPipelineColorBlendAttachmentState> attachmentStates;
        initVkBlendInfo(desc.getBlendState().get(), attachmentStates, blendInfo);

        VkPipelineDynamicStateCreateInfo dynamicInfo;



        // VKTODO: create these here, or do they belong elsewhere?
        //VkRenderPass                         renderPass;
        //VkPipelineCache                      pipelineCache;
        VkPipelineLayout                       pipelineLayout = {};

        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout              = pipelineLayout;
        pipelineCreateInfo.pVertexInputState   = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineCreateInfo.pTessellationState  = &tessellationInfo;
        pipelineCreateInfo.pViewportState      = &viewportInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
        pipelineCreateInfo.pMultisampleState   = &multisampleInfo;
        pipelineCreateInfo.pDepthStencilState  = &depthStencilInfo;
        pipelineCreateInfo.pColorBlendState    = &blendInfo;
        pipelineCreateInfo.pDynamicState       = &dynamicInfo;
        //pipelineCreateInfo.renderPass        = &renderPass;

   
        /*if (VK_FAILED(vkCreateGraphicsPipelines(gpDevice->getApiHandle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline)))
        {
            logError("Could not create Pipeline.");
            return false;
        } */

        return true;
    }

    bool GraphicsStateObject::apiInit()
    {
        createGraphicsPipeline(mApiHandle, mDesc);

        return true;
    }
}