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
#include "ShaderCommon.h"
#include "Shading.h"
#define _COMPILE_DEFAULT_VS
#include "VertexAttrib.h"
#include "Effects/CsmData.h"
#include "Helpers.h"

cbuffer PerFrameCB : register(b0)
{
#foreach p in _LIGHT_SOURCES
    LightData $(p);
#endforeach

	vec3 gAmbient;
    CsmData gCsmData;
    mat4 camVpAtLastCsmUpdate;
};

Texture2D gEnvMap;
SamplerState gSampler;

struct MainVsOut
{
    VS_OUT vsData;
    float shadowsDepthC : DEPTH;
};

struct PsOut
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
};

PsOut main(MainVsOut vOut)
{
    PsOut psOut;

    ShadingAttribs shAttr;
    prepareShadingAttribs(gMaterial, vOut.vsData.posW, gCam.position, vOut.vsData.normalW, vOut.vsData.bitangentW, vOut.vsData.texC, shAttr);

    ShadingOutput result;
    result.finalValue = 0;
    float4 finalColor = 0;
    float envMapFactor = 1;

#foreach p in _LIGHT_SOURCES
    evalMaterial(shAttr, $(p), result, $(_valIndex) == 0);
#ifdef _ENABLE_SHADOWS
    if($(_valIndex) == 0)
    {
        float shadowFactor = calcShadowFactor(gCsmData, vOut.shadowsDepthC, shAttr.P, vOut.vsData.posH.xy/vOut.vsData.posH.w);
        result.finalValue *= shadowFactor;
        envMapFactor -= 1 - shadowFactor;
    }
#endif
#endforeach

    finalColor = vec4(result.finalValue, 1.f);

#ifdef _ENABLE_REFLECTIONS
    // Calculate the view vector
    float3 view = reflect(-shAttr.E, shAttr.N);
    float2 texC = dirToSphericalCrd(view);
    float rough = shAttr.preparedMat.values.layers[1].albedo.a;
    float lod = rough * 16;
    float3 envMapVal = gEnvMap.SampleLevel(gSampler, texC, lod).rgb;

    envMapFactor = saturate(envMapFactor + 0.07);
    finalColor.rgb += (result.specularAlbedo) * envMapVal * envMapFactor * evalGGXDistribution(float3(0, 0, 1), shAttr.N, rough) * 0.5;
#endif

    // add ambient
    finalColor.rgb += gAmbient * getDiffuseColor(shAttr).rgb;
    
    psOut.color = finalColor;
    psOut.normal = float4(vOut.vsData.normalW * 0.5f + 0.5f, 1.0f);

    return psOut;
}
