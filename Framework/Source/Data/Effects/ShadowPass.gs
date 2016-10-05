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
#include "hlslglslcommon.h"
#include "VertexAttrib.h"
#include "ShaderCommon.h"
#include "csmdata.h"

layout (triangles, invocations = _CASCADE_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;
in int gl_InvocationID;
out int gl_Layer;
in vec2 texCin[3];
out vec2 texC;

CONSTANT_BUFFER(PerLightCB, 0)
{
    CsmData gCsmData;
};

void main()
{
    for(int i = 0 ; i < 3 ; i++)
    {
        gl_Position = gCsmData.globalMat * gl_in[i].gl_Position;

        gl_Position.xyz /= gl_Position.w;
        gl_Position.xyz *= gCsmData.cascadeScale[gl_InvocationID].xyz;
        gl_Position.xyz += gCsmData.cascadeOffset[gl_InvocationID].xyz;
        gl_Position.xyz *= gl_Position.w;

        gl_Layer = gl_InvocationID;
        texC = texCin[i];
        EmitVertex();
    }
}