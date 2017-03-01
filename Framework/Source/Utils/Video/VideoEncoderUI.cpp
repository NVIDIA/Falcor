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
#include "VideoEncoderUI.h"
#include "Utils/OS.h"

namespace Falcor
{
    VideoEncoderUI::UniquePtr VideoEncoderUI::create(uint32_t topLeftX, uint32_t topLeftY, uint32_t width, uint32_t height, Gui::ButtonCallback startCaptureCB, Gui::ButtonCallback endCaptureCB, void* pUserData)
    {
        return UniquePtr(new VideoEncoderUI(topLeftX, topLeftY, width, height, startCaptureCB, endCaptureCB, pUserData));
    }

    VideoEncoderUI::VideoEncoderUI(uint32_t topLeftX, uint32_t topLeftY, uint32_t width, uint32_t height, Gui::ButtonCallback startCaptureCB, Gui::ButtonCallback endCaptureCB, void* pUserData)
    {
        mTopLeftX = topLeftX;
        mTopLeftY = topLeftY;
        mWidth = width;
        mHeight = height;

        initUI();

        mpUI->setVisibility(false);

        mStartCB = startCaptureCB;
        mEndCB = endCaptureCB;
        mpUserData = pUserData;
    }

    VideoEncoderUI::~VideoEncoderUI() = default;

    void VideoEncoderUI::setUIVisibility(bool bVisible)
    {
        mpUI->setVisibility(bVisible);
    }

    void VideoEncoderUI::startCaptureCB(void* pUserData)
    {
        VideoEncoderUI* pUI = (VideoEncoderUI*)pUserData;
        pUI->startCapture();
    }

    void VideoEncoderUI::endCaptureCB(void* pUserData)
    {
        VideoEncoderUI* pUI = (VideoEncoderUI*)pUserData;
        pUI->endCapture();
    }

    void VideoEncoderUI::initUI()
    {
        mpUI = nullptr;
        mpUI = Gui::create("Video Capture");
        mpUI->addIntVar("FPS", (int32_t*)&mFPS, "", 0, 240, 1);

        Gui::dropdown_list CodecID;
        CodecID.push_back({ (int32_t)VideoEncoder::CodecID::RawVideo, std::string("Uncompressed") });
        CodecID.push_back({ (int32_t)VideoEncoder::CodecID::H264, std::string("H.264") });
        CodecID.push_back({ (int32_t)VideoEncoder::CodecID::HEVC, std::string("HEVC(H.265)") });
        CodecID.push_back({ (int32_t)VideoEncoder::CodecID::MPEG2, std::string("MPEG2") });
        CodecID.push_back({ (int32_t)VideoEncoder::CodecID::MPEG4, std::string("MPEG4") });

        mpUI->addDropdown("Codec", CodecID, &mCodec);
        mpUI->addFloatVar("Bitrate (Mbps)", &mBitrate, "Codec Options", 0, FLT_MAX, 0.01f);
        mpUI->addIntVar("GOP Size", (int32_t*)&mGopSize, "Codec Options", 0, 100000, 1);

        mpUI->addCheckBox("Capture UI", &mCaptureUI);

        mpUI->addCheckBox("Use Time-Range", &mUseTimeRange);
        mpUI->addFloatVar("Start Time", &mStartTime, "Time Range", 0, FLT_MAX, 0.001f);
        mpUI->addFloatVar("End Time", &mEndTime, "Time Range", 0, FLT_MAX, 0.001f);
        mpUI->addSeparator();
        mpUI->addButton("Start Recording", &VideoEncoderUI::startCaptureCB, this);
        mpUI->addButton("Cancel", &VideoEncoderUI::endCaptureCB, this);

        mpUI->setPosition(mTopLeftX, mTopLeftY);
        mpUI->setSize(mWidth, mHeight);
    }

    void VideoEncoderUI::startCapture()
    {
        if(saveFileDialog(VideoEncoder::getSupportedContainerForCodec(mCodec).c_str(), mFilename))
        {
            if(!mUseTimeRange)
                mCaptureUI = true;
            // Initialize the UI
            if(mCaptureUI)
            {
                uint32_t pos[2];
                mpUI->getPosition(pos);
                uint32_t size[2];
                mpUI->getSize(size);
                mpUI = nullptr;
                mpUI = Gui::create("Video Capture");
                mpUI->setPosition(pos[0], pos[1]);
                mpUI->setSize(size[0], size[1]);
                mpUI->addButton("End Recording", &VideoEncoderUI::endCaptureCB, this);
            }

            // Call the users callback
            mStartCB(mpUserData);
        }
    }

    void VideoEncoderUI::endCapture()
    {
        mEndCB(mpUserData);
        initUI();
    }
}