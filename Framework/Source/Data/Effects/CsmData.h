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
#ifndef CSMDATA_H
#define CSMDATA_H

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

    //Only uses xy
    vec4 cascadeRange[CSM_MAX_CASCADES];  // In camera clip-space

    float depthBias DEFAULTS(0.005f);
    int cascadeCount DEFAULTS(4);
    uint32_t filterMode DEFAULTS(CsmFilterHwPcf);

    int32_t pcfKernelWidth DEFAULTS(5);
    vec3 lightDir;

    float lightBleedingReduction DEFAULTS(0);
    float cascadeBlendThreshold DEFAULTS(0.2f);

    vec2 evsmExponents DEFAULTS(v2(40.0f, 5.0f)); // posExp, negExp
    //If this is only in host code, the final element in the array is not padded
    uint32_t padding;
#ifndef HOST_CODE
    Texture2DArray shadowMap;
    SamplerState csmSampler;
    SamplerComparisonState csmCompareSampler;
#endif
};

#ifdef HOST_CODE
static_assert(sizeof(CsmData) % sizeof(vec4) == 0, "CsmData size should be aligned on vec4 size");
#else

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
        if(depthCamClipSpace < csmData.cascadeRange[i].x)
        {
            return i - 1;
        }
    }
    return getCascadeCount(csmData) - 1;
}

vec3 getCascadeColor(uint32_t cascade)
{
    switch(cascade)
    {
    case 0:
        return float3(1, 0, 0);
    case 1:
        return vec3(0, 1, 0); 
    case 2:
        return float3(0, 0, 1); 
    case 3:
        return float3(1, 0, 1); 
    case 4:
        return float3(1, 1, 0); 
    case 5:
        return float3(0, 1, 1); 
    case 6:
        return float3(0.7, 0.5, 1); 
    case 7:
        return float3(1, 0.5, 0.7); 
    default:
        return float3(0, 0, 0);
    }
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
    vec2 texelSize = 1 / vec2(textureSize(csmData.shadowMap, 0).xy);
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
    float res = csmData.shadowMap.SampleCmpLevelZero(csmData.csmCompareSampler, float3(texC, cascadeIndex), depthRef).r;    
    return saturate(res);
}

float csmFixedSizePcf(const CsmData csmData, const vec2 texC, float depthRef, uint32_t cascadeIndex)
{
    float3 dim;
    csmData.shadowMap.GetDimensions(dim.x, dim.y, dim.z);
    vec2 pixelSize = 1.0 / dim.xy;
    float res = 0;

    int halfKernelSize = csmData.pcfKernelWidth / 2u;
    for(int i = -halfKernelSize; i <= halfKernelSize; i++)
    {
        for(int j = -halfKernelSize; j <= halfKernelSize; j++)
        {
            vec2 sampleCrd = texC + vec2(i, j) * pixelSize;
            res += csmData.shadowMap.SampleCmpLevelZero(csmData.csmCompareSampler, float3(sampleCrd, cascadeIndex), depthRef).r;
        }
    }

    return res / (csmData.pcfKernelWidth * csmData.pcfKernelWidth);
}

float csmStochasticFilter(const CsmData csmData, const vec2 texC, float depthRef, uint32_t cascadeIndex, vec2 posSxy)
{
    float3 dim;
    csmData.shadowMap.GetDimensions(dim.x, dim.y, dim.z);
    vec2 pixelSize = 1.0 / dim.xy;

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

    vec2 halfKernelSize = pixelSize * float(csmData.pcfKernelWidth) / 4;

    float res = 0;
    float2 pos = posSxy / dim.xy;
    const int numStochasticSamples = 4;
    for(int i = 0; i < numStochasticSamples; i++)
    {
        int idx = int(i + 71 * pos.x + 37 * pos.y) & 3;// Temporal version: (csmData.temporalSampleCount*numStochasticSamples + i)&15;
        vec2 texelOffset = poissonDisk[idx] * halfKernelSize;
        vec2 sampleCrd = texelOffset + texC;
        res += csmData.shadowMap.SampleCmpLevelZero(csmData.csmCompareSampler, float3(sampleCrd, cascadeIndex), depthRef).r;
    }

    return res / float(numStochasticSamples);
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
    vec2 moments = csmData.shadowMap.SampleGrad(csmData.csmSampler, float3(texC, float(cascadeIndex)), drvX, drvY).rg;
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
    vec4 moments = csmData.shadowMap.SampleGrad(csmData.csmSampler, vec3(texC, float(cascadeIndex)), drvX, drvY);
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

float calcShadowFactorWithCascadeIdx(const CsmData csmData, const uint32_t cascadeIndex, vec3 posW, vec2 posSxy, bool calcDrv, inout vec2 drvX, inout vec2 drvY)
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

    switch(getFilterMode(csmData))
    {
    case CsmFilterPoint:
    case CsmFilterHwPcf:
        return csmFilterUsingHW(csmData, shadowPos.xy, shadowPos.z, cascadeIndex);
    case CsmFilterFixedPcf:
        return csmFixedSizePcf(csmData, shadowPos.xy, shadowPos.z, cascadeIndex); 
    case CsmFilterVsm:
        return csmVsmFilter(csmData, shadowPos.xy, shadowPos.z, cascadeIndex, calcDrv, drvX, drvY); 
    case CsmFilterEvsm2:
    case CsmFilterEvsm4:
        return csmEvsmFilter(csmData, shadowPos.xy, shadowPos.z, cascadeIndex, calcDrv, drvX, drvY); 
    case CsmFilterStochasticPcf:
        return csmStochasticFilter(csmData, shadowPos.xy, shadowPos.z, cascadeIndex, posSxy); 
    default:
        return 0;
    }
}

float calcShadowFactor(const CsmData csmData, const float cameraDepth, vec3 posW, vec2 posSxy)
{
    float weight = 0;
    float s;
    int cascadeIndex = 0;
    bool blend = false;
#if !defined(_CSM_CASCADE_COUNT) || (_CSM_CASCADE_COUNT != 1)
    cascadeIndex = getCascadeIndex(csmData, cameraDepth);
    // Get the prev cascade factor
    weight = (cameraDepth - csmData.cascadeRange[cascadeIndex].x) / csmData.cascadeRange[cascadeIndex].y;
    blend = weight < csmData.cascadeBlendThreshold;
#endif

    if(blend)
    {       
        // Getting the previous mip-level to calculate the derivative, otherwise we get cracks between cascade with *VSM.
        // On the edge of the cascade we only use the previous cascade factor (weight == 0), but we get the derivatives from the current cascade, which probably chooses a different mip-level.
        // This should work though, since mip-levels should appear continuous
        weight = smoothstep(0.0, csmData.cascadeBlendThreshold, weight);
        int prevCascade = max(0, cascadeIndex - 1);
        float2 drvX = float2(0, 0);
        float2 drvY = float2(0, 0);
        float s1 = calcShadowFactorWithCascadeIdx(csmData, prevCascade, posW, posSxy, true, drvX, drvY);
        s = calcShadowFactorWithCascadeIdx(csmData, cascadeIndex, posW, posSxy, false, drvX, drvY);
        s = lerp(s1, s, weight);
    }
    else
    {
        vec2 drvX, drvY;
        s = calcShadowFactorWithCascadeIdx(csmData, cascadeIndex, posW, posSxy, true, drvX, drvY);
    }

    return s;
}
#endif
#endif //CSMDATA_H
