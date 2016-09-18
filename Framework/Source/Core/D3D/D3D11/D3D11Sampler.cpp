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
#ifdef FALCOR_D3D11
#include "Framework.h"
#include "Core/Sampler.h"
#include "glm/gtc/type_ptr.hpp"

namespace Falcor
{
    Sampler::~Sampler() = default;

    D3D11_FILTER_TYPE getFilterType(Sampler::Filter filter)
    {
        switch(filter)
        {
        case Sampler::Filter::Point:
            return D3D11_FILTER_TYPE_POINT;
        case Sampler::Filter::Linear:
            return D3D11_FILTER_TYPE_LINEAR;
        default:
            should_not_get_here();
            return (D3D11_FILTER_TYPE)0;
        }
    }

    D3D11_FILTER getD3DFilter(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipFilter, bool isComparison, bool isAnisotropic)
    {
        D3D11_FILTER filter;
        D3D11_FILTER_REDUCTION_TYPE reduction = isComparison ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;

        if(isAnisotropic)
        {
            filter = D3D11_ENCODE_ANISOTROPIC_FILTER(reduction);
        }
        else
        {
            D3D11_FILTER_TYPE dxMin = getFilterType(minFilter);
            D3D11_FILTER_TYPE dxMag = getFilterType(magFilter);
            D3D11_FILTER_TYPE dxMip = getFilterType(mipFilter);
            filter = D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, reduction);
        }

        return filter;
    };

    D3D11_TEXTURE_ADDRESS_MODE getD3DAddressMode(Sampler::AddressMode mode)
    {
        switch(mode)
        {
        case Sampler::AddressMode::Wrap:
            return D3D11_TEXTURE_ADDRESS_WRAP;
        case Sampler::AddressMode::Mirror:
            return D3D11_TEXTURE_ADDRESS_MIRROR;
        case Sampler::AddressMode::Clamp:
            return D3D11_TEXTURE_ADDRESS_CLAMP;
        case Sampler::AddressMode::Border:
            return D3D11_TEXTURE_ADDRESS_BORDER;
        case Sampler::AddressMode::MirrorOnce:
            return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
        default:
            should_not_get_here();
            return (D3D11_TEXTURE_ADDRESS_MODE)0;
        }
    }

    D3D11_COMPARISON_FUNC getD3DComparisonFunc(Sampler::ComparisonMode mode)
    {
        switch(mode)
        {
        case Falcor::Sampler::ComparisonMode::NoComparison:
        case Falcor::Sampler::ComparisonMode::Always:
            return D3D11_COMPARISON_ALWAYS;
        case Falcor::Sampler::ComparisonMode::Never:
            return D3D11_COMPARISON_NEVER;
        case Falcor::Sampler::ComparisonMode::LessEqual:
            return D3D11_COMPARISON_LESS_EQUAL;
        case Falcor::Sampler::ComparisonMode::GreaterEqual:
            return D3D11_COMPARISON_GREATER_EQUAL;
        case Falcor::Sampler::ComparisonMode::Less:
            return D3D11_COMPARISON_LESS;
        case Falcor::Sampler::ComparisonMode::Greater:
            return D3D11_COMPARISON_GREATER;
        case Falcor::Sampler::ComparisonMode::Equal:
            return D3D11_COMPARISON_EQUAL;
        case Falcor::Sampler::ComparisonMode::NotEqual:
            return D3D11_COMPARISON_NOT_EQUAL;
        default:
            should_not_get_here();
            return (D3D11_COMPARISON_FUNC)0;
        }
    }

    uint32_t Sampler::getApiMaxAnisotropy()
    {
        return D3D11_MAX_MAXANISOTROPY;
    }

    Sampler::SharedPtr Sampler::create(const Desc& desc)
    {
        SharedPtr pSampler = SharedPtr(new Sampler(desc));

        // Set max anisotropy
        if(desc.mMaxAnisotropy < 1 || getApiMaxAnisotropy() < desc.mMaxAnisotropy)
        {
            std::string err = "Error in Sampler::Sampler() - MaxAnisotropy should be in range [1, " + std::to_string(getApiMaxAnisotropy()) + "]. " + std::to_string(desc.mMaxAnisotropy) + " provided)\n";
            Logger::log(Logger::Level::Error, err);
            return nullptr;
        }

        D3D11_SAMPLER_DESC dxDesc;
        dxDesc.Filter = getD3DFilter(desc.mMinFilter, desc.mMagFilter, desc.mMipFilter, (desc.mComparisonMode != ComparisonMode::NoComparison), (desc.mMaxAnisotropy > 1));
        dxDesc.AddressU = getD3DAddressMode(desc.mModeU);
        dxDesc.AddressV = getD3DAddressMode(desc.mModeV);
        dxDesc.AddressW = getD3DAddressMode(desc.mModeW);
        dxDesc.MipLODBias = desc.mLodBias;
        dxDesc.MaxAnisotropy = desc.mMaxAnisotropy;
        dxDesc.ComparisonFunc = getD3DComparisonFunc(desc.mComparisonMode);
        memcpy(dxDesc.BorderColor, glm::value_ptr(desc.mBorderColor), sizeof(dxDesc.BorderColor));
        dxDesc.MinLOD = desc.mMinLod;
        dxDesc.MaxLOD = desc.mMaxLod;

        d3d_call(getD3D11Device()->CreateSamplerState(&dxDesc, &pSampler->mApiHandle));
        return pSampler;
    }
}
#endif //#ifdef FALCOR_D3D11
