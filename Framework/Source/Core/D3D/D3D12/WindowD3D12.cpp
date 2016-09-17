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
#include "CommandListD3D12.h"
#include "DescriptorHeapD3D12.h"

namespace Falcor
{
    struct D3D12Data
    {
        ID3D12DevicePtr pDevice = nullptr;
        IDXGISwapChain3Ptr pSwapChain = nullptr;
        uint32_t currentBackBufferIndex;
        CommandList::SharedPtr pCmdList;
        DescriptorHeap::SharedPtr mRtvHeap;
        ID3D12ResourcePtr mpRenderTargets[kSwapChainBuffers];
    };

    D3D12Data gD3D12Data;

    ID3D12DevicePtr getD3D12Device()
    {
        return gD3D12Data.pDevice;
    }

    bool createHeaps()
    {
        gD3D12Data.mRtvHeap = DescriptorHeap::create(DescriptorHeap::Type::RenderTargetView, 256, false);

        return gD3D12Data.mRtvHeap != nullptr;
    }

    bool createDevice(IDXGIFactory4* pFactory, D3D_FEATURE_LEVEL featureLevel)
    {
        // Find the HW adapter
        IDXGIAdapter1Ptr pAdapter;

        for(uint32_t i = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(i, &pAdapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            pAdapter->GetDesc1(&desc);

            // Skip SW adapters
            if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            // Try and create a D3D12 device
            if(D3D12CreateDevice(pAdapter, featureLevel, IID_PPV_ARGS(&gD3D12Data.pDevice)) == S_OK)
            {
                return true;
            }
        }

        Logger::log(Logger::Level::Fatal, "Could not find a GPU that supports D3D12 device");
        return false;
    }

    bool createSwapChain(IDXGIFactory4* pFactory, HWND hWnd, const Window::Desc& desc)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = kSwapChainBuffers;
        swapChainDesc.Width = 800;//desc.swapChainDesc.width;
        swapChainDesc.Height = 600;//desc.swapChainDesc.height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//getDxgiFormat(desc.swapChainDesc.colorFormat);
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        MAKE_SMART_COM_PTR(IDXGISwapChain1);
        IDXGISwapChain1Ptr pSwapChain;

        if(FAILED(pFactory->CreateSwapChainForHwnd(gD3D12Data.pCmdList->getNativeCommandQueue().GetInterfacePtr(), hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain)))
        {
            return false;
        }
        pSwapChain->QueryInterface(IID_PPV_ARGS(&gD3D12Data.pSwapChain));

        gD3D12Data.currentBackBufferIndex = gD3D12Data.pSwapChain->GetCurrentBackBufferIndex();
        return true;
    }

    bool createRTVs()
    {
        for(uint32_t i = 0; i < kSwapChainBuffers; i++)
        {
            DescriptorHeap::CpuHandle rtv = gD3D12Data.mRtvHeap->getFreeCpuHandle();
            if(FAILED(gD3D12Data.pSwapChain->GetBuffer(i, IID_PPV_ARGS(&gD3D12Data.mpRenderTargets[i]))))
            {
                logError("Failed to get back-buffer " + std::to_string(i) + " from the swap-chain");
                return false;
            }
            gD3D12Data.pDevice->CreateRenderTargetView(gD3D12Data.mpRenderTargets[i], nullptr, rtv);
        }

        return true;
    }

    IUnknown* createDevice(const Window::Desc& desc, HWND hWnd)
    {
        if(getD3D12Device())
        {
            Logger::log(Logger::Level::Error, "D3D12 backend doesn't support more than a single device.");
            return nullptr;
        }

#if defined(_DEBUG)
        {
            ID3D12DebugPtr pDebug;
            if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
            {
                pDebug->EnableDebugLayer();
            }
        }
#endif
        // Create the DXGI factory
        IDXGIFactory4Ptr pDxgiFactory;
        d3d_call(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)));

        // Create the device
        if(createDevice(pDxgiFactory, getD3DFeatureLevel(desc.apiMajorVersion, desc.apiMinorVersion)) == false)
        {
            return nullptr;
        }

        // Create command-queues
        gD3D12Data.pCmdList = CommandList::create(kSwapChainBuffers);
        if(gD3D12Data.pCmdList == nullptr)
        {
            return nullptr;
        }

        // Create Heaps
        if(createHeaps() == false)
        {
            return nullptr;
        }

        // Create the swap-chain
        if(createSwapChain(pDxgiFactory, hWnd, desc) == false)
        {
            return nullptr;
        }

        // Create RTVs
        if(createRTVs() == false)
        {
            return nullptr;
        }

        return gD3D12Data.pDevice;
    }
}

#endif //#ifdef FALCOR_D3D12
