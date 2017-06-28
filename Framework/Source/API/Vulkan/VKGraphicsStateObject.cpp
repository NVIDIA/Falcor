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

    bool GraphicsStateObject::apiInit()
    {
        // Vertex Input State
        VertexInputStateCreateInfo vertexInputInfo;
        initVkVertexLayoutInfo(mDesc.getVertexLayout().get(), vertexInputInfo);

        // Input Assembly State
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        initVkInputAssemblyInfo(mDesc.getVao().get(), inputAssemblyInfo);

        // Tessellation State
        VkPipelineTessellationStateCreateInfo tessellationInfo = {};

        // Viewport State
        // Viewport and Scissors will be dynamic, but the count is still described here in the info struct
        VkPipelineViewportStateCreateInfo viewportStateInfo = {};
        viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateInfo.viewportCount = getMaxViewportCount();
        viewportStateInfo.scissorCount = getMaxViewportCount();

        // Rasterizerization State
        VkPipelineRasterizationStateCreateInfo rasterizerInfo;
        initVkRasterizerInfo(mDesc.getRasterizerState().get(), rasterizerInfo);

        // Multisample State
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        initVkMultiSampleInfo(mDesc.getBlendState().get(), mDesc.getFboDesc(), mDesc.getSampleMask(), multisampleInfo);

        // Depth Stencil State
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        initVkDepthStencilInfo(mDesc.getDepthStencilState().get(), depthStencilInfo);

        // Color Blend State
        BlendStateCreateInfo blendInfo;
        initVkBlendInfo(mDesc.getBlendState().get(), blendInfo);

        // Dynamic State
        VkPipelineDynamicStateCreateInfo dynamicInfo = {};
        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicInfo.dynamicStateCount = arraysize(dynamicStates);
        dynamicInfo.pDynamicStates = dynamicStates;

        // Render Pass
        RenderPassCreateInfo renderPassInfo;
        VkRenderPass renderPass;
        initVkRenderPassInfo(mDesc.getFboDesc(), mDesc.getDepthStencilState().get(), renderPassInfo);
        vkCreateRenderPass(gpDevice->getApiHandle(), &renderPassInfo.info, nullptr, &renderPass);

        // #VKTODO get this from somewhere
        VkPipelineLayout pipelineLayout = {};

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo.info;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineCreateInfo.pTessellationState = &tessellationInfo;
        pipelineCreateInfo.pViewportState = &viewportStateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
        pipelineCreateInfo.pColorBlendState = &blendInfo.info;
        pipelineCreateInfo.pDynamicState = &dynamicInfo;
        pipelineCreateInfo.renderPass = renderPass;

        if (VK_FAILED(vkCreateGraphicsPipelines(gpDevice->getApiHandle(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mApiHandle)))
        {
            logError("Could not create graphics pipeline.");
            return false;
        }

        return true;
    }
}