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
#ifdef FALCOR_DX11
#include "Core/FBO.h"
#include "Core/Texture.h"
#include "glm/gtc/type_ptr.hpp"
#include <map>

namespace Falcor
{
    struct FboData
    {
        std::vector<ID3D11RenderTargetViewPtr> pRtv;
        ID3D11DepthStencilViewPtr pDsv;
    };

    Fbo::Fbo(bool initApiHandle)
    {
        mColorAttachments.resize(getMaxColorTargetCount());
        FboData* pFboData = new FboData;
        pFboData->pRtv.resize(getMaxColorTargetCount());
        mpPrivateData = pFboData;

        mApiHandle = -1;
    }

    Fbo::~Fbo()
    {
        delete (FboData*)mpPrivateData;
        mpPrivateData = nullptr;
    }

    uint32_t Fbo::getApiHandle() const
    {
        UNSUPPORTED_IN_DX11("CFbo Api Handle");
        return mApiHandle;
    }

    uint32_t Fbo::getMaxColorTargetCount()
    {
        return D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    }

    void initializeRtvDesc(const Texture* pTexture, uint32_t mipLevel, uint32_t arraySlice, D3D11_RENDER_TARGET_VIEW_DESC& desc)
    {
        uint32_t arrayMultiplier = (pTexture->getType() == Texture::Type::TextureCube) ? 6 : 1;
        uint32_t arraySize = 1;

        if(arraySlice == Fbo::kAttachEntireMipLevel)
        {
            arraySlice = 0;
            arraySize = pTexture->getArraySize();
        }

        switch(pTexture->getType())
        {
        case Texture::Type::Texture1D:
            if(pTexture->getArraySize() > 1)
            {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.ArraySize = arraySize;
                desc.Texture1DArray.FirstArraySlice = arraySlice;
                desc.Texture1DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture2D:
        case Texture::Type::TextureCube:
            if(pTexture->getArraySize() * arrayMultiplier > 1)
            {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.ArraySize = arraySize * arrayMultiplier;
                desc.Texture2DArray.FirstArraySlice = arraySlice * arrayMultiplier;
                desc.Texture2DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture3D:
            assert(pTexture->getArraySize() == 1);
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.FirstWSlice = arraySlice;
            desc.Texture3D.MipSlice = mipLevel;
            desc.Texture3D.WSize = arraySize;
            break;
        case Texture::Type::Texture2DMultisample:
            if(pTexture->getArraySize() > 1)
            {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.ArraySize = arraySize;
                desc.Texture2DMSArray.FirstArraySlice = arraySlice;
            }
            else
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            break;
        default:
            should_not_get_here();
        }
        desc.Format = getDxgiFormat(pTexture->getFormat());
    }

    void initializeDsvDesc(const Texture* pTexture, uint32_t mipLevel, uint32_t arraySlice, D3D11_DEPTH_STENCIL_VIEW_DESC& desc)
    {
        uint32_t arrayMultiplier = (pTexture->getType() == Texture::Type::TextureCube) ? 6 : 1;
        uint32_t arraySize = 1;

        if(arraySlice == Fbo::kAttachEntireMipLevel)
        {
            arraySlice = 0;
            arraySize = pTexture->getArraySize();
        }

        switch(pTexture->getType())
        {
        case Texture::Type::Texture1D:
            if(pTexture->getArraySize() > 1)
            {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.ArraySize = arraySize;
                desc.Texture1DArray.FirstArraySlice = arraySlice;
                desc.Texture1DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture2D:
        case Texture::Type::TextureCube:
            if(pTexture->getArraySize() * arrayMultiplier > 1)
            {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.ArraySize = arraySize * arrayMultiplier;
                desc.Texture2DArray.FirstArraySlice = arraySlice * arrayMultiplier;
                desc.Texture2DArray.MipSlice = mipLevel;
            }
            else
            {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = mipLevel;
            }
            break;
        case Texture::Type::Texture2DMultisample:
            if(pTexture->getArraySize() > 1)
            {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.ArraySize = arraySize;
                desc.Texture2DMSArray.FirstArraySlice = arraySlice;
            }
            else
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            break;
        default:
            should_not_get_here();
        }
        desc.Format = getDxgiFormat(pTexture->getFormat());
        desc.Flags = 0;
    }

    void Fbo::applyColorAttachment(uint32_t rtIndex)
    {
        FboData* pData = (FboData*)mpPrivateData;
        pData->pRtv[rtIndex] = nullptr;
        const auto pTexture = mColorAttachments[rtIndex].pTexture;
        if(pTexture)
        {
            ID3D11Resource* pResource = pTexture->getApiHandle();
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            initializeRtvDesc(pTexture.get(), mColorAttachments[rtIndex].mipLevel, mColorAttachments[rtIndex].arraySlice, rtvDesc);
            ID3D11RenderTargetViewPtr pRTV;
            dx11_call(getD3D11Device()->CreateRenderTargetView(pResource, &rtvDesc, &pData->pRtv[rtIndex]));
        }
    }

    void Fbo::applyDepthAttachment()
    {
        FboData* pData = (FboData*)mpPrivateData;

        pData->pDsv = nullptr;
        const auto pDepth = mDepthStencil.pTexture;
        if(pDepth)
        {
            ID3D11Resource* pResource = pDepth->getApiHandle();
            D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc;
            initializeDsvDesc(pDepth.get(), mDepthStencil.mipLevel, mDepthStencil.arraySlice, DsvDesc);
            dx11_call(getD3D11Device()->CreateDepthStencilView(pResource, &DsvDesc, &pData->pDsv));
        }
    }

    bool Fbo::checkStatus() const
    {
        FboData* pData = (FboData*)mpPrivateData;

        if(mIsDirty)
        {
            mIsDirty = false;
            return calcAndValidateProperties();
        }

        return true;
    }
    
    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::vec4& color) const
    {
        FboData* pFboData = (FboData*)mpPrivateData;
        const auto pTexture = mColorAttachments[rtIndex].pTexture;
        if(pTexture == nullptr)
        {
            Logger::log(Logger::Level::Warning, "Trying to clear a color render-target, but the texture does not exist in CFbo.");
            return;
        }
        if(checkStatus())
        {
            getD3D11ImmediateContext()->ClearRenderTargetView(pFboData->pRtv[rtIndex], glm::value_ptr(color));
        }
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::uvec4& color) const
    {
        UNSUPPORTED_IN_DX11("unsigned int version of ClearColorTarget()");
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::ivec4& color) const
    {
        UNSUPPORTED_IN_DX11("int version of ClearColorTarget()");
    }

    void Fbo::captureToFile(uint32_t rtIndex, const std::string& filename, Bitmap::FileFormat fileFormat, uint32_t exportFlags)
    {
        UNSUPPORTED_IN_DX11("captureToPng()");
    }

    void Fbo::clearDepthStencil(float depth, uint8_t stencil, bool clearDepth, bool clearStencil) const
    {
        FboData* pFboData = (FboData*)mpPrivateData;
        const auto pTexture = mDepthStencil.pTexture;
        if(pTexture == nullptr)
        {
            Logger::log(Logger::Level::Warning, "Trying to clear a depth buffer, but the texture does not exist in CFbo.");
            return;
        }

        if(!clearDepth && !clearStencil)
        {
            Logger::log(Logger::Level::Warning, "Trying to clear a depth buffer, but both bClearDepth and bClearStencil are false.");
            return;
        }

        if(checkStatus())
        {
            uint32_t clearFlags = 0;
            clearFlags |= clearDepth ? D3D11_CLEAR_DEPTH : 0;
            clearFlags |= clearStencil ? D3D11_CLEAR_STENCIL : 0;
            getD3D11ImmediateContext()->ClearDepthStencilView(pFboData->pDsv, clearFlags, depth, stencil);
        }
    }

    ID3D11DepthStencilView* Fbo::getDepthStencilView() const
    {
        FboData* pFboData = (FboData*)mpPrivateData;
        return pFboData->pDsv;
    }

    ID3D11RenderTargetView* Fbo::getRenderTargetView(uint32_t RtIndex) const
    {
        FboData* pFboData = (FboData*)mpPrivateData;
        return pFboData->pRtv[RtIndex];
    }

}
#endif //#ifdef FALCOR_DX11
