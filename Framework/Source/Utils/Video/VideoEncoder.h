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
#include <string>

struct AVFormatContext;
struct AVStream;
struct AVFrame;
struct SwsContext;
struct AVCodecContext;

namespace Falcor
{        
    class VideoEncoder
    {
    public:
        using UniquePtr = std::unique_ptr<VideoEncoder>;
        using UniqueConstPtr = std::unique_ptr<const VideoEncoder>;

        enum class CodecID : int32_t
        {
            RawVideo,
            H264,
            HEVC,
            MPEG2,
            MPEG4,
        };

        enum class InputFormat
        {
            R8G8B8A8,
        };

        struct Desc
        {
            uint32_t fps = 60;
            uint32_t width = 0;
            uint32_t height = 0;
            float bitrateMbps = 4;
            uint32_t gopSize = 10;
            CodecID codec = CodecID::RawVideo;
            InputFormat format = InputFormat::R8G8B8A8;
            bool flipY = false;
            std::string filename;
        };

        ~VideoEncoder();

        static UniquePtr create(const Desc& desc);
        void appendFrame(const void* pData);
        void endCapture();

        static const std::string getSupportedContainerForCodec(CodecID codec);
    private:
        VideoEncoder(const std::string& filename);
        bool init(const Desc& desc);

        AVFormatContext* mpOutputContext = nullptr;
        AVStream*        mpOutputStream  = nullptr;
        AVFrame*         mpFrame         = nullptr;
        SwsContext*      mpSwsContext    = nullptr;
        AVCodecContext*  mpCodecContext = nullptr;

        const std::string mFilename;
        InputFormat mForamt;
        uint32_t mRowPitch = 0;
        uint8_t* mpFlippedImage = nullptr; // Used in case the image memory layout if bottom->top
    };
}