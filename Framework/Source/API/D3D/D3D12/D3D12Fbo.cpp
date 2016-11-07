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
#include "API/FBO.h"
#include "Api/LowLevel/DescriptorHeap.h"
#include "API/D3D/D3DViews.h"
#include "API/Device.h"

namespace Falcor
{
    struct FboData
    {
        std::map<const Texture*, uint32_t> rtvMap;
        std::map<const Texture*, uint32_t> dsvMap;
    };

    template<>
    D3D12_RTV_DIMENSION getViewDimension<D3D12_RTV_DIMENSION>(Texture::Type type, uint32_t arraySize)
    {
        switch(type)
        {
        case Texture::Type::Texture1D:
            return (arraySize > 1) ? D3D12_RTV_DIMENSION_TEXTURE1DARRAY : D3D12_RTV_DIMENSION_TEXTURE1D;
        case Texture::Type::Texture2D:
            return (arraySize > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2D;
        case Texture::Type::Texture3D:
            assert(arraySize == 1);
            return D3D12_RTV_DIMENSION_TEXTURE3D;
        case Texture::Type::Texture2DMultisample:
            return (arraySize > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D12_RTV_DIMENSION_TEXTURE2DMS;
        case Texture::Type::TextureCube:
            return D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        default:
            should_not_get_here();
            return D3D12_RTV_DIMENSION_UNKNOWN;
        }
    }

    template<>
    D3D12_DSV_DIMENSION getViewDimension<D3D12_DSV_DIMENSION>(Texture::Type type, uint32_t arraySize)
    {
        switch(type)
        {
        case Texture::Type::Texture1D:
            return (arraySize > 1) ? D3D12_DSV_DIMENSION_TEXTURE1DARRAY : D3D12_DSV_DIMENSION_TEXTURE1D;
        case Texture::Type::Texture2D:                                  
            return (arraySize > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;
        case Texture::Type::Texture2DMultisample:
            return (arraySize > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D12_DSV_DIMENSION_TEXTURE2DMS;
        case Texture::Type::TextureCube:
            return D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        default:
            should_not_get_here();
            return D3D12_DSV_DIMENSION_UNKNOWN;
        }
    }

    Fbo::Fbo(bool initApiHandle)
    {
        mApiHandle = -1;
        FboData* pData = new FboData;
        mpPrivateData = pData;
        mColorAttachments.resize(getMaxColorTargetCount());
    }

    Fbo::~Fbo()
    {
        FboData* pData = (FboData*)mpPrivateData;
        safe_delete(pData);
    }

    uint32_t Fbo::getApiHandle() const
    {
        UNSUPPORTED_IN_D3D12("Fbo::getApiHandle()");
        return mApiHandle;
    }

    RtvHandle Fbo::getRenderTargetView(uint32_t rtIndex) const
    {
        FboData* pData = (FboData*)mpPrivateData;
        const auto& pTexture = getColorTexture(rtIndex).get();
        auto& it = pData->rtvMap.find(pTexture);
        uint32_t rtvIndex;

        DescriptorHeap::SharedPtr&& pHeap = gpDevice->getRtvDescriptorHeap();

        if(it == pData->rtvMap.end())
        {
            // Create the render-target view
            ID3D12ResourcePtr pResource = pTexture ? pTexture->getApiHandle() : nullptr;
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            if(pTexture)
            {
                initializeRtvDesc<D3D12_RENDER_TARGET_VIEW_DESC>(pTexture, mColorAttachments[rtIndex].mipLevel, mColorAttachments[rtIndex].arraySlice, rtvDesc);
            }
            else
            {
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            }
            rtvIndex = pHeap->allocateHandle();
            DescriptorHeap::CpuHandle rtv = pHeap->getCpuHandle(rtvIndex);
            gpDevice->getApiHandle()->CreateRenderTargetView(pResource, &rtvDesc, rtv);
            pData->rtvMap[pTexture] = rtvIndex;
        }
        else
        {
            rtvIndex = it->second;
        }

        return pHeap->getCpuHandle(rtvIndex);
    }

    DsvHandle Fbo::getDepthStencilView() const
    {
        FboData* pData = (FboData*)mpPrivateData;
        const auto& pTexture = getDepthStencilTexture().get();
        auto& it = pData->dsvMap.find(pTexture);
        uint32_t dsvIndex;

        DescriptorHeap::SharedPtr&& pHeap = gpDevice->getDsvDescriptorHeap();

        if(it == pData->dsvMap.end())
        {
            // Create the render-target view
            ID3D12ResourcePtr pResource = pTexture ? pTexture->getApiHandle() : nullptr;
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            if(pTexture)
            {
                initializeDsvDesc<D3D12_DEPTH_STENCIL_VIEW_DESC>(pTexture, mDepthStencil.mipLevel, mDepthStencil.arraySlice, dsvDesc);
            }
            else
            {
                dsvDesc.Format = DXGI_FORMAT_D16_UNORM;
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            }
            dsvIndex = pHeap->allocateHandle();
            DescriptorHeap::CpuHandle dsv = pHeap->getCpuHandle(dsvIndex);
            gpDevice->getApiHandle()->CreateDepthStencilView(pResource, &dsvDesc, dsv);
            pData->dsvMap[pTexture] = dsvIndex;
        }
        else
        {
            dsvIndex = it->second;
        }

        return pHeap->getCpuHandle(dsvIndex);
    }

    uint32_t Fbo::getMaxColorTargetCount()
    {
        return D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    }

    void Fbo::applyColorAttachment(uint32_t rtIndex)
    {
    }

    void Fbo::applyDepthAttachment()
    {
    }

    void Fbo::resetViews()
    {
        FboData* pData = (FboData*)mpPrivateData;
        pData->dsvMap.clear();
        pData->rtvMap.clear();
    }

    bool Fbo::checkStatus() const
    {
        if (mIsDirty)
        {
            mIsDirty = false;
            return calcAndValidateProperties();
        }
        return true;
    }

    void Fbo::captureToFile(uint32_t rtIndex, const std::string& filename, Bitmap::FileFormat fileFormat)
    {
    }
}
