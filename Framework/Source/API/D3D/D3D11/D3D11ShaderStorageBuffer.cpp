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
#ifdef FALCOR_D3D11
#include "../../ShaderStorageBuffer.h"
#include "glm/glm.hpp"

namespace Falcor
{
    ShaderStorageBuffer::SharedPtr ShaderStorageBuffer::create(const ProgramVersion* pProgram, const std::string& bufferName, size_t overrideSize)
    {
        UNSUPPORTED_IN_D3D11("ShaderStorageBuffer");
        return nullptr;
    }

    ShaderStorageBuffer::ShaderStorageBuffer(const std::string& bufferName) : ConstantBuffer(bufferName)
    {

    }

    void ShaderStorageBuffer::readFromGPU(size_t offset, size_t size) const
    {
    }

    ShaderStorageBuffer::~ShaderStorageBuffer() = default;

    #define get_constant_offset(c_type_) \
    template<> void ShaderStorageBuffer::getVariable(size_t Offset, c_type_& value) const    \
    {                                                           \
    }

    get_constant_offset(bool);
    get_constant_offset(glm::bvec2);
    get_constant_offset(glm::bvec3);
    get_constant_offset(glm::bvec4);

    get_constant_offset(uint32_t);
    get_constant_offset(glm::uvec2);
    get_constant_offset(glm::uvec3);
    get_constant_offset(glm::uvec4);

    get_constant_offset(int32_t);
    get_constant_offset(glm::ivec2);
    get_constant_offset(glm::ivec3);
    get_constant_offset(glm::ivec4);

    get_constant_offset(float);
    get_constant_offset(glm::vec2);
    get_constant_offset(glm::vec3);
    get_constant_offset(glm::vec4);

    get_constant_offset(glm::mat2);
    get_constant_offset(glm::mat2x3);
    get_constant_offset(glm::mat2x4);

    get_constant_offset(glm::mat3);
    get_constant_offset(glm::mat3x2);
    get_constant_offset(glm::mat3x4);

    get_constant_offset(glm::mat4);
    get_constant_offset(glm::mat4x2);
    get_constant_offset(glm::mat4x3);

    get_constant_offset(uint64_t);

#undef get_constant_offset

#define get_constant_string(c_type_) \
    template<> void ShaderStorageBuffer::getVariable(const std::string& Name, c_type_& value) const    \
        {                                                           \
        }

    get_constant_string(bool);
    get_constant_string(glm::bvec2);
    get_constant_string(glm::bvec3);
    get_constant_string(glm::bvec4);

    get_constant_string(uint32_t);
    get_constant_string(glm::uvec2);
    get_constant_string(glm::uvec3);
    get_constant_string(glm::uvec4);

    get_constant_string(int32_t);
    get_constant_string(glm::ivec2);
    get_constant_string(glm::ivec3);
    get_constant_string(glm::ivec4);

    get_constant_string(float);
    get_constant_string(glm::vec2);
    get_constant_string(glm::vec3);
    get_constant_string(glm::vec4);

    get_constant_string(glm::mat2);
    get_constant_string(glm::mat2x3);
    get_constant_string(glm::mat2x4);

    get_constant_string(glm::mat3);
    get_constant_string(glm::mat3x2);
    get_constant_string(glm::mat3x4);

    get_constant_string(glm::mat4);
    get_constant_string(glm::mat4x2);
    get_constant_string(glm::mat4x3);

    get_constant_string(uint64_t);
#undef get_constant_string

#define get_constant_array_offset(c_type_) \
    template<> void ShaderStorageBuffer::getVariableArray(size_t offset, size_t count, c_type_ value[]) const   \
    {                                                               \
    }

    get_constant_array_offset(bool);
    get_constant_array_offset(glm::bvec2);
    get_constant_array_offset(glm::bvec3);
    get_constant_array_offset(glm::bvec4);

    get_constant_array_offset(uint32_t);
    get_constant_array_offset(glm::uvec2);
    get_constant_array_offset(glm::uvec3);
    get_constant_array_offset(glm::uvec4);

    get_constant_array_offset(int32_t);
    get_constant_array_offset(glm::ivec2);
    get_constant_array_offset(glm::ivec3);
    get_constant_array_offset(glm::ivec4);

    get_constant_array_offset(float);
    get_constant_array_offset(glm::vec2);
    get_constant_array_offset(glm::vec3);
    get_constant_array_offset(glm::vec4);

    get_constant_array_offset(glm::mat2);
    get_constant_array_offset(glm::mat2x3);
    get_constant_array_offset(glm::mat2x4);

    get_constant_array_offset(glm::mat3);
    get_constant_array_offset(glm::mat3x2);
    get_constant_array_offset(glm::mat3x4);

    get_constant_array_offset(glm::mat4);
    get_constant_array_offset(glm::mat4x2);
    get_constant_array_offset(glm::mat4x3);

    get_constant_array_offset(uint64_t);

#undef get_constant_array_offset

#define get_constant_array_string(c_type_) \
    template<> void ShaderStorageBuffer::getVariableArray(const std::string& name, size_t count, c_type_ value[]) const    \
    {                                                                                                       \
    }

    get_constant_array_string(bool);
    get_constant_array_string(glm::bvec2);
    get_constant_array_string(glm::bvec3);
    get_constant_array_string(glm::bvec4);

    get_constant_array_string(uint32_t);
    get_constant_array_string(glm::uvec2);
    get_constant_array_string(glm::uvec3);
    get_constant_array_string(glm::uvec4);

    get_constant_array_string(int32_t);
    get_constant_array_string(glm::ivec2);
    get_constant_array_string(glm::ivec3);
    get_constant_array_string(glm::ivec4);

    get_constant_array_string(float);
    get_constant_array_string(glm::vec2);
    get_constant_array_string(glm::vec3);
    get_constant_array_string(glm::vec4);

    get_constant_array_string(glm::mat2);
    get_constant_array_string(glm::mat2x3);
    get_constant_array_string(glm::mat2x4);

    get_constant_array_string(glm::mat3);
    get_constant_array_string(glm::mat3x2);
    get_constant_array_string(glm::mat3x4);

    get_constant_array_string(glm::mat4);
    get_constant_array_string(glm::mat4x2);
    get_constant_array_string(glm::mat4x3);

    get_constant_array_string(uint64_t);
#undef get_constant_array_string
}
#endif //#ifdef FALCOR_D3D11
