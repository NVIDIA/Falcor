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
#include "Core/Window.h"
#include "core/Texture.h"
#include "Core/FBO.h"
#include "Core/RenderContext.h"
#include "Core/GpuFence.h"
#include "Core/CopyContext.h"

namespace Falcor
{
    class Device
    {
    public:
        using SharedPtr = std::shared_ptr<Device>;
        using SharedConstPtr = std::shared_ptr<const Device>;
        using ApiHandle = DeviceHandle;

		/** Device configuration
		*/
        struct Desc
        {
			ResourceFormat colorFormat = ResourceFormat::RGBA8UnormSrgb;    ///< The color buffer format
			ResourceFormat depthFormat = ResourceFormat::D24UnormS8;        ///< The depth buffer format
            int apiMajorVersion = DEFAULT_API_MAJOR_VERSION; ///< Requested API major version. Context creation fails if this version is not supported.
            int apiMinorVersion = DEFAULT_API_MINOR_VERSION; ///< Requested API minor version. Context creation fails if this version is not supported.
            bool useDebugContext = false;             ///< create a debug context. NOTE: Debug configuration always creates a debug context
            std::vector<std::string> requiredExtensions; ///< Extensions required by the sample
			bool enableVsync = false;           ///< Controls vertical-sync
        };

		/** Create a new device.
		\param[in] pWindow a previously-created window object
		\param[in] desc Device configuration desctiptor.
		\return nullptr if the function failed, otherwise a new device object
		*/
		static SharedPtr create(Window::SharedPtr& pWindow, const Desc& desc);

		/** Destructor
		*/
        ~Device();

		/** Enable/disable vertical sync
		*/
		void setVSync(bool enable);

		/** Check if the window is occluded
		*/
		bool isWindowOccluded() const;

		/** Check if the device support an extension
		*/
		static bool isExtensionSupported(const std::string & name);

		/** Get the FBO object associated with the swap-chain.
			This can change each frame, depending on the API used
		*/
        Fbo::SharedPtr getSwapChainFbo() const;

		/** Get the default render-context.
			The default render-context is managed completly by the device. The user should just queue commands into it, the device will take care of allocation, submission and synchronization
		*/
		RenderContext::SharedPtr getRenderContext() const { return mpRenderContext; }

        /** Get the copy context
        */
        CopyContext::SharedPtr getCopyContext() const { return mpCopyContext; }

		/** Get the native API handle
		*/
		DeviceHandle getApiHandle() { return mApiHandle; }

		/** Get the global frame fence object
			DISABLED_FOR_D3D12 Device should be a singleton
		*/
		GpuFence::SharedPtr getFrameGpuFence() { return mpFrameFence; }

		/** Present the back-buffer to the window
		*/
		void present();

		/** Check if vertical sync is enabled
		*/
		bool isVsyncEnabled() const { return mVsyncOn; }
    private:
		Device(Window::SharedPtr pWindow) : mpWindow(pWindow) {}
		bool init(const Desc& desc);
        bool updateDefaultFBO(uint32_t width, uint32_t height, uint32_t sampleCount, ResourceFormat colorFormat, ResourceFormat depthFormat);

		Window::SharedPtr mpWindow;
		ApiHandle mApiHandle;
		GpuFence::SharedPtr mpFrameFence;
		void* mpPrivateData;
		RenderContext::SharedPtr mpRenderContext;
        CopyContext::SharedPtr mpCopyContext;
		bool mVsyncOn;
	};

    extern Device::SharedPtr gpDevice;
    bool checkExtensionSupport(const std::string& name);
}