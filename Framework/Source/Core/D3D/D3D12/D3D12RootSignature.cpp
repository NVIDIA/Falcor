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
#include "Core/RootSignature.h"
#include "Core/D3D/D3DState.h"

namespace Falcor
{
    using StaticSamplerVec = std::vector<D3D12_STATIC_SAMPLER_DESC>;
    using RootParameterVec = std::vector<D3D12_ROOT_PARAMETER>;

    D3D12_SHADER_VISIBILITY getShaderVisibility(ShaderVisibility visibility)
    {
        // D3D12 doesn't support a combination of flags, it's either ALL or a single stage
        if (isPowerOf2(visibility))
        {
            return D3D12_SHADER_VISIBILITY_ALL;
        }
        else if ((visibility & ShaderVisibility::Vertex) != ShaderVisibility::None)
        {
            return D3D12_SHADER_VISIBILITY_VERTEX;
        }
        else if ((visibility & ShaderVisibility::Pixel) != ShaderVisibility::None)
        {
            return D3D12_SHADER_VISIBILITY_PIXEL;
        }
        else if ((visibility & ShaderVisibility::Geometry) != ShaderVisibility::None)
        {
            return D3D12_SHADER_VISIBILITY_GEOMETRY;
        }
        else if ((visibility & ShaderVisibility::Domain) != ShaderVisibility::None)
        {
            return D3D12_SHADER_VISIBILITY_DOMAIN;
        }
        else if ((visibility & ShaderVisibility::Hull) != ShaderVisibility::None)
        {
            return D3D12_SHADER_VISIBILITY_HULL;
        }
        should_not_get_here();
        return (D3D12_SHADER_VISIBILITY)-1;
    }

    void convertSamplerDesc(const RootSignature::SamplerDesc& falcorDesc, D3D12_STATIC_SAMPLER_DESC& desc)
    {
        initD3DSamplerDesc(falcorDesc.pSampler.get(), falcorDesc.borderColor, desc);
        desc.ShaderRegister = falcorDesc.regIndex;
        desc.RegisterSpace = falcorDesc.regSpace;
        desc.ShaderVisibility = getShaderVisibility(falcorDesc.visibility);
    }

    bool RootSignature::apiInit()
    {
        StaticSamplerVec samplerVec(mDesc.mSamplers.size());
        for (size_t i = 0 ; i < samplerVec.size() ; i++)
        {
            convertSamplerDesc(mDesc.mSamplers[i], samplerVec[i]);
        }
        return true;
    }
}
#endif //FALCOR_D3D12