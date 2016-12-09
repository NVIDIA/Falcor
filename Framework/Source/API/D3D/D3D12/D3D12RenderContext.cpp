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
#include "D3D12Resource.h"
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
        bool commandsPending = false;
	};
	
	RenderContext::~RenderContext()
	{
		delete (RenderContextData*)mpApiData;
		mpApiData = nullptr;
	}

    void flushCopyCommands(const RenderContextData* pApiData)
    {
        if (pApiData->pCopyContext->hasPendingCommands())
        {
            assert(pApiData->commandsPending == false);
            pApiData->pCopyContext->flush(pApiData->pFence.get());
            pApiData->pFence->syncGpu(pApiData->pCommandQueue);
        }
    }

    void flushGraphicsCommands(RenderContext* pContext, const RenderContextData* pApiData)
    {
        if (pApiData->commandsPending)
        {
            assert(pApiData->pCopyContext->hasPendingCommands() == false);
            pContext->flush();
            pApiData->pFence->syncGpu(pApiData->pCopyContext->getCommandQueue().GetInterfacePtr());
        }
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
        assert(pApiData->commandsPending == false);
		// Skip to the next allocator        
		pApiData->pAllocator = pApiData->pAllocatorPool->newObject();
        d3d_call(pApiData->pList->Close());
        d3d_call(pApiData->pAllocator->Reset());
		d3d_call(pApiData->pList->Reset(pApiData->pAllocator, nullptr));
        bindDescriptorHeaps();
        pApiData->pCopyContext->reset();
	}

    void RenderContext::resourceBarrier(const Resource* pResource, Resource::State newState)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        if(pResource->getState() != newState)
        {
            D3D12_RESOURCE_BARRIER barrier;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = pResource->getApiHandle();
            barrier.Transition.StateBefore =  getD3D12ResourceState(pResource->getState());
            barrier.Transition.StateAfter = getD3D12ResourceState(newState);
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;   // OPTME: Need to do that only for the subresources we will actually use

            pApiData->pList->ResourceBarrier(1, &barrier);
            pResource->mState = newState;
        }
    }

    template<typename ClearType>
    void clearUavCommon(RenderContext* pContext, const UnorderedAccessView* pUav, const ClearType& clear, void* pData)
    {
        pContext->resourceBarrier(pUav->getResource(), Resource::State::UnorderedAccess);
        RenderContextData* pApiData = (RenderContextData*)pData;
        UavHandle clearHandle = pUav->getHandleForClear();
        UavHandle uav = pUav->getApiHandle();        
        if (typeid(ClearType) == typeid(vec4))
        {
            pApiData->pList->ClearUnorderedAccessViewFloat(uav->getGpuHandle(), clearHandle->getCpuHandle(), pUav->getResource()->getApiHandle(), (float*)value_ptr(clear), 0, nullptr);
        }
        else if(typeid(ClearType) == typeid(uvec4))
        {
            pApiData->pList->ClearUnorderedAccessViewUint(uav->getGpuHandle(), clearHandle->getCpuHandle(), pUav->getResource()->getApiHandle(), (uint32_t*)value_ptr(clear), 0, nullptr);
        }
        else
        {
            should_not_get_here();
        }
    }

    void RenderContext::clearUAV(const UnorderedAccessView* pUav, const vec4& value)
    {
        clearUavCommon(this, pUav, value, mpApiData);
    }

    void RenderContext::clearUAV(const UnorderedAccessView* pUav, const uvec4& value)
    {
        clearUavCommon(this, pUav, value, mpApiData);
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
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        resourceBarrier(pRtv->getResource(), Resource::State::RenderTarget);
        pApiData->pList->ClearRenderTargetView(pRtv->getApiHandle()->getCpuHandle(), glm::value_ptr(color), 0, nullptr);
    }

    void RenderContext::clearDsv(const DepthStencilView* pDsv, float depth, uint8_t stencil, bool clearDepth, bool clearStencil)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        uint32_t flags = clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0;
        flags |= clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0;

        resourceBarrier(pDsv->getResource(), Resource::State::DepthStencil);
        pApiData->pList->ClearDepthStencilView(pDsv->getApiHandle()->getCpuHandle(), D3D12_CLEAR_FLAGS(flags), depth, stencil, 0, nullptr);
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

        pApiData->pList->OMSetRenderTargets(colorTargets, pRTV.data(), FALSE, &pDSV);
    }

    static void D3D12SetViewports(const RenderContextData* pApiData, const GraphicsState::Viewport* vp)
    {
        static_assert(offsetof(GraphicsState::Viewport, originX) == offsetof(D3D12_VIEWPORT, TopLeftX), "VP originX offset");
        static_assert(offsetof(GraphicsState::Viewport, originY) == offsetof(D3D12_VIEWPORT, TopLeftY), "VP originY offset");
        static_assert(offsetof(GraphicsState::Viewport, width) == offsetof(D3D12_VIEWPORT, Width), "VP Width offset");
        static_assert(offsetof(GraphicsState::Viewport, height) == offsetof(D3D12_VIEWPORT, Height), "VP Height offset");
        static_assert(offsetof(GraphicsState::Viewport, minDepth) == offsetof(D3D12_VIEWPORT, MinDepth), "VP MinDepth offset");
        static_assert(offsetof(GraphicsState::Viewport, maxDepth) == offsetof(D3D12_VIEWPORT, MaxDepth), "VP TopLeftX offset");

        pApiData->pList->RSSetViewports(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, (D3D12_VIEWPORT*)vp);
    }

    static void D3D12SetScissors(const RenderContextData* pApiData, const GraphicsState::Scissor* sc)
    {
        static_assert(offsetof(GraphicsState::Scissor, originX) == offsetof(D3D12_RECT, left), "Scissor originX offset");
        static_assert(offsetof(GraphicsState::Scissor, originY) == offsetof(D3D12_RECT, top), "Scissor originY offset");
        static_assert(offsetof(GraphicsState::Scissor, width) == offsetof(D3D12_RECT, right), "Scissor Width offset");
        static_assert(offsetof(GraphicsState::Scissor, height) == offsetof(D3D12_RECT, bottom), "Scissor Height offset");

        pApiData->pList->RSSetScissorRects(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, (D3D12_RECT*)sc);
    }

    void RenderContext::prepareForDraw()
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        assert(mpGraphicsState);

        // Bind the root signature and the root signature data
        // FIXME D3D12 what to do if there are no vars?
        if (mpGraphicsVars)
        {
            mpGraphicsVars->apply(const_cast<RenderContext*>(this));
        }

        pApiData->pList->IASetPrimitiveTopology(getD3DPrimitiveTopology(mpGraphicsState->getVao()->getPrimitiveTopology()));
        D3D12SetVao(pApiData, mpGraphicsState->getVao().get());
        D3D12SetFbo(this, pApiData, mpGraphicsState->getFbo().get());
        D3D12SetViewports(pApiData, &mpGraphicsState->getViewport(0));
        D3D12SetScissors(pApiData, &mpGraphicsState->getScissors(0));
        pApiData->pList->SetPipelineState(mpGraphicsState->getGSO()->getApiHandle());

        flushCopyCommands(pApiData);
    }

    void RenderContext::prepareForDispatch()
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        assert(mpComputeState);

        // Bind the root signature and the root signature data
        // FIXME D3D12 what to do if there are no vars?
        if (mpComputeVars)
        {
            mpComputeVars->apply(const_cast<RenderContext*>(this));
        }

        pApiData->pList->SetPipelineState(mpComputeState->getCSO()->getApiHandle());

        flushCopyCommands(pApiData);
    }

    void RenderContext::dispatch(uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ)
    {
        prepareForDispatch();
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pList->Dispatch(groupSizeX, groupSizeY, groupSizeZ);
        pApiData->commandsPending = true;
    }

    void RenderContext::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        pApiData->pList->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
        pApiData->commandsPending = true;
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
        pApiData->commandsPending = true;
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        drawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
    }

    void RenderContext::applyProgramVars() {}
    void RenderContext::applyGraphicsState() {}
    void RenderContext::applyComputeVars() {}
    void RenderContext::applyComputeState() {}

    void RenderContext::flush(bool wait)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        flushCopyCommands(pApiData);

        d3d_call(pApiData->pList->Close());
        ID3D12CommandList* pList = pApiData->pList.GetInterfacePtr();
        pApiData->pCommandQueue->ExecuteCommandLists(1, &pList);
        pApiData->pFence->gpuSignal(pApiData->pCommandQueue);
        pApiData->pList->Reset(pApiData->pAllocator, nullptr);
        bindDescriptorHeaps();
        pApiData->commandsPending = false;

        if(wait)
        {
            pApiData->pFence->syncCpu();
        }
    }

    void RenderContext::bindDescriptorHeaps()
    {
        auto& pList = getCommandListApiHandle();
        ID3D12DescriptorHeap* pHeaps[] = { gpDevice->getSamplerDescriptorHeap()->getApiHandle(), gpDevice->getSrvDescriptorHeap()->getApiHandle() };
        pList->SetDescriptorHeaps(arraysize(pHeaps), pHeaps);
    }

    void RenderContext::updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset, size_t size)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        flushGraphicsCommands(this, pApiData);
        pApiData->pCopyContext->updateBuffer(pBuffer, pData, offset, size);
    }

    void RenderContext::updateTexture(const Texture* pTexture, const void* pData)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        flushGraphicsCommands(this, pApiData);
        pApiData->pCopyContext->updateTexture(pTexture, pData);
    }

    void RenderContext::updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        flushGraphicsCommands(this, pApiData);
        pApiData->pCopyContext->updateTextureSubresource(pTexture, subresourceIndex, pData);
    }

    void RenderContext::updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData)
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        flushGraphicsCommands(this, pApiData);
        pApiData->pCopyContext->updateTextureSubresources(pTexture, firstSubresource, subresourceCount, pData);
    }

    void RenderContext::copyResource(const Resource* pDst, const Resource* pSrc)
    {
        // FIXEM: I'd like that to be part of the CopyContext. The problem is that the CopyContext is not allowed to copy into the default FBO.
        // I can either leave it here or restrict the usage for the default FBO
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        resourceBarrier(pDst, Resource::State::CopyDest);
        resourceBarrier(pSrc, Resource::State::CopySource);
        pApiData->pList->CopyResource(pDst->getApiHandle(), pSrc->getApiHandle());
    }

    GpuFence::SharedPtr RenderContext::getFence() const
    {
        RenderContextData* pApiData = (RenderContextData*)mpApiData;
        return pApiData->pFence;
    }
}
