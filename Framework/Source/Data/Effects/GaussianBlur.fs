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
#version 450
#include "HlslGlslCommon.h"
#expect _KERNEL_WIDTH

UNIFORM_BUFFER(PerImageCB, 0)
{
#ifdef _USE_TEX2D_ARRAY
    Tex2DArray(gSrcTex);
#else
    Tex2D(gSrcTex);
#endif
};

in vec2 texC;
out vec4 fragColor;

float getWeight(int i)
{
#if _KERNEL_WIDTH == 1
    const float w[1] = {1};
#elif _KERNEL_WIDTH == 3
    const float w[3] = {0.27901, 0.44198, 0.27901};
#elif _KERNEL_WIDTH == 5
    const float w[5] = {0.06136, 0.24477, 0.38774, 0.24477, 0.06136};
#elif _KERNEL_WIDTH == 7
    const float w[7] = {0.00598, 0.060626, 0.241843, 0.383103, 0.241843, 0.060626, 0.00598};
#elif _KERNEL_WIDTH == 9
    const float w[9] = {0.000229, 0.005977, 0.060598, 0.241732, 0.382928, 0.241732, 0.060598, 0.005977, 0.000229};
#elif _KERNEL_WIDTH == 11
    const float w[11] = {0.000003, 0.000229, 0.005977, 0.060598, 0.24173, 0.382925, 0.24173, 0.060598, 0.005977, 0.000229, 0.000003};
#else
Error. Kernel size must be an odd number in the range [1,11]
#endif
    return w[i];
}

vec4 blur(vec2 texC, const ivec2 direction)
{
    ivec2 offset = -(_KERNEL_WIDTH / 2) * direction;

    vec4 c = vec4(0,0,0,0);
    for(int i = 0 ; i < _KERNEL_WIDTH ; i++)
    {
#ifdef _USE_TEX2D_ARRAY
        c += textureLodOffset(gSrcTex, vec3(texC, gl_Layer), 0, offset)*getWeight(i);
#else
        c += textureLodOffset(gSrcTex, texC, 0, offset)*getWeight(i);
#endif
        offset += direction;
    }
    return c;
}

void main()
{
#ifdef _HORIZONTAL_BLUR
    fragColor = blur(texC, ivec2(1, 0));
#elif defined _VERTICAL_BLUR
    fragColor = blur(texC, ivec2(0, 1));
#else
    Error. Need to define either _HORIZONTAL_BLUR or _VERTICAL_BLUR
#endif
}