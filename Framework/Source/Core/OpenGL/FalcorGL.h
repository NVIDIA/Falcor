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
#include "gl/glew.h"
#include "GlEnum2Str.h"
#include "Core/Formats.h"


#if _LOG_ENABLED
#define gl_call(a)\
	a; {\
		GLenum _err = glGetError();\
		if (_err!=GL_NO_ERROR) {\
			std::string str = "GL error when calling\n\""+std::string(#a)+"\"\n\nOpenGL error "+Falcor::GlEnum2Str::error(_err)+", in file "+__FILE__+", line "+std::to_string(__LINE__);\
			Falcor::Logger::log(Falcor::Logger::Level::Error, str);\
		}\
	}
#else
#define gl_call(a) a;
#endif

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    struct GlFormatDesc
    {
        ResourceFormat format;
        GLenum type;
        GLenum baseFormat;
        GLenum sizedFormat;
    };

    extern const GlFormatDesc kGlFormatDesc[];
    /** Get the OpenGL base format Type (GL_UNSIGNED_BYTE / GL_FLOAT / etc.)
    */
    inline GLenum getGlFormatType(ResourceFormat format)
    {
        assert(kGlFormatDesc[(uint32_t)format].format == format);
        return kGlFormatDesc[(uint32_t)format].type;
    }

    /** Get the OpenGL format components (GL_RGB / GL_RGBA / GL_DEPTH_COMPONENT / etc.)
    */
    inline GLenum getGlBaseFormat(ResourceFormat format)
    {
        assert(kGlFormatDesc[(uint32_t)format].format == format);
        return kGlFormatDesc[(uint32_t)format].baseFormat;
    }

    /** Get the OpenGL internal format (GL_R8 / GL_RGB8UI / GL_COMPRESSED_RG / etc.)
    */
    inline GLenum getGlSizedFormat(ResourceFormat format)
    {
        assert(kGlFormatDesc[(uint32_t)format].format == format);
        return kGlFormatDesc[(uint32_t)format].sizedFormat;
    }


	extern const GLenum kGlTextureTarget[];
	/** Return GL target namne for a given texture Type.
	*/
	inline GLenum getGlTextureTarget(int texType)
	{
		return kGlTextureTarget[texType];
	}

    /** Get the "natural" GLSL image string prefix
    */
    inline std::string getGlImagePrefixString(ResourceFormat format)
    {
        GlFormatDesc fdesc = kGlFormatDesc[(uint32_t)format];
        assert(fdesc.format == format);

        switch(fdesc.baseFormat)
        {
        case(GL_RED_INTEGER) :
        case(GL_RG_INTEGER) :
        case(GL_RGBA_INTEGER) :
                              if((fdesc.type == GL_UNSIGNED_BYTE) || (fdesc.type == GL_UNSIGNED_SHORT) || (fdesc.type == GL_UNSIGNED_INT))
                                  return "u";
                              else
                                  return "i";

        case(GL_RGB_INTEGER) :
            //Format not supported by images
            should_not_get_here();
        default:
            return "";
        }

    }

    /*! @} */

    using TextureHandle             = GLuint;
    using BufferHandle              = GLuint;
    using VaoHandle                 = GLuint;
    using VertexShaderHandle        = GLuint;
    using FragmentShaderHandle      = GLuint;
    using DomainShaderHandle        = GLuint;
    using HullShaderHandle          = GLuint;
    using GeometryShaderHandle      = GLuint;
    using ComputeShaderHandle       = GLuint;
    using ProgramHandle             = GLuint;
    using DepthStencilStateHandle   = GLuint;
    using RasterizerStateHandle     = GLuint;
    using BlendStateHandle          = GLuint;
    using SamplerApiHandle          = GLuint;
    using ShaderResourceViewHandle  = GLuint;
}

#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3dll.lib")

#define DEFAULT_API_MAJOR_VERSION 4
#define DEFAULT_API_MINOR_VERSION 4

#define UNSUPPORTED_IN_GL(msg_) {Falcor::Logger::log(Falcor::Logger::Level::Warning, msg_ + std::string(" is not supported in DX11. Ignoring call."));}
