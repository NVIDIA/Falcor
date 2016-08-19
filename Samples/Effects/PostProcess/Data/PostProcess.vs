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
#include "hlslglslcommon.h"
#include "VertexAttrib.h"

UNIFORM_BUFFER (PerFrameCB, 0)
{
    mat4 gWorldMat;
    mat4 gWvpMat;
    Tex2D(gEnvMap);
    vec3 gEyePosW;
    float gLightIntensity;
    float gSurfaceRoughness;
};

#ifdef FALCOR_GLSL
out vec3 normalW;
out vec3 posW;
layout(location = VERTEX_POSITION_LOC) in vec4 posL;
layout(location = VERTEX_NORMAL_LOC)   in vec3 normalL;

void main()
#elif defined FALCOR_HLSL
void main(in vec4 posL : POSITION, in vec3 normalL : NORMAL, out vec3 normalW : NORMAL, out vec4 gl_Position : SV_POSITION)
#endif
{
    posW = (mul(gWvpMat,posL)).xyz;
	gl_Position = mul(gWvpMat,posL);
	normalW = (mul(gWorldMat, vec4(normalL, 0))).xyz;
}
