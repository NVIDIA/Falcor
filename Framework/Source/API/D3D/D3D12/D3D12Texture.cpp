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
#include "API/Texture.h"
#include "API/Device.h"
#include "API/D3D/D3DViews.h"
#include <vector>

namespace Falcor
{
    template<>
    D3D12_SRV_DIMENSION getViewDimension<D3D12_SRV_DIMENSION>(Texture::Type type, uint32_t arraySize)
    {
        switch(type)
        {
        case Texture::Type::Texture1D:
            return (arraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY : D3D12_SRV_DIMENSION_TEXTURE1D;
        case Texture::Type::Texture2D:
            return (arraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;
        case Texture::Type::Texture3D:
            assert(arraySize == 1);
            return D3D12_SRV_DIMENSION_TEXTURE3D;
        case Texture::Type::Texture2DMultisample:
            return (arraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D12_SRV_DIMENSION_TEXTURE2DMS;
        case Texture::Type::TextureCube:
            return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        default:
            should_not_get_here();
            return D3D12_SRV_DIMENSION_UNKNOWN;
        }
    }

    static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
    {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };

    Texture::~Texture()
    {
    }

    uint64_t Texture::makeResident(const Sampler* pSampler) const
    {
        UNSUPPORTED_IN_D3D12("Texture::makeResident()");
        return 0;
    }

    void Texture::evict(const Sampler* pSampler) const
    {
        UNSUPPORTED_IN_D3D12("Texture::evict()");
    }

    Texture::SharedPtr Texture::create1D(uint32_t width, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData)
    {
        return nullptr;
    }
    
    Texture::SharedPtr Texture::create2D(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData)
    {
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, height, 1, 1, mipLevels, arraySize, format, Type::Texture2D));

        D3D12_RESOURCE_DESC desc = {};
        desc.MipLevels = uint16_t(mipLevels);
        desc.Format = getDxgiFormat(format);
        desc.Width = align_to(getFormatWidthCompressionRatio(format), width);
        desc.Height = align_to(getFormatHeightCompressionRatio(format), height);
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        desc.DepthOrArraySize = arraySize;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Alignment = 0;

        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_CLEAR_VALUE* pClearVal = nullptr;
        if (isCompressedFormat(format) == false)
        {
            desc.Flags = isDepthStencilFormat(format) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            clearValue.Format = desc.Format;
            if (isDepthStencilFormat(format))
            {
                clearValue.DepthStencil.Depth = 1.0f;
            }
            pClearVal = &clearValue;
        }
        
        d3d_call(gpDevice->getApiHandle()->CreateCommittedResource(&kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, pClearVal, IID_PPV_ARGS(&pTexture->mApiHandle)));

        if(pData)
        {
            auto& pCopyCtx = gpDevice->getCopyContext();
            pCopyCtx->updateTexture(pTexture.get(), pData);
            pCopyCtx->flush();
        }
        return pTexture;
    }

    SrvHandle Texture::getShaderResourceView() const
    {
        DescriptorHeap::SharedPtr&& pHeap = gpDevice->getSrvDescriptorHeap();
        if(mSrvIndex == -1)
        {
            // Create the shader-resource view
            D3D12_SHADER_RESOURCE_VIEW_DESC desc;
            initializeSrvDesc(mFormat, mType, mArraySize, desc);
            mSrvIndex = pHeap->getCurrentIndex();
            DescriptorHeap::CpuHandle srv = pHeap->getFreeCpuHandle();
            gpDevice->getApiHandle()->CreateShaderResourceView(mApiHandle, &desc, srv);
        }

        return pHeap->getHandle(mSrvIndex);
    }

    Texture::SharedPtr Texture::create3D(uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels, const void* pData, bool isSparse)
    {
        return nullptr;
    }

    // Texture Cube
    Texture::SharedPtr Texture::createCube(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData)
    {
        return nullptr;
    }

    Texture::SharedPtr Texture::create2DMS(uint32_t width, uint32_t height, ResourceFormat format, uint32_t sampleCount, uint32_t arraySize, bool useFixedSampleLocations)
    {
        return nullptr;
    }

    Texture::SharedPtr Texture::create2DFromView(uint32_t apiHandle, uint32_t width, uint32_t height, ResourceFormat format)
    {
        return nullptr;
    }

    uint32_t Texture::getMipLevelDataSize(uint32_t mipLevel) const
    {
        UNSUPPORTED_IN_D3D12("Texture::getMipLevelDataSize");
        return 0;
    }

    void Texture::readSubresourceData(void* pData, uint32_t dataSize, uint32_t mipLevel, uint32_t arraySlice) const
    {
        UNSUPPORTED_IN_D3D12("Texture::readSubresourceData");
    }

    void Texture::uploadSubresourceData(const void* pData, uint32_t dataSize, uint32_t mipLevel, uint32_t arraySlice)
    {
        UNSUPPORTED_IN_D3D12("Texture::uploadSubresourceData()");
    }

    void Texture::compress2DTexture()
    {
        UNSUPPORTED_IN_D3D12("Texture::compress2DTexture");
    }

	void Texture::generateMips() const
	{
		UNSUPPORTED_IN_D3D12("Texture::GenerateMips");
	}

    Texture::SharedPtr Texture::createView(uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount) const
    {
        UNSUPPORTED_IN_D3D12("createView");
        return nullptr;
    }
}
#endif //#ifdef FALCOR_D3D11
