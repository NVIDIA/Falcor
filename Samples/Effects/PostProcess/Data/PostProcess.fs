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
#version 430
#include "hlslglslcommon.h"

#define PI 3.141591

UNIFORM_BUFFER (PerFrameCB, 0)
{
    mat4 gWorldMat;
    mat4 gWvpMat;
    sampler2D gEnvMap;
    vec3 gEyePosW;
    float gLightIntensity;
    float gSurfaceRoughness;
};

vec4 calcColor(vec3 normalW, vec3 posW)
{
    vec3 p = normalize(normalW);
    vec2 uv;
    uv.x = ( 1 + atan(-p.z, p.x) / PI) * 0.5;
    uv.y = -acos(p.y) / PI;
    vec4 color = texture(gEnvMap, uv);
    color.rgb *= gLightIntensity;
#ifdef _TEXTURE_ONLY
    return color;
#endif
    // compute halfway vector
    vec3 eyeDir = normalize(gEyePosW - posW);
    vec3 h = normalize(eyeDir + normalW);
    float edoth = dot(eyeDir, h);
    float intensity = pow(clamp(edoth, 0, 1), gSurfaceRoughness);

    color.rgb *= intensity;
    return color;
}

#ifdef FALCOR_HLSL
vec4 main(in vec3 normalW : NORMAL, in vec3 posW : POSITION) : SV_TARGET
{
    return calcColor(normalW, posW);
}

#else
in vec3 normalW;
in vec3 posW;
out vec4 fragColor;

void main()
{
    fragColor = calcColor(normalW, posW);
}
#endif