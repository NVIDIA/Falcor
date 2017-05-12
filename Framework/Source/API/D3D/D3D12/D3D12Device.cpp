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
#include "Sample.h"
#include "API/Device.h"
#include "API/LowLevel/DescriptorHeap.h"
#include "API/LowLevel/GpuFence.h"

namespace Falcor
{
    Device::SharedPtr gpDevice;

	struct DeviceData
	{
		IDXGISwapChain3Ptr pSwapChain = nullptr;
		uint32_t currentBackBufferIndex;

        struct ResourceRelease
        {
            size_t frameID;
            ApiObjectHandle pApiObject;
        };

        struct
        {
            Fbo::SharedPtr pFbo;
        } frameData[kSwapChainBuffers];

        std::queue<ResourceRelease> deferredReleases;
        uint32_t syncInterval = 0;
		bool isWindowOccluded = false;
        GpuFence::SharedPtr pFrameFence;
	};

    void releaseFboData(DeviceData* pData)
    {
        // First, delete all FBOs
        for (uint32_t i = 0; i < arraysize(pData->frameData); i++)
        {
            pData->frameData[i].pFbo->attachColorTarget(nullptr, 0);
            pData->frameData[i].pFbo->attachDepthStencilTarget(nullptr);
        }

        // Now execute all deferred releases
        decltype(pData->deferredReleases)().swap(pData->deferredReleases);
    }

    void d3dTraceHR(const std::string& msg, HRESULT hr)
	{
		char hr_msg[512];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);

		std::string error_msg = msg + ".\nError! " + hr_msg;
		logError(error_msg);
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

	ID3D12DevicePtr createDevice(IDXGIFactory4* pFactory, D3D_FEATURE_LEVEL featureLevel, Device::Desc::CreateDeviceFunc createFunc)
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
            if (createFunc)
            {
                pDevice = createFunc(pAdapter, featureLevel);
                if (pDevice) return pDevice;
            }
            else if (D3D12CreateDevice(pAdapter, featureLevel, IID_PPV_ARGS(&pDevice)) == S_OK)
			{
				return pDevice;
			}
		}

		logErrorAndExit("Could not find a GPU that supports D3D12 device");
		return nullptr;
	}

	bool Device::updateDefaultFBO(uint32_t width, uint32_t height, ResourceFormat colorFormat, ResourceFormat depthFormat)
	{
        DeviceData* pData = (DeviceData*)mpPrivateData;

		for (uint32_t i = 0; i < kSwapChainBuffers; i++)
		{
            // Create a texture object
            auto pColorTex = Texture::SharedPtr(new Texture(width, height, 1, 1, 1, 1, colorFormat, Texture::Type::Texture2D, Texture::BindFlags::RenderTarget));            
            HRESULT hr = pData->pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pColorTex->mApiHandle));
            if(FAILED(hr))
            {
                d3dTraceHR("Failed to get back-buffer " + std::to_string(i) + " from the swap-chain", hr);
                return false;
            }

            // Create the FBO if it's required
            if (pData->frameData[i].pFbo == nullptr)
            {
                pData->frameData[i].pFbo = Fbo::create();
            }
            pData->frameData[i].pFbo->attachColorTarget(pColorTex, 0);

            // Create a depth texture
            if(depthFormat != ResourceFormat::Unknown)
            {
                auto pDepth = Texture::create2D(width, height, depthFormat, 1, 1, nullptr, Texture::BindFlags::DepthStencil);
                pData->frameData[i].pFbo->attachDepthStencilTarget(pDepth);
            }

            pData->currentBackBufferIndex = pData->pSwapChain->GetCurrentBackBufferIndex();
		}

		return true;
	}

    void Device::cleanup()
    {
        mpRenderContext->flush(true);
        // Release all the bound resources. Need to do that before deleting the RenderContext
        mpRenderContext->setGraphicsState(nullptr);
        mpRenderContext->setGraphicsVars(nullptr);
        mpRenderContext->setComputeState(nullptr);
        mpRenderContext->setComputeVars(nullptr);
        DeviceData* pData = (DeviceData*)mpPrivateData;
        releaseFboData(pData);
        mpRenderContext.reset();
        mpResourceAllocator.reset();
        safe_delete(pData);
        mpWindow.reset();
    }

	Device::SharedPtr Device::create(Window::SharedPtr& pWindow, const Device::Desc& desc)
	{
        if(gpDevice)
        {
            logError("D3D12 backend only supports a single device");
            return false;
        }
        gpDevice = SharedPtr(new Device(pWindow));
        if(gpDevice->init(desc) == false)
        {
            gpDevice = nullptr;
        }
		return gpDevice;
	}

    
    Fbo::SharedPtr Device::getSwapChainFbo() const
    {
        DeviceData* pData = (DeviceData*)mpPrivateData;
        return pData->frameData[pData->currentBackBufferIndex].pFbo;
    }

	void Device::present()
	{
		DeviceData* pData = (DeviceData*)mpPrivateData;

        mpRenderContext->resourceBarrier(pData->frameData[pData->currentBackBufferIndex].pFbo->getColorTexture(0).get(), Resource::State::Present);
        mpRenderContext->flush();
        pData->pSwapChain->Present(pData->syncInterval, 0);
        pData->pFrameFence->gpuSignal(mpRenderContext->getLowLevelData()->getCommandQueue().GetInterfacePtr());
        executeDeferredReleases();
        mpRenderContext->reset();
        pData->currentBackBufferIndex = (pData->currentBackBufferIndex + 1) % kSwapChainBuffers;
        mFrameID++;
    }

	bool Device::init(const Desc& desc)
    {
		DeviceData* pData = new DeviceData;
		mpPrivateData = pData;

        if(desc.enableDebugLayer)
		{
			ID3D12DebugPtr pDebug;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
			{
				pDebug->EnableDebugLayer();
			}
		}

        // Create the DXGI factory
		IDXGIFactory4Ptr pDxgiFactory;
		d3d_call(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)));

		// Create the device
        mApiHandle = createDevice(pDxgiFactory, getD3DFeatureLevel(desc.apiMajorVersion, desc.apiMinorVersion), desc.createDeviceFunc);
		if (mApiHandle == nullptr)
		{
			return false;
		}

        // Create the descriptor heaps
        mpSrvHeap = DescriptorHeap::create(DescriptorHeap::Type::SRV, 16 * 1024);
        mpSamplerHeap = DescriptorHeap::create(DescriptorHeap::Type::Sampler, 2048);
        mpRtvHeap = DescriptorHeap::create(DescriptorHeap::Type::RTV, 1024, false);
        mpDsvHeap = DescriptorHeap::create(DescriptorHeap::Type::DSV, 1024, false);
        mpUavHeap = mpSrvHeap;
        mpCpuUavHeap = DescriptorHeap::create(DescriptorHeap::Type::SRV, 2*1024, false);

		// Create the swap-chain
        mpRenderContext = RenderContext::create();
        mpResourceAllocator = ResourceAllocator::create(1024 * 1024 * 2, mpRenderContext->getLowLevelData()->getFence());
        pData->pSwapChain = createSwapChain(pDxgiFactory, mpWindow.get(), mpRenderContext->getLowLevelData()->getCommandQueue(), desc.colorFormat);
		if(pData->pSwapChain == nullptr)
		{
			return false;
		}

        mVsyncOn = desc.enableVsync;

        // Update the FBOs
        if (updateDefaultFBO(mpWindow->getClientAreaWidth(), mpWindow->getClientAreaHeight(), desc.colorFormat, desc.depthFormat) == false)
        {
            return false;
        }

        pData->pFrameFence = GpuFence::create();
		return true;
    }

    void Device::releaseResource(ApiObjectHandle pResource)
    {
        if(pResource)
        {
            DeviceData* pData = (DeviceData*)mpPrivateData;
            pData->deferredReleases.push({ pData->pFrameFence->getCpuValue(), pResource });
        }
    }

    void Device::executeDeferredReleases()
    {
        mpResourceAllocator->executeDeferredReleases();
        DeviceData* pData = (DeviceData*)mpPrivateData;
        uint64_t gpuVal = pData->pFrameFence->getGpuValue();
        while (pData->deferredReleases.size() && pData->deferredReleases.front().frameID < gpuVal)
        {
            pData->deferredReleases.pop();
        }
    }

    Fbo::SharedPtr Device::resizeSwapChain(uint32_t width, uint32_t height)
    {
        mpRenderContext->flush(true);
        
        DeviceData* pData = (DeviceData*)mpPrivateData;

        // Store the FBO parameters
        ResourceFormat colorFormat = pData->frameData[0].pFbo->getColorTexture(0)->getFormat();
        const auto& pDepth = pData->frameData[0].pFbo->getDepthStencilTexture();
        ResourceFormat depthFormat = pDepth ? pDepth->getFormat() : ResourceFormat::Unknown;
        assert(pData->frameData[0].pFbo->getSampleCount() == 1);

        // Delete all the FBOs
        releaseFboData(pData);

        DXGI_SWAP_CHAIN_DESC desc;
        d3d_call(pData->pSwapChain->GetDesc(&desc));
        d3d_call(pData->pSwapChain->ResizeBuffers(kSwapChainBuffers, width, height, desc.BufferDesc.Format, desc.Flags));
        updateDefaultFBO(width, height, colorFormat, depthFormat);

        return getSwapChainFbo();
    }

	void Device::setVSync(bool enable)
	{
		DeviceData* pData = (DeviceData*)mpPrivateData;
		pData->syncInterval = enable ? 1 : 0;
	}

	bool Device::isWindowOccluded() const
    {
		DeviceData* pData = (DeviceData*)mpPrivateData;
        if(pData->isWindowOccluded)
        {
			pData->isWindowOccluded = (pData->pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED);
        }
        return pData->isWindowOccluded;
    }

    bool Device::isExtensionSupported(const std::string& name)
    {
        return _ENABLE_NVAPI;
    }
}