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
#include "Core/ScreenCapture.h"
#include "Utils/OS.h"
#include "Utils/Bitmap.h"

namespace Falcor
{
    namespace ScreenCapture
    {
        void captureToMemory(uint32_t screenWidth, uint32_t screenHeight, ResourceFormat format, uint8_t* pData)
        {
            gl_call(glPixelStorei(GL_PACK_ALIGNMENT, 1));
            gl_call(glNamedFramebufferReadBuffer(0, GL_BACK));
            GLenum glFormat = getGlBaseFormat(format);
            GLenum glType = getGlFormatType(format);

            // Store the current read FB
            GLint boundFB;
            gl_call(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundFB));

            // Capture
            gl_call(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
            gl_call(glReadPixels(0, 0, screenWidth, screenHeight, glFormat, glType, pData));

            // Restore the read FB
            gl_call(glBindFramebuffer(GL_READ_FRAMEBUFFER, boundFB));
        }

        void captureToPng(uint32_t screenWidth, uint32_t screenHeight, const std::string& filename)
        {
            ResourceFormat format = ResourceFormat::BGRA8Unorm;
            uint32_t bytesPerPixel = getFormatBytesPerBlock(format);
            uint32_t bufferSize = screenHeight*screenWidth * bytesPerPixel;

            std::vector<uint8_t> data(bufferSize);
            captureToMemory(screenWidth, screenHeight, format, data.data());
            Bitmap::saveImage(filename, screenWidth, screenHeight, Bitmap::FileFormat::PngFile, bytesPerPixel, false, data.data());
        }
    }
}
#endif //#ifdef FALCOR_GL
