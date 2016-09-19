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
#ifdef FALCOR_D3D12
#include "Framework.h"
#include "Core/FBO.h"
#include "D3D12DescriptorHeap.h"
#include "Core/D3D/D3DViews.h"
#include "Core/Device.h"

namespace Falcor
{
    struct FboData
    {
        static DescriptorHeap::SharedPtr spRtvHeap;
        static DescriptorHeap::SharedPtr spDsvHeap;
        static const uint32_t kMaxRenderTargetViews = 16;
        static const uint32_t kMaxDepthStencilViews = 16;

        std::map<ID3D12Resource*, uint32_t> rtvMap;
        std::map<ID3D12Resource*, uint32_t> dsvMap;
    };

    DescriptorHeap::SharedPtr FboData::spRtvHeap;
    DescriptorHeap::SharedPtr FboData::spDsvHeap;

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
        if(pData->spRtvHeap == nullptr)
        {
            assert(pData->spDsvHeap == nullptr);
            pData->spRtvHeap = DescriptorHeap::create(DescriptorHeap::Type::RenderTargetView, FboData::kMaxRenderTargetViews, false);
            pData->spDsvHeap = DescriptorHeap::create(DescriptorHeap::Type::DepthStencilView, FboData::kMaxDepthStencilViews, false);
        }
        mColorAttachments.resize(getMaxColorTargetCount());
    }

    Fbo::~Fbo()
    {
    }

    uint32_t Fbo::getApiHandle() const
    {
        UNSUPPORTED_IN_D3D12("Fbo::getApiHandle()");
        return mApiHandle;
    }

    RtvHandle Fbo::getRenderTargetView(uint32_t rtIndex) const
    {
        FboData* pData = (FboData*)mpPrivateData;
        const auto& pTexture = getColorTexture(rtIndex);
        assert(pTexture);
        const auto& it = pData->rtvMap.find(pTexture->getApiHandle());
        assert(it != pData->rtvMap.end());
        return pData->spRtvHeap->getHandle(it->second);
    }

    DsvHandle Fbo::getDepthStencilView() const
    {
        FboData* pData = (FboData*)mpPrivateData;
        const auto& pTexture = getDepthStencilTexture();
        assert(pTexture);
        const auto& it = pData->dsvMap.find(pTexture->getApiHandle());
        assert(it != pData->dsvMap.end());
        return pData->spDsvHeap->getHandle(it->second);
    }

    uint32_t Fbo::getMaxColorTargetCount()
    {
        return D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    }

    void Fbo::applyColorAttachment(uint32_t rtIndex)
    {
        FboData* pData = (FboData*)mpPrivateData;
        const auto pTexture = mColorAttachments[rtIndex].pTexture;
        if(pTexture)
        {
            // Check if we already created an RTV for the texture
            ID3D12ResourcePtr pResource = pTexture->getApiHandle();
            if(pData->rtvMap.find(pResource) == pData->rtvMap.end())
            {
                // Create an RTV
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
                initializeRtvDesc<D3D12_RENDER_TARGET_VIEW_DESC>(pTexture.get(), mColorAttachments[rtIndex].mipLevel, mColorAttachments[rtIndex].arraySlice, rtvDesc);
                uint32_t rtvIndedx = pData->spRtvHeap->getCurrentIndex();
                DescriptorHeap::CpuHandle rtv = pData->spRtvHeap->getFreeCpuHandle();
                Device::getApiHandle()->CreateRenderTargetView(pResource, &rtvDesc, rtv);
                pData->rtvMap[pResource] = rtvIndedx;
            }
        }
    }

    void Fbo::applyDepthAttachment()
    {
        FboData* pData = (FboData*)mpPrivateData;
        if(mDepthStencil.pTexture)
        {
            // Check if we already created an DSV for the texture
            ID3D12ResourcePtr pResource = mDepthStencil.pTexture->getApiHandle();
            if(pData->dsvMap.find(pResource) == pData->dsvMap.end())
            {
                // Create an RTV
                D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                initializeDsvDesc<D3D12_DEPTH_STENCIL_VIEW_DESC>(mDepthStencil.pTexture.get(), mDepthStencil.mipLevel, mDepthStencil.arraySlice, dsvDesc);
                uint32_t dsvIndex = pData->spDsvHeap->getCurrentIndex();
                DescriptorHeap::CpuHandle dsv = pData->spDsvHeap->getFreeCpuHandle();
                Device::getApiHandle()->CreateDepthStencilView(pResource, &dsvDesc, dsv);
                pData->dsvMap[pResource] = dsvIndex;
            }
        }
    }

    bool Fbo::checkStatus() const
    {
        return true;
    }
    
    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::vec4& color) const
    {
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::uvec4& color) const
    {
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::ivec4& color) const
    {
    }

    void Fbo::captureToFile(uint32_t rtIndex, const std::string& filename, Bitmap::FileFormat fileFormat)
    {
    }

    void Fbo::clearDepthStencil(float depth, uint8_t stencil, bool clearDepth, bool clearStencil) const
    {
    }
}
#endif //#ifdef FALCOR_D3D12
