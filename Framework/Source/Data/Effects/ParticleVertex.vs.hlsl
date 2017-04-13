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
#include "ParticleData.h"

static const int xOffset[4] = { -1, -1, 1, 1 };
static const int yOffset[4] = { 1, -1, 1, -1 };
static const float xTex[4] = { 0.f, 0.f, 1.f, 1.f };
static const float yTex[4] = { 0.f, 1.f, 0.f, 1.f };

cbuffer PerFrame
{
    VSPerFrame frameData;
};

float2 vectorRotate(float2 vec, float sinAngle, float cosAngle)
{
    float2 result;
    result.x = vec.x * cosAngle - vec.y * sinAngle;
    result.y = vec.y * cosAngle + vec.x * sinAngle;
    return result;
}

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 texCoords : TEXCOORD;
    uint particleIndex : ID;
};

StructuredBuffer<uint> IndexList;
RWStructuredBuffer<Particle> ParticlePool;

VSOut main(uint vId : SV_VertexID, uint iId : SV_InstanceID)
{
    VSOut output;
    uint particleIndex = iId;
    Particle p = ParticlePool[IndexList[particleIndex]];
    uint billboardIndex = vId;

    float4 viewPos = mul(frameData.view, float4(p.pos, 1.f));
    float2 rotOffset = vectorRotate(float2(xOffset[billboardIndex], yOffset[billboardIndex]), sin(p.rot), cos(p.rot));
    viewPos.xy += float2(p.scale, p.scale) * rotOffset;
    output.pos = mul(frameData.proj, viewPos);
    output.texCoords = float2(xTex[billboardIndex], yTex[billboardIndex]);
    output.particleIndex = particleIndex;
    return output;
}

