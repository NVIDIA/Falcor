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
#include "LowLevel/D3D12FencedPool.h"
#include "API/D3D/D3DState.h"

namespace Falcor
{
    // TODO: A lot of the calls to pList->set*() looks very similar to D3D11 calls. We can share some code
	struct RenderContextData
	{
		GraphicsCommandAllocatorPool::SharedPtr pAllocatorPool;
        ID3D12CommandAllocatorPtr pAllocator;
		ID3D12GraphicsCommandListPtr pList;
        ID3D12CommandQueuePtr pCommandQueue;
        GpuFence::SharedPtr pFence;
        CopyContext::SharedPtr pCopyContext;
	};
	
	RenderContext::~RenderContext()
	{
        mpPipelineState = nullptr;
        mpProgramVars = nullptr;
		delete (RenderContextData*)mpApiData;
		mpApiData = nullptr;
	}

    RenderContext::SharedPtr RenderContext::create(uint32_t allocatorsCount)
    {
        SharedPtr pCtx = SharedPtr(new RenderContext());
        RenderContextData* pApiData = new RenderContextData;
        pApiData->pCopyContext = CopyContext::create();
        pApiData->pFence = GpuFence::create();
		pCtx->mpApiData = pApiData;

        // Create a command allocator
		pApiData->pAllocatorPool = GraphicsCommandAllocatorPool::create(pApiData->pFence);
        pApiData->pAllocator = pApiData->pAllocatorPool->newObject();
		auto pDevice = gpDevice->getApiHandle();
		
        // Create the command queue
        D3D12_COMMAND_QUEUE_DESC cqDesc = {};
        cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        if (FAILED(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pApiData->pCommandQueue))))
        {
            Logger::log(Logger::Level::Error, "Failed to create command queue");
            return nullptr;
        }

		// Create a command list
		if (FAILED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pApiData->pAllocator, nullptr, IID_PPV_ARGS(&pApiData->pList))))
		{
			Logger::log(Logger::Level::Error, "Failed to create command list for RenderContext");
			return nullptr;
		}

        pCtx->bindDescriptorHeaps();

        return pCtx;
    }

	CommandListHandle RenderContext::getCommandListApiHandle() const
	{
		const RenderContextData* pApiData = (RenderContextData*)mpApiData;
		return pApiData->pList;
	}

    CommandQueueHandle RenderContext::getCommandQueue() const
    {
        const RenderContextData* pApiData = (RenderContextData*)mpApiData;
        return pApiData->pCommandQueue.GetInterfacePtr();
    }

	void RenderContext::reset()
	{
		RenderContextData* pApiData = (RenderContextData*)mpApiData;
		// Skip to the next allocator        
		pApiData->pAllocator = pApiData->pAllocatorPool->newObject();
        d3d_call(pApiData->pList->Close());
        d3d_call(pApiData->pAllocator->Reset());
		d3d_call(pApiData->pList->Reset(pApiData->pAllocator, nullptr));
        bindDescriptorHeaps();
        pApiData->pCopyContext->reset();
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
            for(uint32_t i = 0 ; i < Fbo::getMaxColorTargetCount() ; i++)
            {
                const Texture* pTexture = pFbo->getColorTexture(i).get();
                if(pTexture)
                {
                    RtvHandle rtv = pFbo->getRenderTargetView(i);
                    resourceBarrier(pTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
                    pApiData->pList->ClearRenderTargetView(rtv, glm::value_ptr(color), 0, nullptr);
                }
            }
        }

        if(clearDepth | clearStencil)
        {
            const Texture* pTexture = pFbo->getDepthStencilTexture().get();
            resourceBarrier(pTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            DsvHandle dsv = pFbo->getDepthStencilView();
            pApiData->pList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
        }
	}

    void RenderContext::blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, bool useLinearFiltering, FboAttachmentType copyFlags, uint32_t srcIdx, uint32_t dstIdx)
	{
        UNSUPPORTED_IN_D3D12("BlitFbo");
	}


    static void D3D12SetVao(const RenderContextData* pApiData, const Vao* pVao)
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

        pApiData->pList->IASetVertexBuffers(0, arraysize(vb), vb);
        pApiData->pList->IASetIndexBuffer(&ib);
    }

    static void D3D12SetFbo(RenderContext* pCtx, const RenderContextData* pApiData, const Fbo* pFbo)
    {
        // FIXME D3D12
        uint32_t colorTargets = Fbo::getMaxColorTargetCount();
        std::vector<RtvHandle> pRTV(colorTargets);
        DsvHandle pDSV;

        if (pFbo)
        {
            for (uint32_t i = 0; i < colorTargets; i++)
            {
                pRTV[i] = pFbo->getRenderTargetView(i);
                auto& pTexture = pFbo->getColorTexture(i);
                if (pTexture)
                {
                    pCtx->resourceBarrier(pTexture.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
                }
            }

            pDSV = pFbo->getDepthStencilView();
            auto& pTexture = pFbo->getDepthStencilTexture();
            if (pTexture)
            {
                pCtx->resourceBarrier(pTexture.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
            }
        }

        pApiData->pList->OMSetRenderTargets(colorTargets, pRTV.data(), FALSE, &pDSV);
    }

    static void D3D12SetViewports(const RenderContextData* pApiData, const PipelineState::Viewport* vp)
    {
        static_assert(offsetof(PipelineState::Viewport, originX) == offsetof(D3D12_VIEWPORT, TopLeftX), "VP originX offset");
        static_assert(offsetof(PipelineState::Viewport, originY) == offsetof(D3D12_VIEWPORT, TopLeftY), "VP originY offset");
        static_assert(offsetof(PipelineState::Viewport, width) == offsetof(D3D12_VIEWPORT, Width), "VP Width offset");
        static_assert(offsetof(PipelineState::Viewport, height) == offsetof(D3D12_VIEWPORT, Height), "VP Height offset");
        static_assert(offsetof(PipelineState::Viewport, minDepth) == offsetof(D3D12_VIEWPORT, MinDepth), "VP MinDepth offset");
        static_assert(offsetof(PipelineState::Viewport, maxDepth) == offsetof(D3D12_VIEWPORT, MaxDepth), "VP TopLeftX offset");

        pApiData->pList->RSSetViewports(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, (D3D12_VIEWPORT*)vp);
    }

    static void D3D12SetScissors(const RenderContextData* pApiData, const PipelineState::Scissor* sc)
    {
        static_assert(offsetof(PipelineState::Scissor, originX) == offsetof(D3D12_RECT, left), "Scissor originX offset");
        static_assert(offsetof(PipelineState::Scissor, originY) == offsetof(D3D12_RECT, top), "Scissor originY offset");
        static_assert(offsetof(PipelineState::Scissor, width) == offsetof(D3D12_RECT, right), "Scissor Width offset");
        static_assert(offsetof(PipelineState::Scissor, height) == offsetof(D3D12_RECT, bottom), "Scissor Height offset");

        pApiData->pList->RSSetScissorRects(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, (D3D12_RECT*)sc);
    }

    void RenderContext::prepareForDrawApi()
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        assert(mpPipelineState);

        // Bind the root signature and the root signature data
        // FIXME D3D12 what to do if there are no vars?
        if (mpProgramVars)
        {
            mpProgramVars->setIntoRenderContext(const_cast<RenderContext*>(this));
        }

        pApiData->pList->IASetPrimitiveTopology(getD3DPrimitiveTopology(mpPipelineState->getVao()->getPrimitiveTopology()));
        D3D12SetVao(pApiData, mpPipelineState->getVao().get());
        D3D12SetFbo(this, pApiData, mpPipelineState->getFbo().get());
        D3D12SetViewports(pApiData, &mpPipelineState->getViewport(0));
        D3D12SetScissors(pApiData, &mpPipelineState->getScissors(0));
        pApiData->pList->SetPipelineState(mpPipelineState->getPSO()->getApiHandle());

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

    void RenderContext::applyProgramVars()
    {

    }

    void RenderContext::applyPipelineState()
    {

    }

    void RenderContext::flush()
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;

        if (pApiData->pCopyContext->isDirty())
        {
            pApiData->pCopyContext->flush(pApiData->pFence.get());
            pApiData->pFence->syncGpu(pApiData->pCommandQueue);
        }

        d3d_call(pApiData->pList->Close());
        ID3D12CommandList* pList = pApiData->pList.GetInterfacePtr();
        pApiData->pCommandQueue->ExecuteCommandLists(1, &pList);
        pApiData->pFence->gpuSignal(pApiData->pCommandQueue);
        pApiData->pList->Reset(pApiData->pAllocator, nullptr);
        bindDescriptorHeaps();
    }

    void RenderContext::waitForCompletion()
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pFence->syncCpu();
    }

    void RenderContext::bindDescriptorHeaps()
    {
        auto& pList = getCommandListApiHandle();
        ID3D12DescriptorHeap* pHeaps[] = { gpDevice->getSamplerDescriptorHeap()->getApiHandle(), gpDevice->getSrvDescriptorHeap()->getApiHandle() };
        pList->SetDescriptorHeaps(arraysize(pHeaps), pHeaps);
    }

    void RenderContext::updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset, size_t size) const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pCopyContext->updateBuffer(pBuffer, pData, offset, size);
    }

    void RenderContext::updateTexture(const Texture* pTexture, const void* pData) const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pCopyContext->updateTexture(pTexture, pData);
    }

    void RenderContext::updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData) const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pCopyContext->updateTextureSubresource(pTexture, subresourceIndex, pData);
    }

    void RenderContext::updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData) const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pCopyContext->updateTextureSubresources(pTexture, firstSubresource, subresourceCount, pData);
    }

    GpuFence::SharedPtr RenderContext::getFence() const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        return pApiData->pFence;
    }
}
