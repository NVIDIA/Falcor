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
#include "VideoEncoder.h"
#include "Utils/BinaryFileStream.h"
#include <direct.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

namespace Falcor
{
    AVPixelFormat getPictureFormatFromCodec(AVCodecID codec)
    {
        switch(codec)
        {
        case AV_CODEC_ID_RAWVIDEO:
            return AV_PIX_FMT_BGR24;
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_HEVC:
        case AV_CODEC_ID_MPEG2VIDEO:
            return AV_PIX_FMT_YUV422P;
        case AV_CODEC_ID_MPEG4:
            return AV_PIX_FMT_YUV420P;
        default:
            should_not_get_here();
            return AV_PIX_FMT_NONE;
        }
    }

    AVPixelFormat getPictureFormatFromFalcorFormat(VideoEncoder::InputFormat format)
    {
        switch(format)
        {
        case VideoEncoder::InputFormat::R8G8B8A8:
            return AV_PIX_FMT_RGBA;
        default:
            should_not_get_here();
            return AV_PIX_FMT_NONE;
        }
    }

    int32_t getInputFormatBytesPerPixel(VideoEncoder::InputFormat format)
    {
        switch(format)
        {
        case VideoEncoder::InputFormat::R8G8B8A8:
            return 4;
        default:
            should_not_get_here();
            return 0;
        }
    }

    AVCodecID getCodecID(VideoEncoder::CodecID codec)
    {
        switch(codec)
        {
        case VideoEncoder::CodecID::RawVideo:
            return AV_CODEC_ID_RAWVIDEO;
        case VideoEncoder::CodecID::H264:
            return AV_CODEC_ID_H264;
        case VideoEncoder::CodecID::HEVC:
            return AV_CODEC_ID_HEVC;
        case VideoEncoder::CodecID::MPEG2:
            return AV_CODEC_ID_MPEG2VIDEO;
        case VideoEncoder::CodecID::MPEG4:
            return AV_CODEC_ID_MPEG4;
        default:
            should_not_get_here();
            return AV_CODEC_ID_NONE;
        }
    }

    static bool error(const std::string& filename, const std::string& msg)
    {
        std::string s("Error when creating video capture file ");
        s += filename + ".\n" + msg;
        Logger::log(Logger::Level::Error, msg);
        return false;
    }

    AVStream* createVideoStream(AVFormatContext* pCtx, uint32_t width, uint32_t height, uint32_t fps, float bitrateMbps, uint32_t gopSize, AVCodecID codecID, const std::string& filename, AVCodec** ppCodec)
    {
        // Get the encoder
        *ppCodec = avcodec_find_encoder(codecID);
        if(*ppCodec == nullptr)
        {
            error(filename, std::string("Can't find ") + avcodec_get_name(codecID) + " encoder.");
            return nullptr;
        }

        // create the video stream
        AVStream* pStream = avformat_new_stream(pCtx, *ppCodec);
        if(pStream == nullptr)
        {
            error(filename, "Failed to create video stream.");
            return nullptr;
        }
        pStream->id = pCtx->nb_streams - 1;

        // Initialize the codec context
        AVCodecContext* pCodecCtx = pStream->codec;
        pCodecCtx->codec_id = codecID;
        pCodecCtx->bit_rate = (int)(bitrateMbps * 1000 * 1000);
        pCodecCtx->width = width;
        pCodecCtx->height = height;
        pCodecCtx->time_base = {1, (int)fps};
        pCodecCtx->gop_size = gopSize;
        pCodecCtx->pix_fmt = getPictureFormatFromCodec(codecID);

        // Some formats want stream headers to be separate.
        if(pCtx->oformat->flags & AVFMT_GLOBALHEADER)
        {
            pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }

        return pStream;
    }

    bool openVideo(AVCodec* pCodec, AVCodecContext* pCodecCtx, AVFrame** ppFrame, AVPicture* pYUVPicture, const std::string& filename)
    {
        if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
        {
            // H.264 defaults to lossless currently. This should be changed in the future.

            AVDictionary *param = nullptr;
            av_dict_set(&param, "qp", "0", 0);
            /*
            Change options to trade off compression efficiency against encoding speed. If you specify a preset, the changes it makes will be applied before all other parameters are applied.
            You should generally set this option to the slowest you can bear.
            Values available: ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, placebo.
            */
            av_dict_set(&param, "preset", "veryslow", 0);
            /*
            Tune options to further optimize them for your input content. If you specify a tuning, the changes will be applied after --preset but before all other parameters.
            If your source content matches one of the available tunings you can use this, otherwise leave unset.
            Values available: film, animation, grain, stillimage, psnr, ssim, fastdecode, zerolatency.
            */
            av_dict_set(&param, "tune", "film", 0);

            // Open the codec
            if (avcodec_open2(pCodecCtx, pCodec, &param) < 0)
            {
                return error(filename, "Can't open video codec.");
            }
        }
        else
        {
            // Open the codec
            if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
            {
                return error(filename, "Can't open video codec.");
            }
        }

        // create a frame
        *ppFrame = av_frame_alloc();
        if(*ppFrame == nullptr)
        {
            return error(filename, "Video frame allocation failed.");
        }

        // Allocate the encoded picture
        if(avpicture_alloc(pYUVPicture, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height) < 0)
        {
            return error(filename, "Can't allocate destination picture");
        }

        *((AVPicture*)(*ppFrame)) = *pYUVPicture;

        return true;
    }

    VideoEncoder::VideoEncoder(const std::string& filename) : mFilename(filename)
    {
    }

    VideoEncoder::~VideoEncoder()
    {
        endCapture();
    }

    VideoEncoder::UniquePtr VideoEncoder::create(const Desc& desc)
    {
        UniquePtr pVC = UniquePtr(new VideoEncoder(desc.filename));
        if(pVC == nullptr)
        {
            error(desc.filename, "Failed to create CVideoCapture object");
            return nullptr;
        }

        if(pVC->init(desc) == false)
        {
            pVC = nullptr;
        }
        return pVC;
    }

    bool VideoEncoder::init(const Desc& desc)
    {
        // Register the codecs
        avcodec_register_all();
        av_register_all();

        // create the output context
        avformat_alloc_output_context2(&mpOutputContext, nullptr, nullptr, mFilename.c_str());
        if(mpOutputContext == nullptr)
        {
            // The sample tries again, while explicitly requesting mpeg format. I chose not to do it, since it might lead to a container with a wrong extension
            return error(mFilename, "File output format not recognized. Make sure you use a known file extension (avi/mpeg/mp4)");
        }

        // Get the output format of the container
        AVOutputFormat* pOutputFormat = mpOutputContext->oformat;
        assert((pOutputFormat->flags & AVFMT_NOFILE) == 0); // Problem. We want a file.

        // Set the codecs
        pOutputFormat->video_codec = getCodecID(desc.codec);
        pOutputFormat->audio_codec = AV_CODEC_ID_NONE;

        // create the video codec
        AVCodec* pVideoCodec;
        mpOutputStream = createVideoStream(mpOutputContext, desc.width, desc.height, desc.fps, desc.bitrateMbps, desc.gopSize, pOutputFormat->video_codec, mFilename, &pVideoCodec);
        if(mpOutputStream == nullptr)
        {
            return false;
        }

        // Open the video stream
        mpYUVPicture = new AVPicture;
        if(openVideo(pVideoCodec, mpOutputStream->codec, &mpFrame, mpYUVPicture, mFilename) == false)
        {
            return false;
        }

        av_dump_format(mpOutputContext, 0, mFilename.c_str(), 1);

        // Check/create a directory
        const size_t lastSlashIdx = mFilename.find_last_of("\\/");
        if(std::string::npos != lastSlashIdx)
        {
            std::string dir = mFilename.substr(0, lastSlashIdx);
            if(0 != _mkdir(dir.c_str()) && errno != EEXIST)
                return error(mFilename, "Can't create output directory.");
        }

        // Open the output file
        assert((pOutputFormat->flags & AVFMT_NOFILE) == 0); // No output file required. Not sure if/when this happens.
        if(avio_open(&mpOutputContext->pb, mFilename.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            return error(mFilename, "Can't open output file.");
        }

        // Write the stream header
        if(avformat_write_header(mpOutputContext, nullptr) < 0)
        {
            return error(mFilename, "Can't write file header.");
        }

        if(mpFrame)
        {
            mpFrame->pts = 0;
        }

        mForamt = desc.format;
        mRowPitch = getInputFormatBytesPerPixel(desc.format) * desc.width;
        if(desc.flipY)
        {
            mpFlippedImage = new uint8_t[desc.height * mRowPitch];
        }

        AVPixelFormat pixFormat = getPictureFormatFromCodec(pOutputFormat->video_codec);

        mpSwsContext = sws_getContext(desc.width, desc.height, getPictureFormatFromFalcorFormat(desc.format), desc.width, desc.height, pixFormat, SWS_POINT, nullptr, nullptr, nullptr);
        if(mpSwsContext == nullptr)
        {
            return error(mFilename, "Failed to allocate SWScale context");
        }
        return true;
    }

    void VideoEncoder::endCapture()
    {
        if(mpOutputStream)
        {
            avcodec_close(mpOutputStream->codec);
            mpOutputStream = nullptr;
        }

        if(mpYUVPicture)
        {
            av_free(mpYUVPicture->data[0]);
            av_free(mpFrame);
            safe_delete(mpYUVPicture);
            mpFrame = nullptr;
        }

        if(mpOutputContext)
        {
            av_write_trailer(mpOutputContext);
            avio_close(mpOutputContext->pb);
            avformat_free_context(mpOutputContext);
            mpOutputContext = nullptr;
        }
            
        if(mpSwsContext)
        {
            sws_freeContext(mpSwsContext);
            mpSwsContext = nullptr;
        }
        safe_delete(mpFlippedImage)
    }

    void VideoEncoder::appendFrame(const void* pData)
    {
        if(mpFlippedImage)
        {
            // Flip the image
            for(int32_t h = 0; h < mpOutputStream->codec->height; h++)
            {
                const uint8_t* pSrc = (uint8_t*)pData + h * mRowPitch;
                uint8_t* pDst = mpFlippedImage + (mpOutputStream->codec->height - 1 - h) * mRowPitch;
                memcpy(pDst, pSrc, mRowPitch);
            }

            pData = mpFlippedImage;
        }

        // Convert input data to YUV
        sws_scale(mpSwsContext, (uint8_t**)&pData, (int32_t*)&mRowPitch, 0, mpOutputStream->codec->height, mpYUVPicture->data, mpYUVPicture->linesize);

        // Initialize the packet
        AVPacket packet = {0};
        av_init_packet(&packet);
        int32_t gotPacket;

        // Encode the frame
        if(avcodec_encode_video2(mpOutputStream->codec, &packet, mpFrame, &gotPacket) < 0)
        {
            error(mFilename, "Can't encode video frame");
            return;
        }

        // If the size of the frame is zero, the frame was buffered
        if(gotPacket && (packet.size > 0))
        {
            // Write the frame to the file
            packet.stream_index = mpOutputStream->index;
            if(av_interleaved_write_frame(mpOutputContext, &packet) < 0)
            {
                error(mFilename, "Failed when writing encoded frame to file");
            }
        }

        mFrameCount++;
        mpFrame->pts += av_rescale_q(1, mpOutputStream->codec->time_base, mpOutputStream->time_base);
    }

    const std::string VideoEncoder::getSupportedContainerForCodec(CodecID codec)
    {
        const std::string AVI = std::string("AVI (Audio Video Interleaved)") + '\0' + "*.avi" + '\0';
        const std::string MP4 = std::string("MP4 (MPEG-4 Part 14)") + '\0' + "*.mp4" + '\0';
        const std::string MKV = std::string("MKV (Matroska)\0*.mkv") + '\0' + "*.mkv" + '\0';

        std::string s;
        switch(codec)
        {
        case VideoEncoder::CodecID::RawVideo:
            s += AVI;
            break;
        case VideoEncoder::CodecID::H264:
        case VideoEncoder::CodecID::MPEG2:
        case VideoEncoder::CodecID::MPEG4:
            s += MP4 + MKV + AVI;
            break;
        case VideoEncoder::CodecID::HEVC:
            s += MP4 + MKV;
            break;
        default:
            should_not_get_here();
        }

        s += "\0";
        return s;
    }
}