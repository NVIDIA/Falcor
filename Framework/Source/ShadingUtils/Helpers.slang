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

#ifndef _FALCOR_SHADING_HELPERS_H_
#define _FALCOR_SHADING_HELPERS_H_

// Make sure we get the macros like `_fn` and `_ref`
// TODO: just eliminate these since we know this is pure Slang.
#include "HostDeviceData.h"

/*******************************************************************
					Sampling functions
*******************************************************************/

static const float M_PIf = 3.14159265359f;
static const float M_1_PIf = 0.31830988618379f;

_fn float2 sample_disk(float rnd1, float rnd2, float minRSq = 0.0f)
{
    float2 p;
    const float r = sqrt(max(minRSq, rnd1));
    const float phi = 2.0f * M_PIf * rnd2;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    return p;
}

_fn float3 sample_gauss(float rnd1, float rnd2)
{
    float3 p;
    const float r = sqrt(-2.f * log(1.f - rnd1));
    const float phi = 2.0f * M_PIf * rnd2;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    p.z = 0.0f;
    return p;
}

_fn inline float eval_gauss2_1d(float x2)
{
    return exp(-.5f * x2) / sqrt(2.0f * M_PIf);
}
_fn inline float eval_gauss2_2d(float x2)
{
    return exp(-.5f * x2) / (2.0f * M_PIf);
}
_fn inline float eval_gauss2_3d(float x2)
{
    return exp(-.5f * x2) / pow(2.0f * M_PIf, 1.5f);
}
// "band limit" (k sigma in frequency space): k / (2pi sigma)
_fn inline float eval_gauss_1d(float x)
{
    return eval_gauss2_1d(x * x);
}
_fn inline float eval_gauss_2d(float3 x)
{
    return eval_gauss2_2d(x.x * x.x + x.y * x.y);
}
_fn inline float eval_gauss_3d(float3 x)
{
    return eval_gauss2_3d(x.x * x.x + x.y * x.y + x.z * x.z);
}

// [0, 2] -> [1, 0]
// "band limit" (1st root in frequency space): 1/2
// Therefore x = t / step =F=> limit = 1/2 / step = 1/2 f_sampling
_fn inline float eval_cos2_wnd(float x)
{
    float c = abs(x) < 2.0f ? cos((M_PIf * 0.25f) * x) : 0.0f;
    return c * c;
}

_fn float3 cosine_sample_hemisphere(float rnd1, float rnd2)
{
    float3 p;
    p.xy = sample_disk(rnd1, rnd2);
    // Project up to hemisphere.
    p.z = sqrt(max(0.0f, 1.0f - p.x * p.x - p.y * p.y));
    return p;
}

_fn float3 uniform_sample_sphere(float rnd1, float rnd2)
{
    float3 p;
    p.z = 2.0f * rnd1 - 1.0f;
    const float r = sqrt(max(0.0f, 1.0f - p.z * p.z));
    const float phi = 2.0f * M_PIf * rnd2;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    return p;
}

_fn float3 uniform_sample_hemisphere(float rnd1, float rnd2)
{
    float3 p;
    p.z = rnd1;
    const float r = sqrt(max(0.0f, 1.0f - p.z * p.z));
    const float phi = 2.0f * M_PIf * rnd2;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    return p;
}
/**
	Random numbers based on Mersenne Twister
*/
_fn uint rand_init(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0;
    uint v1 = val1;
    uint s0 = 0;

    for(uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1<<4)+0xa341316c)^(v1+s0)^((v1>>5)+0xc8013ea4);
        v1 += ((v0<<4)+0xad90777d)^(v0+s0)^((v0>>5)+0x7e95761e);
    }

    return v0;
}

_fn float rand_next(_ref(uint) s)
{
    const uint LCG_A = 1664525u;
    const uint LCG_C = 1013904223u;
    s = (LCG_A * s + LCG_C);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

/*******************************************************************
					Shading routines
*******************************************************************/

// Ideal specular refraction
_fn float3 refract(const float3 I, const float3 N, float eta)
{
    float dNI = dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - dNI * dNI);
    float3 R;
    if (k < 0.0f) 
    {
        R = float3(0.0f, 0.0f, 0.0f);
    }
    else 
    {
        R = eta * I - (eta * dNI + sqrt(k)) * N;
    }
    return R;
}

/*******************************************************************
					Geometric routines
*******************************************************************/

void _fn createTangentFrame(in const float3 normal, _ref(float3) bitangent)
{
	if(abs(normal.x) > abs(normal.y))
		bitangent = float3(normal.z, 0.f, -normal.x) / length(float2(normal.x, normal.z));
	else
		bitangent = float3(0.f, normal.z, -normal.y) / length(float2(normal.y, normal.z));
}

void _fn reflectFrame(float3 n, float3 reflect, _ref(float3) t, _ref(float3) b)
{
    if(abs(dot(n, reflect)) > 0.999f)
        reflect = abs(n.z) < .8f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    b = normalize(cross(n, reflect));
    t = cross(b, n);
}

/*******************************************************************
					Texturing routines
*******************************************************************/

float4 _fn sampleTexture(Texture2D t, SamplerState s, const ShadingAttribs attr)
{
#ifndef _MS_USER_DERIVATIVES
    return t.SampleBias(s, attr.UV, attr.lodBias);
#else
	return t.SampleGrad(s, attr.UV, attr.DPDX, attr.DPDY);
#endif
}

#ifndef CUDA_CODE
float4 _fn sampleTexture(Texture2DArray t, SamplerState s, const ShadingAttribs attr, int arrayIndex)
{
#ifndef _MS_USER_DERIVATIVES
    return t.SampleBias(s, float3(attr.UV, arrayIndex), attr.lodBias);
#else
    return t.SampleGrad(s, float3(attr.UV, arrayIndex), attr.DPDX, attr.DPDY);
#endif
}
#endif

float4 _fn evalTex(in uint32_t hasTexture, in const Texture2D tex, SamplerState s, in const ShadingAttribs attr, in float4 defaultValue)
{
#ifndef _MS_DISABLE_TEXTURES
	if(hasTexture != 0)
    {
        // MAT_CODE
        defaultValue = sampleTexture(tex, s, attr);
    }
#endif
	return defaultValue;
}

float4 _fn evalWithColor(in uint32_t hasTexture, in const Texture2D tex, SamplerState s, float4 color, in const ShadingAttribs attr)
{
	return evalTex(hasTexture, tex, s, attr, color);
}

/*******************************************************************
					Normal mapping
*******************************************************************/

float3 _fn normalToRGB(in const float3 normal)
{
	return normal * 0.5f + 0.5f;
}

float3 _fn RGBToNormal(in const float3 rgbval)
{
    return rgbval * 2.f - 1.f;
}

float3 _fn fromLocal(in float3 v, in float3 t, in float3 b, in float3 n)
{
    return t * v.x + b * v.y + n * v.z;
}

float3 _fn toLocal(in float3 v, in float3 t, in float3 b, in float3 n)
{
    return float3(dot(v, t), dot(v, b), dot(v, n));
}

void _fn applyNormalMap(in float3 texValue, _ref(float3) n, _ref(float3) t, _ref(float3) b)
{
	const float3 normalMap = normalize(texValue);
	n = fromLocal(normalMap, normalize(t), normalize(b), normalize(n));
    // Orthogonalize tangent frame
    b = normalize(b - n * dot(b, n));
    t = normalize(cross(b, n));
}

// Forward declare it, just in case someone overrides it later
void _fn perturbNormal(in const MaterialData mat, _ref(ShadingAttribs) attr, bool forceSample = false);

#ifndef _MS_USER_NORMAL_MAPPING
void _fn perturbNormal(in const MaterialData mat, _ref(ShadingAttribs) attr, bool forceSample)
{
	if(forceSample || mat.desc.hasNormalMap != 0)
	{
		float3 texValue = sampleTexture(mat.textures.normalMap, mat.samplerState, attr).rgb;
        applyNormalMap(RGBToNormal(texValue), attr.N, attr.T, attr.B);
	}
}
#endif

/*******************************************************************
					Alpha test
*******************************************************************/

bool _fn alphaTestEnabled(in const MaterialData mat)
{
    return mat.desc.hasAlphaMap != 0;
}

bool _fn alphaTestPassed(in const MaterialData mat, in const ShadingAttribs attr)
{
#ifndef _MS_DISABLE_ALPHA_TEST
    if(sampleTexture(mat.textures.alphaMap, mat.samplerState, attr).x < mat.values.alphaThreshold)
        return false;
#endif
    return true;
}

void _fn applyAlphaTest(in const MaterialData mat, in const ShadingAttribs attr)
{
#ifndef _MS_DISABLE_ALPHA_TEST
    if(alphaTestEnabled(mat))
    {
        if(!alphaTestPassed(mat, attr))
            discard;
    }
#endif
}

/*******************************************************************
    Spherical map sampling
*******************************************************************/
float2 dirToSphericalCrd(float3 direction)
{
    static const float PI = 3.14159265f;
    float3 p = normalize(direction);
    float2 uv;
    uv.x = (1 + atan2(-p.z, p.x) / PI) * 0.5;
    uv.y = 1 - (-acos(p.y) / PI);
    return uv;
}

/*******************************************************************
					Color conversion
*******************************************************************/

float4 _fn applyAmbientOcclusion(float4 color, Texture2D aoTex, SamplerState s, float2 texC)
{
    float aoFactor = aoTex.SampleLevel(s, texC, 0).r;
    return float4(color.rgb * aoFactor, color.a);
}

#endif	// _FALCOR_SHADING_HELPERS_H_