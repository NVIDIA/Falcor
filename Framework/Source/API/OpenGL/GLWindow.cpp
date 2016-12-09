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
#ifdef FALCOR_GL
#include "..\Window.h"
#include "Sample.h"
#include "Utils/UserInput.h"
#include "Utils/OS.h"
#include "API/FBO.h"
#include "API/Texture.h"
#include <glm/vec2.hpp>

#define GLFW_DLL
#include "glfw3.h"

namespace Falcor
{
    // GLFW callbacks doesn't have user data
    static std::map<void*, Window*> gWindowMap;

    class ApiCallbacks
    {
    public:
        static void framebufferSizeChangeCallback(GLFWwindow* pGlfwWindow, int width, int height)
        {
            // We also get here in case the window was minimized, so need to ignore it
            if(width == 0 || height == 0)
                return;


            Window* pWindow = gWindowMap[pGlfwWindow];

            // Resize the default FBO object
            auto pFBO = pWindow->getDefaultFBO();
            if(pFBO->getWidth() != width || pFBO->getHeight() != height)
            {
                uint32_t sampleCount = pFBO->getSampleCount();
                ResourceFormat colorFormat = pFBO->getColorTexture(0)->getFormat();
                ResourceFormat depthFormat = pFBO->getDepthStencilTexture()->getFormat();
                pWindow->updateDefaultFBO(width, height, sampleCount, colorFormat, depthFormat);
                pWindow->mpCallbacks->handleFrameBufferSizeChange(pWindow->getDefaultFBO());
            }
        }

        static void keyboardCallback(GLFWwindow* pGlfwWin, int key, int scanCode, int action, int modifiers)
        {
            KeyboardEvent event;
            if(prepareKeyboardEvent(key, action, modifiers, event))
            {
                Window* pWindow = gWindowMap[pGlfwWin];
                pWindow->mpCallbacks->handleKeyboardEvent(event);
            }
        }

        static void mouseMoveCallback(GLFWwindow* pGlfwWin, double mouseX, double mouseY)
        {
            Window* pWindow = gWindowMap[pGlfwWin];

            // Prepare the mouse data
            MouseEvent event;
            event.type = MouseEvent::Type::Move;
            event.pos = calcMousePos(mouseX, mouseY, pWindow->getMouseScale());
            event.wheelDelta = glm::vec2(0, 0);

           pWindow->mpCallbacks->handleMouseEvent(event);
        }

        static void mouseButtonCallback(GLFWwindow* pGlfwWin, int button, int action, int modifiers)
        {
            MouseEvent event;
            // Prepare the mouse data
            switch(button)
            {
            case GLFW_MOUSE_BUTTON_LEFT:
                event.type = (action == GLFW_PRESS) ? MouseEvent::Type::LeftButtonDown : MouseEvent::Type::LeftButtonUp;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                event.type = (action == GLFW_PRESS) ? MouseEvent::Type::MiddleButtonDown : MouseEvent::Type::MiddleButtonUp;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                event.type = (action == GLFW_PRESS) ? MouseEvent::Type::RightButtonDown : MouseEvent::Type::RightButtonUp;
                break;
            default:
                // Other keys are not supported
                break;
            }

            Window* pWindow = gWindowMap[pGlfwWin];

            // Modifiers
            event.mods = createInputModifiersFromGlfwMask(modifiers);
            double x, y;
            glfwGetCursorPos(pGlfwWin, &x, &y);
            event.pos = calcMousePos(x, y, pWindow->getMouseScale());

            pWindow->mpCallbacks->handleMouseEvent(event);
        }

        static void mouseWheelCallback(GLFWwindow* pGlfwWin, double scrollX, double scrollY)
        {
            Window* pWindow = gWindowMap[pGlfwWin];

            MouseEvent event;
            event.type = MouseEvent::Type::Wheel;
            double x, y;
            glfwGetCursorPos(pGlfwWin, &x, &y);
            event.pos = calcMousePos(x, y, pWindow->getMouseScale());
            event.wheelDelta = (glm::vec2(float(scrollX), float(scrollY)));

            pWindow->mpCallbacks->handleMouseEvent(event);
        }

    private:
        static inline KeyboardEvent::Key glfwToFalcorKey(int glfw)
        {
            static_assert(GLFW_KEY_ESCAPE == 256, "GLFW_KEY_ESCAPE is expected to be 256");
            if(glfw < GLFW_KEY_ESCAPE)
            {
                // Printable keys are expected to have the same value
                return (KeyboardEvent::Key)glfw;
            }

            switch(glfw)
            {
            case GLFW_KEY_ESCAPE:
                return KeyboardEvent::Key::Escape;
            case GLFW_KEY_ENTER:
                return KeyboardEvent::Key::Enter;
            case GLFW_KEY_TAB:
                return KeyboardEvent::Key::Tab;
            case GLFW_KEY_BACKSPACE:
                return KeyboardEvent::Key::Backspace;
            case GLFW_KEY_INSERT:
                return KeyboardEvent::Key::Insert;
            case GLFW_KEY_DELETE:
                return KeyboardEvent::Key::Del;
            case GLFW_KEY_RIGHT:
                return KeyboardEvent::Key::Right;
            case GLFW_KEY_LEFT:
                return KeyboardEvent::Key::Left;
            case GLFW_KEY_DOWN:
                return KeyboardEvent::Key::Down;
            case GLFW_KEY_UP:
                return KeyboardEvent::Key::Up;
            case GLFW_KEY_PAGE_UP:
                return KeyboardEvent::Key::PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return KeyboardEvent::Key::PageDown;
            case GLFW_KEY_HOME:
                return KeyboardEvent::Key::Home;
            case GLFW_KEY_END:
                return KeyboardEvent::Key::End;
            case GLFW_KEY_CAPS_LOCK:
                return KeyboardEvent::Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK:
                return KeyboardEvent::Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK:
                return KeyboardEvent::Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN:
                return KeyboardEvent::Key::PrintScreen;
            case GLFW_KEY_PAUSE:
                return KeyboardEvent::Key::Pause;
            case GLFW_KEY_F1:
                return KeyboardEvent::Key::F1;
            case GLFW_KEY_F2:
                return KeyboardEvent::Key::F2;
            case GLFW_KEY_F3:
                return KeyboardEvent::Key::F3;
            case GLFW_KEY_F4:
                return KeyboardEvent::Key::F4;
            case GLFW_KEY_F5:
                return KeyboardEvent::Key::F5;
            case GLFW_KEY_F6:
                return KeyboardEvent::Key::F6;
            case GLFW_KEY_F7:
                return KeyboardEvent::Key::F7;
            case GLFW_KEY_F8:
                return KeyboardEvent::Key::F8;
            case GLFW_KEY_F9:
                return KeyboardEvent::Key::F9;
            case GLFW_KEY_F10:
                return KeyboardEvent::Key::F10;
            case GLFW_KEY_F11:
                return KeyboardEvent::Key::F11;
            case GLFW_KEY_F12:
                return KeyboardEvent::Key::F12;
            case GLFW_KEY_KP_0:
                return KeyboardEvent::Key::Keypad0;
            case GLFW_KEY_KP_1:
                return KeyboardEvent::Key::Keypad1;
            case GLFW_KEY_KP_2:
                return KeyboardEvent::Key::Keypad2;
            case GLFW_KEY_KP_3:
                return KeyboardEvent::Key::Keypad3;
            case GLFW_KEY_KP_4:
                return KeyboardEvent::Key::Keypad4;
            case GLFW_KEY_KP_5:
                return KeyboardEvent::Key::Keypad5;
            case GLFW_KEY_KP_6:
                return KeyboardEvent::Key::Keypad6;
            case GLFW_KEY_KP_7:
                return KeyboardEvent::Key::Keypad7;
            case GLFW_KEY_KP_8:
                return KeyboardEvent::Key::Keypad8;
            case GLFW_KEY_KP_9:
                return KeyboardEvent::Key::Keypad9;
            case GLFW_KEY_KP_DECIMAL:
                return KeyboardEvent::Key::KeypadDel;
            case GLFW_KEY_KP_DIVIDE:
                return KeyboardEvent::Key::KeypadDivide;
            case GLFW_KEY_KP_MULTIPLY:
                return KeyboardEvent::Key::KeypadMultiply;
            case GLFW_KEY_KP_SUBTRACT:
                return KeyboardEvent::Key::KeypadSubtract;
            case GLFW_KEY_KP_ADD:
                return KeyboardEvent::Key::KeypadAdd;
            case GLFW_KEY_KP_ENTER:
                return KeyboardEvent::Key::KeypadEnter;
            case GLFW_KEY_KP_EQUAL:
                return KeyboardEvent::Key::KeypadEqual;
            case GLFW_KEY_LEFT_SHIFT:
                return KeyboardEvent::Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL:
                return KeyboardEvent::Key::LeftControl;
            case GLFW_KEY_LEFT_ALT:
                return KeyboardEvent::Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER:
                return KeyboardEvent::Key::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:
                return KeyboardEvent::Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL:
                return KeyboardEvent::Key::RightControl;
            case GLFW_KEY_RIGHT_ALT:
                return KeyboardEvent::Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER:
                return KeyboardEvent::Key::RightSuper;
            case GLFW_KEY_MENU:
                return KeyboardEvent::Key::Menu;
            default:
                should_not_get_here();
                return (KeyboardEvent::Key)0;
            }
        }

        static inline InputModifiers createInputModifiersFromGlfwMask(int mask)
        {
            InputModifiers mods;
            mods.isAltDown = (mask & GLFW_MOD_ALT) != 0;
            mods.isCtrlDown = (mask & GLFW_MOD_CONTROL) != 0;
            mods.isShiftDown = (mask & GLFW_MOD_SHIFT) != 0;
            return mods;
        }

        static inline glm::vec2 calcMousePos(double xPos, double yPos, const glm::vec2& mouseScale)
        {
            glm::vec2 pos = glm::vec2(float(xPos), float(yPos));
            pos *= mouseScale;
            return pos;
        }

        static inline bool prepareKeyboardEvent(int key, int action, int modifiers, KeyboardEvent& event)
        {
            if(action == GLFW_REPEAT)
            {
                return false;
            }

            event.type = (action == GLFW_RELEASE ? KeyboardEvent::Type::KeyReleased : KeyboardEvent::Type::KeyPressed);

            event.key = glfwToFalcorKey(key);
            event.mods = createInputModifiersFromGlfwMask(modifiers);
            return true;
        }
    };

    void errorFunction(int errorCode, const char* pDescription)
    {
        Logger::log(Logger::Level::Fatal, pDescription);
    }
    
    static std::vector<std::string> gRequiredExtensions =
    {
        "GL_NV_uniform_buffer_unified_memory",
        "GL_ARB_bindless_texture",
        "GL_EXT_texture_filter_anisotropic",
        "GL_NV_shader_buffer_load"
    };

    bool checkExtensionSupport(const std::string& name)
    {
        return glfwExtensionSupported(name.c_str()) == GL_TRUE;
    }

    static bool checkRequiredExtensionsSupport(const std::vector<std::string>& userExtensions)
    {
        std::string s = "Falcor requires the following extensions, which are missing:\n";
        bool foundAll = true;
        gRequiredExtensions.insert(gRequiredExtensions.end(), userExtensions.begin(), userExtensions.end());
        for(const auto& ext : gRequiredExtensions)
        {
            if(glfwExtensionSupported(ext.c_str()) == false)
            {
                foundAll = false;
                s += "\t" + ext + "\n";
            }
        }

        if(foundAll == false)
        {
            Logger::log(Logger::Level::Fatal, s);
        }
        return foundAll;
    }

    // Static functions
    static void GLAPIENTRY glDebugCallback(GLenum source, GLenum Type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* pUserParam)
    {
        Logger::Level level = Logger::Level::Warning;

        switch(severity)
        {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            level = Logger::Level::Info;
            break;
        case GL_DEBUG_SEVERITY_LOW:
            level = Logger::Level::Warning;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            level = (Type == GL_DEBUG_TYPE_PERFORMANCE) ? Logger::Level::Warning : Logger::Level::Error;
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            level = Logger::Level::Error;
            break;
        }
        std::string s("OpenGL debug callback:\n");
        s += msg;
        Logger::log(level, s);
    }

    GLFWwindow* createWindowAndContext(const Window::Desc& desc)
    {
        bool useSrgb = isSrgbFormat(desc.swapChainDesc.colorFormat);
        GLFWmonitor* pMonitor = desc.fullScreen ? glfwGetPrimaryMonitor() : nullptr;
        glfwWindowHint(GLFW_SAMPLES, desc.swapChainDesc.sampleCount);
        glfwWindowHint(GLFW_SRGB_CAPABLE, useSrgb);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, desc.apiMajorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, desc.apiMinorVersion);
        glfwWindowHint(GLFW_RESIZABLE, desc.resizableWindow);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // Block legacy API.
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, desc.useDebugContext);
#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

        GLFWwindow* pWindow = glfwCreateWindow(desc.swapChainDesc.width, desc.swapChainDesc.height, desc.title.c_str(), pMonitor, nullptr);

        if(pWindow)
        {
            glfwMakeContextCurrent(pWindow);
            if(checkRequiredExtensionsSupport(desc.requiredExtensions) == false)
            {
                glfwDestroyWindow(pWindow);
                return nullptr;
            }

            if(useSrgb)
            {
                gl_call(glEnable(GL_FRAMEBUFFER_SRGB));
            }
        }

        return pWindow;
    }

    Window::Window(ICallbacks* pCallbacks) : mpCallbacks(pCallbacks)
    {
    }

    Window::~Window()
    {
        // Can't destroy the context before the destructor is called. The base class destructor will want to release it's graphics resources and so need the context alive
        glfwDestroyWindow((GLFWwindow*)mpPrivateData);
        mpPrivateData = nullptr;
        if(gWindowMap.size() == 0)
        {
            glfwTerminate();
        }
    }

    void Window::shutdown()
    {
        glfwSetWindowShouldClose((GLFWwindow*)mpPrivateData, true);
    }

    bool verifyContextParameters(const Window::Desc& windowDesc)
    {
#define bits_error(name, attachment, channel, expected) \
    gl_call(glGetNamedFramebufferAttachmentParameteriv(0, attachment, channel, &bits)); \
    if(bits != expected) \
    {             \
        logError(std::string("Deafult framebuffer ") + name + " channel has " + std::to_string(bits) + " bits. Expecting " + std::to_string(expected) + " bits.");  \
        return false;   \
    }

        GLint bits;
        bits_error("red",   GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, 8);
        bits_error("green", GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, 8);
        bits_error("blue",  GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,  8);
        bits_error("alpha", GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, 8);
        bits_error("depth", GL_DEPTH,     GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, 24);
        bits_error("stencil", GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, 8);
#undef bits_error

        // Check sample count
        uint32_t sampleCount;
        gl_call(glGetIntegerv(GL_SAMPLES, (GLint*)&sampleCount));
        if(sampleCount != windowDesc.swapChainDesc.sampleCount)
        {
            logError("Can't find a framebuffer configuation which supports " + std::to_string(windowDesc.swapChainDesc.sampleCount) + " samples");
            return false;
        }

        // I really want to check sRGB, but there's a driver in the bug. It will always report GL_LINEAR for the default framebuffer.
        // Not to worry, there's another bug. glEnable(GL_FRAMEBUFFER_SRGB) always forces sRGB default framebuffer, even if it was requested linear.
//         GLint Encoding;
//         gl_call(glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &Encoding));
//         bool bSrgb = (Encoding == GL_SRGB);
//         if(bSrgb != WindowDesc.SwapChainDesc.bSrgb)
//         {
//            logError(std::string("Can't create a ") + (WindowDesc.SwapChainDesc.bSrgb ? "sRGB" : "linear") + " framebuffer.");
//            return false;
//         }

        return true;
    }

    Window::UniquePtr Window::create(const Desc& desc, ICallbacks* pCallbacks)
    {
        // Set error callback
        glfwSetErrorCallback(errorFunction);
        // Init GLFW
        if(!glfwInit())
        {
            logError("GLFW initialization failed");
            return nullptr;
        }

        auto pWindow = UniquePtr(new Window(pCallbacks));
        assert(pWindow);

        // create the window
        pWindow->mpPrivateData = createWindowAndContext(desc);

        if(pWindow->mpPrivateData == nullptr)
        {
            logError("Failed to create a window");
            return nullptr;
        }

        gWindowMap[pWindow->mpPrivateData] = pWindow.get();

        // Initialize GLEW
        glewExperimental = true;
        if(glewInit() != GLEW_OK)
        {
            logError("GLEW initialization failed");
            return nullptr;
        }

        // Set the debug callback
#ifdef _DEBUG
        glDebugMessageCallback(glDebugCallback, pWindow.get());
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glEnable(GL_DEBUG_OUTPUT);
#endif

        // Clear the GL error flag
        glGetError();

        // GLFW treats some configuration options as recommendation. Falcor doesn't. Make sure that we got what we requested. We do it here since we need GLEW and the debug layer
        if(verifyContextParameters(desc) == false)
        {
            return nullptr;
        }

        // Get the actual window size. Might be different if the requested size is larger than the screen size
        uint32_t w, h;
        glfwGetFramebufferSize((GLFWwindow*)pWindow->mpPrivateData, (int*)&w, (int*)&h);
        // Just making sure that the framebuffer size is correct. GLFW documentation is confusing - glfwCreateWindow() get window width/height, which is ambigous (is it entire window or client area?).
        if((w != desc.swapChainDesc.width) || (h != desc.swapChainDesc.height))
        {
            logWarning("Size of glfw framebuffer does not match the requested one!");
        }


        // create the framebuffer. Need to create the color texture here since Texture is a friend of CWindow
        pWindow->updateDefaultFBO(w, h, desc.swapChainDesc.sampleCount,desc.swapChainDesc.colorFormat, desc.swapChainDesc.depthFormat);

        // Set clip-controls to match DX
        gl_call(glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE));

        return pWindow;
    }

    void Window::updateDefaultFBO(uint32_t width, uint32_t height, uint32_t sampleCount, ResourceFormat colorFormat, ResourceFormat depthFormat)
    {
        releaseDefaultFboResources();
        auto pColorTex = Texture::SharedPtr(new Texture(width, height, 1, 1, 1, sampleCount, colorFormat, sampleCount > 1 ? Texture::Type::Texture2DMultisample : Texture::Type::Texture2D));
        auto pDepthTex = Texture::SharedPtr(new Texture(width, height, 1, 1, 1, sampleCount, depthFormat, sampleCount > 1 ? Texture::Type::Texture2DMultisample : Texture::Type::Texture2D));
        attachDefaultFboResources(pColorTex, pDepthTex);
        mMouseScale.x = 1 / float(width);
        mMouseScale.y = 1 / float(height);
    }

    void Window::setVSync(bool enable)
    {
        glfwSwapInterval(enable ? 1 : 0);
    }

    void Window::resize(uint32_t width, uint32_t height)
    {
        if(mpDefaultFBO->getWidth() != width || mpDefaultFBO->getHeight() != height)
        {
            GLFWwindow* pWindow = (GLFWwindow*)mpPrivateData;
            glfwSetWindowSize(pWindow, (int)width, (int)height);

            // We assume that the a screen-coordinate has a 1:1 mapping with pixels. The next assertion verifies it.
            int w, h;
            glfwGetFramebufferSize(pWindow, &w, &h);
            if((w != width) || (h != height))
            {
                logWarning("Size of glfw framebuffer does not match the requested one!");
            }
        }
    }

    void Window::msgLoop()
    {
        PROFILE(msgLoop);
        // Set glfw callbacks
        GLFWwindow* pWindow = (GLFWwindow*)mpPrivateData;
        glfwSetFramebufferSizeCallback(pWindow, &ApiCallbacks::framebufferSizeChangeCallback);
        glfwSetKeyCallback(pWindow, &ApiCallbacks::keyboardCallback);
        glfwSetMouseButtonCallback(pWindow, &ApiCallbacks::mouseButtonCallback);
        glfwSetCursorPosCallback(pWindow, &ApiCallbacks::mouseMoveCallback);
        glfwSetScrollCallback(pWindow, &ApiCallbacks::mouseWheelCallback);

        // Clear the GL error flag
        glGetError();

        while(glfwWindowShouldClose(pWindow) == false)
        {
            glGetError();
            mpCallbacks->renderFrame();
            {
                PROFILE(present);
                glfwSwapBuffers(pWindow);
            }
            glfwPollEvents();
        }
    }
    
    void Window::pollForEvents()
    {
        glfwPollEvents();
    }

    void Window::setWindowTitle(std::string title)
    {
        GLFWwindow* pWindow = (GLFWwindow*)mpPrivateData;
        glfwSetWindowTitle(pWindow, title.c_str());
    }
}
#endif //#ifdef FALCOR_GL
