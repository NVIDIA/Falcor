#pragma once

#include "API/vulkan/FalcorVK.h"

bool FalcorVK::Instance(VKDeviceData *devData)
{
	auto err = VK_SUCCESS;

	VkInstanceCreateInfo InstanceCreateInfo;
	InstanceCreateInfo.sType		           = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceCreateInfo.pNext			       = NULL;
	InstanceCreateInfo.pApplicationInfo		   = NULL;
	InstanceCreateInfo.enabledExtensionCount   = 0;
	InstanceCreateInfo.ppEnabledExtensionNames = nullptr;
	InstanceCreateInfo.enabledLayerCount	   = 0;
	InstanceCreateInfo.ppEnabledLayerNames	   = nullptr;
	err = vkCreateInstance(&InstanceCreateInfo, nullptr, &devData->falcorVKInstance);

	return err == VK_SUCCESS;
}

bool FalcorVK::PhysicalDevice(VKDeviceData *dd)
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
	//queueNodeIndex = graphicsQueueNodeIndex;
}

bool FalcorVK::LogicalDevice(VKDeviceData *dd)
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
}

bool FalcorVK::DeviceQueue(VKDeviceData *dd)
{
	vkGetDeviceQueue(dd->falcorVKLogicalDevice, dd->graphicsQueueNodeIndex, 0, &dd->deviceQueue);
	return true;

}