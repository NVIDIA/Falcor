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
#pragma once
#include <string>

namespace Falcor
{
    class GlEnum2Str
    {
    public:
#define enum_case(_e) case _e: return #_e;
        static inline const std::string error(GLenum error)
        {
            switch(error)
            {
                enum_case(GL_NO_ERROR);
                enum_case(GL_INVALID_ENUM);
                enum_case(GL_INVALID_VALUE);
                enum_case(GL_INVALID_OPERATION);
                enum_case(GL_STACK_OVERFLOW);
                enum_case(GL_STACK_UNDERFLOW);
                enum_case(GL_OUT_OF_MEMORY);
                enum_case(GL_INVALID_FRAMEBUFFER_OPERATION);
                enum_case(GL_TABLE_TOO_LARGE);
            default:
                return "Unknown GL error " + std::to_string(error);
            }
        }

        static inline const std::string uniformType(GLenum type)
        {
            switch(type)
            {
                enum_case(GL_FLOAT);
                enum_case(GL_FLOAT_VEC2);
                enum_case(GL_FLOAT_VEC3);
                enum_case(GL_FLOAT_VEC4);
                enum_case(GL_DOUBLE);
                enum_case(GL_DOUBLE_VEC2);
                enum_case(GL_DOUBLE_VEC3);
                enum_case(GL_DOUBLE_VEC4);
                enum_case(GL_INT);
                enum_case(GL_INT_VEC2);
                enum_case(GL_INT_VEC3);
                enum_case(GL_INT_VEC4);
                enum_case(GL_UNSIGNED_INT);
                enum_case(GL_UNSIGNED_INT_VEC2);
                enum_case(GL_UNSIGNED_INT_VEC3);
                enum_case(GL_UNSIGNED_INT_VEC4);
                enum_case(GL_BOOL);
                enum_case(GL_BOOL_VEC2);
                enum_case(GL_BOOL_VEC3);
                enum_case(GL_BOOL_VEC4);
                enum_case(GL_FLOAT_MAT2);
                enum_case(GL_FLOAT_MAT3);
                enum_case(GL_FLOAT_MAT4);
                enum_case(GL_FLOAT_MAT2x3);
                enum_case(GL_FLOAT_MAT2x4);
                enum_case(GL_FLOAT_MAT3x2);
                enum_case(GL_FLOAT_MAT3x4);
                enum_case(GL_FLOAT_MAT4x2);
                enum_case(GL_FLOAT_MAT4x3);
                enum_case(GL_DOUBLE_MAT2);
                enum_case(GL_DOUBLE_MAT3);
                enum_case(GL_DOUBLE_MAT4);
                enum_case(GL_DOUBLE_MAT2x3);
                enum_case(GL_DOUBLE_MAT2x4);
                enum_case(GL_DOUBLE_MAT3x2);
                enum_case(GL_DOUBLE_MAT3x4);
                enum_case(GL_DOUBLE_MAT4x2);
                enum_case(GL_DOUBLE_MAT4x3);
                enum_case(GL_SAMPLER_1D);
                enum_case(GL_SAMPLER_2D);
                enum_case(GL_SAMPLER_3D);
                enum_case(GL_SAMPLER_CUBE);
                enum_case(GL_SAMPLER_1D_SHADOW);
                enum_case(GL_SAMPLER_2D_SHADOW);
                enum_case(GL_SAMPLER_1D_ARRAY);
                enum_case(GL_SAMPLER_2D_ARRAY);
                enum_case(GL_SAMPLER_1D_ARRAY_SHADOW);
                enum_case(GL_SAMPLER_2D_ARRAY_SHADOW);
                enum_case(GL_SAMPLER_2D_MULTISAMPLE);
                enum_case(GL_SAMPLER_2D_MULTISAMPLE_ARRAY);
                enum_case(GL_SAMPLER_CUBE_SHADOW);
                enum_case(GL_SAMPLER_BUFFER);
                enum_case(GL_SAMPLER_2D_RECT);
                enum_case(GL_SAMPLER_2D_RECT_SHADOW);
                enum_case(GL_INT_SAMPLER_1D);
                enum_case(GL_INT_SAMPLER_2D);
                enum_case(GL_INT_SAMPLER_3D);
                enum_case(GL_INT_SAMPLER_CUBE);
                enum_case(GL_INT_SAMPLER_1D_ARRAY);
                enum_case(GL_INT_SAMPLER_2D_ARRAY);
                enum_case(GL_INT_SAMPLER_2D_MULTISAMPLE);
                enum_case(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
                enum_case(GL_INT_SAMPLER_BUFFER);
                enum_case(GL_INT_SAMPLER_2D_RECT);
                enum_case(GL_UNSIGNED_INT_SAMPLER_1D);
                enum_case(GL_UNSIGNED_INT_SAMPLER_2D);
                enum_case(GL_UNSIGNED_INT_SAMPLER_3D);
                enum_case(GL_UNSIGNED_INT_SAMPLER_CUBE);
                enum_case(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY);
                enum_case(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
                enum_case(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE);
                enum_case(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
                enum_case(GL_UNSIGNED_INT_SAMPLER_BUFFER);
                enum_case(GL_UNSIGNED_INT_SAMPLER_2D_RECT);
                enum_case(GL_IMAGE_1D);
                enum_case(GL_IMAGE_2D);
                enum_case(GL_IMAGE_3D);
                enum_case(GL_IMAGE_2D_RECT);
                enum_case(GL_IMAGE_CUBE);
                enum_case(GL_IMAGE_BUFFER);
                enum_case(GL_IMAGE_1D_ARRAY);
                enum_case(GL_IMAGE_2D_ARRAY);
                enum_case(GL_IMAGE_2D_MULTISAMPLE);
                enum_case(GL_IMAGE_2D_MULTISAMPLE_ARRAY);
                enum_case(GL_INT_IMAGE_1D);
                enum_case(GL_INT_IMAGE_2D);
                enum_case(GL_INT_IMAGE_3D);
                enum_case(GL_INT_IMAGE_2D_RECT);
                enum_case(GL_INT_IMAGE_CUBE);
                enum_case(GL_INT_IMAGE_BUFFER);
                enum_case(GL_INT_IMAGE_1D_ARRAY);
                enum_case(GL_INT_IMAGE_2D_ARRAY);
                enum_case(GL_INT_IMAGE_2D_MULTISAMPLE);
                enum_case(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
                enum_case(GL_UNSIGNED_INT_IMAGE_1D);
                enum_case(GL_UNSIGNED_INT_IMAGE_2D);
                enum_case(GL_UNSIGNED_INT_IMAGE_3D);
                enum_case(GL_UNSIGNED_INT_IMAGE_2D_RECT);
                enum_case(GL_UNSIGNED_INT_IMAGE_CUBE);
                enum_case(GL_UNSIGNED_INT_IMAGE_BUFFER);
                enum_case(GL_UNSIGNED_INT_IMAGE_1D_ARRAY);
                enum_case(GL_UNSIGNED_INT_IMAGE_2D_ARRAY);
                enum_case(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE);
                enum_case(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
                enum_case(GL_UNSIGNED_INT_ATOMIC_COUNTER);
            default:
                return "Unknown GL uniform Type " + std::to_string(type);
            }
        }

        static inline const std::string textureType(GLenum type)
        {
            switch(type)
            {
                enum_case(GL_TEXTURE_1D);
                enum_case(GL_TEXTURE_2D);
                enum_case(GL_TEXTURE_3D);
                enum_case(GL_TEXTURE_1D_ARRAY);
                enum_case(GL_TEXTURE_2D_ARRAY);
                enum_case(GL_TEXTURE_RECTANGLE);
                enum_case(GL_TEXTURE_CUBE_MAP);
                enum_case(GL_TEXTURE_CUBE_MAP_ARRAY);
                enum_case(GL_TEXTURE_BUFFER);
                enum_case(GL_TEXTURE_2D_MULTISAMPLE);
                enum_case(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
            default:
                return "Unknown GL texture Type " + std::to_string(type);
            }
        }
    };
#undef enum_case
}