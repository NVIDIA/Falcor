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
#include "Framework.h"
#ifdef FALCOR_D3D11
#include "Core/DepthStencilState.h"

namespace Falcor
{
    DepthStencilState::~DepthStencilState() = default;

    D3D11_COMPARISON_FUNC getD3DComparisonFunc(DepthStencilState::Func func)
    {
        switch(func)
        {
        case DepthStencilState::Func::Never:
            return D3D11_COMPARISON_NEVER;
        case DepthStencilState::Func::Always:
            return D3D11_COMPARISON_ALWAYS;
        case DepthStencilState::Func::Less:
            return D3D11_COMPARISON_LESS;
        case DepthStencilState::Func::Equal:
            return D3D11_COMPARISON_EQUAL;
        case DepthStencilState::Func::NotEqual:
            return D3D11_COMPARISON_NOT_EQUAL;
        case DepthStencilState::Func::LessEqual:
            return D3D11_COMPARISON_LESS_EQUAL;
        case DepthStencilState::Func::Greater:
            return D3D11_COMPARISON_GREATER;
        case DepthStencilState::Func::GreaterEqual:
            return D3D11_COMPARISON_GREATER_EQUAL;
        default:
            should_not_get_here();
            return (D3D11_COMPARISON_FUNC)0;
        }
    }

    D3D11_STENCIL_OP getD3DStencilOp(DepthStencilState::StencilOp op)
    {
        switch(op)
        {
        case DepthStencilState::StencilOp::Keep:
            return D3D11_STENCIL_OP_KEEP;
        case DepthStencilState::StencilOp::Zero:
            return D3D11_STENCIL_OP_ZERO;
        case DepthStencilState::StencilOp::Replace:
            return D3D11_STENCIL_OP_REPLACE;
        case DepthStencilState::StencilOp::Increase:
            return D3D11_STENCIL_OP_INCR;
        case DepthStencilState::StencilOp::IncreaseSaturate:
            return D3D11_STENCIL_OP_INCR_SAT;
        case DepthStencilState::StencilOp::Decrease:
            return D3D11_STENCIL_OP_DECR;
        case DepthStencilState::StencilOp::DecreaseSaturate:
            return D3D11_STENCIL_OP_DECR_SAT;
        case DepthStencilState::StencilOp::Invert:
            return D3D11_STENCIL_OP_INVERT;
        default:
            should_not_get_here();
            return (D3D11_STENCIL_OP)0;
        }
    }

    D3D11_DEPTH_STENCILOP_DESC getD3DStencilOpDesc(const DepthStencilState::StencilDesc& desc)
    {
        D3D11_DEPTH_STENCILOP_DESC dxDesc;
        dxDesc.StencilFunc = getD3DComparisonFunc(desc.func);
        dxDesc.StencilDepthFailOp = getD3DStencilOp(desc.depthFailOp);
        dxDesc.StencilFailOp = getD3DStencilOp(desc.stencilFailOp);
        dxDesc.StencilPassOp = getD3DStencilOp(desc.depthStencilPassOp);

        return dxDesc;
    }

    DepthStencilState::SharedPtr DepthStencilState::create(const Desc& desc)
    {
        auto pDsState = SharedPtr(new DepthStencilState(desc));
        D3D11_DEPTH_STENCIL_DESC dxDesc;

        dxDesc.DepthEnable = dxBool(desc.mDepthEnabled);
        dxDesc.DepthFunc = getD3DComparisonFunc(desc.mDepthFunc);
        dxDesc.DepthWriteMask = desc.mWriteDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        dxDesc.StencilEnable = dxBool(desc.mStencilEnabled);
        dxDesc.StencilReadMask = desc.mStencilReadMask;
        dxDesc.StencilWriteMask = desc.mStencilWriteMask;
        dxDesc.FrontFace = getD3DStencilOpDesc(desc.mStencilFront);
        dxDesc.BackFace = getD3DStencilOpDesc(desc.mStencilBack);

        d3d11_call(getD3D11Device()->CreateDepthStencilState(&dxDesc, &pDsState->mApiHandle));

        return pDsState;
    }

    DepthStencilStateHandle DepthStencilState::getApiHandle() const
    {
        return mApiHandle;
    }
}
#endif //#ifdef FALCOR_D3D11
