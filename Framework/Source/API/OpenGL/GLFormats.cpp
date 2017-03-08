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
#include "Framework.h"
#ifdef FALCOR_GL
#include "API/Formats.h"

namespace Falcor
{
    const GlFormatDesc kGlFormatDesc[] = 
    {
        // Format                                     Type                        BaseFormat         SizedFormat
        {ResourceFormat::Unknown,                   GL_NONE,                    GL_NONE,            GL_NONE},
        {ResourceFormat::R8Unorm,                   GL_UNSIGNED_BYTE,           GL_RED,             GL_R8},
        {ResourceFormat::R8Snorm,                   GL_BYTE,                    GL_RED,             GL_R8_SNORM},
        {ResourceFormat::R16Unorm,                  GL_UNSIGNED_SHORT,          GL_RED,             GL_R16},
        {ResourceFormat::R16Snorm,                  GL_SHORT,                   GL_RED,             GL_R16_SNORM},
        {ResourceFormat::RG8Unorm,                  GL_UNSIGNED_BYTE,           GL_RG,              GL_RG8},
        {ResourceFormat::RG8Snorm,                  GL_SHORT,                   GL_RG,              GL_RG8_SNORM},
        {ResourceFormat::RG16Unorm,                 GL_UNSIGNED_SHORT,          GL_RG,              GL_RG16},
        {ResourceFormat::RG16Snorm,                 GL_SHORT,                   GL_RG,              GL_RG16_SNORM},
        {ResourceFormat::RGB16Unorm,                GL_UNSIGNED_SHORT,          GL_RGB,             GL_RGB16},
        {ResourceFormat::RGB16Snorm,                GL_SHORT,                   GL_RGB,             GL_RGB16_SNORM},
        {ResourceFormat::RGB5A1Unorm,               GL_UNSIGNED_SHORT_5_5_5_1,  GL_RGBA,            GL_RGB5_A1},
        {ResourceFormat::RGBA8Unorm,                GL_UNSIGNED_BYTE,           GL_RGBA,            GL_RGBA8},
        {ResourceFormat::RGBA8Snorm,                GL_BYTE,                    GL_RGBA,            GL_RGBA8_SNORM},
        {ResourceFormat::RGB10A2Unorm,              GL_UNSIGNED_INT_10_10_10_2, GL_RGBA,            GL_RGB10_A2},
        {ResourceFormat::RGB10A2Uint,               GL_UNSIGNED_INT_10_10_10_2, GL_RGBA,            GL_RGB10_A2UI},
        {ResourceFormat::RGBA16Unorm,               GL_UNSIGNED_SHORT,          GL_RGBA,            GL_RGBA16},
        {ResourceFormat::RGBA8UnormSrgb,            GL_UNSIGNED_BYTE,           GL_RGBA,            GL_SRGB8_ALPHA8},
        {ResourceFormat::R16Float,                  GL_HALF_FLOAT,              GL_RED,             GL_R16F},
        {ResourceFormat::RG16Float,                 GL_HALF_FLOAT,              GL_RG,              GL_RG16F},
        {ResourceFormat::RGB16Float,                GL_HALF_FLOAT,              GL_RGB,             GL_RGB16F},
        {ResourceFormat::RGBA16Float,               GL_HALF_FLOAT,              GL_RGBA,            GL_RGBA16F},
        {ResourceFormat::R32Float,                  GL_FLOAT,                   GL_RED,             GL_R32F},
        {ResourceFormat::RG32Float,                 GL_FLOAT,                   GL_RG,              GL_RG32F},
        {ResourceFormat::RGB32Float,                GL_FLOAT,                   GL_RGB,             GL_RGB32F},
        {ResourceFormat::RGBA32Float,               GL_FLOAT,                   GL_RGBA,            GL_RGBA32F},
        {ResourceFormat::R11G11B10Float,            GL_FLOAT,                   GL_RGB,             GL_R11F_G11F_B10F},
        {ResourceFormat::RGB9E5Float,               GL_FLOAT,                   GL_RGB,             GL_RGB9_E5},
        {ResourceFormat::R8Int,                     GL_BYTE,                    GL_RED_INTEGER,     GL_R8I},
        {ResourceFormat::R8Uint,                    GL_UNSIGNED_BYTE,           GL_RED_INTEGER,     GL_R8UI},
        {ResourceFormat::R16Int,                    GL_SHORT,                   GL_RED_INTEGER,     GL_R16I},
        {ResourceFormat::R16Uint,                   GL_UNSIGNED_SHORT,          GL_RED_INTEGER,     GL_R16UI},
        {ResourceFormat::R32Int,                    GL_INT,                     GL_RED_INTEGER,     GL_R32I},
        {ResourceFormat::R32Uint,                   GL_UNSIGNED_INT,            GL_RED_INTEGER,     GL_R32UI},
        {ResourceFormat::RG8Int,                    GL_BYTE,                    GL_RG_INTEGER,      GL_RG8I},
        {ResourceFormat::RG8Uint,                   GL_UNSIGNED_BYTE,           GL_RG_INTEGER,      GL_RG8UI},
        {ResourceFormat::RG16Int,                   GL_SHORT,                   GL_RG_INTEGER,      GL_RG16I},
        {ResourceFormat::RG16Uint,                  GL_UNSIGNED_SHORT,          GL_RG_INTEGER,      GL_RG16UI},
        {ResourceFormat::RG32Int,                   GL_INT,                     GL_RG_INTEGER,      GL_RG32I},
        {ResourceFormat::RG32Uint,                  GL_UNSIGNED_INT,            GL_RG_INTEGER,      GL_RG32UI},
        {ResourceFormat::RGB16Int,                  GL_SHORT,                   GL_RGB_INTEGER,     GL_RGB16I},
        {ResourceFormat::RGB16Uint,                 GL_UNSIGNED_SHORT,          GL_RGB_INTEGER,     GL_RGB16UI},
        {ResourceFormat::RGB32Int,                  GL_INT,                     GL_RGB_INTEGER,     GL_RGB32I},
        {ResourceFormat::RGB32Uint,                 GL_UNSIGNED_INT,            GL_RGB_INTEGER,     GL_RGB32UI},
        {ResourceFormat::RGBA8Int,                  GL_BYTE,                    GL_RGBA_INTEGER,    GL_RGBA8I},
        {ResourceFormat::RGBA8Uint,                 GL_UNSIGNED_BYTE,           GL_RGBA_INTEGER,    GL_RGBA8UI},
        {ResourceFormat::RGBA16Int,                 GL_SHORT,                   GL_RGBA_INTEGER,    GL_RGBA16I},
        {ResourceFormat::RGBA16Uint,                GL_UNSIGNED_SHORT,          GL_RGBA_INTEGER,    GL_RGBA16UI},
        {ResourceFormat::RGBA32Int,                 GL_INT,                     GL_RGBA_INTEGER,    GL_RGBA32I},
        {ResourceFormat::RGBA32Uint,                GL_UNSIGNED_INT,            GL_RGBA_INTEGER,    GL_RGBA32UI},
        {ResourceFormat::BGRA8Unorm,                GL_UNSIGNED_BYTE,           GL_BGRA,            GL_RGBA8},
        {ResourceFormat::BGRA8UnormSrgb,            GL_UNSIGNED_BYTE,           GL_BGRA,            GL_SRGB8_ALPHA8},
        {ResourceFormat::RGBX8Unorm,                GL_UNSIGNED_BYTE,           GL_RGBA,            GL_RGBA8},
        {ResourceFormat::RGBX8UnormSrgb,            GL_UNSIGNED_BYTE,           GL_RGBA,            GL_SRGB8_ALPHA8},
        {ResourceFormat::BGRX8Unorm,                GL_UNSIGNED_BYTE,           GL_BGRA,            GL_RGBA8},
        {ResourceFormat::BGRX8UnormSrgb,            GL_UNSIGNED_BYTE,           GL_BGRA,            GL_SRGB8_ALPHA8},
        {ResourceFormat::Alpha8Unorm,               GL_UNSIGNED_BYTE,           GL_ALPHA,           GL_ALPHA8},
        {ResourceFormat::Alpha32Float,              GL_FLOAT,                   GL_ALPHA,           GL_ALPHA32F_ARB},
        {ResourceFormat::R5G6B5Unorm,               GL_UNSIGNED_SHORT_5_6_5,    GL_RGB,             GL_RGB565},
        {ResourceFormat::D32Float,                  GL_NONE,                    GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F},
        {ResourceFormat::D24Unorm,                  GL_NONE,                    GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT24},
        {ResourceFormat::D16Unorm,                  GL_NONE,                    GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16},
        {ResourceFormat::D32FloatS8X24,             GL_NONE,                    GL_DEPTH_STENCIL,   GL_DEPTH32F_STENCIL8},
        {ResourceFormat::D24UnormS8,                GL_NONE,                    GL_DEPTH_STENCIL,   GL_DEPTH24_STENCIL8},
        {ResourceFormat::BC1Unorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_RGB_S3TC_DXT1_EXT},
        {ResourceFormat::BC1UnormSrgb,              GL_NONE,                    GL_NONE,            GL_COMPRESSED_SRGB_S3TC_DXT1_EXT},
        {ResourceFormat::BC2Unorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT},
        {ResourceFormat::BC2UnormSrgb,              GL_NONE,                    GL_NONE,            GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT},
        {ResourceFormat::BC3Unorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_RGBA_S3TC_DXT5_EXT},
        {ResourceFormat::BC3UnormSrgb,              GL_NONE,                    GL_NONE,            GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT},
        {ResourceFormat::BC4Unorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_RED_RGTC1},
        {ResourceFormat::BC4Snorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_SIGNED_RED_RGTC1},
        {ResourceFormat::BC5Unorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_RG_RGTC2},
        {ResourceFormat::BC5Snorm,                  GL_NONE,                    GL_NONE,            GL_COMPRESSED_SIGNED_RG_RGTC2},
    };

    static_assert(arraysize(kGlFormatDesc) == (uint32_t)ResourceFormat::BC5Snorm + 1, "gGlFormatDesc[] array size mismatch.");


	const GLenum kGlTextureTarget[] =
	{
		GL_TEXTURE_1D,	//Texture1D
		GL_TEXTURE_2D,	//Texture2D
		GL_TEXTURE_3D,	//Texture3D
		GL_TEXTURE_CUBE_MAP,	//TextureCube
		GL_IMAGE_2D_MULTISAMPLE	//Texture2DMultisample
	};
}
#endif //#ifdef FALCOR_GL
