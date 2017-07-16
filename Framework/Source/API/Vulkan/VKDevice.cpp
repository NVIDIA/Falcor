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
#include "API/LowLevel/DescriptorPool.h"
#include "API/LowLevel/GpuFence.h"
#include "API/Vulkan/FalcorVK.h"
#include <set>
#include "Falcor.h"

// #define VK_REPORT_PERF_WARNINGS // Uncomment this to see performance warnings
namespace Falcor
{
#ifdef DEFAULT_ENABLE_DEBUG_LAYER
    VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       pUserData)
    {
        std::string type = "FalcorVK ";
        type += ((flags | VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "Error: " : "Warning: ");
        printToDebugWindow(type + std::string(pMessage) + "\n");
        return VK_FALSE;
    }
#endif

    uint32_t getMaxViewportCount()
    {
        assert(gpDevice);
        return gpDevice->getPhysicalDeviceLimits().maxViewports;
    }

    struct DeviceApiData
    {
        VkSwapchainKHR swapchain;
        VkPhysicalDeviceProperties properties;
        uint32_t falcorToVulkanQueueType[Device::kQueueTypeCount];
        uint32_t falcorToVkMemoryType[(uint32_t)Device::MemoryType::Count];
        VkPhysicalDeviceLimits deviceLimits;
        std::vector<VkExtensionProperties> deviceExtensions;

#ifdef DEFAULT_ENABLE_DEBUG_LAYER
        VkDebugReportCallbackEXT debugReportCallbackHandle;
#endif
    };

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlagBits memFlagBits)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((memProperties.memoryTypes[i].propertyFlags & memFlagBits) == memFlagBits)
            {
                return i;
            }
        }
        return -1;
    }

    static bool initMemoryTypes(VkPhysicalDevice physicalDevice, DeviceApiData* pApiData)
    {
        VkMemoryPropertyFlagBits bits[(uint32_t)Device::MemoryType::Count];
        bits[(uint32_t)Device::MemoryType::Default] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bits[(uint32_t)Device::MemoryType::Upload] = VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        bits[(uint32_t)Device::MemoryType::Readback] = VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

        for(uint32_t i = 0 ; i < arraysize(bits) ; i++)
        {
            pApiData->falcorToVkMemoryType[i] = findMemoryType(physicalDevice, bits[i]);
            if (pApiData->falcorToVkMemoryType[i] == -1)
            {
                logError("Missing memory type " + std::to_string(i));
                return false;
            }
        }
        return true;
    }

    bool Device::getApiFboData(uint32_t width, uint32_t height, ResourceFormat colorFormat, ResourceFormat depthFormat, std::vector<ResourceHandle>& apiHandles, uint32_t& currentBackBufferIndex)
    {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(mApiHandle, mpApiData->swapchain, &imageCount, nullptr);
        assert(imageCount == apiHandles.size());

        std::vector<VkImage> swapchainImages(imageCount);
        vkGetSwapchainImagesKHR(mApiHandle, mpApiData->swapchain, &imageCount, swapchainImages.data());
        for (size_t i = 0; i < swapchainImages.size(); i++)
        {
            apiHandles[i] = ResourceHandle::create(swapchainImages[i], nullptr);
        }

        // Get the back-buffer
        vk_call(vkAcquireNextImageKHR(mApiHandle, mpApiData->swapchain, std::numeric_limits<uint64_t>::max(), mpFrameFence->getApiHandle(), VK_NULL_HANDLE, &currentBackBufferIndex));

        return true;
    }

    void Device::destroyApiObjects()
    {
        PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = VK_NULL_HANDLE;
        DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(mApiHandle, "vkDestroyDebugReportCallbackEXT");
        DestroyDebugReportCallback(mApiHandle, mpApiData->debugReportCallbackHandle, nullptr);
        vkDestroySwapchainKHR(mApiHandle, mpApiData->swapchain, nullptr);
        safe_delete(mpApiData);
    }

    VkInstance createInstance(DeviceApiData* pData, bool enableDebugLayers, uint32_t apiMajorVersion, uint32_t apiMinorVersion)
    {
        // Find out which layers are supported
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> allLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, allLayers.data());

        for (const VkLayerProperties& layer : allLayers)
        {
            logInfo("Available Vulkan Layer: " + std::string(layer.layerName) + " - VK Spec Version: " + std::to_string(layer.specVersion) + " - Implementation Version: " + std::to_string(layer.implementationVersion));
        }

        // Layers to use when creating instance
        std::vector<const char*> layerNames;
        auto findLayer = [allLayers](const std::string& layer) 
        {
            for (const auto& l : allLayers)
            {
                if (std::string(l.layerName) == layer) return true;
            }
            return false;
        };

#define enable_layer_if_present(_layer) if(findLayer(_layer)) layerNames.push_back(_layer)

        if (enableDebugLayers)
        {
            if (!findLayer("VK_LAYER_LUNARG_standard_validation"))
            {
                logError("Can't enable the Vulkan debug layer. Please install the Vulkan SDK or use non-debug device");
                return nullptr;
            }
            layerNames.push_back("VK_LAYER_LUNARG_standard_validation");
            enable_layer_if_present("VK_LAYER_NV_nsight");
        }

        // Enumerate implicitly available extensions. The debug layers above just have VK_EXT_debug_report
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> defaultExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, defaultExtensions.data());

        for (const VkExtensionProperties& extension : defaultExtensions)
        {
            logInfo("Available Instance Extension: " + std::string(extension.extensionName) + " - VK Spec Version: " + std::to_string(extension.specVersion));
        }

        // Extensions to use when creating instance
        std::vector<const char*> extensionNames = { "VK_KHR_surface", "VK_KHR_win32_surface" };
        if (enableDebugLayers)
        {
            extensionNames.push_back("VK_EXT_debug_report");
        }

        if (enableOpenVR)
        {
            dosomething
        }
        VkApplicationInfo appInfo = {};
        appInfo.pEngineName = "Falcor";
        appInfo.engineVersion = VK_MAKE_VERSION(FALCOR_MAJOR_VERSION, FALCOR_MINOR_VERSION, 0);
        appInfo.apiVersion = VK_MAKE_VERSION(apiMajorVersion, apiMinorVersion, 0);

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledLayerCount = (uint32_t)layerNames.size();
        instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
        instanceCreateInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
        instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

        VkInstance instance;
        if (VK_FAILED(vkCreateInstance(&instanceCreateInfo, nullptr, &instance)))
        {
            logError("Failed to create Vulkan instance");
            return nullptr;
        }

        // Hook up callbacks for VK_EXT_debug_report
        if (enableDebugLayers)
        {
            VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
            callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
#ifdef VK_REPORT_PERF_WARNINGS
            callbackCreateInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
#endif
            callbackCreateInfo.pfnCallback = &debugReportCallback;
            callbackCreateInfo.pUserData = nullptr;

            // Function to create a debug callback has to be dynamically queried from the instance...
            PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
            CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

            if (VK_FAILED(CreateDebugReportCallback(instance, &callbackCreateInfo, nullptr, &pData->debugReportCallbackHandle)))
            {
                logWarning("Could not initialize debug report callbacks.");
            }
        }

        return instance;
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

    VkPhysicalDevice initPhysicalDevice(VkInstance instance, DeviceApiData* pData)
    {
        // Enumerate devices
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        assert(count > 0);

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());

        // Pick a device
        VkPhysicalDevice physicalDevice = selectPhysicalDevice(devices);
        vkGetPhysicalDeviceProperties(physicalDevice, &pData->properties);
        pData->deviceLimits = pData->properties.limits;

        // Get queue families and match them to what type they are
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        // Init indices
        for (uint32_t i = 0 ; i < arraysize(pData->falcorToVulkanQueueType) ; i++)
        {
            pData->falcorToVulkanQueueType[i]= (uint32_t)-1;
        }

        // Determine which queue is what type
        uint32_t& graphicsQueueIndex = pData->falcorToVulkanQueueType[(uint32_t)LowLevelContextData::CommandQueueType::Direct];
        uint32_t& computeQueueIndex = pData->falcorToVulkanQueueType[(uint32_t)LowLevelContextData::CommandQueueType::Compute];
        uint32_t& transferQueue = pData->falcorToVulkanQueueType[(uint32_t)LowLevelContextData::CommandQueueType::Copy];

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

        return physicalDevice;
    }

    bool isExtensionSupport(const std::string& str, const std::vector<VkExtensionProperties>& vec)
    {
        for (const auto& s : vec)
        {
            if (str == s.extensionName) return true;
        }
        return false;
    }

    VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, DeviceApiData *pData, const Device::Desc& desc, std::vector<CommandQueueHandle> cmdQueues[Device::kQueueTypeCount])
    {
        // Features
        VkPhysicalDeviceFeatures requiredFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &requiredFeatures);

        // Queues
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        std::vector<std::vector<float>> queuePriorities(arraysize(pData->falcorToVulkanQueueType));

        // Set up info to create queues for each type
        for (uint32_t type = 0; type < arraysize(pData->falcorToVulkanQueueType); type++)
        {
            const uint32_t queueCount = desc.cmdQueues[type];
            queuePriorities[type].resize(queueCount, 1.0f); // Setting all priority at max for now            
            cmdQueues[type].resize(queueCount); // Save how many queues of each type there will be so we can retrieve them easier after device creation

            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueCount = queueCount;
            info.queueFamilyIndex = pData->falcorToVulkanQueueType[type];
            info.pQueuePriorities = queuePriorities[type].data();

            if (info.queueCount > 0)
            {
                queueInfos.push_back(info);
            }
        }

        // Extensions
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        pData->deviceExtensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, pData->deviceExtensions.data());

        for (const VkExtensionProperties& extension : pData->deviceExtensions)
        {
            logInfo("Available Device Extension: " + std::string(extension.extensionName) + " - VK Spec Version: " + std::to_string(extension.specVersion));
        }

        std::vector<const char*> extensionNames = { "VK_KHR_swapchain" };
        assert(isExtensionSupported(extensionNames[0], pData->deviceExtensions));

#ifdef FALCOR_VK
        if (desc.enableVR)
        {
            const auto requiredOpenVRExt = VRSystem::getRequiredVkDeviceExtensions(physicalDevice);
            for (const auto& a : requiredOpenVRExt)
            {
                if(isExtensionSupport(a, pData->deviceExtensions) == false)
                {
                    logError("Can't start OpenVR. Missing device extension " + a);
                }
                else
                {
                    extensionNames.push_back(a.c_str());
                }
            }
        }
#endif

        for (const auto& a : desc.requiredExtensions)
        {
            if (isExtensionSupported(a, pData->deviceExtensions))
            {
                extensionNames.push_back(a.c_str());
            }
            else
            {
                logWarning("The device doesn't support the requested '" + a + "` extension");
            }
        }

        // Logical Device
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = (uint32_t)queueInfos.size();
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
        deviceInfo.ppEnabledExtensionNames = extensionNames.data();
        deviceInfo.pEnabledFeatures = &requiredFeatures;

        VkDevice device;
        if (VK_FAILED(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device)))
        {
            logError("Could not create Vulkan logical device.");
            return nullptr;
        }

        // Get the queues we created
        for (uint32_t type = 0; type < arraysize(pData->falcorToVulkanQueueType); type++)
        {
            for (uint32_t i = 0; i < (uint32_t)cmdQueues[type].size(); i++)
            {
                vkGetDeviceQueue(device, pData->falcorToVulkanQueueType[type], i, &cmdQueues[type][i]);
            }
        }

        return device;
    }

    VkSurfaceKHR createSurface(VkInstance instance, VkPhysicalDevice physicalDevice, DeviceApiData *pData, const Window* pWindow)
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = pWindow->getApiHandle();
        createInfo.hinstance = GetModuleHandle(nullptr);

        VkSurfaceKHR surface;
        if (VK_FAILED(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface)))
        {
            logError("Could not create Vulkan surface.");
            return nullptr;
        }

        VkBool32 supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, pData->falcorToVulkanQueueType[uint32_t(LowLevelContextData::CommandQueueType::Direct)], surface, &supported);
        assert(supported);

        return surface;
    }

    bool Device::createSwapChain(ResourceFormat colorFormat)
    {
        // Select/Validate SwapChain creation settings
        // Surface size
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mApiHandle, mApiHandle, &surfaceCapabilities);
        assert(surfaceCapabilities.supportedUsageFlags & (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));

        VkExtent2D swapchainExtent = {};
        if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
        {
            swapchainExtent.width = mpWindow->getClientAreaWidth();
            swapchainExtent.height = mpWindow->getClientAreaWidth();
        }
        else
        {
            swapchainExtent = surfaceCapabilities.currentExtent;
        }

        // Validate Surface format
        if (isSrgbFormat(colorFormat) == false)
        {
            logError("Can't create a swap-chain with linear-space color format");
            return false;
        }

        const VkFormat requestedFormat = getVkFormat(colorFormat);
        const VkColorSpaceKHR requestedColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mApiHandle, mApiHandle, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mApiHandle, mApiHandle, &formatCount, surfaceFormats.data());

        bool formatValid = false;
        for (const VkSurfaceFormatKHR& format : surfaceFormats)
        {
            if (format.format == requestedFormat && format.colorSpace == requestedColorSpace)
            {
                formatValid = true;
                break;
            }
        }

        if (formatValid == false)
        {
            logError("Requested Swapchain format is not available");
            return false;
        }

        // Select present mode
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(mApiHandle, mApiHandle, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(mApiHandle, mApiHandle, &presentModeCount, presentModes.data());

        // Select present mode, FIFO for VSync, otherwise preferring MAILBOX -> IMMEDIATE -> FIFO
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        if (mVsyncOn == false)
        {
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
        }

        // Swapchain Creation
        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = mApiHandle;
        info.minImageCount = clamp(kSwapChainBuffers, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
        info.imageFormat = requestedFormat;
        info.imageColorSpace = requestedColorSpace;
        info.imageExtent = { swapchainExtent.width, swapchainExtent.height };
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        info.preTransform = surfaceCapabilities.currentTransform;
        info.imageArrayLayers = 1;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;     // Only needed if VK_SHARING_MODE_CONCURRENT
        info.pQueueFamilyIndices = nullptr; // Only needed if VK_SHARING_MODE_CONCURRENT
        info.presentMode = presentMode;
        info.clipped = true;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.oldSwapchain = VK_NULL_HANDLE;

        if (VK_FAILED(vkCreateSwapchainKHR(mApiHandle, &info, nullptr, &mpApiData->swapchain)))
        {
            logError("Could not create swapchain.");
            return false;
        }

        return true;
    }

    void Device::apiPresent()
    {
        VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        info.swapchainCount = 1;
        info.pSwapchains = &mpApiData->swapchain;
        info.pImageIndices = &mCurrentBackBufferIndex;
        vk_call(vkQueuePresentKHR(mpRenderContext->getLowLevelData()->getCommandQueue(), &info));

        // Get the next back-buffer
        vk_call(vkAcquireNextImageKHR(mApiHandle, mpApiData->swapchain, std::numeric_limits<uint64_t>::max(), mpFrameFence->getApiHandle(), VK_NULL_HANDLE, &mCurrentBackBufferIndex));        
    }

    bool Device::apiInit(const Desc& desc)
    {
        mpApiData = new DeviceApiData;
        VkInstance instance = createInstance(mpApiData, desc.enableDebugLayer, desc.apiMajorVersion, desc.apiMinorVersion);
        if (!instance) return false;
        VkPhysicalDevice physicalDevice = initPhysicalDevice(instance, mpApiData);
        if (!physicalDevice) return false;
        VkSurfaceKHR surface = createSurface(instance, physicalDevice, mpApiData, mpWindow.get());
        if (!surface) return false;
        VkDevice device = createLogicalDevice(this, physicalDevice, mpApiData, desc, mCmdQueues);
        if (!device) return false;
        if (initMemoryTypes(physicalDevice, mpApiData) == false) return false;

        mApiHandle = DeviceHandle::create(instance, physicalDevice, device, surface);
        mGpuTimestampFrequency = getPhysicalDeviceLimits().timestampPeriod / (1000 * 1000);
        return true;
    }

    void Device::apiResizeSwapChain(uint32_t width, uint32_t height, ResourceFormat colorFormat)
    {
        vkDestroySwapchainKHR(mApiHandle, mpApiData->swapchain, nullptr);
        createSwapChain(colorFormat);
    }

    bool Device::isWindowOccluded() const
    {
        // #VKTODO Is there a test for it?
        return false;
    }

    bool Device::isExtensionSupported(const std::string& name) const
    {
        for (const auto& ext : mpApiData->deviceExtensions)
        {
            if (name == ext.extensionName) return true;
        }
        return false;
    }

    ApiCommandQueueType Device::getApiCommandQueueType(LowLevelContextData::CommandQueueType type) const
    {
        return mpApiData->falcorToVulkanQueueType[(uint32_t)type];
    }

    uint32_t Device::getVkMemoryType(MemoryType falcorType) const
    {
        return mpApiData->falcorToVkMemoryType[(uint32_t)falcorType];
    }

    const VkPhysicalDeviceLimits& Device::getPhysicalDeviceLimits() const
    {
        return mpApiData->deviceLimits;
    }
}
