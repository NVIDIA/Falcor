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
#include "SystemTestBase.h"

SystemTestBase::SystemTestBase() : mpScene(nullptr), mpSceneRenderer(nullptr), mDerivedName("UNINITALIZED"),
mLoadTime(0.f), mFpsLoggingTime(0.f), mFpsFrames(300, 600), mNumScreencaps(0), mCaughtException(false) {}

void SystemTestBase::onLoad()
{
    try 
    {
        mpScene = Scene::loadFromFile(mDerivedName + ".fscene", Model::GenerateTangentSpace);
        mpSceneRenderer = SceneRenderer::create(mpScene);
        toggleText(false);
        getTestingParameters();
        load();
    }
    catch (...)
    {
        mCaughtException = true;
        shutdownApp();
    }
}

void SystemTestBase::onFrameRender()
{
    try {
        logTestingData();
        render();
    }
    catch (...)
    {
        mCaughtException = true;
        shutdownApp();
    }
}

void SystemTestBase::onShutdown()
{
    try
    {
        outputXML();
        shutdown();
    }
    catch (...)
    {
        mCaughtException = true;
    }
}

void SystemTestBase::onResizeSwapChain()
{
    //Update viewport
    GraphicsState::Viewport vp;
    vp.height = static_cast<float>(mpDefaultFBO->getHeight());
    vp.width = static_cast<float>(mpDefaultFBO->getWidth());
    mpDefaultPipelineState->setViewport(0, vp);

    //Update Camera
    mpScene->getActiveCamera()->setFovY(static_cast<float>(M_PI / 3.f)); //pi/3 == 60 degrees
    float aspect = (vp.width / vp.height);
    mpScene->getActiveCamera()->setAspectRatio(aspect);
}

void SystemTestBase::getTestingParameters()
{
    const std::string screencapName = "screencap_frames";
    const std::string fpsRangeName = "fps_frames";

    mScreencapFrames = mpScene->getUserVariable(screencapName);

    Scene::UserVariable fpsFrames = mpScene->getUserVariable(fpsRangeName);
    if (fpsFrames.type == Scene::UserVariable::Type::Unknown)
        std::cout << "Failed to find fps_frames";
    else
        mFpsFrames = fpsFrames.vec2;
}

void SystemTestBase::logTestingData()
{
    uint32_t currentFrame = frameRate().getFrameCount();
    //loadtime
    if (currentFrame == 2)
        mLoadTime = frameRate().getLastFrameTime();

    //Screencaps
    handleScreenCapture(currentFrame);

    //FPS 
     if (frameRate().getFrameCount() > static_cast<uint32_t>(mFpsFrames.x) &&
        frameRate().getFrameCount() < static_cast<uint32_t>(mFpsFrames.y))
    {
        mFpsLoggingTime += frameRate().getLastFrameTime();
    }
    else if (frameRate().getFrameCount() == static_cast<uint32_t>(mFpsFrames.y))
    {
        shutdownApp();
    }
}

void SystemTestBase::handleScreenCapture(uint32_t currentFrame)
{
    switch (mScreencapFrames.type)
    {
    case Scene::UserVariable::Type::Uint:
    {
        if (mScreencapFrames.u32 == currentFrame)
        {
            mNumScreencaps = 1;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Uint64:
    {
        if (mScreencapFrames.u64 == currentFrame)
        {
            mNumScreencaps = 1;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Int:
    {
        if (mScreencapFrames.i32 == currentFrame)
        {
            mNumScreencaps = 1;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Int64:
    {
        if (mScreencapFrames.i64 == currentFrame)
        {
            mNumScreencaps = 1;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Double:
    {
        if (mScreencapFrames.d64 == currentFrame)
        {
            mNumScreencaps = 1;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Vec2:
    {
        if (mScreencapFrames.vec2[0] == currentFrame || mScreencapFrames.vec2[1] == currentFrame)
        {
            mNumScreencaps = 2;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Vec3:
    {
        if (mScreencapFrames.vec3[0] == currentFrame || mScreencapFrames.vec3[1] == currentFrame
            || mScreencapFrames.vec3[2] == currentFrame)
        {
            mNumScreencaps = 3;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Vec4:
    {
        if (mScreencapFrames.vec4[0] == currentFrame || mScreencapFrames.vec4[1] == currentFrame
            || mScreencapFrames.vec4[2] == currentFrame || mScreencapFrames.vec4[3] == currentFrame)
        {
            mNumScreencaps = 4;
            mCaptureScreen = true;
        }

        break;
    }
    case Scene::UserVariable::Type::Vector:
    {
        //using index rather than it so I can name the screencaps more easily
        for (uint32_t i = 0; i < mScreencapFrames.vector.size(); ++i)
        {
            if (mScreencapFrames.vector[i] == currentFrame)
            {
                mNumScreencaps = static_cast<uint32>(mScreencapFrames.vector.size());
                mCaptureScreen = true;
                break;
            }
        }

        break;
    }
    default:
        std::cout << "Unrecognized type or failed to find user var for screen cap frames" << std::endl;
    }
}

void SystemTestBase::outputXML()
{
    std::ofstream of;

    of.open(mDerivedName + "_TestingLog.xml");
    of << "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n";
    of << "<TestLog>\n";
    of << "<SystemResults\n";
    of << "\tLoadTime=\"" << std::to_string(mLoadTime) << "\"\n";
    of << "\tFrameTime=\"" << std::to_string( mFpsLoggingTime / (mFpsFrames.y - mFpsFrames.x)) << "\"\n";
    of << "\tNumScreenshots=\"" << std::to_string(mNumScreencaps) << "\"\n";
    of << "/>\n";
    of << "</TestLog>";
    of.close();
}