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
#include "FalcorConfig.h"
#include <stdint.h>
#include <memory>
#include "glm/glm.hpp"

#ifndef arraysize
	#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#endif
#ifndef offsetof
	#define offsetof(s, m) (size_t)( (ptrdiff_t)&reinterpret_cast<const volatile char&>((((s *)0)->m)) )
#endif

#ifdef assert
#undef assert
#endif

#ifdef _DEBUG
#define assert(a)\
	if (!(a)) {\
		std::string str = "assertion failed(" + std::string(#a) + ")\nFile " + __FILE__ + ", line " + std::to_string(__LINE__);\
		Falcor::Logger::log(Falcor::Logger::Level::Fatal, str);\
	}
#define should_not_get_here() assert(false);
#else
#define assert(a)
#define should_not_get_here() __assume(0)
#endif

#define safe_delete(_a) {delete _a; _a = nullptr;}
#define safe_delete_array(_a) {delete[] _a; _a = nullptr;}

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    /** Falcor shader types
    */
    enum class ShaderType
    {
        Vertex,         ///< Vertex shader
        Fragment,       ///< Fragment shader
        Hull,           ///< Hull shader (AKA Tessellation control shader)
        Domain,         ///< Domain shader (AKA Tessellation evaluation shader)
        Geometry,       ///< Geometry shader
        Compute,        ///< Compute shader

        Count           ///< Shader Type count
    };

    /** Framebuffer target flags. Used for clears and copy operations
    */
    enum class FboAttachmentType
    {
        None    = 0,        ///< Nothing. Here just for completeness
        Color   = 1,        ///< Operate on the color buffer.
        Depth   = 2,        ///< Operate on the the depth buffer.
        Stencil = 4,        ///< Operate on the the stencil buffer.
         
        All = Color | Depth | Stencil ///< Operate on all targets
    };

    inline FboAttachmentType operator& (FboAttachmentType a, FboAttachmentType b)
    {
        return static_cast<FboAttachmentType>(static_cast<int>(a)& static_cast<int>(b));
    }


    inline FboAttachmentType operator| (FboAttachmentType a, FboAttachmentType b)
    {
        return static_cast<FboAttachmentType>(static_cast<int>(a)| static_cast<int>(b));
    }

    template<typename T>
    inline T min(T a, T b)
    {
        return (a < b) ? a : b;
    }

    template<typename T>
    inline T max(T a, T b)
    {
        return (a > b) ? a : b;
    }
    /*! @} */
}

#include "Utils/Logger.h"
#include "Utils/Profiler.h"

#ifdef FALCOR_GL
#include "Core/OpenGL/FalcorGL.h"
#elif defined FALCOR_D3D11
#include "Core/D3D//D3D11/FalcorD3D11.h"
#else
#error Undefined falcor backend. Make sure that a backend is selected in "FalcorConfig.h"
#endif

namespace Falcor
{
    inline const std::string to_string(ShaderType Type)
    {
        switch(Type)
        {
        case ShaderType::Vertex:
            return "vertex";
        case ShaderType::Fragment:
            return "pixel";
        case ShaderType::Hull:
            return "hull";
        case ShaderType::Domain:
            return "domain";
        case ShaderType::Geometry:
            return "geometry";
        default:
            should_not_get_here();
            return "";
        }
    }
}