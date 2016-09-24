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
#include <vector>
#include "Core/Shader.h"

namespace Falcor
{
    GLenum getGlShaderType(ShaderType Type)
    {
        switch(Type)
        {
        case ShaderType::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderType::Pixel:
            return GL_FRAGMENT_SHADER;
        case ShaderType::Hull:
            return GL_TESS_CONTROL_SHADER;
        case ShaderType::Domain:
            return GL_TESS_EVALUATION_SHADER;
        case ShaderType::Geometry:
            return GL_GEOMETRY_SHADER;
        case ShaderType::Compute:
            return GL_COMPUTE_SHADER;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    void SetApiHandle(void* pData, GLuint handle) { *(GLuint*)pData = handle; }

    template<>
    VertexShaderHandle Shader::getApiHandle<VertexShaderHandle>() const
    {
        return *(VertexShaderHandle*)mpPrivateData;
    }

    Shader::Shader(ShaderType shaderType) : mType(shaderType)
    {
        mpPrivateData = new GLenum;
        uint32_t apiHandle = gl_call(glCreateShader(getGlShaderType(mType)));
        SetApiHandle(mpPrivateData, apiHandle);
    }

    Shader::~Shader()
    {
        glDeleteShader(getApiHandle<VertexShaderHandle>());
        GLenum* pEnum = (GLenum*)mpPrivateData;
        safe_delete(pEnum);
    }

    Shader::SharedPtr Shader::create(const std::string& shaderString, ShaderType shaderType, std::string& log)
    {
        auto pShader = SharedPtr(new Shader(shaderType));

        uint32_t apiHandle = pShader->getApiHandle<uint32_t>();

        // Set the source
        GLint shaderLength = (GLint)shaderString.size();
        const GLchar* pStr = shaderString.c_str();
        gl_call(glShaderSource(apiHandle, 1, &pStr, &shaderLength));

        gl_call(glCompileShader(apiHandle));
        GLint success;
        gl_call(glGetShaderiv(apiHandle, GL_COMPILE_STATUS, &success));

        if(success == 0)
        {
            GLint logLength;
            // Get the size of the info log
            gl_call(glGetShaderiv(apiHandle, GL_INFO_LOG_LENGTH, &logLength));

            std::vector<GLchar> infoLog(logLength + 1);
            gl_call(glGetShaderInfoLog(apiHandle, logLength + 1, &logLength, &infoLog[0]));

            log = std::string(&infoLog[0]);
            return nullptr;
        }
        else
        {
            log.clear();
        }

        return pShader;
    }
}
#endif