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

#define VERTEX_POSITION_LOC          0
#define VERTEX_NORMAL_LOC            1
#define VERTEX_TANGENT_LOC           2
#define VERTEX_BITANGENT_LOC         3
#define VERTEX_TEXCOORD_LOC          4
#define VERTEX_BONE_WEIGHT_LOC       5
#define VERTEX_BONE_ID_LOC           6
#define VERTEX_DIFFUSE_COLOR_LOC     7

#define VERTEX_LOCATION_COUNT        8

#define VERTEX_USER_ELEM_COUNT       4
#define VERTEX_USER0_LOC            (VERTEX_LOCATION_COUNT)

#define VERTEX_POSITION_NAME 	 	"POSITION"
#define VERTEX_NORMAL_NAME    	 	"NORMAL"
#define VERTEX_TANGENT_NAME       	"TANGENT"
#define VERTEX_BITANGENT_NAME     	"BITANGENT"
#define VERTEX_TEXCOORD_NAME		"TEXCOORD"
#define VERTEX_BONE_WEIGHT_NAME     "BONE_WEIGHTS"
#define VERTEX_BONE_ID_NAME         "BONE_IDS"
#define VERTEX_DIFFUSE_COLOR_NAME   "DIFFUSE_COLOR"

#ifdef _COMPILE_DEFAULT_VS
#include "ShaderCommon.h"

struct VS_IN
{
    float4 pos         : POSITION;
    float3 normal      : NORMAL;
    float3 tangent     : TANGENT;
    float3 bitangent   : BITANGENT;
#ifdef HAS_TEXCRD
    float2 texC        : TEXCOORD;
#endif
#ifdef HAS_COLORS
    float3 color       : DIFFUSE_COLOR;
#endif
#ifdef _VERTEX_BLENDING
    float4 boneWeights : BONE_WEIGHTS;
    uint4  boneIds     : BONE_IDS;
#endif
    uint instanceID : SV_INSTANCEID;
};

struct VS_OUT
{
    float3 normalW    : NORMAL;
    float3 tangentW   : TANGENT;
    float3 bitangentW : BITANGENT;
    float2 texC       : TEXCRD;
    float3 posW       : POSW;
    float3 colorV     : COLOR;
    float4 posH       : SV_POSITION;
};

#ifdef _SINGLE_PASS_STEREO
#extension GL_NV_viewport_array2: require
#extension GL_NV_stereo_view_rendering: require
layout(secondary_view_offset=1) out int gl_Layer;
#endif

float4x4 getWorldMat(VS_IN vIn)
{
#ifdef _VERTEX_BLENDING
    float4x4 worldMat = blendVertices(vIn.boneWeights, vIn.boneIds);
#else
    float4x4 worldMat = gWorldMat[vIn.instanceID];
#endif
    return worldMat;
}

VS_OUT defaultVS(VS_IN vIn)
{
    VS_OUT vOut;
    float4x4 worldMat = getWorldMat(vIn);
    float4 posW = mul(worldMat, vIn.pos);
    vOut.posW = posW.xyz;
    vOut.posH = mul(gCam.viewProjMat, posW);
#ifdef HAS_TEXCRD
    vOut.texC = vIn.texC;
#else
    vOut.texC = 0;
#endif
#ifdef HAS_COLORS
    vOut.colorV = vIn.color;
#else
    vOut.colorV = 0;
#endif
    vOut.normalW = mul((float3x3)worldMat, vIn.normal).xyz;
    vOut.tangentW = mul((float3x3)worldMat, vIn.tangent).xyz;
    vOut.bitangentW = mul((float3x3)worldMat, vIn.bitangent).xyz;

#ifdef _SINGLE_PASS_STEREO
  gl_SecondaryPositionNV.x = (gCam.rightEyeViewProjMat * vec4(posW, 1)).x;
  gl_SecondaryPositionNV.yzw = gl_Position.yzw;
  gl_Layer = 0;
#endif
  return vOut;
}
#endif
