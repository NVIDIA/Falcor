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
#ifdef FALCOR_GLSL
#define UNIFORM_BUFFER(name, index) layout(binding = index) uniform name
#define mul(a,b) (a*b)
#define static


#define textureBias(tex_, crd_, bias_)	texture(tex_, crd_, bias_)

#elif defined FALCOR_HLSL

#define UNIFORM_BUFFER(name, index) cbuffer name : register(b##index)
#define mix lerp
#define fract frac

/*** Vectors / Matrices ***/
#define VECTOR(gl, dx) typedef vector<dx, 2> gl##2; typedef vector<dx, 3> gl##3; typedef vector<dx, 4> gl##4;
VECTOR(bvec, bool);
VECTOR(uvec, uint);
VECTOR(ivec, int);
VECTOR(vec, float);
VECTOR(dvec, double);
#undef VECTOR

#define mat2 float2x2
#define mat3 float2x2
#define mat4 float4x4

#define MATRIXNxM(n,m) typedef matrix<float, n, m> mat##n##x##m;
MATRIXNxM(2,2);
MATRIXNxM(2,3);
MATRIXNxM(2,4);
MATRIXNxM(3,2);
MATRIXNxM(3,3);
MATRIXNxM(3,4);
MATRIXNxM(4,2);
MATRIXNxM(4,3);
MATRIXNxM(4,4);
#undef MATRIXNxM
/***** End Vectors */

/** Textures and samplers types**/
#define make_sampler_struct(_a, _b) struct Sampler##_a##_b {Texture ## _a ## _b t; SamplerState s;} 
#define make_sampler_struct_with_array(_a, _b) make_sampler_struct(_a, _b) ; make_sampler_struct(_a, _b##Array)
#define make_typed_sampler_struct(_a, _b, _c) struct Sampler##_a##_b {Texture ## _a ## _b<_c> t; SamplerState s;} 

make_sampler_struct_with_array(1, D);
make_sampler_struct_with_array(2, D);
make_sampler_struct_with_array(Cu, be);
make_sampler_struct(3, D);
make_typed_sampler_struct(2, DMS, float);
make_typed_sampler_struct(2, DMSArray, float);

#define sampler1D Sampler1D
#define sampler2D Sampler2D
#define sampler3D Sampler3D
#define samplerCube SamplerCube
#define sampler1DArray Sampler1DArray
#define sampler2DArray Sampler2DArray
#define samplerCubeArray SamplerCubeArray
#define sampler2DMS Sampler2DMS
#define sampler2DMSArray Sampler2DMSArray

/*** Texture methods **/
vec4 texelFetch(sampler2D tex, ivec2 crd, int lod) { return tex.t.Load(ivec3(crd, lod)); }

#define texture(tex_, crd_) 				tex_.t.Sample(tex_.s, crd_)
#define textureOffset(tex_, crd_, offset_) 	tex_.Sample(tex_##Sampler, crd_, offset_)
#define textureLod(tex_, crd_, lod_) 		tex_.SampleLevel(tex_##Sampler, crd_, lod_)
#define textureGrad(tex_, crd_, ddx_, ddy_)	tex_.SampleGrad(tex_##Sampler, crd_, ddx_, ddy_)
#define textureBias(tex_, crd_, bias_)	tex_.SampleBias(tex_##Sampler, crd_, bias_)

/*** Texture operations **/
#endif

float projectTexCoord(vec2 u) { return u.x   / u.y; }
vec2  projectTexCoord(vec3 u) { return u.xy  / u.z; }
vec3  projectTexCoord(vec4 u) { return u.xyz / u.w; }

ivec2 formTexCoordForLoad(int   u, int lod) { return ivec2(u, lod); }
ivec3 formTexCoordForLoad(ivec2 u, int lod) { return ivec3(u, lod); }
ivec4 formTexCoordForLoad(ivec3 u, int lod) { return ivec4(u, lod); }

