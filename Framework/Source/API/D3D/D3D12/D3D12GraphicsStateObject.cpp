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
#include "API/GraphicsStateObject.h"
#include "API/D3D/D3DState.h"
#include "API/FBO.h"
#include "API/Texture.h"
#include "API/Device.h"

namespace Falcor
{
#if _ENABLE_NVAPI
    struct NvApiPsoExDesc
    {
        ShaderType mShaderType;
        NVAPI_D3D12_PSO_VERTEX_SHADER_DESC mVsExDesc;
        NVAPI_D3D12_PSO_HULL_SHADER_DESC   mHsExDesc;
        NVAPI_D3D12_PSO_DOMAIN_SHADER_DESC mDsExDesc;
        NVAPI_D3D12_PSO_GEOMETRY_SHADER_DESC mGsExDesc;
        std::vector<NV_CUSTOM_SEMANTIC> mCustomSemantics;
    };

    // TODO add these functions for Hs, Ds, Gs
    void createNvApiVsExDesc(NvApiPsoExDesc& ret)
    {
        ret.mShaderType = ShaderType::Vertex;

        auto& desc = ret.mVsExDesc;

        std::memset(&desc, 0, sizeof(desc));

        desc.psoExtension = NV_PSO_VERTEX_SHADER_EXTENSION;
        desc.version = NV_VERTEX_SHADER_PSO_EXTENSION_DESC_VER;
        desc.baseVersion = NV_PSO_EXTENSION_DESC_VER;
        desc.NumCustomSemantics = 2;

        ret.mCustomSemantics.resize(2);

        //desc.pCustomSemantics = (NV_CUSTOM_SEMANTIC *)malloc(2 * sizeof(NV_CUSTOM_SEMANTIC));
        memset(ret.mCustomSemantics.data(), 0, (2 * sizeof(NV_CUSTOM_SEMANTIC)));

        ret.mCustomSemantics[0].version = NV_CUSTOM_SEMANTIC_VERSION;
        ret.mCustomSemantics[0].NVCustomSemanticType = NV_X_RIGHT_SEMANTIC;
        strcpy_s(&(ret.mCustomSemantics[0].NVCustomSemanticNameString[0]), NVAPI_LONG_STRING_MAX, "NV_X_RIGHT");

        ret.mCustomSemantics[1].version = NV_CUSTOM_SEMANTIC_VERSION;
        ret.mCustomSemantics[1].NVCustomSemanticType = NV_VIEWPORT_MASK_SEMANTIC;
        strcpy_s(&(ret.mCustomSemantics[1].NVCustomSemanticNameString[0]), NVAPI_LONG_STRING_MAX, "NV_VIEWPORT_MASK");

        desc.pCustomSemantics = ret.mCustomSemantics.data();

    }

    std::vector<NvApiPsoExDesc> getNvApiPsoDesc(const GraphicsStateObject::Desc& desc)
    {
        std::vector<NvApiPsoExDesc> nvApiPsoExDescs;
        auto ret = NvAPI_Initialize();

        if (ret != NVAPI_OK)
        {
            logError("Failed to initialize NvApi", true);
            return std::vector<NvApiPsoExDesc>();
        }

        if (desc.getSinglePassStereoEnabled())
        {
            nvApiPsoExDescs.push_back(NvApiPsoExDesc());
            createNvApiVsExDesc(nvApiPsoExDescs.back());
        }
        return nvApiPsoExDescs;
    }
    
    GraphicsStateObject::ApiHandle getNvApiPsoHandle(const std::vector<NvApiPsoExDesc>& nvDescVec, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
    {
        const NVAPI_D3D12_PSO_EXTENSION_DESC* ppPSOExtensionsDesc[4];

        for (uint32_t ex = 0; ex < nvDescVec.size(); ex++)
        {
            switch (nvDescVec[ex].mShaderType)
            {
            case ShaderType::Vertex:   ppPSOExtensionsDesc[ex] = &nvDescVec[ex].mVsExDesc; break;
            case ShaderType::Hull:     ppPSOExtensionsDesc[ex] = &nvDescVec[ex].mHsExDesc; break;
            case ShaderType::Domain:   ppPSOExtensionsDesc[ex] = &nvDescVec[ex].mDsExDesc; break;
            case ShaderType::Geometry: ppPSOExtensionsDesc[ex] = &nvDescVec[ex].mGsExDesc; break;
            default: should_not_get_here();
            }
        }
        const NVAPI_D3D12_PSO_EXTENSION_DESC* add = &nvDescVec[0].mVsExDesc;
        GraphicsStateObject::ApiHandle apiHandle;
        auto ret = NvAPI_D3D12_CreateGraphicsPipelineState(gpDevice->getApiHandle(), &desc, 1u, &add, &apiHandle);

        if (ret != NVAPI_OK || apiHandle == nullptr)
        {
            logError("Failed to create a graphics pipeline state object with NVAPI extensions", true);
            return nullptr;
        }

        return apiHandle;
    }

#else
    using NvApiPsoExDesc = uint32_t;
    void createNvApiVsExDesc(NvApiPsoExDesc& ret)    {should_not_get_here();}
    std::vector<NvApiPsoExDesc> getNvApiPsoDesc(const GraphicsStateObject::Desc& desc) { should_not_get_here(); return{0}; }
    GraphicsStateObject::ApiHandle getNvApiPsoHandle(const std::vector<NvApiPsoExDesc>& psoDesc, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) { should_not_get_here(); return nullptr; }
#endif
    
    bool GraphicsStateObject::apiInit()
    {
        bool nvApiRequired = mDesc.mSinglePassStereoEnabled;
        std::vector<NvApiPsoExDesc> nvApiDesc;
        if (nvApiRequired)
        {
            nvApiDesc = getNvApiPsoDesc(mDesc);
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        assert(mDesc.mpProgram);
#define get_shader_handle(_type) mDesc.mpProgram->getShader(_type) ? mDesc.mpProgram->getShader(_type)->getApiHandle() : D3D12_SHADER_BYTECODE{}

        desc.VS = get_shader_handle(ShaderType::Vertex);
        desc.PS = get_shader_handle(ShaderType::Pixel);
        desc.GS = get_shader_handle(ShaderType::Geometry);
        desc.HS = get_shader_handle(ShaderType::Hull);
        desc.DS = get_shader_handle(ShaderType::Domain);
#undef get_shader_handle

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

        if (nvApiRequired)
        {
            mApiHandle = getNvApiPsoHandle(nvApiDesc, desc);
        }
        else
        {
            d3d_call(gpDevice->getApiHandle()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mApiHandle)));
        }
        return true;
    }
}