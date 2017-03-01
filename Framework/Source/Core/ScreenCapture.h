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
#include "Formats.h"
#include "Utils/Bitmap.h"

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    /** Helper class for capturing screenshots
    */
    namespace ScreenCapture
    {
        /** Capture a screenshot to a pre-allocated memory buffer.
        \param[in] screenWidth The width of the screen
        \param[in] screenHeight The height of the screen
        \param[in] format The requested data format
        \param[in] pData Destination buffer. Buffer should be allocated by the user. Buffer size should be ScreenWidth*ScreenHeight*getFormatBytesPerBlock(Format)
        */
        void captureToMemory(uint32_t screenWidth, uint32_t screenHeight, ResourceFormat format, uint8_t* pData);
        
        /** Capture a screenshot to a PNG file.
            \param[in] screenWidth The width of the screen
            \param[in] screenHeight The height of the screen
            \param[in] filename Output filename
        */
        void captureToPng(uint32_t screenWidth, uint32_t screenHeight, const std::string& filename);
        void captureToImage(uint32_t screenWidth, uint32_t screenHeight, const std::string& filename, Bitmap::FileFormat fileFormat, Bitmap::ExportFlags flags);

        /*! @} */
    };
}