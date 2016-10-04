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
#ifdef FALCOR_D3D
#include <vector>
#include "API/RenderContext.h"
#include "API/PipelineState.h"
#include "API/LowLevel/RootSignature.h"

namespace Falcor
{
    class BlendState;
    class RasterizerState;
    class DepthStencilState;
    class VertexLayout;

    using InputElementDescVec = std::vector < D3Dx(INPUT_ELEMENT_DESC) >;
    void initD3DBlendDesc(const BlendState* pFalcorDesc, D3Dx(BLEND_DESC)& d3dDesc);
    void initD3DRasterizerDesc(const RasterizerState* pState, D3Dx(RASTERIZER_DESC)& desc);
    void initD3DDepthStencilDesc(const DepthStencilState* pState, D3Dx(DEPTH_STENCIL_DESC)& desc);
    void initD3DVertexLayout(const VertexLayout* pLayout, InputElementDescVec& elemDesc);
    void initD3DSamplerDesc(const Sampler* pSampler, D3Dx(SAMPLER_DESC)& desc);

#ifdef FALCOR_D3D12
    void initD3DSamplerDesc(const Sampler* pSampler, RootSignature::BorderColor borderColor, D3D12_STATIC_SAMPLER_DESC& desc);
#endif

    inline D3D_PRIMITIVE_TOPOLOGY getD3DPrimitiveTopology(RenderContext::Topology topology)
    {
        switch (topology)
        {
        case RenderContext::Topology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case RenderContext::Topology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case RenderContext::Topology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RenderContext::Topology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            should_not_get_here();
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
    }

    inline D3Dx(PRIMITIVE_TOPOLOGY_TYPE) getD3DPrimitiveType(PipelineState::PrimitiveType type)
    {
        switch (type)
        {
        case PipelineState::PrimitiveType::Point:
            return D3Dx(PRIMITIVE_TOPOLOGY_TYPE_POINT);
        case PipelineState::PrimitiveType::Line:
            return D3Dx(PRIMITIVE_TOPOLOGY_TYPE_LINE);
        case PipelineState::PrimitiveType::Triangle:
            return D3Dx(PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        case PipelineState::PrimitiveType::Patch:
            return D3Dx(PRIMITIVE_TOPOLOGY_TYPE_PATCH);
        default:
            should_not_get_here();
            return D3Dx(PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED);
        }
    }
}
#endif //#ifdef FALCOR_D3D
