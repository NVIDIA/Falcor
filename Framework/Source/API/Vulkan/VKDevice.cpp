/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
#include "API/vulkan/FalcorVK.h"

namespace Falcor
{
    Device::SharedPtr gpDevice;

    struct DeviceData
    {
        VkSwapchainKHR swapchain;
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

        // Vulkan
        VkInstance          pInstance;
        VkPhysicalDevice    pPhysicalDevice;
        VkDevice            pDevice;
        VkSurfaceKHR        pSurface;
        uint32_t            physicalDevCount;
        uint32_t            queueFamilyPropsCount;

        VkPhysicalDeviceMemoryProperties    devMemoryProperties;
        uint32_t                            graphicsQueueNodeIndex;
        uint32_t                            presentQueueNodeIndex;
        VkPhysicalDeviceFeatures            supportedFeatures;
        VkQueue                             deviceQueue;

        std::vector<VkQueueFamilyProperties> falcorVKQueueFamilyProps;

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
        //decltype(pData->deferredReleases)().swap(pData->deferredReleases);
    }

    bool Device::updateDefaultFBO(uint32_t width, uint32_t height, ResourceFormat colorFormat, ResourceFormat depthFormat)
    {
        DeviceData* pData = (DeviceData*)mpPrivateData;

        for (uint32_t i = 0; i < kSwapChainBuffers; i++)
        {
            // Create a texture object
            auto pColorTex = Texture::SharedPtr(new Texture(width, height, 1, 1, 1, 1, colorFormat, Texture::Type::Texture2D, Texture::BindFlags::RenderTarget));
            //HRESULT hr = pData->pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pColorTex->mApiHandle));
            //if (FAILED(hr))
            //{
            //	d3dTraceHR("Failed to get back-buffer " + std::to_string(i) + " from the swap-chain", hr);
            //	return false;
            //}

            // Create the FBO if it's required
            if (pData->frameData[i].pFbo == nullptr)
            {
                pData->frameData[i].pFbo = Fbo::create();
            }
            pData->frameData[i].pFbo->attachColorTarget(pColorTex, 0);

            // Create a depth texture
            //if (depthFormat != ResourceFormat::Unknown)
            //{
            //	auto pDepth = Texture::create2D(width, height, depthFormat, 1, 1, nullptr, Texture::BindFlags::DepthStencil);
            //	pData->frameData[i].pFbo->attachDepthStencilTarget(pDepth);
            //}

            //pData->currentBackBufferIndex = pData->pSwapChain->GetCurrentBackBufferIndex();
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
        //releaseFboData(pData);
        mpRenderContext.reset();
        mpResourceAllocator.reset();

        vkDestroySwapchainKHR(pData->pDevice, pData->swapchain, nullptr);
        vkDestroySurfaceKHR(pData->pInstance, pData->pSurface, nullptr);
        vkDestroyDevice(pData->pDevice, nullptr);
        vkDestroyInstance(pData->pInstance, nullptr);

        safe_delete(pData);
        mpWindow.reset();
    }

    Device::SharedPtr Device::create(Window::SharedPtr& pWindow, const Device::Desc& desc)
    {
        if (gpDevice)
        {
            logError("D3D12 backend only supports a single device");
            return false;
        }
        gpDevice = SharedPtr(new Device(pWindow));
        if (gpDevice->init(desc) == false)
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
        //pData->pSwapChain->Present(pData->syncInterval, 0);
        //pData->pFrameFence->gpuSignal(mpRenderContext->getLowLevelData()->getCommandQueue().GetInterfacePtr());
        executeDeferredReleases();
        mpRenderContext->reset();
        pData->currentBackBufferIndex = (pData->currentBackBufferIndex + 1) % kSwapChainBuffers;
        mFrameID++;
    }

    bool createInstance(DeviceData *pData)
    {
        VkInstanceCreateInfo InstanceCreateInfo;
        InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceCreateInfo.pNext = NULL;
        InstanceCreateInfo.pApplicationInfo = NULL;
        InstanceCreateInfo.enabledExtensionCount = 0;
        InstanceCreateInfo.ppEnabledExtensionNames = nullptr;
        InstanceCreateInfo.enabledLayerCount = 0;
        InstanceCreateInfo.ppEnabledLayerNames = nullptr;

        if (VK_FAILED(vkCreateInstance(&InstanceCreateInfo, nullptr, &pData->pInstance)))
        {
            logError("Failed to create Vulkan instance");
            return false;
        }

        return true;
    }

    bool createPhysicalDevice(DeviceData *pData)
    {
        // First figure out how many devices are in the system.
        // By convention, call once to get count, call again to get handles
        vkEnumeratePhysicalDevices(pData->pInstance, &pData->physicalDevCount, nullptr);
        vkEnumeratePhysicalDevices(pData->pInstance, &pData->physicalDevCount, &pData->pPhysicalDevice);

        logInfo("Physical devices: " + std::to_string(pData->physicalDevCount));

        vkGetPhysicalDeviceMemoryProperties(pData->pPhysicalDevice, &pData->devMemoryProperties);
        vkGetPhysicalDeviceQueueFamilyProperties(pData->pPhysicalDevice, &pData->queueFamilyPropsCount, nullptr);
        pData->falcorVKQueueFamilyProps.resize(pData->queueFamilyPropsCount);
        vkGetPhysicalDeviceQueueFamilyProperties(pData->pPhysicalDevice, &pData->queueFamilyPropsCount, pData->falcorVKQueueFamilyProps.data());

        // Search for a graphics and a present queue in the array of queue families, try to find one that supports both
        for (uint32_t i = 0; i < pData->queueFamilyPropsCount; i++)
        {
            if ((pData->falcorVKQueueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (pData->graphicsQueueNodeIndex == UINT32_MAX)
                {
                    pData->graphicsQueueNodeIndex = i;
                    break;
                }
            }
        }
        //queueNodeIndex = graphicsQueueNodeIndex;

        return true;
    }

    bool createLogicalDevice(DeviceData *pData)
    {
        VkPhysicalDeviceFeatures requiredFeatures = {};
        vkGetPhysicalDeviceFeatures(pData->pPhysicalDevice, &pData->supportedFeatures);
        requiredFeatures.multiDrawIndirect = pData->supportedFeatures.multiDrawIndirect;
        requiredFeatures.tessellationShader = VK_TRUE;
        requiredFeatures.geometryShader = VK_TRUE;

        const VkDeviceQueueCreateInfo deviceQueueCreateInfo =
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            pData->graphicsQueueNodeIndex,
            1,
            nullptr
        };

        const VkDeviceCreateInfo deviceCreateInfo =
        {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            nullptr,
            0,
            1,
            &deviceQueueCreateInfo,
            0,
            nullptr,
            0,
            nullptr,
            &requiredFeatures
        };

        if (VK_FAILED(vkCreateDevice(pData->pPhysicalDevice, &deviceCreateInfo, nullptr, &pData->pDevice)))
        {
            logError("Could not create Vulkan logical device.");
            return false;
        }

        return true;
    }

    bool createSurface(const Window* pWindow, DeviceData *pData)
    {
        VkWin32SurfaceCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = pWindow->getApiHandle();
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (VK_FAILED(vkCreateWin32SurfaceKHR(pData->pInstance, &createInfo, nullptr, &pData->pSurface)))
        {
            logError("Could not create Vulkan surface.");
            return false;
        }

        return true;
    }

    bool createSwapChain(const Window* pWindow, DeviceData *pData)
    {
        VkResult err;
        VkSurfaceCapabilitiesKHR surfCaps;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pData->pPhysicalDevice, pData->pSurface, &surfCaps);

        uint32_t presentModeCount;
        err = vkGetPhysicalDeviceSurfacePresentModesKHR(pData->pPhysicalDevice, pData->pSurface, &presentModeCount, NULL);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        err = vkGetPhysicalDeviceSurfacePresentModesKHR(pData->pPhysicalDevice, pData->pSurface, &presentModeCount, presentModes.data());

        VkExtent2D swapchainExtent = {};
        if (surfCaps.currentExtent.width == -1)
        {
            swapchainExtent.width = pWindow->getClientAreaWidth();
            swapchainExtent.height = pWindow->getClientAreaWidth();
        }
        else
        {
            swapchainExtent = surfCaps.currentExtent;
        }

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        for (size_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((presentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
            {
                presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
        if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
        {
            desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
        }

        VkSurfaceTransformFlagsKHR preTransform;
        if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            preTransform = surfCaps.currentTransform;
        }

        VkSwapchainCreateInfoKHR scCreateInfo;
        scCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scCreateInfo.pNext = NULL;
        scCreateInfo.surface = pData->pSurface;
        scCreateInfo.minImageCount = desiredNumberOfSwapchainImages;
        scCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        scCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        scCreateInfo.imageExtent = { swapchainExtent.width, swapchainExtent.height };
        scCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scCreateInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
        scCreateInfo.imageArrayLayers = 1;
        scCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        scCreateInfo.queueFamilyIndexCount = 0;
        scCreateInfo.pQueueFamilyIndices = NULL;
        scCreateInfo.presentMode = presentMode;
        scCreateInfo.clipped = true;
        scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scCreateInfo.oldSwapchain = VK_NULL_HANDLE;


        if (VK_FAILED(vkCreateSwapchainKHR(pData->pDevice, &scCreateInfo, nullptr, &pData->swapchain)))
        {
            logError("Could not create swapchain.");
            return false;
        }

        //err = vkGetSwapchainImagesKHR(pData->falcorVKLogicalDevice, pData->swapchain, &pData->imageCount, NULL);
        //pData->swapChainImages.resize(pData->imageCount);
        //err = vkGetSwapchainImagesKHR(pData->falcorVKLogicalDevice, pData->swapchain, &pData->imageCount, pData->swapChainImages.data());

        //// For each of the Swapchain images, create image views out of them.
        //pData->FrameDatas.resize(pData->imageCount);
        //for (uint32_t i = 0; i < pData->imageCount; i++)
        //{
        //    VkImageViewCreateInfo colorAttachmentView = {};
        //    colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        //    colorAttachmentView.pNext = NULL;
        //    colorAttachmentView.format = VK_FORMAT_B8G8R8A8_UNORM;
        //    colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //    colorAttachmentView.subresourceRange.baseMipLevel = 0;
        //    colorAttachmentView.subresourceRange.levelCount = 1;
        //    colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        //    colorAttachmentView.subresourceRange.layerCount = 1;
        //    colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        //    colorAttachmentView.flags = 0;
        //    colorAttachmentView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

        //    pData->FrameDatas[i].image = pData->swapChainImages[i];
        //    colorAttachmentView.image = pData->FrameDatas[i].image;
        //    err = vkCreateImageView(pData->falcorVKLogicalDevice, &colorAttachmentView, nullptr, &pData->FrameDatas[i].view);
        //}

        return true;
    }

    bool Device::init(const Desc& desc)
    {
        DeviceData* pData = new DeviceData;
        mpPrivateData = pData;

        if (desc.enableDebugLayer)
        {
            // TODO: add some Vulkan layers here.
        }

        if (createInstance(pData) == false)
        {
            return false;
        }
        
        if (createPhysicalDevice(pData) == false)
        {
            return false;
        }

        if (createLogicalDevice(pData) == false)
        {
            return false;
        }

        if (createSurface(mpWindow.get(), pData) == false)
        {
            return false;
        }

        mApiHandle = pData->pDevice;

        //// Create the descriptor heaps
        //mpSrvHeap = DescriptorHeap::create(DescriptorHeap::Type::SRV, 16 * 1024);
        //mpSamplerHeap = DescriptorHeap::create(DescriptorHeap::Type::Sampler, 2048);
        //mpRtvHeap = DescriptorHeap::create(DescriptorHeap::Type::RTV, 1024, false);
        //mpDsvHeap = DescriptorHeap::create(DescriptorHeap::Type::DSV, 1024, false);
        //mpUavHeap = mpSrvHeap;
        //mpCpuUavHeap = DescriptorHeap::create(DescriptorHeap::Type::SRV, 2 * 1024, false);

        //// Create the swap-chain
        mpRenderContext = RenderContext::create();
        
        //mpResourceAllocator = ResourceAllocator::create(1024 * 1024 * 2, mpRenderContext->getLowLevelData()->getFence());

        createSwapChain(mpWindow.get(), pData);

        mVsyncOn = desc.enableVsync;

        //// Update the FBOs
        //if (updateDefaultFBO(mpWindow->getClientAreaWidth(), mpWindow->getClientAreaHeight(), desc.colorFormat, desc.depthFormat) == false)
        //{
        //    return false;
        //}

        //pData->pFrameFence = GpuFence::create();
        return true;
    }

    void Device::releaseResource(ApiObjectHandle pResource)
    {
        if (pResource)
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
        //mpRenderContext->flush(true);

        //DeviceData* pData = (DeviceData*)mpPrivateData;

        //// Store the FBO parameters
        //ResourceFormat colorFormat = pData->frameData[0].pFbo->getColorTexture(0)->getFormat();
        //const auto& pDepth = pData->frameData[0].pFbo->getDepthStencilTexture();
        //ResourceFormat depthFormat = pDepth ? pDepth->getFormat() : ResourceFormat::Unknown;
        //assert(pData->frameData[0].pFbo->getSampleCount() == 1);

        //// Delete all the FBOs
        //releaseFboData(pData);

        //DXGI_SWAP_CHAIN_DESC desc;
        //d3d_call(pData->pSwapChain->GetDesc(&desc));
        //d3d_call(pData->pSwapChain->ResizeBuffers(kSwapChainBuffers, width, height, desc.BufferDesc.Format, desc.Flags));
        //updateDefaultFBO(width, height, colorFormat, depthFormat);

        return getSwapChainFbo();
    }

    void Device::setVSync(bool enable)
    {
        DeviceData* pData = (DeviceData*)mpPrivateData;
        pData->syncInterval = enable ? 1 : 0;
    }

    bool Device::isWindowOccluded() const
    {
        //DeviceData* pData = (DeviceData*)mpPrivateData;
        //if (pData->isWindowOccluded)
        //{
        //    pData->isWindowOccluded = (pData->pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED);
        //}
        //return pData->isWindowOccluded;
        return false;
    }

    bool Device::isExtensionSupported(const std::string& name)
    {
        return _ENABLE_NVAPI;
    }
}
