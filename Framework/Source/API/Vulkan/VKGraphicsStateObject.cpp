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
#include "API/Vulkan/FalcorVK.h"

namespace Falcor
{
    bool setViewportState(VkPipelineViewportStateCreateInfo& vpState, GraphicsStateObject::Desc& desc)
    {
        vpState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vpState.viewportCount = 1;
        vpState.scissorCount  = 1;

        return true;
    }

    bool setDepthStencilState(VkPipelineDepthStencilStateCreateInfo& dsState, GraphicsStateObject::Desc& desc)
    {
        dsState.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        dsState.depthTestEnable  = VK_TRUE;
        dsState.depthWriteEnable = VK_TRUE;

        return true;
    }

    // VKTODO: add later
    bool setDynamicStates()
    {
        return true;
    }

    bool setVertexInputState(VkPipelineVertexInputStateCreateInfo& vertexState, GraphicsStateObject::Desc& desc)
    {
        vertexState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        
        // VKTODO: Fill these when using vertices

        //vertexState.vertexBindingDescriptionCount
        //vertexState.pVertexBindingDescriptions
        //vertexState.vertexAttributeDescriptionCount
        //vertexState.pVertexAttributeDescriptions

        return true;
    }

    bool setShaderStages(VkPipelineShaderStageCreateInfo& shaderStage, GraphicsStateObject::Desc& desc)
    {
        return true;
    }

    bool createPipelineLayout(VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo, VkPipelineLayout& pipelineLayout, GraphicsStateObject::Desc& desc)
    {
        /*pipelineLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext          = nullptr;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

        vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);*/

        if (VK_FAILED(vkCreatePipelineLayout(gpDevice->getApiHandle(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)))
        {
            logError("Could not create Pipeline layout.");
            return false;
        }

        return true;
    }


    bool setRasterizationState(VkPipelineRasterizationStateCreateInfo& rasterState, GraphicsStateObject::Desc& desc)
    {
        rasterState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterState.polygonMode             = [](RasterizerState::FillMode mode) {
                                                    switch (mode)
                                                    {
                                                        case RasterizerState::FillMode::Solid: return VK_POLYGON_MODE_FILL;                                                    
                                                    }
                                                    return VK_POLYGON_MODE_LINE;
                                              }(desc.getRasterizerState()->getFillMode());

        rasterState.cullMode                = [](RasterizerState::CullMode mode) {
                                                    switch (mode)
                                                    {
                                                        case RasterizerState::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
                                                        case RasterizerState::CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
                                                    }
                                                    return VK_CULL_MODE_NONE;
                                              }(desc.getRasterizerState()->getCullMode());

        rasterState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterState.depthClampEnable        = VK_FALSE;
        rasterState.rasterizerDiscardEnable = VK_FALSE;
        rasterState.depthBiasEnable         = VK_FALSE;
        rasterState.lineWidth               = 1.0f;

        return true;
    }

    bool setMultiSampleState(VkPipelineMultisampleStateCreateInfo &msState, GraphicsStateObject::Desc& desc)
    {
        msState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        msState.pSampleMask          = nullptr;

        return true;
    }

    bool setInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState, GraphicsStateObject::Desc& desc)
    {
        inputAssemblyState.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        return true;
    }

    bool createGraphicsPipeline(VkPipeline& graphicsPipeline, GraphicsStateObject::Desc& desc)
    {
        VkPipelineVertexInputStateCreateInfo   vertexInputState;
        VkPipelineRasterizationStateCreateInfo rasterState;
        VkPipelineMultisampleStateCreateInfo   msState;
        VkPipelineViewportStateCreateInfo      vpState;
        VkPipelineDepthStencilStateCreateInfo  dsState;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
        VkPipelineLayoutCreateInfo             pipelineLayoutCreateInfo;
             
        // VKTODO: create these here, or do they belong elsewhere?
        //VkRenderPass                         renderPass;
        //VkPipelineCache                      pipelineCache;
        VkPipelineLayout                       pipelineLayout;

        // Set all states. VKTODO: set all needed!
        setVertexInputState(vertexInputState, desc);
        setRasterizationState(rasterState, desc);
        createPipelineLayout(pipelineLayoutCreateInfo, pipelineLayout, desc);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout              = pipelineLayout;        
        pipelineCreateInfo.pVertexInputState   = &vertexInputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterState;        
        pipelineCreateInfo.pMultisampleState   = &msState;
        pipelineCreateInfo.pViewportState      = &vpState;
        pipelineCreateInfo.pDepthStencilState  = &dsState;
        //pipelineCreateInfo.renderPass        = &renderPass;
        //pipelineCreateInfo.pColorBlendState  = &colorBlendState;
        //pipelineCreateInfo.pDynamicState     = &dynamicState;

   
        /*if (VK_FAILED(vkCreateGraphicsPipelines(gpDevice->getApiHandle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline)))
        {
            logError("Could not create Pipeline.");
            return false;
        } */

        return true;
    }

    bool createRenderPass()
    {
        return true;
    }

    bool GraphicsStateObject::apiInit()
    {
        createGraphicsPipeline(mApiHandle, mDesc);

        return true;
    }
}