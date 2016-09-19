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
#ifdef FALCOR_D3D
#include "Framework.h"

namespace Falcor
{
    template<typename ViewType>
    ViewType getViewDimension(Texture::Type type, uint32_t arraySize);

    template<typename DescType>
    inline void initializeRtvDesc(const Texture* pTexture, uint32_t mipLevel, uint32_t arraySlice, DescType& desc)
    {
        desc = {};
        uint32_t arrayMultiplier = (pTexture->getType() == Texture::Type::TextureCube) ? 6 : 1;
        uint32_t arraySize = 1;

        if(arraySlice == Fbo::kAttachEntireMipLevel)
        {
            arraySlice = 0;
            arraySize = pTexture->getArraySize();
        }

        desc.ViewDimension = getViewDimension<decltype(desc.ViewDimension)>(pTexture->getType(), arraySize);

        switch(pTexture->getType())
        {
        case Texture::Type::Texture1D:
            if(pTexture->getArraySize() > 1)
            {
                desc.Texture1DArray.ArraySize = arraySize;
                desc.Texture1DArray.FirstArraySlice = arraySlice;
                desc.Texture1DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.Texture1D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture2D:
        case Texture::Type::TextureCube:
            if(pTexture->getArraySize() * arrayMultiplier > 1)
            {
                desc.Texture2DArray.ArraySize = arraySize * arrayMultiplier;
                desc.Texture2DArray.FirstArraySlice = arraySlice * arrayMultiplier;
                desc.Texture2DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.Texture2D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture3D:
            desc.Texture3D.FirstWSlice = arraySlice;
            desc.Texture3D.MipSlice = mipLevel;
            desc.Texture3D.WSize = arraySize;
            break;
        case Texture::Type::Texture2DMultisample:
            if(pTexture->getArraySize() > 1)
            {
                desc.Texture2DMSArray.ArraySize = arraySize;
                desc.Texture2DMSArray.FirstArraySlice = arraySlice;
            }
            break;
        default:
            should_not_get_here();
        }
        desc.Format = getDxgiFormat(pTexture->getFormat());
    }

    template<typename DescType>
    inline void initializeDsvDesc(const Texture* pTexture, uint32_t mipLevel, uint32_t arraySlice, DescType& desc)
    {
        uint32_t arrayMultiplier = (pTexture->getType() == Texture::Type::TextureCube) ? 6 : 1;
        uint32_t arraySize = 1;

        if(arraySlice == Fbo::kAttachEntireMipLevel)
        {
            arraySlice = 0;
            arraySize = pTexture->getArraySize();
        }

        desc.ViewDimension = getViewDimension(pTexture->getType());

        switch(pTexture->getType())
        {
        case Texture::Type::Texture1D:
            if(pTexture->getArraySize() > 1)
            {
                desc.Texture1DArray.ArraySize = arraySize;
                desc.Texture1DArray.FirstArraySlice = arraySlice;
                desc.Texture1DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.Texture1D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture2D:
        case Texture::Type::TextureCube:
            if(pTexture->getArraySize() * arrayMultiplier > 1)
            {
                desc.Texture2DArray.ArraySize = arraySize * arrayMultiplier;
                desc.Texture2DArray.FirstArraySlice = arraySlice * arrayMultiplier;
                desc.Texture2DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.Texture2D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture2DMultisample:
            if(pTexture->getArraySize() > 1)
            {
                desc.Texture2DMSArray.ArraySize = arraySize;
                desc.Texture2DMSArray.FirstArraySlice = arraySlice;
            }
            break;
        default:
            should_not_get_here();
        }
        desc.Format = getDxgiFormat(pTexture->getFormat());
        desc.Flags = 0;
    }
}
#endif //#ifdef FALCOR_D3D
