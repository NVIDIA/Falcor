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


// @Kai-Hwa: This is used as a hack for now. There are some build errors I see inside GLM when I define
// the WIN32 macro.
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>

namespace Falcor
{
    Device::SharedPtr gpDevice;

    typedef struct _FrameData 
    {
        VkImage     image;
        VkImageView view;
    } FrameData;

    // TODO: I don't like the names falcorVKXXX. Try something better.
    struct VKDeviceData
    {
        VKDeviceData()
        {
            graphicsQueueNodeIndex = UINT32_MAX;
            presentQueueNodeIndex  = UINT32_MAX;
        }

        VkInstance                           falcorVKInstance;
        VkPhysicalDevice                     falcorVKPhysDevice;
        VkDevice                             falcorVKLogicalDevice;
        uint32_t                             falcorVKPhysDevCount;
        uint32_t                             falcorVKQueueFamilyPropsCount;

        VkPhysicalDeviceMemoryProperties     devMemoryProperties;
        uint32_t                             graphicsQueueNodeIndex;
        uint32_t                             presentQueueNodeIndex;
        uint32_t                             queueNodeIndex;
        VkPhysicalDeviceFeatures             supportedFeatures;
        VkQueue                              deviceQueue;
        VkSurfaceKHR                         surface;
        VkSwapchainKHR                       swapchain;
        uint32_t                             imageCount;

        std::vector<VkQueueFamilyProperties> falcorVKQueueFamilyProps;
        std::vector<FrameData>               FrameDatas;
        std::vector<VkImage>                 swapChainImages;
    };

    struct DeviceData
    {
        IDXGISwapChain3Ptr pSwapChain = nullptr;
        uint32_t currentBackBufferIndex;

        struct ResourceRelease
        {
            size_t frameID;
            ApiObjectHandle pApiObject;
        };

        std::queue<ResourceRelease> deferredReleases;
        uint32_t syncInterval = 0;
        bool isWindowOccluded = false;
        GpuFence::SharedPtr pFrameFence;
    };

    bool createSwapChain(VKDeviceData *dd, const Window* pWindow)
    {
        VkResult err;
        VkSurfaceCapabilitiesKHR surfCaps;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dd->falcorVKPhysDevice, dd->surface, &surfCaps);

        uint32_t presentModeCount;
        err = vkGetPhysicalDeviceSurfacePresentModesKHR(dd->falcorVKPhysDevice, dd->surface, &presentModeCount, NULL);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        err = vkGetPhysicalDeviceSurfacePresentModesKHR(dd->falcorVKPhysDevice, dd->surface, &presentModeCount, presentModes.data());

        VkExtent2D swapchainExtent = {};
        if (surfCaps.currentExtent.width == -1)
        {
            swapchainExtent.width  = pWindow->getClientAreaWidth();
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
        scCreateInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scCreateInfo.pNext                 = NULL;
        scCreateInfo.surface               = dd->surface;
        scCreateInfo.minImageCount         = desiredNumberOfSwapchainImages;
        scCreateInfo.imageFormat           = VK_FORMAT_B8G8R8A8_UNORM;
        scCreateInfo.imageColorSpace       = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        scCreateInfo.imageExtent           = { swapchainExtent.width, swapchainExtent.height };
        scCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scCreateInfo.preTransform          = (VkSurfaceTransformFlagBitsKHR)preTransform;
        scCreateInfo.imageArrayLayers      = 1;
        scCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        scCreateInfo.queueFamilyIndexCount = 0;
        scCreateInfo.pQueueFamilyIndices   = NULL;
        scCreateInfo.presentMode           = presentMode;
        scCreateInfo.clipped               = true;
        scCreateInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scCreateInfo.oldSwapchain          = VK_NULL_HANDLE;
        vkCreateSwapchainKHR(dd->falcorVKLogicalDevice, &scCreateInfo, nullptr, &dd->swapchain);

        err = vkGetSwapchainImagesKHR(dd->falcorVKLogicalDevice, dd->swapchain, &dd->imageCount, NULL);
        dd->swapChainImages.resize(dd->imageCount);
        err = vkGetSwapchainImagesKHR(dd->falcorVKLogicalDevice, dd->swapchain, &dd->imageCount, dd->swapChainImages.data());

        // For each of the Swapchain images, create image views out of them.
        dd->FrameDatas.resize(dd->imageCount);
        for (uint32_t i = 0; i < dd->imageCount; i++)
        {
            VkImageViewCreateInfo colorAttachmentView = {};
            colorAttachmentView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorAttachmentView.pNext                           = NULL;
            colorAttachmentView.format                          = VK_FORMAT_B8G8R8A8_UNORM;
            colorAttachmentView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentView.subresourceRange.baseMipLevel   = 0;
            colorAttachmentView.subresourceRange.levelCount     = 1;
            colorAttachmentView.subresourceRange.baseArrayLayer = 0;
            colorAttachmentView.subresourceRange.layerCount     = 1;
            colorAttachmentView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            colorAttachmentView.flags                           = 0;
            colorAttachmentView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

            dd->FrameDatas[i].image   = dd->swapChainImages[i];
            colorAttachmentView.image = dd->FrameDatas[i].image;
            err = vkCreateImageView(dd->falcorVKLogicalDevice, &colorAttachmentView, nullptr, &dd->FrameDatas[i].view);
        }

        return err == VK_SUCCESS;
    }

    bool createInstance(VKDeviceData *devData)
    {
        auto err = VK_SUCCESS;

        // Let's have only Win32 for now.
        std::vector<const char *> extNames;
        extNames.push_back("VK_KHR_surface");
        extNames.push_back("VK_KHR_win32_surface");

        VkInstanceCreateInfo InstanceCreateInfo;
        InstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceCreateInfo.pNext                   = NULL;
        InstanceCreateInfo.pApplicationInfo        = NULL;
        InstanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extNames.size());
        InstanceCreateInfo.ppEnabledExtensionNames = extNames.data();
        InstanceCreateInfo.enabledLayerCount       = 0;
        InstanceCreateInfo.ppEnabledLayerNames     = nullptr;
        err = vkCreateInstance(&InstanceCreateInfo, nullptr, &devData->falcorVKInstance);

        return err == VK_SUCCESS;
    }

    bool createPhysicalDevice(VKDeviceData *dd)
    {
        // First figure out how many devices are in the system.
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(dd->falcorVKInstance, &dd->falcorVKPhysDevCount, nullptr);
        vkEnumeratePhysicalDevices(dd->falcorVKInstance, &dd->falcorVKPhysDevCount, &dd->falcorVKPhysDevice);

        // TODO: Logger class!
        std::cout << "Physical devices: " << physicalDeviceCount;

        vkGetPhysicalDeviceMemoryProperties(dd->falcorVKPhysDevice, &dd->devMemoryProperties);
        vkGetPhysicalDeviceQueueFamilyProperties(dd->falcorVKPhysDevice, &dd->falcorVKQueueFamilyPropsCount, nullptr);
        dd->falcorVKQueueFamilyProps.resize(dd->falcorVKQueueFamilyPropsCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dd->falcorVKPhysDevice, &dd->falcorVKQueueFamilyPropsCount, dd->falcorVKQueueFamilyProps.data());

        // Search for a graphics and a present queue in the array of queue families, try to find one that supports both
        for (uint32_t i = 0; i < dd->falcorVKQueueFamilyPropsCount; i++)
        {
            if ((dd->falcorVKQueueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (dd->graphicsQueueNodeIndex == UINT32_MAX)
                {
                    dd->graphicsQueueNodeIndex = i;
                    break;
                }
            }
        }
        
        return true;
    }

    bool createLogicalDevice(VKDeviceData *dd)
    {
        auto err = VK_SUCCESS;

        VkPhysicalDeviceFeatures requiredFeatures = {};
        vkGetPhysicalDeviceFeatures(dd->falcorVKPhysDevice, &dd->supportedFeatures);
        requiredFeatures.multiDrawIndirect  = dd->supportedFeatures.multiDrawIndirect;
        requiredFeatures.tessellationShader = VK_TRUE;
        requiredFeatures.geometryShader     = VK_TRUE;

        const VkDeviceQueueCreateInfo deviceQueueCreateInfo =
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            dd->graphicsQueueNodeIndex,
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

        err = vkCreateDevice(dd->falcorVKPhysDevice, &deviceCreateInfo, nullptr, &dd->falcorVKLogicalDevice);
        return true;
    }


    // TODO: This function is too small. Directly calling the VK func now.
    bool createDeviceQueue(VKDeviceData *dd)
    {
        vkGetDeviceQueue(dd->falcorVKLogicalDevice, dd->graphicsQueueNodeIndex, 0, &dd->deviceQueue);
        return true;
    }

    bool Device::updateDefaultFBO(uint32_t width, uint32_t height, ResourceFormat colorFormat, ResourceFormat depthFormat)
    {
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
        mpRenderContext.reset();
        mpResourceAllocator.reset();
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
        return nullptr;
    }

    void Device::present()
    {
    }

    bool createSurface(const Window* pWindow, VKDeviceData *dd)
    {
        VkWin32SurfaceCreateInfoKHR createInfo;
        createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd      = pWindow->getApiHandle();
        createInfo.hinstance = GetModuleHandle(nullptr);

        vkCreateWin32SurfaceKHR(dd->falcorVKInstance, &createInfo, nullptr, &dd->surface);
        return true;
    }

    bool Device::init(const Desc& desc)
    {
        DeviceData* pData = new DeviceData;
        VKDeviceData *pVKData = new VKDeviceData;
        mpPrivateData = pVKData;

        if (desc.enableDebugLayer)
        {
            // TODO: add some Vulkan layers here.
        }
        
        bool res = createInstance(pVKData);
        assert(res);

        res = createPhysicalDevice(pVKData);
        assert(res);

        res = createLogicalDevice(pVKData);
        assert(res);

        vkGetDeviceQueue(pVKData->falcorVKLogicalDevice, pVKData->graphicsQueueNodeIndex, 0, &pVKData->deviceQueue);
        assert(res);

        res = createSurface(mpWindow.get(), pVKData);
        assert(res);

        res = createSwapChain(pVKData, mpWindow.get());
        assert(res);

        mpRenderContext = RenderContext::create();

        //pData->pFrameFence = GpuFence::create();
        return true;
    }

    void Device::releaseResource(ApiObjectHandle pResource)
    {
    }

    void Device::executeDeferredReleases()
    {
    }

    Fbo::SharedPtr Device::resizeSwapChain(uint32_t width, uint32_t height)
    {
        return getSwapChainFbo();
    }

    void Device::setVSync(bool enable)
    {

    }

    bool Device::isWindowOccluded() const
    {

        return true;
    }

    bool Device::isExtensionSupported(const std::string& name)
    {
        return _ENABLE_NVAPI;
    }

    // TODO: These are stuff that'll be removed for the Vulkan backend. 
    // They're here for now just to get the code compiled easily.
    void d3dTraceHR(const std::string& msg, HRESULT hr)
    {

    }

    D3D_FEATURE_LEVEL getD3DFeatureLevel(uint32_t majorVersion, uint32_t minorVersion)
    {
        return (D3D_FEATURE_LEVEL)0;
    }
}
