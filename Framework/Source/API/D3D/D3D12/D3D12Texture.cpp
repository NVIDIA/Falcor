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
#include "API/D3D/D3DViews.h"
#include <vector>
#include "API/Device.h"
#include "API/ProgramVars.h"
#include "Graphics/FullScreenPass.h"
#include "Graphics/GraphicsState.h"
#include "D3D12Resource.h"

namespace Falcor
{
    RtvHandle Texture::spNullRTV;
    DsvHandle Texture::spNullDSV;

    struct GenMipsData
    {
        FullScreenPass::UniquePtr pFullScreenPass;
        GraphicsVars::SharedPtr pVars;
        GraphicsState::SharedPtr pState;
    };

    struct TextureApiData
    {
        TextureApiData() { sObjCount++; }
        ~TextureApiData() { sObjCount--; if (sObjCount == 0) spGenMips = nullptr; }

        static std::unique_ptr<GenMipsData> spGenMips;
        Fbo::SharedPtr pGenMipsFbo;

    private:
        static uint64_t sObjCount;
    };

    uint64_t TextureApiData::sObjCount = 0;
    std::unique_ptr<GenMipsData> TextureApiData::spGenMips;

    template<>
    D3D12_UAV_DIMENSION getViewDimension<D3D12_UAV_DIMENSION>(Texture::Type type, bool isTextureArray)
    {
        switch (type)
        {
        case Texture::Type::Texture1D:
            return (isTextureArray) ? D3D12_UAV_DIMENSION_TEXTURE1DARRAY : D3D12_UAV_DIMENSION_TEXTURE1D;
        case Texture::Type::Texture2D:
            return (isTextureArray) ? D3D12_UAV_DIMENSION_TEXTURE2DARRAY : D3D12_UAV_DIMENSION_TEXTURE2D;
        case Texture::Type::TextureCube:
            return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        default:
            should_not_get_here();
            return D3D12_UAV_DIMENSION_UNKNOWN;
        }
    }

    template<>
    D3D12_SRV_DIMENSION getViewDimension<D3D12_SRV_DIMENSION>(Texture::Type type, bool isTextureArray)
    {
        switch(type)
        {
        case Texture::Type::Texture1D:
            return (isTextureArray) ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY : D3D12_SRV_DIMENSION_TEXTURE1D;
        case Texture::Type::Texture2D:
            return (isTextureArray) ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;
        case Texture::Type::Texture3D:
            assert(isTextureArray == false);
            return D3D12_SRV_DIMENSION_TEXTURE3D;
        case Texture::Type::Texture2DMultisample:
            return (isTextureArray) ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D12_SRV_DIMENSION_TEXTURE2DMS;
        case Texture::Type::TextureCube:
            return (isTextureArray) ? D3D12_SRV_DIMENSION_TEXTURECUBEARRAY : D3D12_SRV_DIMENSION_TEXTURECUBE;
        default:
            should_not_get_here();
            return D3D12_SRV_DIMENSION_UNKNOWN;
        }
    }

    void Texture::apiInit()
    {
        mpApiData = new TextureApiData();
    }

    Texture::~Texture()
    {
        safe_delete(mpApiData);
        gpDevice->releaseResource(mApiHandle);
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

    void createTextureCommon(const Texture* pTexture, Texture::ApiHandle& apiHandle, const void* pData, D3D12_RESOURCE_DIMENSION dim, bool autoGenMips, Texture::BindFlags bindFlags)
    {
        ResourceFormat texFormat = pTexture->getFormat();

        D3D12_RESOURCE_DESC desc = {};

        desc.MipLevels = pTexture->getMipCount();
        desc.Format = getDxgiFormat(texFormat);
        desc.Width = align_to(getFormatWidthCompressionRatio(texFormat), pTexture->getWidth());
        desc.Height = align_to(getFormatHeightCompressionRatio(texFormat), pTexture->getHeight());
        desc.Flags = getD3D12ResourceFlags(bindFlags);
        desc.DepthOrArraySize = (pTexture->getType() == Texture::Type::TextureCube) ? pTexture->getArraySize() * 6 : pTexture->getArraySize();
        desc.SampleDesc.Count = pTexture->getSampleCount();
        desc.SampleDesc.Quality = 0;
        desc.Dimension = dim;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Alignment = 0;

        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_CLEAR_VALUE* pClearVal = nullptr;
        if ((bindFlags & (Texture::BindFlags::RenderTarget | Texture::BindFlags::DepthStencil)) != Texture::BindFlags::None)
        {
            clearValue.Format = desc.Format;
            if ((bindFlags & Texture::BindFlags::DepthStencil) != Texture::BindFlags::None)
            {
                clearValue.DepthStencil.Depth = 1.0f;
            }
            pClearVal = &clearValue;
        }

        //If depth and either ua or sr, set to typeless
        if (isDepthFormat(texFormat) && is_set(bindFlags, Texture::BindFlags::ShaderResource | Texture::BindFlags::UnorderedAccess))
        {
            desc.Format = getTypelessFormatFromDepthFormat(texFormat);
            pClearVal = nullptr;
        }

        d3d_call(gpDevice->getApiHandle()->CreateCommittedResource(&kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, pClearVal, IID_PPV_ARGS(&apiHandle)));

        if (pData)
        {
            auto& pRenderContext = gpDevice->getRenderContext();
            if (autoGenMips)
            {
                // Upload just the first mip-level
                size_t arraySliceSize = pTexture->getWidth() * pTexture->getHeight() * getFormatBytesPerBlock(pTexture->getFormat());
                const uint8_t* pSrc = (uint8_t*)pData;
                uint32_t numFaces = (pTexture->getType() == Texture::Type::TextureCube) ? 6 : 1;
                for (uint32_t i = 0; i < pTexture->getArraySize() * numFaces; i++)
                {
                    uint32_t subresource = pTexture->getSubresourceIndex(i, 0);
                    pRenderContext->updateTextureSubresource(pTexture, subresource, pSrc);
                    pSrc += arraySliceSize;
                }
            }
            else
            {
                pRenderContext->updateTexture(pTexture, pData);
            }

            if (autoGenMips)
            {
                pTexture->generateMips();
                pTexture->invalidateViews();
            }
        }
    }

    Texture::BindFlags updateBindFlags(Texture::BindFlags flags, bool hasInitData, uint32_t mipLevels)
    {
        if ((mipLevels != Texture::kMaxPossible) || (hasInitData == false))
        {
            return flags;
        }

        flags |= Texture::BindFlags::RenderTarget;
        return flags;
    }

    Texture::SharedPtr Texture::create1D(uint32_t width, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData, BindFlags bindFlags)
    {
        bindFlags = updateBindFlags(bindFlags, pData != nullptr, mipLevels);
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, 1, 1, arraySize, mipLevels, 1, format, Type::Texture1D, bindFlags));
        createTextureCommon(pTexture.get(), pTexture->mApiHandle, pData, D3D12_RESOURCE_DIMENSION_TEXTURE1D, (mipLevels == kMaxPossible), bindFlags);
        return pTexture->mApiHandle ? pTexture : nullptr;
    }
    
    Texture::SharedPtr Texture::create2D(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData, BindFlags bindFlags)
    {
        bindFlags = updateBindFlags(bindFlags, pData != nullptr, mipLevels);
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, height, 1, arraySize, mipLevels, 1, format, Type::Texture2D, bindFlags));
        createTextureCommon(pTexture.get(), pTexture->mApiHandle, pData, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (mipLevels == kMaxPossible), bindFlags);
        return pTexture->mApiHandle ? pTexture : nullptr;
    }

    Texture::SharedPtr Texture::create3D(uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels, const void* pData, BindFlags bindFlags, bool isSparse)
    {
        bindFlags = updateBindFlags(bindFlags, pData != nullptr, mipLevels);
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, height, depth, 1, mipLevels, 1, format, Type::Texture3D, bindFlags));
        createTextureCommon(pTexture.get(), pTexture->mApiHandle, pData, D3D12_RESOURCE_DIMENSION_TEXTURE3D, (mipLevels == kMaxPossible), bindFlags);
        return pTexture->mApiHandle ? pTexture : nullptr;
        return nullptr;
    }

    // Texture Cube
    Texture::SharedPtr Texture::createCube(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData, BindFlags bindFlags)
    {
        bindFlags = updateBindFlags(bindFlags, pData != nullptr, mipLevels);
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, height, 1, arraySize, mipLevels, 1, format, Type::TextureCube, bindFlags));
        createTextureCommon(pTexture.get(), pTexture->mApiHandle, pData, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (mipLevels == kMaxPossible), bindFlags);
        return pTexture->mApiHandle ? pTexture : nullptr;
    }

    Texture::SharedPtr Texture::create2DMS(uint32_t width, uint32_t height, ResourceFormat format, uint32_t sampleCount, uint32_t arraySize, BindFlags bindFlags)
    {
        Texture::SharedPtr pTexture = SharedPtr(new Texture(width, height, 1, arraySize, 1, sampleCount, format, Type::Texture2DMultisample, bindFlags));
        createTextureCommon(pTexture.get(), pTexture->mApiHandle, nullptr, D3D12_RESOURCE_DIMENSION_TEXTURE2D, false, bindFlags);
        return pTexture->mApiHandle ? pTexture : nullptr;
    }

    uint32_t Texture::getMipLevelDataSize(uint32_t mipLevel) const
    {
        UNSUPPORTED_IN_D3D12("Texture::getMipLevelDataSize");
        return 0;
    }

    void Texture::compress2DTexture()
    {
        UNSUPPORTED_IN_D3D12("Texture::compress2DTexture");
    }

    void Texture::generateMips() const
    {
        if (mType != Type::Texture2D)
        {
            logWarning("Texture::generateMips() only supports 2D textures");
            return;
        }

        if (mpApiData->spGenMips == nullptr)
        {
            mpApiData->spGenMips = std::make_unique<GenMipsData>();
            mpApiData->spGenMips->pFullScreenPass = FullScreenPass::create("Framework/Shaders/Blit.ps.slang");
            mpApiData->spGenMips->pVars = GraphicsVars::create(mpApiData->spGenMips->pFullScreenPass->getProgram()->getActiveVersion()->getReflector());
            mpApiData->spGenMips->pState = GraphicsState::create();
            Sampler::Desc desc;
            desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
            mpApiData->spGenMips->pVars->setSampler("gSampler", Sampler::create(desc));
        }

        RenderContext* pContext = gpDevice->getRenderContext().get();
        pContext->pushGraphicsState(mpApiData->spGenMips->pState);
        pContext->pushGraphicsVars(mpApiData->spGenMips->pVars);

        if(mpApiData->pGenMipsFbo == nullptr)
        {
            mpApiData->pGenMipsFbo = Fbo::create();
            mpApiData->spGenMips->pState->setFbo(mpApiData->pGenMipsFbo);
        }
        //sometimes on reload, the fbo exists, but the actual texture in its color target is null
        else if (mpApiData->spGenMips->pState->getFbo()->getColorTexture(0) == nullptr)
        {
            mpApiData->spGenMips->pState->setFbo(mpApiData->pGenMipsFbo);
        }

        for (uint32_t i = 0; i < mMipLevels - 1; i++)
        {
            // Create an FBO for the next mip level
            SharedPtr pNonConst = const_cast<Texture*>(this)->shared_from_this();
            mpApiData->pGenMipsFbo->attachColorTarget(pNonConst, 0, i + 1, 0);

            const float width = (float)mpApiData->pGenMipsFbo->getWidth();
            const float height = (float)mpApiData->pGenMipsFbo->getHeight();
            mpApiData->spGenMips->pState->setViewport(0, GraphicsState::Viewport(0.0f, 0.0f, width, height, 0.0f, 1.0f));

            // Create the resource view
            mpApiData->spGenMips->pVars->setSrv(0, pNonConst->getSRV(i, 1, 0, mArraySize));

            // Run the program
            mpApiData->spGenMips->pFullScreenPass->execute(pContext);
        }

        pContext->popGraphicsState();
        pContext->popGraphicsVars();

        logInfo("Releasing RTVs after Texture::generateMips()");
        mRtvs.clear();

        // Detach from circular reference (this -> this->pFbo -> this -> ...)
        mpApiData->pGenMipsFbo->attachColorTarget(nullptr, 0);

        // Detach from shared static state so it doesn't keep our resource alive
        mpApiData->spGenMips->pVars->setSrv(0, nullptr);

        pContext->flush(true); // This shouldn't be here. GitLab issue #69
    }
}
