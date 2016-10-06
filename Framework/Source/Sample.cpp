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
#include "Sample.h"
#include <map>
#include "API/ScreenCapture.h"
#include "API/Window.h"
#include "Graphics/Program.h"
#include "Utils/OS.h"
#include "API/FBO.h"
#include "VR\OpenVR\VRSystem.h"

namespace Falcor
{
    Sample::Sample()
    {
    };

    void Sample::handleWindowSizeChange()
    {
        // DISABLED_FOR_D3D12
        // Always render UI at full resolution
//        mpGui->windowSizeCallback(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight());

        // Reset the UI position
//         uint32_t barSize[2];
//         mpGui->getSize(barSize);
//         mpGui->setPosition(mpDefaultFBO->getWidth() - 10 - barSize[0] - 10, 10);

        onResizeSwapChain();

        // Reset the clock before, so that the first frame duration will be correct
        mFrameRate.resetClock();
    }

    void Sample::handleKeyboardEvent(const KeyboardEvent& keyEvent)
    {
        if (keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            mPressedKeys.insert(keyEvent.key);
        }
        else
        {
            mPressedKeys.erase(keyEvent.key);
        }
        // DISABLED_FOR_D3D12
        // Check if the GUI consumes it
//         if(mpGui->keyboardCallback(keyEvent))
//         {
//             return;
//         }
            
        // Consume system messages first
        if(keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            if(keyEvent.mods.isShiftDown && keyEvent.key == KeyboardEvent::Key::PrintScreen)
            {
                initVideoCapture();
            }
            else if(!keyEvent.mods.isAltDown && !keyEvent.mods.isCtrlDown && !keyEvent.mods.isShiftDown)
            {
                switch(keyEvent.key)
                {
                case KeyboardEvent::Key::PrintScreen:
                    mCaptureScreen = true;
                    break;
#if _PROFILING_ENABLED
                case KeyboardEvent::Key::P:
                    gProfileEnabled = !gProfileEnabled;
                    break;
#endif
                case KeyboardEvent::Key::V:
                    mVsyncOn = !mVsyncOn;
                    gpDevice->setVSync(mVsyncOn);
                    mFrameRate.resetClock();
                    break;
                case KeyboardEvent::Key::F1:
                {
                    uint32_t textMode = ((uint32_t)mTextMode + 1) % (uint32_t)TextMode::Count;
                    setTextMode((TextMode)textMode);
                }
                break;
                case KeyboardEvent::Key::F2:
                    toggleUI(!mShowUI);
                    break;
                case KeyboardEvent::Key::F5:
                    Program::reloadAllPrograms();
                    onDataReload();
                    break;
                case KeyboardEvent::Key::Escape:
                    mpWindow->shutdown();
                    break;
                case KeyboardEvent::Key::F12:
                    if(mVideoCapture.pVideoCapture)
                    {
                        endVideoCapture();
                    }
                    break;
                }
            }
        }

        // If we got here, this is a user specific message
        onKeyEvent(keyEvent);
    }

    void Sample::toggleUI(bool showUI)
    {
        mShowUI = showUI;
        // DISABLED_FOR_D3D12
//        mpGui->setVisibility(mShowUI);
    }

    void Sample::handleMouseEvent(const MouseEvent& mouseEvent)
    {
        // DISABLED_FOR_D3D12
//         if(mpGui->mouseCallback(mouseEvent))
//         {
//             return;
//         }
        onMouseEvent(mouseEvent);
    }


    // CSample functions
    Sample::~Sample()
    {
        if(mVideoCapture.pVideoCapture)
        {
            endVideoCapture();
        }
        VRSystem::cleanup();
    }

    void Sample::run(const SampleConfig& config)
    {
        mTimeScale = config.timeScale;
        mFreezeTime = config.freezeTimeOnStartup;

        // Start the logger
        Logger::init();
        Logger::showBoxOnError(config.showMessageBoxOnError);

        mpWindow = Window::create(config.windowDesc, this);

        if(mpWindow == nullptr)
        {
			logError("Failed to create device and window");
            return;
        }

        gpDevice = Device::create(mpWindow, Device::Desc());
		if (gpDevice == nullptr)
		{
			logError("Failed to create device");
			return;
		}

        // Set the icon
        setActiveWindowIcon("Framework\\Nvidia.ico");

        // Init the UI
        initUI();

        // Init VR
        mVrEnabled = config.enableVR;
        if(mVrEnabled)
        {
			VRSystem::start(mpRenderContext);
        }

        // Get the default objects before calling onLoad()
        mpDefaultFBO = gpDevice->getSwapChainFbo();
        mpRenderContext = gpDevice->getRenderContext();

        // Call the load callback
        onLoad();
		handleWindowSizeChange();
        
        mpWindow->msgLoop();

        onShutdown();
        Logger::shutdown();
    }

    void Sample::calculateTime()
    {
        if(mVideoCapture.pVideoCapture)
        {
            // We are capturing video at a constant FPS
            mCurrentTime += mVideoCapture.timeDelta * mTimeScale;
        }
        else if(mFreezeTime == false)
        {
            float ElapsedTime = mFrameRate.getLastFrameTime() * mTimeScale;
            mCurrentTime += ElapsedTime;
        }
    }

    void Sample::renderFrame()
    {
		if (gpDevice->isWindowOccluded())
		{
			return;
		}

		mFrameRate.newFrame();
        {
			// The swap-chain FBO might have changed between frames, so get it
			mpDefaultFBO = gpDevice->getSwapChainFbo();
            mpRenderContext = gpDevice->getRenderContext();
            PROFILE(onFrameRender);
            calculateTime();

			// Bind the default state
			mpRenderContext->setFbo(mpDefaultFBO);
            
            // Set the viewport
            // D3D12 Code - This should actually be done inside RenderContext api specific code
            //              We need to record the new state from the previous frame into the command list
            RenderContext::Viewport vp;
            vp.height = (float)mpDefaultFBO->getHeight();
            vp.width = (float)mpDefaultFBO->getWidth();
            mpRenderContext->setViewport(0, vp);

            // DISABLED_FOR_D3D12
//            mpRenderContext->setRenderState(nullptr);

             onFrameRender();
        }

        {
            PROFILE(DrawGUI);
            if(mShowUI)
            {
                // DISABLED_FOR_D3D12
//                mpGui->drawAll();
            }
        }
        captureVideoFrame();
        if(mCaptureScreen)
        {
            captureScreen();
        }
        printProfileData();
        gpDevice->present();
    }

    void Sample::captureScreen()
    {
        std::string filename = getExecutableName();

        // Now we have a folder and a filename, look for an available filename (we don't overwrite existing files)
        std::string prefix = std::string(filename);
        std::string executableDir = getExecutableDirectory();
        std::string pngFile;
        if(findAvailableFilename(prefix, executableDir, "png", pngFile))
        {
            ScreenCapture::captureToPng(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), pngFile);
        }
        else
        {
            Logger::log(Logger::Level::Error, "Could not find available filename when capturing screen");
        }
        mCaptureScreen = false;
    }

    void Sample::initUI()
    {
// DISABLED_FOR_D3D12
//         Gui::initialize(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), mpRenderContext);
//         mpGui = Gui::create("Sample UI");
//         const std::string sampleGroup = "Global Sample Controls";
//         mpGui->addDoubleVar("Global Time", &mCurrentTime, sampleGroup, 0, DBL_MAX);
//         mpGui->addFloatVar("Time Scale", &mTimeScale, sampleGroup, 0, FLT_MAX);
//         mpGui->addCheckBox("Freeze Time", &mFreezeTime, sampleGroup);
//         mpGui->addButton("Screen Capture", &Sample::captureScreenCB, this, sampleGroup);
//         mpGui->addButton("Video Capture", &Sample::initVideoCaptureCB, this, sampleGroup);
//         mpGui->addSeparator();
// 
//         // Set the UI size
//         uint32_t BarSize[2];
//         mpGui->getSize(BarSize);
//         mpGui->setSize(BarSize[0] + 20, BarSize[1]);
// 
        mpTextRenderer = TextRenderer::create();
    }

    const std::string Sample::getGlobalSampleMessage(bool includeHelpMsg) const
    {
        std::string s;
        if(mTextMode != TextMode::NoText)
        {
            float msPerFrame = mFrameRate.getAverageFrameTime();
            s = std::to_string(int(ceil(1000 / msPerFrame))) + " FPS (" + std::to_string(msPerFrame) + " ms/frame)";
            if(mVsyncOn) s += std::string(", VSync");
            if(mTextMode != TextMode::FpsOnly)
            {
                s += "\n";
                if(includeHelpMsg)
                {
                    s += "  'F2'      - Show GUI\n";
                    s += "  'F5'      - Reload shaders\n";
                    s += "  'ESC'     - Quit\n";
                    s += "  'V'       - Toggle VSync\n";
                    s += "  'PrtScr'  - Capture screenshot\n";
                    s += "  'Shift+PrtScr' - Video capture\n";
#if _PROFILING_ENABLED
                    s += "  'P'       - Enable profiling\n";
#endif
                }
            }
        }
        return s;
    }

    void Sample::setTextMode(Sample::TextMode mode)
    {
        mTextMode = mode;
    }

    void Sample::resizeSwapChain(uint32_t width, uint32_t height)
    {
        mpWindow->resize(width, height);
    }

    bool Sample::isKeyPressed(const KeyboardEvent::Key& key) const 
    {
        return mPressedKeys.find(key) != mPressedKeys.cend();
    }

    void Sample::renderText(const std::string& msg, const glm::vec2& position, const glm::vec2 shadowOffset) const
    {
        if(mTextMode != TextMode::NoText)
        {
            PROFILE(renderText);
            // Render outline first
            if(shadowOffset.x != 0.f || shadowOffset.y != 0)
            {
                const glm::vec3 oldColor = mpTextRenderer->getTextColor();
                mpTextRenderer->setTextColor(glm::vec3(0.f));   // Black outline 
                mpTextRenderer->begin(mpRenderContext, position + shadowOffset);
                mpTextRenderer->renderLine(msg);
                mpTextRenderer->end();
                mpTextRenderer->setTextColor(oldColor);
            }
            mpTextRenderer->begin(mpRenderContext, position);
            mpTextRenderer->renderLine(msg);
            mpTextRenderer->end();
        }
    }

    void Sample::printProfileData()
    {
#if _PROFILING_ENABLED
        if(gProfileEnabled)
        {
            std::string profileMsg;
            Profiler::endFrame(profileMsg);
            renderText(profileMsg, glm::vec2(10, 300));
        }
#endif
    }

    void Sample::captureScreenCB(void* pUserData)
    {
        Sample* pSample = (Sample*)pUserData;
        pSample->mCaptureScreen = true;
    }

    void Sample::initVideoCaptureCB(void* pUserData)
    {
        Sample* pSample = (Sample*)pUserData;
        pSample->initVideoCapture();
    }

    void Sample::startVideoCaptureCB(void* pUserData)
    {
        Sample* pSample = (Sample*)pUserData;
        pSample->startVideoCapture();
    }

    void Sample::endVideoCaptureCB(void* pUserData)
    {
        Sample* pSample = (Sample*)pUserData;
        pSample->endVideoCapture();
    }

    void Sample::initVideoCapture()
    {
        if(mVideoCapture.pUI == nullptr)
        {
            mVideoCapture.pUI = VideoEncoderUI::create(20, 300, 240, 220, &Sample::startVideoCaptureCB, &Sample::endVideoCaptureCB, this);
        }
    }

    void Sample::startVideoCapture()
    {
        // create the capture object and frame buffer
        VideoEncoder::Desc desc;
        desc.flipY     = true;
        desc.codec      = mVideoCapture.pUI->getCodec();
        desc.filename   = mVideoCapture.pUI->getFilename();
        desc.format     = VideoEncoder::InputFormat::R8G8B8A8;
        desc.fps        = mVideoCapture.pUI->getFPS();
        desc.height     = mpDefaultFBO->getHeight();
        desc.width      = mpDefaultFBO->getWidth();
        desc.bitrateMbps = mVideoCapture.pUI->getBitrate();
        desc.gopSize    = mVideoCapture.pUI->getGopSize();

        mVideoCapture.pVideoCapture = VideoEncoder::create(desc);

        assert(mVideoCapture.pVideoCapture);
        mVideoCapture.pFrame = new uint8_t[desc.width*desc.height * 4];

        mVideoCapture.timeDelta = 1 / (float)desc.fps;

        if(mVideoCapture.pUI->useTimeRange())
        {
            if(mVideoCapture.pUI->getStartTime() > mVideoCapture.pUI->getEndTime())
            {
                mVideoCapture.timeDelta = -mVideoCapture.timeDelta;
            }
            mCurrentTime = mVideoCapture.pUI->getStartTime();
            if(!mVideoCapture.pUI->captureUI())
                mShowUI = false;
        }
    }

    void Sample::endVideoCapture()
    {
        if(mVideoCapture.pVideoCapture)
        {
            mVideoCapture.pVideoCapture->endCapture();
            mShowUI = true;
        }
        mVideoCapture.pUI = nullptr;
        mVideoCapture.pVideoCapture = nullptr;
        safe_delete_array(mVideoCapture.pFrame);
    }

    void Sample::captureVideoFrame()
    {
        if(mVideoCapture.pVideoCapture)
        {
            ScreenCapture::captureToMemory(mpDefaultFBO->getWidth(), mpDefaultFBO->getHeight(), ResourceFormat::RGBA8Unorm, mVideoCapture.pFrame);
            mVideoCapture.pVideoCapture->appendFrame(mVideoCapture.pFrame);

            if(mVideoCapture.pUI->useTimeRange())
            {
                if(mVideoCapture.timeDelta >= 0)
                {
                    if(mCurrentTime >= mVideoCapture.pUI->getEndTime())
                    {
                        endVideoCapture();
                    }
                }
                else if(mCurrentTime < mVideoCapture.pUI->getEndTime())
                {
                    endVideoCapture();
                }
            }
        }
    }

    void Sample::shutdownApp()
    {
        mpWindow->shutdown();
    }

    void Sample::pollForEvents()
    {
        mpWindow->pollForEvents();
    }

    void Sample::setWindowTitle(std::string title)
    {
        mpWindow->setWindowTitle(title);
    }
}
