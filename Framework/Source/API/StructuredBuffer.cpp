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
#include "StructuredBuffer.h"
#include "Buffer.h"

namespace Falcor
{
    bool checkVariableByOffset(ProgramReflection::Variable::Type callType, size_t offset, size_t count, const ProgramReflection::BufferReflection* pBufferDesc);
    bool checkVariableType(ProgramReflection::Variable::Type callType, ProgramReflection::Variable::Type shaderType, const std::string& name, const std::string& bufferName);

    ShaderStorageBuffer::SharedPtr ShaderStorageBuffer::create(const ProgramReflection::BufferReflection::SharedConstPtr& pReflection, size_t overrideSize)
    {
        auto pBuffer = SharedPtr(new ShaderStorageBuffer(pReflection));
        if(pBuffer->init(overrideSize, false) == false)
        {
            return nullptr;
        }

        return pBuffer;
    }

    void ShaderStorageBuffer::readFromGPU(size_t offset, size_t size) const
    {
        if(size == -1)
        {
            size = mSize - offset;
        }
        if(size + offset > mSize)
        {
            Logger::log(Logger::Level::Warning, "ShaderStorageBuffer::readFromGPU() - trying to read more data than what the buffer contains. Call is ignored.");
            return;
        }
        if(mGpuCopyDirty)
        {
            mGpuCopyDirty = false;
            mpBuffer->readData((void*)mData.data(), offset, size);
        }
    }

    ShaderStorageBuffer::~ShaderStorageBuffer() = default;

    void ShaderStorageBuffer::readBlob(void* pDest, size_t offset, size_t size) const   
    {    
        if(size + offset > mSize)
        {
            Logger::log(Logger::Level::Warning, "ShaderStorageBuffer::readBlob() - trying to read more data than what the buffer contains. Call is ignored.");
            return;
        }
        readFromGPU();
        memcpy(pDest, mData.data() + offset, size);
    }

    #define get_constant_offset(_var_type, _c_type) \
    template<> void ShaderStorageBuffer::getVariable(size_t offset, _c_type& value) const    \
    {                                                           \
        if(checkVariableByOffset(ProgramReflection::Variable::Type::_var_type, offset, 1, mpReflector.get())) \
        {                                                       \
            readFromGPU();                                      \
            const uint8_t* pVar = mData.data() + offset;        \
            value = *(const _c_type*)pVar;                      \
        }                                                       \
    }

    get_constant_offset(Bool, bool);
    get_constant_offset(Bool2, glm::bvec2);
    get_constant_offset(Bool3, glm::bvec3);
    get_constant_offset(Bool4, glm::bvec4);

    get_constant_offset(Uint, uint32_t);
    get_constant_offset(Uint2, glm::uvec2);
    get_constant_offset(Uint3, glm::uvec3);
    get_constant_offset(Uint4, glm::uvec4);

    get_constant_offset(Int, int32_t);
    get_constant_offset(Int2, glm::ivec2);
    get_constant_offset(Int3, glm::ivec3);
    get_constant_offset(Int4, glm::ivec4);

    get_constant_offset(Float, float);
    get_constant_offset(Float2, glm::vec2);
    get_constant_offset(Float3, glm::vec3);
    get_constant_offset(Float4, glm::vec4);

    get_constant_offset(Float2x2, glm::mat2);
    get_constant_offset(Float2x3, glm::mat2x3);
    get_constant_offset(Float2x4, glm::mat2x4);

    get_constant_offset(Float3x3, glm::mat3);
    get_constant_offset(Float3x2, glm::mat3x2);
    get_constant_offset(Float3x4, glm::mat3x4);

    get_constant_offset(Float4x4, glm::mat4);
    get_constant_offset(Float4x2, glm::mat4x2);
    get_constant_offset(Float4x3, glm::mat4x3);

    get_constant_offset(GpuPtr, uint64_t);

#undef get_constant_offset

#define get_constant_string(_var_type, _c_type) \
    template<> void ShaderStorageBuffer::getVariable(const std::string& name, _c_type& value) const    \
        {                                                                   \
            size_t offset;                                                  \
            const auto* pData = mpReflector->getVariableData(name, offset, false);     \
            if((_LOG_ENABLED == 0) || (pData && checkVariableType(ProgramReflection::Variable::Type::_var_type, pData->type, name, mpReflector->getName()))) \
            {                                                               \
                getVariable(offset, value);                                 \
            }                                                               \
        }

    get_constant_string(Bool, bool);
    get_constant_string(Bool2, glm::bvec2);
    get_constant_string(Bool3, glm::bvec3);
    get_constant_string(Bool4, glm::bvec4);

    get_constant_string(Uint, uint32_t);
    get_constant_string(Uint2, glm::uvec2);
    get_constant_string(Uint3, glm::uvec3);
    get_constant_string(Uint4, glm::uvec4);

    get_constant_string(Int, int32_t);
    get_constant_string(Int2, glm::ivec2);
    get_constant_string(Int3, glm::ivec3);
    get_constant_string(Int4, glm::ivec4);

    get_constant_string(Float, float);
    get_constant_string(Float2, glm::vec2);
    get_constant_string(Float3, glm::vec3);
    get_constant_string(Float4, glm::vec4);

    get_constant_string(Float2x2, glm::mat2);
    get_constant_string(Float2x3, glm::mat2x3);
    get_constant_string(Float2x4, glm::mat2x4);

    get_constant_string(Float3x3, glm::mat3);
    get_constant_string(Float3x2, glm::mat3x2);
    get_constant_string(Float3x4, glm::mat3x4);

    get_constant_string(Float4x4, glm::mat4);
    get_constant_string(Float4x2, glm::mat4x2);
    get_constant_string(Float4x3, glm::mat4x3);

    get_constant_string(GpuPtr, uint64_t);
#undef get_constant_string

#define get_constant_array_offset(_var_type, _c_type) \
    template<> void ShaderStorageBuffer::getVariableArray(size_t offset, size_t count, _c_type value[]) const   \
    {                                                               \
        if(checkVariableByOffset(ProgramReflection::Variable::Type::_var_type, offset, count, mpReflector.get()))          \
        {                                                           \
            readFromGPU();                                          \
            const uint8_t* pVar = mData.data() + offset;            \
            const _c_type* pMat = (_c_type*)pVar;                   \
            for(size_t i = 0; i < count; i++)                       \
            {                                                       \
                value[i] = pMat[i];                                 \
            }                                                       \
        }                                                           \
    }

    get_constant_array_offset(Bool, bool);
    get_constant_array_offset(Bool2, glm::bvec2);
    get_constant_array_offset(Bool3, glm::bvec3);
    get_constant_array_offset(Bool4, glm::bvec4);

    get_constant_array_offset(Uint, uint32_t);
    get_constant_array_offset(Uint2, glm::uvec2);
    get_constant_array_offset(Uint3, glm::uvec3);
    get_constant_array_offset(Uint4, glm::uvec4);

    get_constant_array_offset(Int, int32_t);
    get_constant_array_offset(Int2, glm::ivec2);
    get_constant_array_offset(Int3, glm::ivec3);
    get_constant_array_offset(Int4, glm::ivec4);

    get_constant_array_offset(Float, float);
    get_constant_array_offset(Float2, glm::vec2);
    get_constant_array_offset(Float3, glm::vec3);
    get_constant_array_offset(Float4, glm::vec4);

    get_constant_array_offset(Float2x2, glm::mat2);
    get_constant_array_offset(Float2x3, glm::mat2x3);
    get_constant_array_offset(Float2x4, glm::mat2x4);

    get_constant_array_offset(Float3x3, glm::mat3);
    get_constant_array_offset(Float3x2, glm::mat3x2);
    get_constant_array_offset(Float3x4, glm::mat3x4);

    get_constant_array_offset(Float4x4, glm::mat4);
    get_constant_array_offset(Float4x2, glm::mat4x2);
    get_constant_array_offset(Float4x3, glm::mat4x3);

    get_constant_array_offset(GpuPtr, uint64_t);

#undef get_constant_array_offset

#define get_constant_array_string(_var_type, _c_type) \
    template<> void ShaderStorageBuffer::getVariableArray(const std::string& name, size_t count, _c_type value[]) const    \
    {                                                                                                       \
        size_t offset;                                                                                      \
        const auto* pData = mpReflector->getVariableData(name, offset, true);                            \
        if((_LOG_ENABLED == 0) || (pData && checkVariableType(ProgramReflection::Variable::Type::_var_type, pData->type, name, mpReflector->getName()))) \
        {                                                                                                   \
            getVariableArray(offset, count, value);                                                         \
        }                                                                                                   \
    }

    get_constant_array_string(Bool, bool);
    get_constant_array_string(Bool2, glm::bvec2);
    get_constant_array_string(Bool3, glm::bvec3);
    get_constant_array_string(Bool4, glm::bvec4);

    get_constant_array_string(Uint, uint32_t);
    get_constant_array_string(Uint2, glm::uvec2);
    get_constant_array_string(Uint3, glm::uvec3);
    get_constant_array_string(Uint4, glm::uvec4);

    get_constant_array_string(Int, int32_t);
    get_constant_array_string(Int2, glm::ivec2);
    get_constant_array_string(Int3, glm::ivec3);
    get_constant_array_string(Int4, glm::ivec4);

    get_constant_array_string(Float, float);
    get_constant_array_string(Float2, glm::vec2);
    get_constant_array_string(Float3, glm::vec3);
    get_constant_array_string(Float4, glm::vec4);

    get_constant_array_string(Float2x2, glm::mat2);
    get_constant_array_string(Float2x3, glm::mat2x3);
    get_constant_array_string(Float2x4, glm::mat2x4);

    get_constant_array_string(Float3x3, glm::mat3);
    get_constant_array_string(Float3x2, glm::mat3x2);
    get_constant_array_string(Float3x4, glm::mat3x4);

    get_constant_array_string(Float4x4, glm::mat4);
    get_constant_array_string(Float4x2, glm::mat4x2);
    get_constant_array_string(Float4x3, glm::mat4x3);

    get_constant_array_string(GpuPtr, uint64_t);
#undef get_constant_array_string
}