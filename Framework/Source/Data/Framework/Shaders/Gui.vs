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

#ifdef FALCOR_HLSL
cbuffer PerFrameCB : register(b0)
{
    float2 scale;
    float2 offset;
};

struct VsIn
{
    float2 pos : POSITION;
    float4 color : COLOR;
    float2 texC : TEXCOORD0;
};

struct VsOut
{
    float4 color : COLOR;
    float2 texC : TEXCOORD0;
    float4 pos : SV_POSITION;
};

VsOut main(VsIn vIn)
{
    VsOut vOut;
    vOut.color = vIn.color;
    vOut.texC = vIn.texC;
    vOut.pos.xy = vIn.pos.xy * scale + offset;
    vOut.pos.zw = float2(0,1);
    return vOut;
}
#endif

#ifdef FALCOR_GLSL
layout(set = 0, binding = 0) uniform PerFrameCB
{
    vec2 scale;
    vec2 offset;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texCIn;
layout(location = 2) in vec4 colorIn;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 texC;

void main()
{
    color = colorIn;
    texC = texCIn;
    gl_Position.xy = pos.xy * scale + offset;
    gl_Position.zw = vec2(0, 1);
}
#endif