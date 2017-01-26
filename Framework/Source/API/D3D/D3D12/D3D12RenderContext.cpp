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
#include "API/RenderContext.h"
#include "API/LowLevel/DescriptorHeap.h"
#include "API/Device.h"
#include "glm/gtc/type_ptr.hpp"
#include "D3D12Resource.h"
#include "API/D3D/D3DState.h"

namespace Falcor
{
    RenderContext::~RenderContext() = default;
    
    RenderContext::SharedPtr RenderContext::create()
    {
        SharedPtr pCtx = SharedPtr(new RenderContext());
        pCtx->mpLowLevelData = LowLevelContextData::create(LowLevelContextData::CommandListType::Direct);
        if (pCtx->mpLowLevelData == nullptr)
        {
            return nullptr;
        }

        pCtx->bindDescriptorHeaps();
        return pCtx;
    }
    
    void RenderContext::clearFbo(const Fbo* pFbo, const glm::vec4& color, float depth, uint8_t stencil, FboAttachmentType flags)
	{
        bool clearDepth = (flags & FboAttachmentType::Depth) != FboAttachmentType::None;
        bool clearColor = (flags & FboAttachmentType::Color) != FboAttachmentType::None;
        bool clearStencil = (flags & FboAttachmentType::Stencil) != FboAttachmentType::None;

        if(clearColor)
        {
            for(uint32_t i = 0 ; i < Fbo::getMaxColorTargetCount() ; i++)
            {
                if(pFbo->getColorTexture(i))
                {
                    clearRtv(pFbo->getRenderTargetView(i).get(), color);
                }
            }
        }

        if(clearDepth | clearStencil)
        {
            clearDsv(pFbo->getDepthStencilView().get(), depth, stencil, clearDepth, clearStencil);
        }
	}

    void RenderContext::clearRtv(const RenderTargetView* pRtv, const glm::vec4& color)
    {
        resourceBarrier(pRtv->getResource(), Resource::State::RenderTarget);
        mpLowLevelData->getCommandList()->ClearRenderTargetView(pRtv->getApiHandle()->getCpuHandle(), glm::value_ptr(color), 0, nullptr);
        mCommandsPending = true;
    }

    void RenderContext::clearDsv(const DepthStencilView* pDsv, float depth, uint8_t stencil, bool clearDepth, bool clearStencil)
    {
        uint32_t flags = clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0;
        flags |= clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0;

        resourceBarrier(pDsv->getResource(), Resource::State::DepthStencil);
        mpLowLevelData->getCommandList()->ClearDepthStencilView(pDsv->getApiHandle()->getCpuHandle(), D3D12_CLEAR_FLAGS(flags), depth, stencil, 0, nullptr);
        mCommandsPending = true;
    }

    void RenderContext::blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, bool useLinearFiltering, FboAttachmentType copyFlags, uint32_t srcIdx, uint32_t dstIdx)
	{
        UNSUPPORTED_IN_D3D12("BlitFbo");
	}


    static void D3D12SetVao(CommandListHandle pList, const Vao* pVao)
    {
        D3D12_VERTEX_BUFFER_VIEW vb[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
        D3D12_INDEX_BUFFER_VIEW ib = {};

        if (pVao)
        {
            // Get the vertex buffers
            for (uint32_t i = 0; i < pVao->getVertexBuffersCount(); i++)
            {
                const Buffer* pVB = pVao->getVertexBuffer(i).get();
                if (pVB)
                {
                    vb[i].BufferLocation = pVB->getGpuAddress();
                    vb[i].SizeInBytes = (uint32_t)pVB->getSize();
                    vb[i].StrideInBytes = pVao->getVertexLayout()->getBufferLayout(i)->getStride();
                }
            }

            const Buffer* pIB = pVao->getIndexBuffer().get();
            if (pIB)
            {
                ib.BufferLocation = pIB->getGpuAddress();
                ib.SizeInBytes = (uint32_t)pIB->getSize();
                ib.Format = getDxgiFormat(pVao->getIndexBufferFormat());
            }
        }

        pList->IASetVertexBuffers(0, arraysize(vb), vb);
        pList->IASetIndexBuffer(&ib);
    }

    static void D3D12SetFbo(RenderContext* pCtx, const Fbo* pFbo)
    {
        // We are setting the entire RTV array to make sure everything that was previously bound is detached
        uint32_t colorTargets = Fbo::getMaxColorTargetCount();
        std::vector<DescriptorHeap::CpuHandle> pRTV(colorTargets, RenderTargetView::getNullView()->getApiHandle()->getCpuHandle());
        DescriptorHeap::CpuHandle pDSV = DepthStencilView::getNullView()->getApiHandle()->getCpuHandle();

        if (pFbo)
        {
            for (uint32_t i = 0; i < colorTargets; i++)
            {
                auto& pTexture = pFbo->getColorTexture(i);
                if (pTexture)
                {
                    pRTV[i] = pFbo->getRenderTargetView(i)->getApiHandle()->getCpuHandle();
                    pCtx->resourceBarrier(pTexture.get(), Resource::State::RenderTarget);
                }
            }

            auto& pTexture = pFbo->getDepthStencilTexture();
            if(pTexture)
            {
                pDSV = pFbo->getDepthStencilView()->getApiHandle()->getCpuHandle();
                if (pTexture)
                {
                    pCtx->resourceBarrier(pTexture.get(), Resource::State::DepthStencil);
                }
            }
        }

        pCtx->getLowLevelData()->getCommandList()->OMSetRenderTargets(colorTargets, pRTV.data(), FALSE, &pDSV);
    }

    static void D3D12SetViewports(CommandListHandle pList, const GraphicsState::Viewport* vp)
    {
        static_assert(offsetof(GraphicsState::Viewport, originX) == offsetof(D3D12_VIEWPORT, TopLeftX), "VP originX offset");
        static_assert(offsetof(GraphicsState::Viewport, originY) == offsetof(D3D12_VIEWPORT, TopLeftY), "VP originY offset");
        static_assert(offsetof(GraphicsState::Viewport, width) == offsetof(D3D12_VIEWPORT, Width), "VP Width offset");
        static_assert(offsetof(GraphicsState::Viewport, height) == offsetof(D3D12_VIEWPORT, Height), "VP Height offset");
        static_assert(offsetof(GraphicsState::Viewport, minDepth) == offsetof(D3D12_VIEWPORT, MinDepth), "VP MinDepth offset");
        static_assert(offsetof(GraphicsState::Viewport, maxDepth) == offsetof(D3D12_VIEWPORT, MaxDepth), "VP TopLeftX offset");

        pList->RSSetViewports(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, (D3D12_VIEWPORT*)vp);
    }

    static void D3D12SetScissors(CommandListHandle pList, const GraphicsState::Scissor* sc)
    {
        std::vector<D3D12_RECT> scRects;
        scRects.resize(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);

        for (uint32_t i = 0u; i < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
        {
            scRects[i].left = sc[i].originX;
            scRects[i].top = sc[i].originY;
            scRects[i].right = sc[i].originX + sc[i].width;
            scRects[i].bottom = sc[i].originY + sc[i].height;
        }

        pList->RSSetScissorRects(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, scRects.data());
    }

    void RenderContext::prepareForDraw()
    {
        assert(mpGraphicsState);

        // Bind the root signature and the root signature data
        if (mpGraphicsVars)
        {
            mpGraphicsVars->apply(const_cast<RenderContext*>(this));
        }
        else
        {
            mpLowLevelData->getCommandList()->SetGraphicsRootSignature(RootSignature::getEmpty()->getApiHandle());
        }

        CommandListHandle pList = mpLowLevelData->getCommandList();
        pList->IASetPrimitiveTopology(getD3DPrimitiveTopology(mpGraphicsState->getVao()->getPrimitiveTopology()));
        D3D12SetVao(pList, mpGraphicsState->getVao().get());
        D3D12SetFbo(this, mpGraphicsState->getFbo().get());
        D3D12SetViewports(pList, &mpGraphicsState->getViewport(0));
        D3D12SetScissors(pList, &mpGraphicsState->getScissors(0));
        pList->SetPipelineState(mpGraphicsState->getGSO()->getApiHandle());
        mCommandsPending = true;
    }

    void RenderContext::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        mpLowLevelData->getCommandList()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        drawInstanced(vertexCount, 1, startVertexLocation, 0);
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        mpLowLevelData->getCommandList()->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        drawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
    }

    void RenderContext::applyProgramVars() {}
    void RenderContext::applyGraphicsState() {}
}
