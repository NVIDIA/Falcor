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
//#pragma once
#include "Data/HostDeviceData.h"

#define CSM_MAX_CASCADES 8

#define CsmFilterPoint 0
#define CsmFilterHwPcf 1
#define CsmFilterFixedPcf 2
#define CsmFilterVsm   3
#define CsmFilterEvsm2 4
#define CsmFilterEvsm4 5
#define CsmFilterStochasticPcf 6

struct CsmData
{
    mat4 globalMat;
    vec4 cascadeScale[CSM_MAX_CASCADES];
    vec4 cascadeOffset[CSM_MAX_CASCADES];

    float cascadeStartDepth[CSM_MAX_CASCADES];  // In camera clip-space
    float cascadeRange[CSM_MAX_CASCADES];  // In camera clip-space

    float depthBias DEFAULTS(0.0011f);
    int cascadeCount DEFAULTS(4);
    uint32_t filterMode DEFAULTS(CsmFilterHwPcf);

    int32_t sampleKernelSize DEFAULTS(5);
    vec3 lightDir;

    float lightBleedingReduction DEFAULTS(0);
    float cascadeBlendThreshold DEFAULTS(0.2f);

    vec2 evsmExponents DEFAULTS(v2(40.0f, 5.0f)); // posExp, negExp
    //TODO something better than this
#ifdef HOST_CODE
    uint64_t padding;
    uint64_t padding2;
#endif
};

#ifdef HOST_CODE
static_assert(sizeof(CsmData) % sizeof(vec4) == 0, "CsmData size should be aligned on vec4 size");
#else
Texture2DArray shadowMap;
Texture2DArray momentsMap;

SamplerState exampleSampler : register(s0);
SamplerComparisonState compareSampler : register(s1);

int getCascadeCount(const CsmData csmData)
{
#ifdef _CSM_CASCADE_COUNT
    return _CSM_CASCADE_COUNT;
#else
    return csmData.cascadeCount;
#endif
}

uint32_t getFilterMode(const CsmData csmData)
{
#ifdef _CSM_FILTER_MODE
    return _CSM_FILTER_MODE;
#else
    return csmData.filterMode;
#endif
}

int getCascadeIndex(const CsmData csmData, float depthCamClipSpace)
{
    for(int i = 1; i < getCascadeCount(csmData); i++)
    {
        if(depthCamClipSpace < csmData.cascadeStartDepth[i])
        {
            return i - 1;
        }
    }
    return getCascadeCount(csmData) - 1;
}

vec3 getCascadeColor(uint32_t cascade)
{
    float3 result = float3(0, 0, 0);
    //TODO, would rather return from switch, but that gives. 
    //use of potentially uninitialized variable (getCascadeColor)
    switch(cascade)
    {
    case 0:
        result = float3(1, 0, 0); break;
    case 1:
        result = vec3(0, 1, 0); break;
    case 2:
        result = float3(0, 0, 1); break;
    case 3:
        result = float3(1, 0, 1); break;
    case 4:
        result = float3(1, 1, 0); break;
    case 5:
        result = float3(0, 1, 1); break;
    case 6:
        result = float3(0.7, 0.5, 1); break;
    case 7:
        result = float3(1, 0.5, 0.7); break;
    }
    return result;
}
#if 0
float calcReceiverPlaneDepthBias(const CsmData csmData, const vec3 shadowPos)
{
    vec3 posDX = dFdxFine(shadowPos);
    vec3 posDY = dFdyFine(shadowPos);

    // Determinant of inverse-transpose of Jacobian
    float invDet = 1 / (posDX.x * posDY.y - posDX.y * posDY.x);

    // Calculate the depth-derivatives in texture space
    vec2 depthDrv;
    depthDrv.x = posDY.y * posDX.z - posDX.y * posDY.z;
    depthDrv.y = posDX.x * posDY.z - posDY.x * posDX.z;

    depthDrv *= invDet;

    // Calculate the error based on the texture size
    vec2 texelSize = 1 / vec2(textureSize(shadowMap, 0).xy);
    float bias = dot(texelSize, abs(depthDrv));
    return min(bias, csmData.maxReceiverPlaneDepthBias);
}

vec3 calcNormalOffset(const CsmData csmData, vec3 normal, uint32_t cascadeIndex)
{
    // Need to move more when ndotl == 0
    vec3 N = normalize(normal);
    float nDotL = dot(N, -csmData.lightDir);

    float offset = max((1 - nDotL), 0);
    offset *= csmData.normalOffsetScale;
    return N * offset;
}
#endif

vec2 applyEvsmExponents(float depth, vec2 exponents)
{
    depth = 2.0f * depth - 1.0f;
    vec2 expDepth;
    expDepth.x = exp(exponents.x * depth);
    expDepth.y = -exp(-exponents.y * depth);
    return expDepth;
}

float csmFilterUsingHW(const CsmData csmData, const vec2 texC, float depthRef, uint32_t cascadeIndex)
{
    float res = shadowMap.SampleCmpLevelZero(compareSampler, float3(texC, cascadeIndex), depthRef).r;    
    return saturate(res);
}

float csmFixedSizePcf(const CsmData csmData, const vec2 texC, float depthRef, uint32_t cascadeIndex)
{
    float width, height, elements, levels;
    //TODO cleaner version of this fx?
    shadowMap.GetDimensions(cascadeIndex, width, height, elements, levels);
    vec2 pixelSize = 1.0 / float2(width, height);
    float res = 0;

    int halfKernelSize = csmData.sampleKernelSize / 2u;

    //TODO THIS NEEDS TO GO IN LOOP BUT ITS COMPLAINING THAT IT CANT UNROLL THE FOREACH LOOP
    //IT DOESNT LIKE DOING TEXTURE SAMPLING IN A LOOP OF NONCONST SIZE
    //for(int i = -halfKernelSize; i <= halfKernelSize; i++)
    //{
        //for(int j = -halfKernelSize; j <= halfKernelSize; j++)
        //{
            vec2 sampleCrd = texC + vec2(-halfKernelSize, -halfKernelSize) * pixelSize;
            res += shadowMap.Sample(exampleSampler, float3(texC, cascadeIndex)).r;
            //TODO why is this vec4?
            //res += csmData.shadowMap.Sample(exampleSampler, vec4(sampleCrd, float(cascadeIndex), depthRef)).r;
        //}
   //}

    return res / (csmData.sampleKernelSize * csmData.sampleKernelSize);
}

float csmStochasticFilter(const CsmData csmData, const vec2 texC, float depthRef, uint32_t cascadeIndex)
{
    float width, height, elements, levels;
    //TODO cleaner version of this fx?
    shadowMap.GetDimensions(cascadeIndex, width, height, elements, levels);
    vec2 pixelSize = 1.0 / float2(width, height);

    const vec2 poissonDisk[16] = {
        vec2(-0.94201624, -0.39906216),
        vec2(0.94558609, -0.76890725),
        vec2(-0.094184101, -0.92938870),
        vec2(0.34495938, 0.29387760),
        vec2(-0.91588581, 0.45771432),
        vec2(-0.81544232, -0.87912464),
        vec2(-0.38277543, 0.27676845),
        vec2(0.97484398, 0.75648379),
        vec2(0.44323325, -0.97511554),
        vec2(0.53742981, -0.47373420),
        vec2(-0.26496911, -0.41893023),
        vec2(0.79197514, 0.19090188),
        vec2(-0.24188840, 0.99706507),
        vec2(-0.81409955, 0.91437590),
        vec2(0.19984126, 0.78641367),
        vec2(0.14383161, -0.14100790)
    };

    vec2 halfKernelSize = pixelSize * float(csmData.sampleKernelSize) / 4;

    float res = 0;
    //TODO THIS NEEDS TO GO IN LOOP BUT ITS COMPLAINING THAT IT CANT UNROLL THE FOREACH LOOP
    //IT DOESNT LIKE DOING TEXTURE SAMPLING IN A LOOP OF NONCONST SIZE
    //Todo, why cant hlsl unroll this loop? it's using const int
    //const int numStochasticSamples = 4;
    //for(int i = 0; i < numStochasticSamples; i++)
    //{
        //TODO svPosition != gl_FragCoord. well it should but apparently can't use it like this
        //int idx = int(i + 71 * SV_Position.x + 37 * SV_Position.y) & 3;// Tempooral version: (csmData.temporalSampleCount*numStochasticSamples + i)&15;
        //TODO, index should be idx, not 0
        vec2 texelOffset = poissonDisk[0] * halfKernelSize;
        vec2 sampleCrd = texelOffset + texC;
        //TODO why is this vec4?
        //res += texture(csmData.shadowMap, vec4(sampleCrd, float(cascadeIndex), depthRef)).r;
        //DOING THIS 4 times to make up for not looping, look in to that later
        res += shadowMap.Sample(exampleSampler, float3(sampleCrd, cascadeIndex)).r;

    //}

    //TODO this should also be 
    //return res / float(numStochasticSamples);
        return res / 4.f;
}

float linstep(float a, float b, float v)
{
    return clamp((v - a) / (b - a), 0, 1);
}

float calcChebyshevUpperBound(vec2 moments, float depth, float lightBleedingReduction)
{
    float variance = moments.y - moments.x * moments.x;
    float d = depth - moments.x;
    float pMax = variance / (variance + d*d);

    // Reduce light bleeding
    pMax = linstep(lightBleedingReduction, 1.0f, pMax);
    float res = depth <= moments.x ? 1.0f : pMax;
    return res;
}

float csmVsmFilter(const CsmData csmData, const vec2 texC, float sampleDepth, uint32_t cascadeIndex, bool calcDrv, inout vec2 drvX, inout vec2 drvY)
{
    if(calcDrv)
    {
        drvY = ddy_fine(texC);
        drvX = ddx_fine(texC);
    }
    vec2 moments = momentsMap.SampleGrad(exampleSampler, float3(texC, float(cascadeIndex)), drvX, drvY).rg;
    float pMax = calcChebyshevUpperBound(moments, sampleDepth, csmData.lightBleedingReduction);

    return pMax;
}

float csmEvsmFilter(const CsmData csmData, const vec2 texC, float sampleDepth, uint32_t cascadeIndex, bool calcDrv, inout vec2 drvX, inout vec2 drvY)
{
    if(calcDrv)
    {
        drvY = ddy_fine(texC);
        drvX = ddx_fine(texC);
    }
    vec2 expDepth = applyEvsmExponents(sampleDepth, csmData.evsmExponents);
    vec4 moments = momentsMap.SampleGrad(exampleSampler, vec3(texC, float(cascadeIndex)), drvX, drvY);
    // Positive contribution
    float res = calcChebyshevUpperBound(moments.xy, expDepth.x, csmData.lightBleedingReduction);
    if(getFilterMode(csmData) == CsmFilterEvsm4)
    {
        // Negative contribution
        float neg = calcChebyshevUpperBound(moments.zw, expDepth.y, csmData.lightBleedingReduction);
        res = min(neg, res);
    }
    return res;
}

float calcShadowFactorWithCascadeIdx(const CsmData csmData, const uint32_t cascadeIndex, vec3 posW, bool calcDrv, inout vec2 drvX, inout vec2 drvY)
{
    // Apply normal offset
    //posW.xyz += calcNormalOffset(csmData, normal, cascadeIndex);

    // Get the global shadow space position
    vec4 shadowPos = mul(csmData.globalMat, vec4(posW, 1));
    shadowPos /= shadowPos.w;

    // Calculate the texC
    shadowPos *= csmData.cascadeScale[cascadeIndex];
    shadowPos += csmData.cascadeOffset[cascadeIndex];

    // Apply TexC transformation
    shadowPos.xy += 1;
    shadowPos.xy *= 0.5;
    shadowPos.y = 1 - shadowPos.y;

    // Calculate the texC
    shadowPos.z -= csmData.depthBias;

    //TODO, see getCascadecolor, would rather return from switch but cant
    float result = 0;
    switch(getFilterMode(csmData))
    {
    case CsmFilterPoint:
    case CsmFilterHwPcf:
        result = csmFilterUsingHW(csmData, shadowPos.xy, shadowPos.z, cascadeIndex); break;
    case CsmFilterFixedPcf:
        result = csmFixedSizePcf(csmData, shadowPos.xy, shadowPos.z, cascadeIndex); break;
    case CsmFilterVsm:
        result = csmVsmFilter(csmData, shadowPos.xy, shadowPos.z, cascadeIndex, calcDrv, drvX, drvY); break;
    case CsmFilterEvsm2:
    case CsmFilterEvsm4:
        result = csmEvsmFilter(csmData, shadowPos.xy, shadowPos.z, cascadeIndex, calcDrv, drvX, drvY); break;
    case CsmFilterStochasticPcf:
        result = csmStochasticFilter(csmData, shadowPos.xy, shadowPos.z, cascadeIndex); break;
    }
    return result;
}

float calcShadowFactor(const CsmData csmData, const float cameraDepth, vec3 posW)
{
    float weight = 0;
    float s;
    int cascadeIndex = 0;
    bool blend = false;
#if !defined(_CSM_CASCADE_COUNT) || (_CSM_CASCADE_COUNT != 1)
    cascadeIndex = getCascadeIndex(csmData, cameraDepth);
    // Get the prev cascade factor
    weight = (cameraDepth - csmData.cascadeStartDepth[cascadeIndex]) / csmData.cascadeRange[cascadeIndex];
    blend = weight < csmData.cascadeBlendThreshold;
#endif

    //TODO it was blending with 1 cascade. why? dunno. but potential cause of error
    //if(blend)
    //{       
        // Getting the previous mip-level to calculate the derivative, otherwise we get cracks between cascade with *VSM.
        // On the edge of the cascade we only use the previous cascade factor (weight == 0), but we get the derivatives from the current cascade, which probably chooses a different mip-level.
        // This should work though, since mip-levels should appear continuous
        //weight = smoothstep(0.0, csmData.cascadeBlendThreshold, weight);
        //int prevCascade = max(0, cascadeIndex - 1);
        //float2 drvX = float2(0, 0);
        //float2 drvY = float2(0, 0);
        //float s1 = calcShadowFactorWithCascadeIdx(csmData, prevCascade, posW, true, drvX, drvY);
        //s = calcShadowFactorWithCascadeIdx(csmData, cascadeIndex, posW, false, drvX, drvY);
        //s = lerp(s1, s, weight);
    //}
    //else
    //{
        vec2 drvX, drvY;
        //todo 0 should be get cascade index
        s = calcShadowFactorWithCascadeIdx(csmData, 0, posW, true, drvX, drvY);
    //}

    return s;
}
#endif