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
#ifndef LEAN_MAP_DATA_H
#define LEAN_MAP_DATA_H

#include "Data/HostDeviceData.h"

__import Helpers;

void applyLeanMap(in Texture2D leanMap, in SamplerState samplerState, inout ShadingAttribs shAttr)
{
    float4 t = sampleTexture(leanMap, samplerState, shAttr);
    // Reconstruct B 
    float2 B = 2 * t.xy - 1;
    float2 M = t.zw;

    // Apply normal
    float3 unnormalizedNormal = float3(B, 1);
    applyNormalMap(unnormalizedNormal, shAttr.N, shAttr.T, shAttr.B);
    // Reconstruct the diagonal covariance matrix
    float2 maxCov = max((0), M - B*B);     // Use only diagonal of covariance due to float2 aniso roughness

    [unroll]
    for (uint iLayer = 0; iLayer < MatMaxLayers; iLayer++)
    {
        if (shAttr.preparedMat.desc.layers[iLayer].type == MatNone) break;

        if (shAttr.preparedMat.desc.layers[iLayer].type != MatLambert)
        {
            float2 roughness = shAttr.preparedMat.values.layers[iLayer].roughness.xy;
            roughness = sqrt(roughness*roughness + maxCov); // Approximate convolution that works for all
            shAttr.preparedMat.values.layers[iLayer].roughness.rg = roughness;
        }
    }
}

#endif