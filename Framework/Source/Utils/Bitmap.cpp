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
#include "Bitmap.h"
#include "FreeImage.h"
#include "OS.h"

namespace Falcor
{
    const Bitmap* genError(const std::string& errMsg, const std::string& filename)
    {
        std::string err = "Error when loading image file " + filename + '\n' + errMsg + '.';
        logError(err);
        return nullptr;
    }

    Bitmap::UniqueConstPtr Bitmap::createFromFile(const std::string& filename, bool isTopDown)
    {
        std::string fullpath;
        if(findFileInDataDirectories(filename, fullpath) == false)
        {
            return UniqueConstPtr(genError("Can't find the file", filename));
        }

        FREE_IMAGE_FORMAT fifFormat = FIF_UNKNOWN;
        
        fifFormat = FreeImage_GetFileType(fullpath.c_str(), 0);
        if(fifFormat == FIF_UNKNOWN)
        {
            // Can't get the format from the file. Use file extension
            fifFormat = FreeImage_GetFIFFromFilename(fullpath.c_str());

            if(fifFormat == FIF_UNKNOWN)
            {
                return UniqueConstPtr(genError("Image Type unknown", filename));
            }
        }

        // Check the the library supports loading this image Type
        if(FreeImage_FIFSupportsReading(fifFormat) == false)
        {
            return UniqueConstPtr(genError("Library doesn't support the file format", filename));
        }

        // Read the DIB
        FIBITMAP* pDib = FreeImage_Load(fifFormat, fullpath.c_str());
        if(pDib == nullptr)
        {
            return UniqueConstPtr(genError("Can't read image file", filename));
        }

        // create the bitmap
        auto pBmp = new Bitmap;
        pBmp->mHeight = FreeImage_GetHeight(pDib);
        pBmp->mWidth = FreeImage_GetWidth(pDib);

        if(pBmp->mHeight == 0 || pBmp->mWidth == 0 || FreeImage_GetBits(pDib) == nullptr)
        {
            return UniqueConstPtr(genError("Invalid image", filename));
        }

        // Convert the image to RGBA image
        uint32_t bpp = FreeImage_GetBPP(pDib);
        if(bpp == 24)
        {
            bpp = 32;
            auto pNew = FreeImage_ConvertTo32Bits(pDib);
            FreeImage_Unload(pDib);
            pDib = pNew;
        }

        switch(bpp)
        {
        case 128:
            pBmp->mBytesPerPixel = 16;  // 4xfloat32 HDR format
            break;
        case 96:
            pBmp->mBytesPerPixel = 12;  // 3xfloat32 HDR format
            break;
        case 64:
            pBmp->mBytesPerPixel = 8;  // 4xfloat16 HDR format
            break;
        case 48:
            pBmp->mBytesPerPixel = 6;  // 3xfloat16 HDR format
            break;
        case 32:
            pBmp->mBytesPerPixel = 4;
            break;
        case 24:
            pBmp->mBytesPerPixel = 3;
            break;
        case 16:
            pBmp->mBytesPerPixel = 2;
            break;
        case 8:
            pBmp->mBytesPerPixel = 1;
            break;
        default:
            genError("Unknown bits-per-pixel", filename);
            return nullptr;
        }

        pBmp->mpData = new uint8_t[pBmp->mHeight * pBmp->mWidth * pBmp->mBytesPerPixel];
        FreeImage_ConvertToRawBits(pBmp->mpData, pDib, pBmp->mWidth * pBmp->mBytesPerPixel, bpp, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, isTopDown);

        FreeImage_Unload(pDib);
        return UniqueConstPtr(pBmp);
    }

    Bitmap::~Bitmap()
    {
        delete[] mpData;
        mpData = nullptr;
    }

    static FREE_IMAGE_FORMAT toFreeImageFormat(Bitmap::FileFormat fmt)
    {
        switch(fmt)
        {
        case Bitmap::FileFormat::PngFile:
            return FIF_PNG;
        case Bitmap::FileFormat::PfmFile:
            return FIF_PFM;
        default:
            should_not_get_here();
        }
        return FIF_PNG;        
    }

    static FREE_IMAGE_TYPE getImageType(uint32_t bytesPerPixel)
    {
        switch(bytesPerPixel)
        {
        case 4:
            return FIT_BITMAP;
        case 12:
        case 16:
            return FIT_RGBF;
        default:
            should_not_get_here();
        }
        return FIT_BITMAP;
    }

    void Bitmap::saveImage(const std::string& filename, uint32_t width, uint32_t height, FileFormat fileFormat, ResourceFormat resourceFormat, bool isTopDown, void* pData)
    {
        if(pData)
        {
            uint32_t bytesPerPixel = getFormatBytesPerBlock(resourceFormat);
            assert(bytesPerPixel == 32 || getFormatChannelCount(resourceFormat) == 4);
            FIBITMAP* pImage;

            //TODO replace this code for swapping channels. Can't use freeimage masks b/c they only care about 16 bpp images
            //issue #74 in gitlab
            for (uint32_t a = 0; a < width*height; a++)
            {
                uint32_t* pPixel = (uint32_t*)pData;
                pPixel += a;
                uint8_t* ch = (uint8_t*)pPixel;
                if (resourceFormat == ResourceFormat::RGBA8Uint || resourceFormat == ResourceFormat::RGBA8Snorm || resourceFormat == ResourceFormat::RGBA8UnormSrgb)
                {
                    std::swap(ch[0], ch[2]);
                }
                ch[3] = 0xff;
            }
            

            if(fileFormat == Bitmap::FileFormat::PngFile)
                pImage = FreeImage_ConvertFromRawBits((BYTE*)pData, width, height, bytesPerPixel * width, bytesPerPixel*8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, isTopDown);
            else
            {
                if(fileFormat != Bitmap::FileFormat::PfmFile || (bytesPerPixel != 16 && bytesPerPixel != 12))
                    logError("Bitmap::saveImage supports only 32-bit/channel RGB/RGBA images as HDR source.");
                // Upload the image manually
                pImage = FreeImage_AllocateT(getImageType(bytesPerPixel), width, height);
                BYTE* head = (BYTE*)pData;
                for(unsigned y = 0; y < height; y++) {
                    float* dstBits = (float*)FreeImage_GetScanLine(pImage, y);
                    if(bytesPerPixel == 12)
                        memcpy(dstBits, head, bytesPerPixel * width);
                    else
                    {
                        for(unsigned x = 0; x < width; x++) {
                            dstBits[x*3 + 0] = (((float*)head)[x*4 + 0]);
                            dstBits[x*3 + 1] = (((float*)head)[x*4 + 1]);
                            dstBits[x*3 + 2] = (((float*)head)[x*4 + 2]);
                        }
                    }
                    head += bytesPerPixel * width;
                }
            }

            FREE_IMAGE_TYPE type = FreeImage_GetImageType(pImage);

            FreeImage_Save(toFreeImageFormat(fileFormat), pImage, filename.c_str(), 0);
            FreeImage_Unload(pImage);
        }
    }
}