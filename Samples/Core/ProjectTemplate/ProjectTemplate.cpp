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
#include "ProjectTemplate.h"
#include "Core/D3D/FalcorD3D.h"
#include "Core/D3D/D3D12/D3D12CommandList.h"
#include "Core/D3D/D3D12/D3D12DescriptorHeap.h"

namespace Falcor
{
	struct D3D12Data
	{
		IDXGISwapChain3Ptr pSwapChain = nullptr;
		uint32_t currentBackBufferIndex;
		CommandList::SharedPtr pCmdList;
		DescriptorHeap::SharedPtr pRtvHeap;
		ID3D12ResourcePtr pRenderTargets[kSwapChainBuffers];
		uint32_t syncInterval = 0;
	};
}

void ProjectTemplate::initUI()
{
    Gui::setGlobalHelpMessage("Sample application to load and display a model.\nUse the UI to switch between wireframe and solid mode.");
}

void ProjectTemplate::onLoad()
{
//     mpCamera = Camera::create();
//     initUI();
}

void ProjectTemplate::onFrameRender()
{
	D3D12Data* pData = (D3D12Data*)Device::getPrivateData();
	DeviceHandle pDevice = Device::getApiHandle();

	pData->pCmdList->reset();
    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};

    // Indicate that the back buffer will be used as a render target.
    ID3D12GraphicsCommandList* pList = pData->pCmdList->getList();
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = pData->pRenderTargets[pData->currentBackBufferIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    pList->ResourceBarrier(1, &barrier);

    DescriptorHeap::CpuHandle rtv = pData->pRtvHeap->getHandle(pData->currentBackBufferIndex);
    pList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    pList->ResourceBarrier(1, &barrier);

	pData->pCmdList->submit();

	pData->pSwapChain->Present(1, 0);

	pData->currentBackBufferIndex = (pData->currentBackBufferIndex + 1) % kSwapChainBuffers;

//     const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
//     mpDefaultFBO->clear(clearColor, 1.0f, 0, FboAttachmentType::All);
// 
//     renderText(getGlobalSampleMessage(true), glm::vec2(10, 10));
}

void ProjectTemplate::onShutdown()
{

}

bool ProjectTemplate::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return false;
}

bool ProjectTemplate::onMouseEvent(const MouseEvent& mouseEvent)
{
    return false;
}

void ProjectTemplate::onDataReload()
{

}

void ProjectTemplate::onResizeSwapChain()
{
//     RenderContext::Viewport vp;
//     vp.height = (float)mpDefaultFBO->getHeight();
//     vp.width = (float)mpDefaultFBO->getWidth();
//     mpRenderContext->setViewport(0, vp);
//     mpCamera->setFovY(float(M_PI / 8));
//     mpCamera->setAspectRatio(vp.width / vp.height);
//     mpCamera->setDepthRange(0, 1000);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ProjectTemplate sample;
    SampleConfig config;
    config.windowDesc.title = "Falcor Project Template";
    config.windowDesc.resizableWindow = true;
    sample.run(config);
}
