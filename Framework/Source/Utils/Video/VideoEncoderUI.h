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
#include "VideoEncoder.h"
#include "Utils/Gui.h"

namespace Falcor
{
    class VideoEncoderUI
    {
    public:
        using UniquePtr = std::unique_ptr<VideoEncoderUI>;
        using UniqueConstPtr = std::unique_ptr<const VideoEncoderUI>;

        // FIX_GUI
//        static UniquePtr create(uint32_t topLeftX, uint32_t topLeftY, uint32_t width, uint32_t height, Gui::ButtonCallback startCaptureCB, Gui::ButtonCallback endCaptureCB, void* pUserData);
        ~VideoEncoderUI();

        VideoEncoder::CodecID getCodec() const { return mCodec; }
        uint32_t getFPS() const { return mFPS; }
        bool useTimeRange() const { return mUseTimeRange; }
        bool captureUI() const { return mCaptureUI; }
        float getStartTime() const { return mStartTime; }
        float getEndTime() const { return mEndTime; }
        const std::string& getFilename() const { return mFilename; }
        float getBitrate() const {return mBitrate; }
        uint32_t getGopSize() const {return mGopSize; }

    private:
//        VideoEncoderUI(uint32_t topLeftX, uint32_t topLeftY, uint32_t width, uint32_t height, Gui::ButtonCallback startCaptureCB, Gui::ButtonCallback endCaptureCB, void* pUserData);
        static void GUI_CALL startCaptureCB(void* pUserData);
        static void GUI_CALL endCaptureCB(void* pUserData);

        void startCapture();
        void endCapture();

        // FIX_GUI
//         Gui::ButtonCallback mStartCB = nullptr;
//         Gui::ButtonCallback mEndCB = nullptr;
        void* mpUserData = nullptr;

        Gui::UniquePtr mpUI;
        uint32_t mFPS = 60;
        VideoEncoder::CodecID mCodec = VideoEncoder::CodecID::RawVideo;

        bool mUseTimeRange = false;
        bool mCaptureUI = false;
        float mStartTime = 0;
        float mEndTime = FLT_MAX;

        std::string mFilename;
        float mBitrate = 4;
        uint32_t mGopSize = 10;
    };
}