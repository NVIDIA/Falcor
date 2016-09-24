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
#ifdef FALCOR_D3D
#include "D3DState.h"
#include "Core/BlendState.h"
#include "Core/RasterizerState.h"
#include "Core/VertexLayout.h"


namespace Falcor
{
    D3Dx(BLEND) getD3DBlendFunc(BlendState::BlendFunc func)
    {
        switch (func)
        {
        case BlendState::BlendFunc::Zero:
            return D3Dx(BLEND_ZERO);
        case BlendState::BlendFunc::One:
            return D3Dx(BLEND_ONE);
        case BlendState::BlendFunc::SrcColor:
            return D3Dx(BLEND_SRC_COLOR);
        case BlendState::BlendFunc::OneMinusSrcColor:
            return D3Dx(BLEND_INV_SRC_COLOR);
        case BlendState::BlendFunc::DstColor:
            return D3Dx(BLEND_DEST_COLOR);
        case BlendState::BlendFunc::OneMinusDstColor:
            return D3Dx(BLEND_INV_DEST_COLOR);
        case BlendState::BlendFunc::SrcAlpha:
            return D3Dx(BLEND_SRC_ALPHA);
        case BlendState::BlendFunc::OneMinusSrcAlpha:
            return D3Dx(BLEND_INV_SRC_ALPHA);
        case BlendState::BlendFunc::DstAlpha:
            return D3Dx(BLEND_DEST_ALPHA);
        case BlendState::BlendFunc::OneMinusDstAlpha:
            return D3Dx(BLEND_INV_DEST_ALPHA);
        case BlendState::BlendFunc::RgbaFactor:
            return D3Dx(BLEND_BLEND_FACTOR);
        case BlendState::BlendFunc::OneMinusRgbaFactor:
            return D3Dx(BLEND_INV_BLEND_FACTOR);
        case BlendState::BlendFunc::SrcAlphaSaturate:
            return D3Dx(BLEND_SRC_ALPHA_SAT);
        case BlendState::BlendFunc::Src1Color:
            return D3Dx(BLEND_INV_SRC1_COLOR);
        case BlendState::BlendFunc::OneMinusSrc1Color:
            return D3Dx(BLEND_INV_SRC1_COLOR);
        case BlendState::BlendFunc::Src1Alpha:
            return D3Dx(BLEND_SRC1_ALPHA);
        case BlendState::BlendFunc::OneMinusSrc1Alpha:
            return D3Dx(BLEND_INV_SRC1_ALPHA);
        default:
            should_not_get_here();
            return (D3Dx(BLEND))0;
        }

    }

    D3Dx(BLEND_OP) getD3DBlendOp(BlendState::BlendOp op)
    {
        switch (op)
        {
        case BlendState::BlendOp::Add:
            return D3Dx(BLEND_OP_ADD);
        case BlendState::BlendOp::Subtract:
            return D3Dx(BLEND_OP_SUBTRACT);
        case BlendState::BlendOp::ReverseSubtract:
            return D3Dx(BLEND_OP_REV_SUBTRACT);
        case BlendState::BlendOp::Min:
            return D3Dx(BLEND_OP_MIN);
        case BlendState::BlendOp::Max:
            return D3Dx(BLEND_OP_MAX);
        default:
            return (D3Dx(BLEND_OP))0;
        }
    }

    void initD3DBlendDesc(const BlendState* pState, D3Dx(BLEND_DESC)& desc)
    {
        desc.AlphaToCoverageEnable = dxBool(pState->isAlphaToCoverageEnabled());
        desc.IndependentBlendEnable = dxBool(pState->isIndependentBlendEnabled());
        for (size_t rt = 0; rt < pState->getRtCount(); rt++)
        {
            const BlendState::Desc::RenderTargetDesc& rtDesc = pState->getRtDesc(rt);
            D3Dx(RENDER_TARGET_BLEND_DESC)& d3dRtDesc = desc.RenderTarget[rt];

            d3dRtDesc.BlendEnable = dxBool(rtDesc.blendEnabled);
            d3dRtDesc.SrcBlend = getD3DBlendFunc(rtDesc.srcRgbFunc);
            d3dRtDesc.DestBlend = getD3DBlendFunc(rtDesc.dstRgbFunc);
            d3dRtDesc.BlendOp = getD3DBlendOp(rtDesc.rgbBlendOp);
            d3dRtDesc.SrcBlendAlpha = getD3DBlendFunc(rtDesc.srcAlphaFunc);
            d3dRtDesc.DestBlendAlpha = getD3DBlendFunc(rtDesc.dstAlphaFunc);
            d3dRtDesc.BlendOpAlpha = getD3DBlendOp(rtDesc.alphaBlendOp);

            d3dRtDesc.RenderTargetWriteMask = rtDesc.writeMask.writeRed ? D3Dx(COLOR_WRITE_ENABLE_RED) : 0;
            d3dRtDesc.RenderTargetWriteMask |= rtDesc.writeMask.writeGreen ? D3Dx(COLOR_WRITE_ENABLE_GREEN) : 0;
            d3dRtDesc.RenderTargetWriteMask |= rtDesc.writeMask.writeBlue ? D3Dx(COLOR_WRITE_ENABLE_BLUE) : 0;
            d3dRtDesc.RenderTargetWriteMask |= rtDesc.writeMask.writeAlpha ? D3Dx(COLOR_WRITE_ENABLE_ALPHA) : 0;
        }
    }

#ifdef FALCOR_D3D12
#define D3D_FILL_MODE(a) D3D12_FILL_MODE_##a
#elif defined FALCOR_D3D11
#define D3D_FILL_MODE(a) D3D11_FILL_##a
#endif

    D3Dx(FILL_MODE) getD3DFillMode(RasterizerState::FillMode fill)
    {
        switch (fill)
        {
        case RasterizerState::FillMode::Wireframe:
            return D3D_FILL_MODE(WIREFRAME);
        case RasterizerState::FillMode::Solid:
            return D3D_FILL_MODE(SOLID);
        default:
            should_not_get_here();
            return (D3Dx(FILL_MODE))0;
        }
    }

#ifdef FALCOR_D3D12
#define D3D_CULL_MODE(a) D3D12_CULL_MODE_##a
#elif defined FALCOR_D3D11
#define D3D_CULL_MODE(a) D3D11_CULL_##a
#endif

    D3Dx(CULL_MODE) getD3DCullMode(RasterizerState::CullMode cull)
    {
        switch (cull)
        {
        case Falcor::RasterizerState::CullMode::None:
            return D3D_CULL_MODE(NONE);
        case Falcor::RasterizerState::CullMode::Front:
            return D3D_CULL_MODE(FRONT);
        case Falcor::RasterizerState::CullMode::Back:
            return D3D_CULL_MODE(BACK);
        default:
            should_not_get_here();
            return (D3Dx(CULL_MODE))0;
        }
    }

    void initRasterizerDesc(const RasterizerState* pState, D3Dx(RASTERIZER_DESC)& desc)
    {
        desc = {};
        desc.FillMode = getD3DFillMode(pState->getFillMode());
        desc.CullMode = getD3DCullMode(pState->getCullMode());
        desc.FrontCounterClockwise = dxBool(pState->isFrontCounterCW());
        desc.DepthBias = pState->getDepthBias();
        desc.DepthBiasClamp = 0;
        desc.SlopeScaledDepthBias = pState->getSlopeScaledDepthBias();
        desc.DepthClipEnable = dxBool(!pState->isDepthClampEnabled()); // Depth-clamp disables depth-clip
                                                                       // Set the line anti-aliasing mode
        desc.AntialiasedLineEnable = dxBool(pState->isLineAntiAliasingEnabled());
        desc.MultisampleEnable = desc.AntialiasedLineEnable;

#ifdef FALCOR_D3D11
        desc.ScissorEnable = dxBool(pState->isScissorTestEnabled());
#elif defined FALCOR_D3D12
        desc.ConservativeRaster = pState->isConservativeRasterizationEnabled() ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        desc.ForcedSampleCount = pState->getForcedSampleCount();
#endif
    }

#ifdef FALCOR_D3D11
#define INPUT_CLASS(a) D3D11_INPUT_PER_##a##_DATA
#elif defined FALCOR_D3D12
#define INPUT_CLASS(a)  D3D12_INPUT_CLASSIFICATION_PER_##a##_DATA
#endif

    D3Dx(INPUT_CLASSIFICATION) getD3DInputClass(VertexBufferLayout::InputClass inClass)
    {
        switch (inClass)
        {
        case VertexBufferLayout::InputClass::PerVertexData:
            return INPUT_CLASS(VERTEX);
        case VertexBufferLayout::InputClass::PerInstanceData:
            return INPUT_CLASS(INSTANCE);
        default:
            should_not_get_here();
            return (D3Dx(INPUT_CLASSIFICATION)) - 1;
        }
    }

    void initD3DVertexLayout(const VertexLayout* pLayout, InputElementDescVec& elemDesc)
    {
        elemDesc.clear();

        for (size_t vb = 0; vb < pLayout->getLayoutCount(); vb++)
        {
            auto& pVB = pLayout->getBufferLayout(vb);
            if (pVB)
            {
                for (uint32_t elemID = 0; elemID < pVB->getElementCount(); elemID++)
                {
                    D3Dx(INPUT_ELEMENT_DESC) element;
                    element.AlignedByteOffset = pVB->getElementOffset(elemID);
                    element.Format = getDxgiFormat(pVB->getElementFormat(elemID));
                    element.InputSlot = (uint32_t)vb;
                    element.InputSlotClass = getD3DInputClass(pVB->getInputClass());
                    element.InstanceDataStepRate = pVB->getInstanceStepRate();
                    const auto& SemanticName = pVB->getElementName(elemID);

                    for (uint32_t arrayIndex = 0; arrayIndex < pVB->getElementArraySize(elemID); arrayIndex++)
                    {
                        // Reallocating name for each array index simplifies the destructor
                        char* name = new char[SemanticName.size() + 1];
                        memcpy(name, SemanticName.c_str(), SemanticName.size());
                        name[SemanticName.size()] = 0;
                        element.SemanticName = name;
                        element.SemanticIndex = arrayIndex;
                        elemDesc.push_back(element);

                        element.AlignedByteOffset += getFormatBytesPerBlock(pVB->getElementFormat(elemID));
                    }
                }
            }
        }
    }
}
#endif