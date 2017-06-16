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
#version 420
#include "HlslGlslCommon.h"

SamplerState gLuminanceTexSampler : register(s0);
SamplerState gPointSampler : register(s1);

Texture2D gColorTex;
Texture2D gLuminanceTex;

cbuffer PerImageCB : register(b0)
{
    float gExposureKey;
    float gMaxWhiteLuminance;
    float gLuminanceLod;
    float gWhiteScale;
};

float calcLuminance(vec3 color)
{
    return dot(color.xyz, vec3(0.299f, 0.587f, 0.114f));
}

vec3 calcExposedColor(vec3 color, vec2 texC)
{
    float pixelLuminance = calcLuminance(color);
    float avgLuminance = gLuminanceTex.SampleLevel(gLuminanceTexSampler, texC, gLuminanceLod).r;
    avgLuminance = exp2(avgLuminance);
    float exposedLuminance = (gExposureKey / avgLuminance);
    return exposedLuminance*color;
}

vec3 linearToneMap(vec3 color)
{
    return color; // Do nothing
}

// Reinhard
vec3 reinhardToneMap(vec3 color)
{
    float luminance = calcLuminance(color);
    float reinhard = luminance / (luminance + 1);
    return color * (reinhard / luminance);
}

// Reinhard with maximum luminance
vec3 reinhardModifiedToneMap(vec3 color)
{
    float luminance = calcLuminance(color);
    float reinhard = luminance * (1 + luminance / (gMaxWhiteLuminance * gMaxWhiteLuminance)) * (1 + luminance);
    return color * (reinhard / luminance);
}

// John Hable's ALU approximation of Jim Heji's operator
// http://filmicgames.com/archives/75
vec3 hejiHableAluToneMap(vec3 color)
{
    color = max(float(0).rrr, color - 0.004);
    color = (color*(6.2 * color + 0.5f)) / (color * (6.2 * color + 1.7) + 0.06f);

    // Result includes sRGB conversion
    return pow(color, float(2.2f).rrr);
}

// John Hable's Uncharted 2 filmic tone map
// http://filmicgames.com/archives/75
vec3 ApplyUc2Curve(vec3 color)
{
    float A = 0.22;//Shoulder Strength
    float B = 0.3;//Linear Strength
    float C = 0.1;//Linear Angle
    float D = 0.2;//Toe Strength
    float E = 0.01;//Toe Numerator
    float F = 0.3;//Toe Denominator

    color = ((color * (A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-(E/F);
    return color;
}

vec3 Uc2ToneMap(vec3 color)
{
    float exposureBias = 2.0f;
    color = ApplyUc2Curve(exposureBias * color);
    float whiteScale = 1 / ApplyUc2Curve(gWhiteScale.xxx).x;
    color = color * whiteScale;

    return color;
}

vec3 AcesToneMap(vec3 color)
{
    float A = 2.51f;
    float B = 0.03f;
    float C = 2.43f;
    float D = 0.59f;
    float E = 0.14f;

    color = saturate((color*(A*color+B))/(color*(C*color+D)+E));
    return color;
}

vec4 calcColor(vec2 texC)
{
    vec4 color = gColorTex.Sample(gPointSampler, texC);
    vec3 exposedColor = calcExposedColor(color.rgb, texC);
#ifdef _LUMINANCE
    float luminance = calcLuminance(color.xyz);
    luminance = log2(max(0.0001, luminance));
    return vec4(luminance, 0, 0, 1);
#elif defined _CLAMP
    return color;
#elif defined _LINEAR
    return vec4(linearToneMap(exposedColor), color.a);
#elif defined _REINHARD
    return vec4(reinhardToneMap(exposedColor), color.a);
#elif defined _REINHARD_MOD
    return vec4(reinhardModifiedToneMap(exposedColor), color.a);
#elif defined _HEJI_HABLE_ALU
    return vec4(hejiHableAluToneMap(exposedColor), color.a);
#elif defined(_HABLE_UC2)
    return vec4(Uc2ToneMap(exposedColor), color.a);
#elif defined (_ACES)
    return vec4(AcesToneMap(exposedColor), color.a);
#endif
    return vec4(exposedColor, color.a);
}

vec4 main(float2 texC  : TEXCOORD) : SV_TARGET0
{
    return calcColor(texC);
}
