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

#include "VertexAttrib.h"
#include "ShaderCommon.h"
#include "HlslGlslCommon.h"

layout(location = VERTEX_POSITION_LOC)  in vec4 vPos;
layout(location = VERTEX_NORMAL_LOC)    in vec3 vNormal;
layout(location = VERTEX_TANGENT_LOC)   in vec3 vTangent;
layout(location = VERTEX_BITANGENT_LOC) in vec3 vBitangent;
layout(location = VERTEX_TEXCOORD_LOC)  in vec2 vTexC;

#ifdef _VERTEX_BLENDING
layout(location = VERTEX_BONE_WEIGHT_LOC)  in vec4 vBoneWeights;
layout(location = VERTEX_BONE_ID_LOC)      in uvec4 vBoneIds;
#endif

out vec3 normalW;
out vec3 tangentW;
out vec3 bitangentW;
out vec2 texC;
out vec3 posW;

#ifdef _SINGLE_PASS_STEREO
#extension GL_NV_viewport_array2: require
#extension GL_NV_stereo_view_rendering: require
layout(secondary_view_offset=1) out int gl_Layer;
#endif

void main()
{
#ifdef _VERTEX_BLENDING
    mat4 worldMat = gWorldMat[gl_InstanceID]  * blendVertices(vBoneWeights, vBoneIds);
#else
    mat4 worldMat = gWorldMat[gl_InstanceID];
#endif
    posW = (worldMat * vPos).xyz;
    gl_Position = gCam.viewProjMat * worldMat * vPos;
    texC = vTexC;
    normalW = (mat3x3(worldMat) * vNormal).xyz;
    tangentW = (mat3x3(worldMat) * vTangent).xyz;
    bitangentW = (mat3x3(worldMat) * vBitangent).xyz;

#ifdef _SINGLE_PASS_STEREO
  gl_SecondaryPositionNV.x = (gCam.rightEyeViewProjMat * vec4(posW, 1)).x;
  gl_SecondaryPositionNV.yzw = gl_Position.yzw;
  gl_Layer = 0;
#endif
}