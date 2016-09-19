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
#include "Sample.h"
#include "Core/Device.h"
#include "D3D12DescriptorHeap.h"

namespace Falcor
{
	struct D3D12Data
	{
		IDXGISwapChain3Ptr pSwapChain = nullptr;
		uint32_t currentBackBufferIndex;
		DescriptorHeap::SharedPtr pRtvHeap;
		ID3D12ResourcePtr pRenderTargets[kSwapChainBuffers];
		ID3D12CommandQueuePtr pCommandQueue;
		uint32_t syncInterval = 0;
		bool isWindowOccluded = false;
	};

	DeviceHandle Device::sApiHandle;
	void* Device::mpPrivateData = nullptr;

	void d3dTraceHR(const std::string& msg, HRESULT hr)
	{
		char hr_msg[512];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);

		std::string error_msg = msg + ".\nError " + hr_msg;
		Logger::log(Logger::Level::Fatal, error_msg);
	}

    D3D_FEATURE_LEVEL getD3DFeatureLevel(uint32_t majorVersion, uint32_t minorVersion)
    {
        if(majorVersion == 12)
        {
            switch(minorVersion)
            {
            case 0:
                return D3D_FEATURE_LEVEL_12_0;
            case 1:
                return D3D_FEATURE_LEVEL_12_1;
            }
        }
        else if(majorVersion == 11)
        {
            switch(minorVersion)
            {
            case 0:
                return D3D_FEATURE_LEVEL_11_1;
            case 1:
                return D3D_FEATURE_LEVEL_11_0;
            }
        }
        else if(majorVersion == 10)
        {
            switch(minorVersion)
            {
            case 0:
                return D3D_FEATURE_LEVEL_10_0;
            case 1:
                return D3D_FEATURE_LEVEL_10_1;
            }
        }
        else if(majorVersion == 9)
        {
            switch(minorVersion)
            {
            case 1:
                return D3D_FEATURE_LEVEL_9_1;
            case 2:
                return D3D_FEATURE_LEVEL_9_2;
            case 3:
                return D3D_FEATURE_LEVEL_9_3;
            }
        }
        return (D3D_FEATURE_LEVEL)0;
    }

	IDXGISwapChain3Ptr createSwapChain(IDXGIFactory4* pFactory, const Window* pWindow, ID3D12CommandQueue* pCommandQueue, ResourceFormat colorFormat)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = kSwapChainBuffers;
		swapChainDesc.Width = pWindow->getClientAreaWidth();
		swapChainDesc.Height = pWindow->getClientAreaHeight();
		// Flip mode doesn't support SRGB formats, so we strip them down when creating the resource. We will create the RTV as SRGB instead.
		// More details at the end of https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064.aspx
		swapChainDesc.Format = getDxgiFormat(srgbToLinearFormat(colorFormat));
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		// CreateSwapChainForHwnd() doesn't accept IDXGISwapChain3 (Why MS? Why?)
		MAKE_SMART_COM_PTR(IDXGISwapChain1);
		IDXGISwapChain1Ptr pSwapChain;

		HRESULT hr = pFactory->CreateSwapChainForHwnd(pCommandQueue, pWindow->getApiHandle(), &swapChainDesc, nullptr, nullptr, &pSwapChain);
		if (FAILED(hr))
		{
			d3dTraceHR("Failed to create the swap-chain", hr);
			return false;
		}

		IDXGISwapChain3Ptr pSwapChain3;
		d3d_call(pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3)));
		return pSwapChain3;
	}

	ID3D12DevicePtr createDevice(IDXGIFactory4* pFactory, D3D_FEATURE_LEVEL featureLevel)
	{
		// Find the HW adapter
		IDXGIAdapter1Ptr pAdapter;
		ID3D12DevicePtr pDevice;

		for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(i, &pAdapter); i++)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);

			// Skip SW adapters
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			// Try and create a D3D12 device
			if (D3D12CreateDevice(pAdapter, featureLevel, IID_PPV_ARGS(&pDevice)) == S_OK)
			{
				return pDevice;
			}
		}

		Logger::log(Logger::Level::Fatal, "Could not find a GPU that supports D3D12 device");
		return nullptr;
	}

	DescriptorHeap::SharedPtr createHeaps()
	{
		return DescriptorHeap::create(DescriptorHeap::Type::RenderTargetView, 256, false);
	}

	bool createRTVs(ID3D12Device* pDevice, DescriptorHeap* pHeap, IDXGISwapChain3* pSwapChain, ID3D12ResourcePtr pRenderTargets[], uint32_t count, ResourceFormat colorFormat)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			DescriptorHeap::CpuHandle rtv = pHeap->getFreeCpuHandle();
			HRESULT hr = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderTargets[i]));
			if (FAILED(hr))
			{
				d3dTraceHR("Failed to get back-buffer " + std::to_string(i) + " from the swap-chain", hr);
				return false;
			}
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.Format = getDxgiFormat(colorFormat);
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			pDevice->CreateRenderTargetView(pRenderTargets[i], &rtvDesc, rtv);
		}

		return true;
	}

	Device::~Device() = default;

	Device::SharedPtr Device::create(Window::SharedPtr& pWindow, const Device::Desc& desc)
	{
		SharedPtr pDevice = SharedPtr(new Device(pWindow));
		return pDevice->init(desc) ? pDevice : nullptr;
	}

	void Device::present()
	{
		D3D12Data* pData = (D3D12Data*)mpPrivateData;

		// Submit the command list
		auto pGfxList = mpRenderContext->getCommandListApiHandle();
		d3d_call(pGfxList->Close());
		ID3D12CommandList* pList = pGfxList.GetInterfacePtr();
		pData->pCommandQueue->ExecuteCommandLists(1, &pList);

		// Present
		pData->pSwapChain->Present(1, 0);
		pData->currentBackBufferIndex = (pData->currentBackBufferIndex + 1) % kSwapChainBuffers;
	}

	bool Device::init(const Desc& desc)
	{
		if (sApiHandle)
		{
			logError("D3D12 backend only supports a single device");
			return false;
		}

		D3D12Data* pData = new D3D12Data;
		mpPrivateData = pData;

#if defined(_DEBUG)
		{
			ID3D12DebugPtr pDebug;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
			{
				pDebug->EnableDebugLayer();
			}
		}
#endif
		// Create the DXGI factory
		IDXGIFactory4Ptr pDxgiFactory;
		d3d_call(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)));

		// Create the device
		sApiHandle = createDevice(pDxgiFactory, getD3DFeatureLevel(desc.apiMajorVersion, desc.apiMinorVersion));
		if (sApiHandle == nullptr)
		{
			return false;
		}

		// Create Heaps
		pData->pRtvHeap = createHeaps();
		if (pData->pRtvHeap == nullptr)
		{
			return false;
		}

		mpRenderContext = RenderContext::create(kSwapChainBuffers);

		// Create a command queue
		// Create the command queue
		D3D12_COMMAND_QUEUE_DESC cqDesc = {};
		cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ID3D12DevicePtr pDevice = Device::getApiHandle();

		if (FAILED(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pData->pCommandQueue))))
		{
			Logger::log(Logger::Level::Error, "Failed to create command queue");
			return nullptr;
		}

		// Create the swap-chain
		pData->pSwapChain = createSwapChain(pDxgiFactory, mpWindow.get(), pData->pCommandQueue, desc.colorFormat);
		if(pData->pSwapChain == nullptr)
		{
			return false;
		}

		pData->currentBackBufferIndex = pData->pSwapChain->GetCurrentBackBufferIndex();

		// Create RTVs
		if(createRTVs(sApiHandle.GetInterfacePtr(), pData->pRtvHeap.get(), pData->pSwapChain.GetInterfacePtr(), pData->pRenderTargets, arraysize(pData->pRenderTargets), desc.colorFormat) == false)
		{
			return false;
		}

		mVsyncOn = desc.enableVsync;

		return true;
    }

	void Device::setVSync(bool enable)
	{
		D3D12Data* pData = (D3D12Data*)mpPrivateData;
		pData->syncInterval = enable ? 1 : 0;
	}

	bool Device::isWindowOccluded() const
    {
		D3D12Data* pData = (D3D12Data*)mpPrivateData;
        if(pData->isWindowOccluded)
        {
			pData->isWindowOccluded = (pData->pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED);
        }
        return pData->isWindowOccluded;
    }

    bool Device::isExtensionSupported(const std::string& name)
    {
        UNSUPPORTED_IN_D3D("Device::isExtensionSupported()");
        return false;
    }
}
#endif //#ifdef FALCOR_D3D
