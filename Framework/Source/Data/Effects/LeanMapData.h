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
#pragma once
#include "Data/HostDeviceData.h"
#ifndef HOST_CODE
#include "HlslGlslCommon.h"
#include "Helpers.h"
#else
namespace Falcor
{
#endif

#ifndef HOST_CODE
    void applyLeanMap(in sampler2D leanMap, inout ShadingAttribs shAttr)
    {
        vec4 t = sampleTexture(leanMap, shAttr);
        // Reconstruct B 
        vec2 B = 2 * t.xy - 1;
        vec2 M = t.zw;

        // Apply normal
        vec3 unnormalizedNormal = vec3(B, 1);
        applyNormalMap(unnormalizedNormal, shAttr.N, shAttr.T, shAttr.B);
        // Reconstruct the diagonal covariance matrix
        vec2 maxCov = max(vec2(0), M - B*B);     // Use only diagonal of covariance due to vec2 aniso roughness

        FOR_MAT_LAYERS(iLayer, shAttr.preparedMat)
        {
            if(shAttr.preparedMat.desc.layers[iLayer].type != MatLambert)
            {
                vec2 roughness = vec2(shAttr.preparedMat.values.layers[iLayer].roughness.constantColor);
                roughness = sqrt(roughness*roughness + maxCov); // Approximate convolution that works for all
                shAttr.preparedMat.values.layers[iLayer].roughness.constantColor.rg = roughness;
            }
        }
    }
#else
}
#endif
