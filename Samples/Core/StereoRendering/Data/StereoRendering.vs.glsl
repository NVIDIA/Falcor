/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
__import ShaderCommon;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inBitangent;
#ifdef HAS_TEXCRD
layout(location = 3) in vec2 inTexC;
#endif
#ifdef HAS_LIGHTMAP_UV
layout(location = 4) in vec2 inLightmapC;
#endif
#ifdef HAS_COLORS
layout(location = 5) in vec3 inColor;
#endif
#ifdef _VERTEX_BLENDING
layout(location = 6) in vec4 inBoneWeights;
layout(location = 7) in uvec4 inBoneIds;
#endif

layout(location = 0) out vec3 outNormalW;
layout(location = 1) out vec3 outBitangentW;
layout(location = 2) out vec2 outTexC;
layout(location = 3) out vec3 outPosW;
layout(location = 4) out vec3 outColorV;


mat4x4 getWorldMat()
{
#ifdef _VERTEX_BLENDING
    mat4x4 worldMat = getBlendedWorldMat(inBoneWeights, inBoneIds);
#else
    mat4x4 worldMat = gWorldMat[gl_InstanceIndex];
#endif
    return worldMat;
}

mat3x3 getWorldInvTransposeMat()
{
#ifdef _VERTEX_BLENDING
    mat3x3 worldInvTransposeMat = getBlendedInvTransposeWorldMat(inBoneWeights, inBoneIds);
#else
    mat3x3 worldInvTransposeMat = gWorldInvTransposeMat[gl_InstanceIndex];
#endif
    return worldInvTransposeMat;
}

void main()
{
    mat4x4 worldMat = getWorldMat();
    vec4 posW = worldMat * vec4(inPosition, 1.0f);
    outPosW = posW.xyz;

#ifdef HAS_TEXCRD
    outTexC = inTexC;
#else
    outTexC = vec2(0.0f);
#endif

#ifdef HAS_COLORS
    outColorV = inColor;
#else
    outColorV = vec3(0.0f);
#endif

    outNormalW = (getWorldInvTransposeMat() * inNormal).xyz;
    outBitangentW = (mat3x3(worldMat) * inBitangent).xyz;
}
