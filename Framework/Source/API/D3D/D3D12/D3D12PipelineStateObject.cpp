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
#include "API/PipelineStateObject.h"
#include "API/D3D/D3DState.h"
#include "API/FBO.h"
#include "API/Texture.h"
#include "API/Device.h"

namespace Falcor
{
    bool PipelineStateObject::apiInit()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        if (mDesc.mpProgram)
        {
#define get_shader_handle(_type) mDesc.mpProgram->getShader(_type) ? mDesc.mpProgram->getShader(_type)->getApiHandle() : D3D12_SHADER_BYTECODE{}

            desc.VS = get_shader_handle(ShaderType::Vertex);
            desc.PS = get_shader_handle(ShaderType::Pixel);
            desc.GS = get_shader_handle(ShaderType::Geometry);
            desc.HS = get_shader_handle(ShaderType::Hull);
            desc.DS = get_shader_handle(ShaderType::Domain);
#undef get_shader_handle
        }

        initD3DBlendDesc(mDesc.mpBlendState.get(), desc.BlendState);
        initD3DRasterizerDesc(mDesc.mpRasterizerState.get(), desc.RasterizerState);
        initD3DDepthStencilDesc(mDesc.mpDepthStencilState.get(), desc.DepthStencilState);

        InputElementDescVec inputElements;
        if(mDesc.mpLayout)
        {
            initD3DVertexLayout(mDesc.mpLayout.get(), inputElements);
            desc.InputLayout.NumElements = (uint32_t)inputElements.size();
            desc.InputLayout.pInputElementDescs = inputElements.data();
        }
        desc.SampleMask = mDesc.mSampleMask;
        desc.pRootSignature = mDesc.mpRootSignature ? mDesc.mpRootSignature->getApiHandle() : nullptr;

        uint32_t numRtvs = 0;
        for (uint32_t rt = 0; rt < Fbo::getMaxColorTargetCount(); rt++)
        {
            desc.RTVFormats[rt] = getDxgiFormat(mDesc.mFboDesc.getColorTargetFormat(rt));
            if (desc.RTVFormats[rt] != DXGI_FORMAT_UNKNOWN)
            {
                numRtvs = rt + 1;
            }
        }
        desc.NumRenderTargets = numRtvs;
        desc.DSVFormat = getDxgiFormat(mDesc.mFboDesc.getDepthStencilFormat());
        desc.SampleDesc.Count = mDesc.mFboDesc.getSampleCount();

        desc.PrimitiveTopologyType = getD3DPrimitiveType(mDesc.mPrimType);

        d3d_call(gpDevice->getApiHandle()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mApiHandle)));
        return true;
    }
}