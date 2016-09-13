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
#ifdef FALCOR_D3D11
#include "Core\Window.h"
#include "Utils/UserInput.h"
#include "Utils/OS.h"
#include <codecvt>
#include <algorithm>
#include "Core/texture.h"
#include "Core/FBO.h"
#include <Initguid.h>
#include <Windowsx.h>

namespace Falcor
{
    ID3D11DevicePtr gpD3D11Device = nullptr;
    ID3D11DeviceContextPtr gpD3D11ImmediateContext = nullptr;

    struct DxWindowData
    {
        Window* pWindow = nullptr;
        HWND hWnd = nullptr;
        IDXGISwapChainPtr pSwapChain = nullptr;
        uint32_t syncInterval = 0;
        bool isWindowOccluded = false;
    };
    
    static std::wstring string_2_wstring(const std::string& s)
    {
        std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
        std::wstring ws = cvt.from_bytes(s);
        return ws;
    }

    void dx11TraceHR(const std::string& msg, HRESULT hr)
    {
        char hr_msg[512];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);

        std::string error_msg = msg + ". Error " + hr_msg;
        Logger::log(Logger::Level::Fatal, error_msg);
    }

    D3D_FEATURE_LEVEL getD3DFeatureLevel(uint32_t majorVersion, uint32_t minorVersion)
    {
        if(majorVersion == 11)
        {
            switch(minorVersion)
            {
            case 0:
                return D3D_FEATURE_LEVEL_11_1;
            case 1:
                return D3D_FEATURE_LEVEL_11_0;
            }
        }
        else if(majorVersion == 10)
        {
            switch(minorVersion)
            {
            case 0:
                return D3D_FEATURE_LEVEL_10_0;
            case 1:
                return D3D_FEATURE_LEVEL_10_1;
            }
        }
        else if(majorVersion == 9)
        {
            switch(minorVersion)
            {
            case 1:
                return D3D_FEATURE_LEVEL_9_1;
            case 2:
                return D3D_FEATURE_LEVEL_9_2;
            case 3:
                return D3D_FEATURE_LEVEL_9_3;
            }
        }
        return (D3D_FEATURE_LEVEL)0;
    }

    class ApiCallbacks
    {
    public:
        static LRESULT CALLBACK msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            const DxWindowData* pWinData;

            if(msg == WM_CREATE)
            {
                CREATESTRUCT* pCreateStruct = (CREATESTRUCT*)lParam;
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pCreateStruct->lpCreateParams);
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            else
            {
                pWinData = (DxWindowData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                switch(msg)
                {
                case WM_CLOSE:
                    DestroyWindow(hwnd);
                    return 0;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
                case WM_SIZE:
                    if(wParam != SIZE_MINIMIZED)
                    {
                        resizeWindow(pWinData);
                    }
                    break;
                case WM_KEYDOWN:
                case WM_KEYUP:
                    dispatchKeyboardEvent(pWinData, wParam, msg == WM_KEYDOWN);
                    return 0;
                case WM_MOUSEMOVE:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MOUSEWHEEL:
                    dispatchMouseEvent(pWinData, msg, wParam, lParam);
                    return 0;
                }
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
        }

    private:
        static void resizeWindow(const DxWindowData* pWinData)
        {
            RECT r;
            GetClientRect(pWinData->hWnd, &r);
            uint32_t width = r.right - r.left;
            uint32_t height = r.bottom - r.top;

            Window* pWindow = pWinData->pWindow;
            auto pDefaultFBO = pWindow->getDefaultFBO();

            if(width != pDefaultFBO->getWidth() || height != pDefaultFBO->getHeight())
            {
                uint32_t sampleCount = pDefaultFBO->getSampleCount();
                ResourceFormat colorFormat = pDefaultFBO->getColorTexture(0)->getFormat();
                ResourceFormat depthFormat = pDefaultFBO->getDepthStencilTexture()->getFormat();
                pWindow->updateDefaultFBO(width, height, sampleCount, colorFormat, depthFormat);

                // Handle messages
                pWindow->mpCallbacks->handleFrameBufferSizeChange(pDefaultFBO);
            }
        }

        static KeyboardEvent::Key translateKeyCode(WPARAM keyCode)
        {
            switch(keyCode)
            {
            case VK_TAB:
                return KeyboardEvent::Key::Tab;
            case VK_RETURN:
                return KeyboardEvent::Key::Enter;
            case VK_BACK:
                return KeyboardEvent::Key::Backspace;
            case VK_PAUSE:
            case VK_CANCEL:
                return KeyboardEvent::Key::Pause;
            case VK_ESCAPE:
                return KeyboardEvent::Key::Escape;
            case VK_DECIMAL:
                return KeyboardEvent::Key::KeypadDel;
            case VK_DIVIDE:
                return KeyboardEvent::Key::KeypadDivide;
            case VK_MULTIPLY:
                return KeyboardEvent::Key::KeypadMultiply;
            case VK_NUMPAD0:
                return KeyboardEvent::Key::Keypad0;
            case VK_NUMPAD1:
                return KeyboardEvent::Key::Keypad1;
            case VK_NUMPAD2:
                return KeyboardEvent::Key::Keypad2;
            case VK_NUMPAD3:
                return KeyboardEvent::Key::Keypad3;
            case VK_NUMPAD4:
                return KeyboardEvent::Key::Keypad4;
            case VK_NUMPAD5:
                return KeyboardEvent::Key::Keypad5;
            case VK_NUMPAD6:
                return KeyboardEvent::Key::Keypad6;
            case VK_NUMPAD7:
                return KeyboardEvent::Key::Keypad7;
            case VK_NUMPAD8:
                return KeyboardEvent::Key::Keypad8;
            case VK_NUMPAD9:
                return KeyboardEvent::Key::Keypad9;
            case VK_SUBTRACT:
                return KeyboardEvent::Key::KeypadSubtract;
            case VK_CAPITAL:
                return KeyboardEvent::Key::CapsLock;
            case VK_DELETE:
                return KeyboardEvent::Key::Del;
            case VK_DOWN:
                return KeyboardEvent::Key::Down;
            case VK_UP:
                return KeyboardEvent::Key::Up;
            case VK_LEFT:
                return KeyboardEvent::Key::Left;
            case VK_RIGHT:
                return KeyboardEvent::Key::Right;
            case VK_F1:
                return KeyboardEvent::Key::F1;
            case VK_F2:
                return KeyboardEvent::Key::F2;
            case VK_F3:
                return KeyboardEvent::Key::F3;
            case VK_F4:
                return KeyboardEvent::Key::F4;
            case VK_F5:
                return KeyboardEvent::Key::F5;
            case VK_F6:
                return KeyboardEvent::Key::F6;
            case VK_F7:
                return KeyboardEvent::Key::F7;
            case VK_F8:
                return KeyboardEvent::Key::F8;
            case VK_F9:
                return KeyboardEvent::Key::F9;
            case VK_F10:
                return KeyboardEvent::Key::F10;
            case VK_F11:
                return KeyboardEvent::Key::F11;
            case VK_F12:
                return KeyboardEvent::Key::F12;
            case VK_END:
                return KeyboardEvent::Key::End;
            case VK_HOME:
                return KeyboardEvent::Key::Home;
            case VK_INSERT:
                return KeyboardEvent::Key::Insert;
            case VK_LCONTROL:
                return KeyboardEvent::Key::LeftControl;
            case VK_LMENU:
                return KeyboardEvent::Key::LeftAlt;
            case VK_LSHIFT:
                return KeyboardEvent::Key::LeftShift;
            case VK_LWIN:
                return KeyboardEvent::Key::LeftSuper;
            case VK_NUMLOCK:
                return KeyboardEvent::Key::NumLock;
            case VK_RCONTROL:
                return KeyboardEvent::Key::RightControl;
            case VK_RMENU:
                return KeyboardEvent::Key::RightAlt;
            case VK_RSHIFT:
                return KeyboardEvent::Key::RightShift;
            case VK_RWIN:
                return KeyboardEvent::Key::RightSuper;
            case VK_SCROLL:
                return KeyboardEvent::Key::ScrollLock;
            case VK_SNAPSHOT:
                return KeyboardEvent::Key::PrintScreen;
            default:
                // ASCII code
                return (KeyboardEvent::Key)keyCode;
            }
        }

        static InputModifiers getInputModifiers()
        {
            InputModifiers mods;
            mods.isShiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            mods.isCtrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            mods.isAltDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
            return mods;
        }

        static void dispatchKeyboardEvent(const DxWindowData* pWinData, WPARAM keyCode, bool isKeyDown)
        {
            KeyboardEvent keyEvent;
            keyEvent.type = isKeyDown ? KeyboardEvent::Type::KeyPressed : KeyboardEvent::Type::KeyReleased;
            keyEvent.key = translateKeyCode(keyCode);
            keyEvent.mods = getInputModifiers();
            pWinData->pWindow->mpCallbacks->handleKeyboardEvent(keyEvent);
        }

        static void dispatchMouseEvent(const DxWindowData* pWinData, UINT Msg, WPARAM wParam, LPARAM lParam)
        {
            MouseEvent mouseEvent;
            switch(Msg)
            {
            case WM_MOUSEMOVE:
                mouseEvent.type = MouseEvent::Type::Move;
                break;
            case WM_LBUTTONDOWN:
                mouseEvent.type = MouseEvent::Type::LeftButtonDown;
                break;
            case WM_LBUTTONUP:
                mouseEvent.type = MouseEvent::Type::LeftButtonUp;
                break;
            case WM_MBUTTONDOWN:
                mouseEvent.type = MouseEvent::Type::MiddleButtonDown;
                break;
            case WM_MBUTTONUP:
                mouseEvent.type = MouseEvent::Type::MiddleButtonUp;
                break;
            case WM_RBUTTONDOWN:
                mouseEvent.type = MouseEvent::Type::RightButtonDown;
                break;
            case WM_RBUTTONUP:
                mouseEvent.type = MouseEvent::Type::RightButtonUp;
                break;
            case WM_MOUSEWHEEL:
                mouseEvent.type = MouseEvent::Type::Wheel;
                mouseEvent.wheelDelta.y = ((float)GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
                break;
            case WM_MOUSEHWHEEL:
                mouseEvent.type = MouseEvent::Type::Wheel;
                mouseEvent.wheelDelta.x = ((float)GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            default:
                should_not_get_here();
            }

            mouseEvent.pos = glm::vec2(float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)));
            mouseEvent.pos *= pWinData->pWindow->getMouseScale();
            mouseEvent.mods = getInputModifiers();

            pWinData->pWindow->mpCallbacks->handleMouseEvent(mouseEvent);
        }
    };


    ID3D11DevicePtr getD3D11Device()
    {
        return gpD3D11Device;
    }

    ID3D11DeviceContextPtr getD3D11ImmediateContext()
    {
        return gpD3D11ImmediateContext;
    }

    ResourceFormat getSwapChainColorFormat(bool isSrgb)
    {
        return isSrgb ? ResourceFormat::RGBA8UnormSrgb : ResourceFormat::RGBA8Unorm;
    }

    ResourceFormat getSwapChainDepthFormat()
    {
        return ResourceFormat::D24UnormS8;
    }

    uint32_t getSwapChainSampleCount(uint32_t requestedSamples)
    {
        return (requestedSamples == 0) ? 1 : requestedSamples;
    }

    HWND createFalcorWindow(const Window::Desc& desc, void* pUserData)
    {
        const WCHAR* className = L"FalcorWindowClass";
        DWORD winStyle = WS_OVERLAPPED | WS_CAPTION |  WS_SYSMENU;
        if(desc.resizableWindow == true)
        {
            winStyle = winStyle | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        }

        // Register the window class
        WNDCLASS wc = {};
        wc.lpfnWndProc = &ApiCallbacks::msgProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = className;
        wc.hIcon = nullptr;
        
        if(RegisterClass(&wc) == 0)
        {
            Logger::log(Logger::Level::Fatal, "RegisterClass() failed");
            return nullptr;
        }

        // Window size we have is for client area, calculate actual window size
        RECT r{0, 0, desc.swapChainDesc.width, desc.swapChainDesc.height};
        AdjustWindowRect(&r, winStyle, false);

        int windowWidth = r.right - r.left;
        int windowHeight = r.bottom - r.top;

        // create the window
        std::wstring wTitle = string_2_wstring(desc.title);
        HWND hWnd = CreateWindowEx(0, className, wTitle.c_str(), winStyle, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, pUserData);
        if(hWnd == nullptr)
        {
            Logger::log(Logger::Level::Fatal, "CreateWindowEx() failed");
            return nullptr;
        }

        // It might be tempting to call ShowWindow() here, but this fires a WM_SIZE message, which if you look at our MsgProc()
        // calls some device functions. That's a race condition, since the device isn't necessarily initialized yet. 
        return hWnd;
    }

    ID3D11ResourcePtr createDepthTexture(uint32_t width, uint32_t height, uint32_t sampleCount)
    {
        // create the depth stencil resource and view
        D3D11_TEXTURE2D_DESC depthDesc;
        depthDesc.ArraySize = 1;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthDesc.CPUAccessFlags = 0;
        depthDesc.Format = getDxgiFormat(getSwapChainDepthFormat());
        depthDesc.Height = height;
        depthDesc.Width = width;
        depthDesc.MipLevels = 1;
        depthDesc.MiscFlags = 0;
        depthDesc.SampleDesc.Count = sampleCount;

        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        ID3D11Texture2DPtr pDepthResource;
        dx11_call(gpD3D11Device->CreateTexture2D(&depthDesc, nullptr, &pDepthResource));

        return pDepthResource;
    }

    bool createSwapChain(DxWindowData* pWinData, const SwapChainDesc& swapChain, bool isFullScreen)
    {
        IDXGIDevicePtr pDXGIDevice;
        dx11_call(gpD3D11Device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice));
        IDXGIAdapterPtr pDXGIAdapter;
        dx11_call(pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter));
        IDXGIFactoryPtr pIDXGIFactory;
        dx11_call(pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory));

        DXGI_SWAP_CHAIN_DESC dxgiDesc = {0};
        dxgiDesc.BufferCount = 1;
        dxgiDesc.BufferDesc.Format = swapChain.isSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        dxgiDesc.BufferDesc.Height = swapChain.height;
        dxgiDesc.BufferDesc.RefreshRate.Numerator = 60;
        dxgiDesc.BufferDesc.RefreshRate.Denominator = 1;
        dxgiDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        dxgiDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        dxgiDesc.BufferDesc.Width = swapChain.width;
        dxgiDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        dxgiDesc.Flags = 0;
        dxgiDesc.OutputWindow = pWinData->hWnd;
        dxgiDesc.SampleDesc.Count = getSwapChainSampleCount(swapChain.sampleCount);
        dxgiDesc.SampleDesc.Quality = 0;
        dxgiDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        dxgiDesc.Windowed = isFullScreen ? FALSE : TRUE;

        dx11_call(pIDXGIFactory->CreateSwapChain(gpD3D11Device, &dxgiDesc, &pWinData->pSwapChain));
        return true;
    }

    bool createDevice(uint32_t apiMajorVersion, uint32_t apiMinorVersion)
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL level = getD3DFeatureLevel(apiMajorVersion, apiMinorVersion);
        if(level == 0)
        {
            Logger::log(Logger::Level::Fatal, "Unsupported device feature level requested: " + std::to_string(apiMajorVersion) + "." + std::to_string(apiMinorVersion));
            return false;
        }

        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &level, 1, D3D11_SDK_VERSION, &gpD3D11Device, nullptr, nullptr);

        if(FAILED(hr))
        {
            dx11TraceHR("Failed to create DX device", hr);
            return false;
        }

        return true;
    }

    Window::Window(ICallbacks* pCallbacks) : mpCallbacks(pCallbacks)
    {

    }

    Window::~Window()
    {
        DxWindowData* pWinData = (DxWindowData*)mpPrivateData;
        if(pWinData)
        {
            pWinData->pSwapChain = nullptr;
            if(pWinData->hWnd)
            {
                DestroyWindow(pWinData->hWnd);
            }
        }

        safe_delete(mpPrivateData);        
    }

    void Window::shutdown()
    {
        PostQuitMessage(0);
    }

    Window::UniquePtr Window::create(const Desc& desc, ICallbacks* pCallbacks)
    {
        if(gpD3D11Device)
        {
            Logger::log(Logger::Level::Error, "DX11 backend doesn't support more than a single device.");
            return nullptr;
        }

        if(desc.requiredExtensions.size())
        {
            Logger::log(Logger::Level::Warning, "DX doesn't support API extensions. Ignoring requested extensions.");
        }

        UniquePtr pWindow = UniquePtr(new Window(pCallbacks));
        DxWindowData* pWinData = new DxWindowData;
        pWindow->mpPrivateData = pWinData;
        pWinData->pWindow = pWindow.get();
        
        // create the window
        pWinData->hWnd = createFalcorWindow(desc, pWinData);
        if(pWinData->hWnd == nullptr)
        {
            return false;
        }

        // create the DX device
        if(createDevice(desc.apiMajorVersion, desc.apiMinorVersion) == false)
        {
            return nullptr;
        }

        // Get the immediate context
        gpD3D11Device->GetImmediateContext(&gpD3D11ImmediateContext);

        if(createSwapChain(pWinData, desc.swapChainDesc, desc.fullScreen) == false)
        {
            return nullptr;
        }

        // create the framebuffer.
        pWindow->updateDefaultFBO(desc.swapChainDesc.width, desc.swapChainDesc.height, desc.swapChainDesc.sampleCount, getSwapChainColorFormat(desc.swapChainDesc.isSrgb), getSwapChainDepthFormat());
       
        return pWindow;
    }

    void Window::setVSync(bool enable)
    {
        DxWindowData* pWinData = (DxWindowData*)mpPrivateData;
        pWinData->syncInterval = enable ? 1 : 0;
    }

    void Window::resize(uint32_t width, uint32_t height)
    {
        // Resize the swap-chain
        const Texture* pColor = mpDefaultFBO->getColorTexture(0).get();
        if(pColor->getWidth() != width || pColor->getHeight() != height)
        {
            DxWindowData* pData = (DxWindowData*)mpPrivateData;

            // Resize the window
            RECT r = {0, 0, width, height};
            DWORD style = GetWindowLong(pData->hWnd, GWL_STYLE);
            AdjustWindowRect(&r, style, false);
            int windowWidth = r.right - r.left;
            int windowHeight = r.bottom - r.top;

            // The next call will dispatch a WM_SIZE message which will take care of the framebuffer size change
            dx11_call(SetWindowPos(pData->hWnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER));
        }
    }

    void Window::updateDefaultFBO(uint32_t width, uint32_t height, uint32_t sampleCount, ResourceFormat colorFormat, ResourceFormat depthFormat)
    {
        // Resize the swap-chain
        auto pCtx = getD3D11ImmediateContext();
        pCtx->ClearState();
        pCtx->OMSetRenderTargets(0, nullptr, nullptr);
        DxWindowData* pData = (DxWindowData*)mpPrivateData;
        releaseDefaultFboResources();
        dx11_call(pData->pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

        sampleCount = getSwapChainSampleCount(sampleCount);

        // create the depth texture
        Texture::SharedPtr pDepthTex = nullptr;
        if(sampleCount > 1)
        {
            pDepthTex = Texture::create2DMS(width, height, depthFormat, sampleCount, 1);
        }
        else
        {
            pDepthTex = Texture::create2D(width, height, depthFormat, 1, 1, nullptr);
        }

        // Need to re-create the color texture here since Texture is a friend of CWindow
        Texture::SharedPtr pColorTex = Texture::SharedPtr(new Texture(width, height, 1, 1, 1, sampleCount, colorFormat, Texture::Type::Texture2D));
        dx11_call(pData->pSwapChain->GetBuffer(0, __uuidof(pColorTex->mApiHandle), reinterpret_cast<void**>(&pColorTex->mApiHandle)));

        attachDefaultFboResources(pColorTex, pDepthTex);
        mMouseScale.x = 1 / float(width);
        mMouseScale.y = 1 / float(height);
    }

    bool isWindowOccluded(DxWindowData* pWinData)
    {
        if(pWinData->isWindowOccluded)
        {
            pWinData->isWindowOccluded = (pWinData->pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED);
        }
        return pWinData->isWindowOccluded;
    }

    void Window::msgLoop()
    {
        // Show the window
        DxWindowData* pData = (DxWindowData*)mpPrivateData;
        ShowWindow(pData->hWnd, SW_SHOWNORMAL);

        MSG msg;
        while(1) 
        {
            if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if(msg.message == WM_QUIT)
                {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                DxWindowData* pWinData = (DxWindowData*)mpPrivateData;
                if(isWindowOccluded(pWinData) == false)
                {
                    mpCallbacks->renderFrame();
                    pWinData->isWindowOccluded = (pWinData->pSwapChain->Present(pWinData->syncInterval, 0) == DXGI_STATUS_OCCLUDED);
                }
            }
        }
    }

    void Window::swapBuffers()
    {


    }

    void Window::setWindowTitle(std::string title)
    {

    }
    void Window::pollForEvents()
    {
    }

    bool checkExtensionSupport(const std::string& name)
    {
        UNSUPPORTED_IN_DX11("checkExtensionSupport");
        return false;
    }
}
#endif //#ifdef FALCOR_D3D11
