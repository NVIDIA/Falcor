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
#include "HostDeviceData.h"

/*******************************************************************
					Math functions
*******************************************************************/

#ifndef M_PIf
#define M_PIf 3.14159265359f
#endif

#ifndef M_1_PIf
#define M_1_PIf 0.31830988618379f
#endif

_fn vec3 sample_disk(float rnd1, float rnd2, float minR = 0.0f)
{
    vec3 p;
    const float r = max(sqrt(rnd1), minR);
    const float phi = 2.0f * M_PIf * rnd2;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    p.z = 0.0f;
    return p;
}

_fn vec3 sample_gauss(float rnd1, float rnd2)
{
    vec3 p;
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
_fn inline float eval_gauss_2d(vec3 x)
{
    return eval_gauss2_2d(x.x * x.x + x.y * x.y);
}
_fn inline float eval_gauss_3d(vec3 x)
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

_fn vec3 cosine_sample_hemisphere(float rnd1, float rnd2)
{
    vec3 p = sample_disk(rnd1, rnd2);
    // Project up to hemisphere.
    p.z = sqrt(max(0.0f, 1.0f - p.x * p.x - p.y * p.y));
    return p;
}

_fn vec3 uniform_sample_sphere(float rnd1, float rnd2)
{
    vec3 p;
    p.z = 2.0f * rnd1 - 1.0f;
    const float r = sqrt(max(0.0f, 1.0f - p.z * p.z));
    const float phi = 2.0f * M_PIf * rnd2;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    return p;
}

_fn vec3 uniform_sample_hemisphere(float rnd1, float rnd2)
{
    vec3 p;
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
					Geometric routines
*******************************************************************/

void _fn createTangentFrame(in const vec3 normal, _ref(vec3) tangent, _ref(vec3) bitangent)
{
	if(abs(normal.x) > abs(normal.y))
		bitangent = v3(normal.z, 0.f, -normal.x) / length(v2(normal.x, normal.z));
	else
		bitangent = v3(0.f, normal.z, -normal.y) / length(v2(normal.y, normal.z));
	tangent = cross(bitangent, normal);
}

void _fn reflectFrame(vec3 n, vec3 reflect, _ref(vec3) t, _ref(vec3) b)
{
    if(abs(dot(n, reflect)) > 0.999f)
        reflect = abs(n.z) < .8f ? v3(0.f, 0.f, 1.f) : v3(1.f, 0.f, 0.f);
    b = normalize(cross(n, reflect));
    t = cross(b, n);
}

/*******************************************************************
					Texturing routines
*******************************************************************/

vec4 _fn sampleTexture(Texture2D t, SamplerState s, const ShadingAttribs attr)
{
#ifndef _MS_USER_DERIVATIVES
    return t.SampleBias(s, attr.UV, attr.lodBias);
#else
	return t.SampleGrad(s, attr.UV, attr.DPDX, attr.DPDY);
#endif
}

#ifndef CUDA_CODE
vec4 _fn sampleTexture(Texture2DArray t, SamplerState s, const ShadingAttribs attr, int arrayIndex)
{
#ifndef _MS_USER_DERIVATIVES
    return t.SampleBias(s, vec3(attr.UV, arrayIndex), attr.lodBias);
#else
    return t.SampleGrad(s, vec3(attr.UV, arrayIndex), attr.DPDX, attr.DPDY);
#endif
}
#endif

vec4 _fn evalTex(in uint32_t hasTexture, in const Texture2D tex, SamplerState s, in const ShadingAttribs attr, in vec4 defaultValue)
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

vec4 _fn evalWithColor(in uint32_t hasTexture, in const Texture2D tex, SamplerState s, float4 color, in const ShadingAttribs attr)
{
	return evalTex(hasTexture, tex, s, attr, color);
}

/*******************************************************************
					Normal mapping
*******************************************************************/

vec3 _fn normalToRGB(in const vec3 normal)
{
	return normal * 0.5f + 0.5f;
}

vec3 _fn RGBToNormal(in const vec3 rgbval)
{
    return rgbval * 2.f - 1.f;
}

vec3 _fn fromLocal(in vec3 v, in vec3 t, in vec3 b, in vec3 n)
{
    return t * v.x + b * v.y + n * v.z;
}

vec3 _fn toLocal(in vec3 v, in vec3 t, in vec3 b, in vec3 n)
{
    return v3(dot(v, t), dot(v, b), dot(v, n));
}

void _fn applyNormalMap(in vec3 texValue, _ref(vec3) n, _ref(vec3) t, _ref(vec3) b)
{
	const vec3 normalMap = normalize(texValue);
	n = fromLocal(normalMap, normalize(t), normalize(b), normalize(n));
    // Orthogonalize tangent frame
    t = normalize(t - n * dot(t, n));
    b = normalize(b - n * dot(b, n));
}

// Forward declare it, just in case someone overrides it later
void _fn perturbNormal(in const MaterialData mat, _ref(ShadingAttribs) attr, bool forceSample = false);

#ifndef _MS_USER_NORMAL_MAPPING
void _fn perturbNormal(in const MaterialData mat, _ref(ShadingAttribs) attr, bool forceSample)
{
	if(forceSample || mat.desc.hasNormalMap != 0)
	{
		vec3 texValue = sampleTexture(mat.textures.normalMap, mat.samplerState, attr).rgb;
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
					Color conversion
*******************************************************************/


#endif	// _FALCOR_SHADING_HELPERS_H_