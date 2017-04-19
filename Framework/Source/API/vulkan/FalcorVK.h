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
#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <vulkan.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_PROTOTYPES

// TODO: move to the below namespace!
// TODO: I don't like the names falcorVKXXX. Try something better.
struct VKDeviceData
{
	VkInstance		 falcorVKInstance;
	VkPhysicalDevice falcorVKPhysDevice;
	VkDevice	     falcorVKLogicalDevice;
	uint32_t		 falcorVKPhysDevCount;
	uint32_t		 falcorVKQueueFamilyPropsCount;

	VkPhysicalDeviceMemoryProperties devMemoryProperties;
	uint32_t						 graphicsQueueNodeIndex;
	uint32_t						 presentQueueNodeIndex;
	VkPhysicalDeviceFeatures		 supportedFeatures;
	VkQueue							 deviceQueue;


	std::vector<VkQueueFamilyProperties> falcorVKQueueFamilyProps;

	//static VkImageView		imageView;
	//static VkBuffer buffer = VK_NULL_HANDLE;
	//static VkDeviceMemory imageMemory;
	//static VkDeviceMemory bufferMemory;
	//static VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
};

namespace FalcorVK
{
	bool Instance(VKDeviceData *dd);
	bool PhysicalDevice(VKDeviceData *dd);
	bool LogicalDevice(VKDeviceData *dd);
	bool DeviceQueue(VKDeviceData *dd);




	

}