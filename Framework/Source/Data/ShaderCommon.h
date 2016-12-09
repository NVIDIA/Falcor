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

/*******************************************************************
                    Common OGL bindings
*******************************************************************/

#ifndef _FALCOR_SHADER_COMMON_H_
#define _FALCOR_SHADER_COMMON_H_

#include "HostDeviceData.h"

cbuffer InternalPerFrameCB : register(b10)
{
    CameraData gCam;
};

cbuffer InternalPerStaticMeshCB : register(b11)
{
    mat4 gWorldMat[64];
    uint32_t gMeshId;
};

cbuffer InternalPerSkinnedMeshCB : register(b12)
{
    mat4 gBones[64];
};

#ifdef _VERTEX_BLENDING
mat4 blendVertices(vec4 weights, uint4 ids)
{
    mat4 worldMat = gBones[ids.x] * weights.x;
    worldMat += gBones[ids.y] * weights.y;
    worldMat += gBones[ids.z] * weights.z;
    worldMat += gBones[ids.w] * weights.w;

    return worldMat;
}
#endif

cbuffer InternalPerMaterialCB : register(b13)
{
    MaterialData gMaterial;
    MaterialData gTemporalMaterial;
    float gTemporalLODThreshold;
    bool gEnableTemporalNormalMaps;
    bool gDebugTemporalMaterial;
};

/*******************************************************************
                    GLSL Evaluation routines
*******************************************************************/
#if defined(FALCOR_GL) || defined(FALCOR_GLSL)
bool isSamplerBound(in sampler2D sampler)
{
    return any(uvec2(sampler) != 0);
}

vec4 fetchTextureIfFound(in sampler2D sampler, in vec2 uv, in vec2 duvdx, in vec2 duvdy)
{
    vec4 ret = vec4(1.0f);
    if(isSamplerBound(sampler)) 
    {
        ret = textureGrad(sampler, uv, duvdx, duvdy);
    }
    return ret;
}
#endif // defined(FALCOR_GL) || defined(FALCOR_GLSL)
#endif  // _FALCOR_SHADER_COMMON_H_