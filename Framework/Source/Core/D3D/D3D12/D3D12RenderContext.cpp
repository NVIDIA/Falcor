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
#include "Core/RenderContext.h"
#include "D3D12DescriptorHeap.h"
#include "Core/Device.h"
#include "glm/gtc/type_ptr.hpp"
#include "D3D12FencedPool.h"

namespace Falcor
{
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
    }

    void RenderContext::prepareForDrawApi() const
    {

    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        prepareForDraw();
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        prepareForDraw();
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
    }

    void RenderContext::applyViewport(uint32_t index) const
    {
    }

    void RenderContext::applyScissor(uint32_t index) const
    {
    }

    void RenderContext::applyProgramVars()
    {

    }
}
#endif //#ifdef FALCOR_D3D12
