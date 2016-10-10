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
#include "API/RenderContext.h"
#include "API/LowLevel/DescriptorHeap.h"
#include "API/Device.h"
#include "glm/gtc/type_ptr.hpp"
#include "LowLevel/D3D12FencedPool.h"
#include "API/D3D/D3DState.h"

namespace Falcor
{
    // TODO: A lot of the calls to pList->set*() looks very similar to D3D11 calls. We can share some code
	struct RenderContextData
	{
		GraphicsCommandAllocatorPool::SharedPtr pAllocatorPool;
		ID3D12GraphicsCommandListPtr pList;
	};
	
	RenderContext::~RenderContext()
	{
		delete (RenderContextData*)mpApiData;
		mpApiData = nullptr;
	}

    RenderContext::SharedPtr RenderContext::create(uint32_t allocatorsCount)
    {
        SharedPtr pCtx = SharedPtr(new RenderContext());
        RenderContextData* pApiData = new RenderContextData;
		pCtx->mpApiData = pApiData;

        // Create a command allocator
		pApiData->pAllocatorPool = GraphicsCommandAllocatorPool::create(gpDevice->getFrameGpuFence());
		auto pDevice = gpDevice->getApiHandle();
		
		// Create a command list
		if (FAILED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pApiData->pAllocatorPool->getObject(), nullptr, IID_PPV_ARGS(&pApiData->pList))))
		{
			Logger::log(Logger::Level::Error, "Failed to create command list for RenderContext");
			return nullptr;
		}

        pCtx->initCommon(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
        
		return pCtx;
    }

	CommandListHandle RenderContext::getCommandListApiHandle() const
	{
		const RenderContextData* pApiData = (RenderContextData*)mpApiData;
		return pApiData->pList;
	}

	void RenderContext::reset()
	{
		RenderContextData* pApiData = (RenderContextData*)mpApiData;
		// Skip to the next allocator
		ID3D12CommandAllocatorPtr pAllocator = pApiData->pAllocatorPool->getObject();
		d3d_call(pAllocator->Reset());
		d3d_call(pApiData->pList->Reset(pAllocator, nullptr));
	}

    void RenderContext::resourceBarrier(const Texture* pTexture, D3D12_RESOURCE_STATES state)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        if(pTexture->getResourceState() != state)
        {
            D3D12_RESOURCE_BARRIER barrier;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = pTexture->getApiHandle();
            barrier.Transition.StateBefore = pTexture->getResourceState();
            barrier.Transition.StateAfter = state;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            pApiData->pList->ResourceBarrier(1, &barrier);
            pTexture->setResourceState(state);
        }
    }

	void RenderContext::clearFbo(const Fbo* pFbo, const glm::vec4& color, float depth, uint8_t stencil, FboAttachmentType flags)
	{
		RenderContextData* pApiData = (RenderContextData*)mpApiData;
        bool clearDepth = (flags & FboAttachmentType::Depth) != FboAttachmentType::None;
        bool clearColor = (flags & FboAttachmentType::Color) != FboAttachmentType::None;
        bool clearStencil = (flags & FboAttachmentType::Stencil) != FboAttachmentType::None;

        if(clearColor)
        {
            const Texture* pTexture = pFbo->getColorTexture(0).get();
            RtvHandle rtv = pFbo->getRenderTargetView(0);
            resourceBarrier(pTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
            pApiData->pList->ClearRenderTargetView(rtv, glm::value_ptr(color), 0, nullptr);
        }

        if(clearDepth | clearStencil)
        {
            const Texture* pTexture = pFbo->getDepthStencilTexture().get();
            resourceBarrier(pTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            DsvHandle dsv = pFbo->getDepthStencilView();
            pApiData->pList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
        }
	}

    void RenderContext::applyPipelineState() const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        ID3D12PipelineState* pPso = mState.pRenderState ? mState.pRenderState->getApiHandle() : nullptr;
        pApiData->pList->SetPipelineState(pPso);
    }

    void RenderContext::applyVao() const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        D3D12_VERTEX_BUFFER_VIEW vb[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = { };
        D3D12_INDEX_BUFFER_VIEW ib = {};

        const auto pVao = mState.pVao;
        if (pVao)
        {
            // Get the vertex buffers
            for (uint32_t i = 0; i < pVao->getVertexBuffersCount(); i++)
            {
                const Buffer* pVB = pVao->getVertexBuffer(i).get();
                if(pVB)
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
                ib.Format = DXGI_FORMAT_R32_UINT;
            }
        }

        pApiData->pList->IASetVertexBuffers(0, arraysize(vb), vb);
        pApiData->pList->IASetIndexBuffer(&ib);
    }

    void RenderContext::applyFbo()
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        uint32_t colorTargets = Fbo::getMaxColorTargetCount();
        std::vector<RtvHandle> pRTV(colorTargets);
        DsvHandle pDSV;

        if(mState.pFbo)
        {
            for(uint32_t i = 0; i < colorTargets; i++)
            {
                pRTV[i] = mState.pFbo->getRenderTargetView(i);
                auto& pTexture = mState.pFbo->getColorTexture(i);
                if(pTexture)
                {
                    resourceBarrier(pTexture.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
                }
            }

            pDSV = mState.pFbo->getDepthStencilView();
            auto& pTexture = mState.pFbo->getDepthStencilTexture();
            if(pTexture)
            {
                resourceBarrier(pTexture.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
            }
        }

        pApiData->pList->OMSetRenderTargets(colorTargets, pRTV.data(), FALSE, &pDSV);
    }

    void RenderContext::blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, bool useLinearFiltering, FboAttachmentType copyFlags, uint32_t srcIdx, uint32_t dstIdx)
	{
        UNSUPPORTED_IN_D3D12("BlitFbo");
	}

    void RenderContext::applyTopology() const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        D3D_PRIMITIVE_TOPOLOGY topology = getD3DPrimitiveTopology(mState.topology);
        pApiData->pList->IASetPrimitiveTopology(topology);
    }

    void RenderContext::prepareForDrawApi() const
    {
       // Bind the root signature and the root signature data
       // D3D12_CODE what to do if there are no vars?
        if (mState.pProgramVars)
        {
            mState.pProgramVars->setIntoRenderContext(const_cast<RenderContext*>(this));
        }
    }

    void RenderContext::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pList->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        drawInstanced(vertexCount, 1, startVertexLocation, 0);
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pList->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        drawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
    }

    void RenderContext::applyViewport(uint32_t index) const
    {
        static_assert(offsetof(Viewport, originX) == offsetof(D3D12_VIEWPORT, TopLeftX), "VP TopLeftX offset");
        static_assert(offsetof(Viewport, originY) == offsetof(D3D12_VIEWPORT, TopLeftY), "VP TopLeftY offset");
        static_assert(offsetof(Viewport, width) == offsetof(D3D12_VIEWPORT, Width), "VP Width offset");
        static_assert(offsetof(Viewport, height) == offsetof(D3D12_VIEWPORT, Height), "VP Height offset");
        static_assert(offsetof(Viewport, minDepth) == offsetof(D3D12_VIEWPORT, MinDepth), "VP MinDepth offset");
        static_assert(offsetof(Viewport, maxDepth) == offsetof(D3D12_VIEWPORT, MaxDepth), "VP TopLeftX offset");

        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pList->RSSetViewports(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, (D3D12_VIEWPORT*)mState.viewports.data());

        // D3D12 Code: what to do with this? Scissors do not get updated automatically when the VP changes
        D3D12_RECT r;
        r.top = (LONG)mState.viewports[0].originX;
        r.left = (LONG)mState.viewports[0].originY;
        r.bottom = (LONG)mState.viewports[0].height;
        r.right = (LONG)mState.viewports[0].width;

        pApiData->pList->RSSetScissorRects(1, &r);
    }

    void RenderContext::applyScissor(uint32_t index) const
    {
    }

    void RenderContext::applyProgramVars()
    {

    }
}
#endif //#ifdef FALCOR_D3D12
