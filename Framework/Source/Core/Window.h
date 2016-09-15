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
#include "Utils/UserInput.h"
#include <glm/vec2.hpp>
#include "core/Texture.h"
#include "Core/FBO.h"

namespace Falcor
{
    /** Swap-chain description
    */
    struct SwapChainDesc
    {
        uint32_t width = 1920;           ///< The width of the swap-chain.
        uint32_t height = 1080;          ///< The height of the swap-chain.
        uint32_t sampleCount = 0;        ///< Requested sample count. 
        ResourceFormat colorFormat = ResourceFormat::RGBA8UnormSrgb;    ///< The color buffer format
        ResourceFormat depthFormat = ResourceFormat::D24UnormS8;        ///< The depth buffer format
    };

    class Window
    {
    public:
        using UniquePtr = std::unique_ptr<Window>;
        using UniqueConstPtr = std::unique_ptr<const Window>;

        struct Desc
        {
            SwapChainDesc swapChainDesc;
            bool fullScreen = false;               ///< Set to true to run the sample in full-screen mode
            std::string title = "Falcor Sample";    ///< Window title
            int apiMajorVersion = DEFAULT_API_MAJOR_VERSION; ///< Requested API major version. Context creation fails if this version is not supported.
            int apiMinorVersion = DEFAULT_API_MINOR_VERSION; ///< Requested API minor version. Context creation fails if this version is not supported.
            bool resizableWindow = false;          ///< Allow the user to resize the window.
            bool useDebugContext = false;             ///< create a debug context. NOTE: Debug configuration always creates a debug context
            std::vector<std::string> requiredExtensions; ///< Extensions required by the sample
        };

        class ICallbacks
        {
        public:
            virtual void handleFrameBufferSizeChange(const Fbo::SharedPtr& pFBO) = 0;
            virtual void renderFrame() = 0;
            virtual void handleKeyboardEvent(const KeyboardEvent& keyEvent) = 0;
            virtual void handleMouseEvent(const MouseEvent& mouseEvent) = 0;
        };

        static UniquePtr create(const Desc& desc, ICallbacks* pCallbacks);
        ~Window();
        void shutdown();

        void setVSync(bool enable);
        void resize(uint32_t width, uint32_t height);
        void msgLoop();
        void pollForEvents();
        void setWindowTitle(std::string title);

        Fbo::SharedPtr getDefaultFBO() const { return mpDefaultFBO; }

    private:
        friend class ApiCallbacks;

        glm::vec2 mMouseScale;
        Window(ICallbacks* pCallbacks);
        void* mpPrivateData = nullptr;
        Fbo::SharedPtr mpDefaultFBO;
        void releaseDefaultFboResources();
        void attachDefaultFboResources(const Texture::SharedPtr& pColorTexture, const Texture::SharedPtr& pDepthTexture);
        void updateDefaultFBO(uint32_t width, uint32_t height, uint32_t sampleCount, ResourceFormat colorFormat, ResourceFormat depthFormat);
        const glm::vec2& getMouseScale() const { return mMouseScale; }
        ICallbacks* mpCallbacks = nullptr;
    };

    bool checkExtensionSupport(const std::string& name);
}