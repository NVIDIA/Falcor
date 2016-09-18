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
#ifdef FALCOR_D3D12
#include "Framework.h"
#include "Core/BlendState.h"

namespace Falcor
{
    BlendState::~BlendState() = default;
    
    D3D12_BLEND getD3DBlendFunc(BlendState::BlendFunc func)
    {
        switch(func)
        {
        case BlendState::BlendFunc::Zero:
            return D3D12_BLEND_ZERO;
        case BlendState::BlendFunc::One:
            return D3D12_BLEND_ONE;
        case BlendState::BlendFunc::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case BlendState::BlendFunc::OneMinusSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case BlendState::BlendFunc::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case BlendState::BlendFunc::OneMinusDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case BlendState::BlendFunc::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case BlendState::BlendFunc::OneMinusSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendState::BlendFunc::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case BlendState::BlendFunc::OneMinusDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case BlendState::BlendFunc::RgbaFactor:
            return D3D12_BLEND_BLEND_FACTOR;
        case BlendState::BlendFunc::OneMinusRgbaFactor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case BlendState::BlendFunc::SrcAlphaSaturate:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case BlendState::BlendFunc::Src1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case BlendState::BlendFunc::OneMinusSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case BlendState::BlendFunc::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case BlendState::BlendFunc::OneMinusSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            should_not_get_here();
            return (D3D12_BLEND)0;
        }

    }

    D3D12_BLEND_OP getD3DBlendOp(BlendState::BlendOp op)
    {
        switch(op)
        {
        case BlendState::BlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case BlendState::BlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case BlendState::BlendOp::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendState::BlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case BlendState::BlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return (D3D12_BLEND_OP)0;
        }
    }

    BlendState::SharedPtr BlendState::create(const Desc& desc)
    {
        return nullptr;
    }

    BlendStateHandle BlendState::getApiHandle() const
    {
        return mApiHandle;
    }
}
#endif //#ifdef FALCOR_D3D12
