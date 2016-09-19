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

namespace Falcor
{
	struct D3D12Data
	{
		IDXGISwapChain3Ptr pSwapChain = nullptr;
		uint32_t currentBackBufferIndex;
		DescriptorHeap::SharedPtr pRtvHeap;
		ID3D12ResourcePtr pRenderTargets[kSwapChainBuffers];
		uint32_t syncInterval = 0;
		bool isWindowOccluded = false;
	};

	struct ApiData
	{
		std::vector<ID3D12CommandAllocatorPtr> pAllocators;
		ID3D12GraphicsCommandListPtr pList;
		uint32_t activeAllocator = 0;
	};

	RenderContext::~RenderContext()
	{
		delete (ApiData*)mpApiData;
		mpApiData = nullptr;
	}

    RenderContext::SharedPtr RenderContext::create(uint32_t allocatorsCount)
    {
        SharedPtr pCtx = SharedPtr(new RenderContext(D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE));
		ApiData* pApiData = new ApiData;
		pCtx->mpApiData = pApiData;


		// Create a command allocator
		pApiData->pAllocators.resize(allocatorsCount);
		auto pDevice = Device::getApiHandle();

		for (uint32_t i = 0; i < allocatorsCount; i++)
		{
			if (FAILED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pApiData->pAllocators[i]))))
			{
				Logger::log(Logger::Level::Error, "Failed to create command allocator");
				return nullptr;
			}
		}

		// Create a command list
		if (FAILED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pApiData->pAllocators[0], nullptr, IID_PPV_ARGS(&pApiData->pList))))
		{
			Logger::log(Logger::Level::Error, "Failed to create command list");
			return nullptr;
		}

		// We expect the list to be closed before we start using it
		d3d_call(pApiData->pList->Close());

		return pCtx;
    }

	CommandListHandle RenderContext::getCommandListApiHandle() const
	{
		const ApiData* pApiData = (ApiData*)mpApiData;
		return pApiData->pList;
	}

	void RenderContext::reset()
	{
		ApiData* pApiData = (ApiData*)mpApiData;
		// Skip to the next allocator
		pApiData->activeAllocator = (pApiData->activeAllocator + 1) % pApiData->pAllocators.size();
		d3d_call(pApiData->pAllocators[pApiData->activeAllocator]->Reset());
		d3d_call(pApiData->pList->Reset(pApiData->pAllocators[pApiData->activeAllocator], nullptr));
	}

	void RenderContext::clearFbo(const Fbo* pFbo, const glm::vec4& color)
	{
		ApiData* pApiData = (ApiData*)mpApiData;
		D3D12Data* pData = (D3D12Data*)Device::getPrivateData();

		ID3D12GraphicsCommandList* pList = pApiData->pList.GetInterfacePtr();
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = pData->pRenderTargets[pData->currentBackBufferIndex];
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		pList->ResourceBarrier(1, &barrier);

		DescriptorHeap::CpuHandle rtv = pData->pRtvHeap->getHandle(pData->currentBackBufferIndex);
		pList->ClearRenderTargetView(rtv, glm::value_ptr(color), 0, nullptr);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		pList->ResourceBarrier(1, &barrier);


	}

    void RenderContext::applyDepthStencilState() const
    {
        
    }

    void RenderContext::applyRasterizerState() const
    {
    }

    void RenderContext::applyBlendState() const
    {
    }

    void RenderContext::applyProgram() const
    {
    }

    void RenderContext::applyVao() const
    {
    }

    void RenderContext::applyFbo() const
    {
    }

    void RenderContext::blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, bool useLinearFiltering, FboAttachmentType copyFlags, uint32_t srcIdx, uint32_t dstIdx)
	{
        UNSUPPORTED_IN_D3D12("BlitFbo");
	}

    void RenderContext::applyUniformBuffer(uint32_t Index) const
    {
    }

    void RenderContext::applyShaderStorageBuffer(uint32_t index) const
    {
        UNSUPPORTED_IN_D3D12("RenderContext::ApplyShaderStorageBuffer()");
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
}
#endif //#ifdef FALCOR_D3D12
