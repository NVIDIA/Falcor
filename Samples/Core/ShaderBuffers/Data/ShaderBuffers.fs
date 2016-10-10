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
// This is just to show how nesting works
struct Matrices
{
    mat4 worldMat;
    mat4 wvpMat;
};

CONSTANT_BUFFER (PerFrameCB, 0)
{
	Matrices m;
    vec3 surfaceColor;
};

CONSTANT_BUFFER (LightCB, 1)
{
	vec3 worldDir;
	vec3 intensity;
};

vec4 calcColor(vec3 normalW)
{
    vec3 n = normalize(normalW);
    float nDotL = dot(n, -worldDir);
    nDotL = clamp(nDotL, 0, 1);
    vec4 color = vec4(nDotL * intensity * surfaceColor, 1);
    return color;
}

#ifdef FALCOR_HLSL
vec4 main(in vec3 normalW : NORMAL) : SV_TARGET
{
    return calcColor(normalW);
}

#else
in vec3 normalW;
out vec4 fragColor;

layout(binding = 0) buffer PixelCount
{
    uint count;   
} pixelCountBuffer;

void main()
{
    fragColor = calcColor(normalW);
    atomicAdd(pixelCountBuffer.count, 1); // This is a costly operation.
}
#endif