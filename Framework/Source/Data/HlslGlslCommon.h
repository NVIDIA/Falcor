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

#define Tex1D(a) 		sampler1D a
#define Tex2D(a) 		sampler2D a
#define Tex3D(a) 		Textire3D a
#define TexCube(a) 		samplerCube a
#define Tex1DArray(a) 	sampler1DArray a
#define Tex2DArray(a) 	sampler2DArray a
#define TexCubeArray(a) samplerCubeArray a
#define Tex2DMS(a) 		sampler2DMS a
#define Tex2DMSArray(a) sampler2DMSArray a
#define textureBias(tex_, crd_, bias_)	texture(tex_, crd_, bias_)

#elif defined FALCOR_HLSL

#define UNIFORM_BUFFER(name, index) cbuffer name : register(b##index)
#define mix lerp

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
#define sampler1D 		 Texture1D
#define sampler1DArray 	 Texture1DArray
#define sampler2D 	   	 Texture2D
#define sampler2DArray 	 Texture2DArray
#define sampler3D 	   	 Texture3D
#define samplerCube 	 TextureCube
#define samplerCubeArray TextureCubeArray
#define sampler2DMS 	 Texture2DMS
#define sampler2DMSArray Texture2DMSArray

#define Tex1D(a) 		Texture1D a; SamplerState a##Sampler
#define Tex2D(a) 		Texture2D a; SamplerState a##Sampler
#define Tex3D(a) 		Texture3D a; SamplerState a##Sampler
#define TexCube(a) 		TextureCube a; SamplerState a##Sampler
#define Tex2DMS(a) 		Texture2DMS a; SamplerState a##Sampler
#define Tex1DArray(a) 	Texture1DArray a; SamplerState a##Sampler
#define Tex2DArray(a)   Texture2DArray a; SamplerState a##Sampler
#define TexCubeArray(a) TextureCubeArray a; SamplerState a##Sampler
#define Tex2DMSArray(a) Texture2DMSArray a; SamplerState a##Sampler

/** Textures and samplers types**/

/*** Texture methods **/
vec4 texelFetch(Texture2D tex, ivec2 crd, int lod) {return tex.Load(ivec3(crd, lod));}

#define texture(tex_, crd_) 				tex_.Sample(tex_##Sampler, crd_)
#define textureOffset(tex_, crd_, offset_) 	tex_.Sample(tex_##Sampler, crd_, offset_)
#define textureLod(tex_, crd_, lod_) 		tex_.SampleLevel(tex_##Sampler, crd_, lod_)
#define textureGrad(tex_, crd_, ddx_, ddy_)	tex_.SampleGrad(tex_##Sampler, crd_, ddx_, ddy_)
#define textureBias(tex_, crd_, bias_)	tex_.SampleBias(tex_##Sampler, crd_, bias_)

/*** Texture operations **/
#endif
