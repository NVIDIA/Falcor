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
#include "API/Texture.h"
#include "API/Device.h"

namespace Falcor
{
	uint32_t Texture::tempDefaultUint = 0;

    Texture::Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, uint32_t sampleCount, ResourceFormat format, Type type, BindFlags bindFlags) :
        Resource(type, bindFlags), mWidth(width), mHeight(height), mDepth(depth), mMipLevels(mipLevels), mSampleCount(sampleCount), mArraySize(arraySize), mFormat(format)
    {
        if(mMipLevels == kMaxPossible)
        {
            uint32_t dims = width | height | depth;
            unsigned long bits;
            _BitScanReverse(&bits, dims);
            mMipLevels = (uint32_t)bits + 1;
        }
        apiInit();
    }

    uint32_t Texture::getDataSize() const
    {
        uint32_t res = 0;
        for(uint32_t l = 0; l < mMipLevels; l++)
        {
            res += getMipLevelDataSize(l);
        }
        return res;
    }

    void Texture::captureToFile(uint32_t mipLevel, uint32_t arraySlice, const std::string& filename, Bitmap::FileFormat format, Bitmap::ExportFlags exportFlags) const
    {
        uint32_t subresource = getSubresourceIndex(arraySlice, mipLevel);
        std::vector<uint8> textureData = gpDevice->getRenderContext()->readTextureSubresource(this, subresource);
        Bitmap::saveImage(filename, getWidth(mipLevel), getHeight(mipLevel), format, exportFlags, getFormat(), true, textureData.data());
    }
}
