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
#include "API/Device.h"
#include "API/LowLevel/DescriptorHeap.h"
#include "API/LowLevel/GpuFence.h"
#include "API/Vulkan/FalcorVK.h"

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
        VkInstance          instance;
        VkPhysicalDevice    physicalDevice;
        VkDevice            device;
        VkSurfaceKHR        surface;

        VkPhysicalDeviceFeatures supportedFeatures;

        // Map Falcor command queue type to VK device queue family index
        uint32_t queueTypeToFamilyIndex[(uint32_t)LowLevelContextData::CommandQueueType::Count];
        std::vector<VkQueue> queues[(uint32_t)LowLevelContextData::CommandQueueType::Count];
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

        vkDestroySwapchainKHR(pData->device, pData->swapchain, nullptr);
        vkDestroySurfaceKHR(pData->instance, pData->surface, nullptr);
        vkDestroyDevice(pData->device, nullptr);
        vkDestroyInstance(pData->instance, nullptr);

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

    CommandQueueHandle Device::getCommandQueueHandle(LowLevelContextData::CommandQueueType type, uint32_t index) const
    {
        DeviceData* pData = (DeviceData*)mpPrivateData;
        return pData->queues[(uint32_t)type][index];
    }

    ApiCommandQueueType Device::getApiCommandQueueType(LowLevelContextData::CommandQueueType type) const
    {
        DeviceData* pData = (DeviceData*)mpPrivateData;
        return pData->queueTypeToFamilyIndex[(uint32_t)type];
    }

    bool createInstance(DeviceData *pData)
    {
        VkInstanceCreateInfo InstanceCreateInfo = {};
        InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceCreateInfo.pApplicationInfo = NULL;
        InstanceCreateInfo.enabledExtensionCount = 0;
        InstanceCreateInfo.ppEnabledExtensionNames = nullptr;
        InstanceCreateInfo.enabledLayerCount = 0;
        InstanceCreateInfo.ppEnabledLayerNames = nullptr;

        if (VK_FAILED(vkCreateInstance(&InstanceCreateInfo, nullptr, &pData->instance)))
        {
            logError("Failed to create Vulkan instance");
            return false;
        }

        return true;
    }

    /** Select best physical device based on memory
    */
    VkPhysicalDevice selectPhysicalDevice(const std::vector<VkPhysicalDevice>& devices)
    {
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        uint64_t bestMemory = 0;

        for (const VkPhysicalDevice& device : devices)
        {
            VkPhysicalDeviceMemoryProperties properties;
            vkGetPhysicalDeviceMemoryProperties(device, &properties);

            // Get local memory size from device
            uint64_t deviceMemory = 0;
            for (uint32_t i = 0; i < properties.memoryHeapCount; i++)
            {
                if ((properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) > 0)
                {
                    deviceMemory = properties.memoryHeaps[i].size;
                    break;
                }
            }

            // Save if best found so far
            if (bestDevice == VK_NULL_HANDLE || deviceMemory > bestMemory)
            {
                bestDevice = device;
                bestMemory = deviceMemory;
            }
        }

        return bestDevice;
    }

    bool createPhysicalDevice(DeviceData *pData)
    {
        //
        // Enumerate devices
        //

        uint32_t count = 0;
        vkEnumeratePhysicalDevices(pData->instance, &count, nullptr);
        assert(count > 0);

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(pData->instance, &count, devices.data());

        // Pick a device
        pData->physicalDevice = selectPhysicalDevice(devices);
        vkGetPhysicalDeviceFeatures(pData->physicalDevice, &pData->supportedFeatures);

        //
        // Match queue families to what type they are
        //

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pData->physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(pData->physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        // Init indices
        for (auto& index : pData->queueTypeToFamilyIndex)
        {
            index = (uint32_t)-1;
        }

        // Determine which queue is what type
        uint32_t& graphicsQueueIndex = pData->queueTypeToFamilyIndex[(uint32_t)LowLevelContextData::CommandQueueType::Direct];
        uint32_t& computeQueueIndex = pData->queueTypeToFamilyIndex[(uint32_t)LowLevelContextData::CommandQueueType::Compute];
        uint32_t& transferQueue = pData->queueTypeToFamilyIndex[(uint32_t)LowLevelContextData::CommandQueueType::Copy];

        for (uint32_t i = 0; i < (uint32_t)queueFamilyProperties.size(); i++)
        {
            VkQueueFlags flags = queueFamilyProperties[i].queueFlags;

            if ((flags & VK_QUEUE_GRAPHICS_BIT) != 0 && graphicsQueueIndex == (uint32_t)-1)
            {
                graphicsQueueIndex = i;
            }
            else if ((flags & VK_QUEUE_COMPUTE_BIT) != 0 && computeQueueIndex == (uint32_t)-1)
            {
                computeQueueIndex = i;
            }
            else if ((flags & VK_QUEUE_TRANSFER_BIT) != 0 && transferQueue == (uint32_t)-1)
            {
                transferQueue = i;
            }
        }

        return true;
    }

    bool createLogicalDevice(DeviceData *pData, const Device::Desc& desc)
    {
        // Features
        VkPhysicalDeviceFeatures requiredFeatures = {};
        requiredFeatures.multiDrawIndirect = pData->supportedFeatures.multiDrawIndirect;
        requiredFeatures.tessellationShader = VK_TRUE;
        requiredFeatures.geometryShader = VK_TRUE;

        // Queues
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for (uint32_t i = 0; i < (uint32_t)LowLevelContextData::CommandQueueType::Count; i++)
        {
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.flags = 0;
            info.queueCount = (i == (uint32_t)LowLevelContextData::CommandQueueType::Direct) ? desc.additionalQueues[i] + 1 : desc.additionalQueues[i];
            info.queueFamilyIndex = pData->queueTypeToFamilyIndex[i];
            info.pQueuePriorities = nullptr;

            if (info.queueCount > 0)
            {
                queueInfos.push_back(info);
            }
        }

        // Logical Device
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.flags = 0;
        deviceInfo.queueCreateInfoCount = (uint32_t)queueInfos.size();
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.enabledLayerCount = 0;
        deviceInfo.ppEnabledLayerNames = nullptr;
        deviceInfo.enabledExtensionCount = 0;
        deviceInfo.ppEnabledExtensionNames = nullptr;
        deviceInfo.pEnabledFeatures = &requiredFeatures;

        if (VK_FAILED(vkCreateDevice(pData->physicalDevice, &deviceInfo, nullptr, &pData->device)))
        {
            logError("Could not create Vulkan logical device.");
            return false;
        }

        return true;
    }

    bool createSurface(DeviceData *pData, const Window* pWindow)
    {
        VkWin32SurfaceCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = pWindow->getApiHandle();
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (VK_FAILED(vkCreateWin32SurfaceKHR(pData->instance, &createInfo, nullptr, &pData->surface)))
        {
            logError("Could not create Vulkan surface.");
            return false;
        }

        return true;
    }

    bool createSwapChain(DeviceData *pData, const Window* pWindow, ResourceFormat colorFormat)
    {
        VkSurfaceCapabilitiesKHR surfCaps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pData->physicalDevice, pData->surface, &surfCaps);

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(pData->physicalDevice, pData->surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pData->physicalDevice, pData->surface, &presentModeCount, presentModes.data());

        VkExtent2D swapchainExtent = {};
        if (surfCaps.currentExtent.width == (uint32_t)-1)
        {
            swapchainExtent.width = pWindow->getClientAreaWidth();
            swapchainExtent.height = pWindow->getClientAreaWidth();
        }
        else
        {
            swapchainExtent = surfCaps.currentExtent;
        }

        // Select present mode, preferring MAILBOX -> IMMEDIATE -> FIFO
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        for (size_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            else if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        VkSwapchainCreateInfoKHR scCreateInfo = {};
        scCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scCreateInfo.surface = pData->surface;
        scCreateInfo.minImageCount = clamp(kSwapChainBuffers, surfCaps.minImageCount, surfCaps.maxImageCount);
        scCreateInfo.imageFormat = getVkFormat(srgbToLinearFormat(colorFormat));
        scCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        scCreateInfo.imageExtent = { swapchainExtent.width, swapchainExtent.height };
        scCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scCreateInfo.preTransform = surfCaps.currentTransform;
        scCreateInfo.imageArrayLayers = 1;
        scCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        scCreateInfo.queueFamilyIndexCount = 0;
        scCreateInfo.pQueueFamilyIndices = nullptr;
        scCreateInfo.presentMode = presentMode;
        scCreateInfo.clipped = true;
        scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scCreateInfo.oldSwapchain = VK_NULL_HANDLE;


        if (VK_FAILED(vkCreateSwapchainKHR(pData->device, &scCreateInfo, nullptr, &pData->swapchain)))
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

        if (createLogicalDevice(pData, desc) == false)
        {
            return false;
        }

        if (createSurface(pData, mpWindow.get()) == false)
        {
            return false;
        }

        mApiHandle = pData->device;

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

        createSwapChain(pData, mpWindow.get(), desc.colorFormat);

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
