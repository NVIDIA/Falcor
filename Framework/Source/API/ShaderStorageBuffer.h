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
#include <unordered_map>
#include "ConstantBuffer.h"

namespace Falcor
{
    class Buffer;
    class ProgramVersion;
    class Texture;
    class Sampler;

    class ShaderStorageBuffer : public ConstantBuffer, std::enable_shared_from_this<ShaderStorageBuffer>
    {
    public:
        class SsboVar : public CbVar<ShaderStorageBuffer>
        {
        public:
            SsboVar(ShaderStorageBuffer* pBuf, size_t offset) : CbVar(pBuf, offset) {}
            using CbVar::operator=;
            template <typename T> operator T() { T val; mpBuf->getVariable(mOffset, val) ; return val; }
        };

        using SharedPtr = ConstantBuffer::SharedPtrT<SsboVar>;
        using SharedConstPtr = std::shared_ptr<const ShaderStorageBuffer>;

        /** create a new shader storage buffer.\n
            Even though the buffer is created with a specific reflection object, it can be used with other programs as long as the buffer declarations are the same across programs.
            \param[in] pReflector A buffer-reflection object describing the buffer layout
            \param[in] overrideSize - if 0, will use the buffer size as declared in the shader. Otherwise, will use this value as the buffer size. Useful when using buffers with dynamic arrays.
            \return A new buffer object if the operation was successful, otherwise nullptr
        */
        static SharedPtr create(const ProgramReflection::BufferReflection::SharedConstPtr& pReflector, size_t overrideSize = 0);

        /** create a new shader storage buffer.\n
        This function is purely syntactic sugar. It will fetch the requested buffer reflector from the active program version and create the buffer from it
        \param[in] pProgram A program object which defines the buffer
        \param[in] overrideSize - if 0, will use the buffer size as declared in the shader. Otherwise, will use this value as the buffer size. Useful when using buffers with dynamic arrays.
        \return A new buffer object if the operation was successful, otherwise nullptr
        */
        static SharedPtr create(Program::SharedPtr& pProgram, const std::string& name, size_t overrideSize = 0);

        ~ShaderStorageBuffer();

        /** Read a variable from the buffer.
            The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
            \param[in] name The variable name. See notes about naming in the ConstantBuffer class description.
            \param[out] value The value read from the buffer
        */
        template<typename T>
        void getVariable(const std::string& name, T& value) const;

        /** Read an array of variables from the buffer.
            The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
            \param[in] offset The byte offset of the variable inside the buffer
            \param[in] count Number of elements to read
            \param[out] value Pointer to an array of values to read into
        */
        template<typename T>
        void getVariableArray(size_t offset, size_t count, T value[]) const;

        /** Read a variable from the buffer.
        The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] offset The byte offset of the variable inside the buffer
        \param[out] value The value read from the buffer
        */
        template<typename T>
        void getVariable(size_t offset, T& value) const;

        /** Read an array of variables from the buffer.
            The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
            \param[in] name The variable name. See notes about naming in the ConstantBuffer class description.
            \param[in] count Number of elements to read
            \param[out] value Pointer to an array of values to read into
        */
        template<typename T>
        void getVariableArray(const std::string& name, size_t count, T value[]) const;

        /** Read a block of data from the buffer.\n
            If Offset + Size will result in buffer overflow, the call will be ignored and log an error.
            \param pDst Pointer to a buffer to write the data into
            \param offset Byte offset to start reading from the buffer
            \param size Number of bytes to read from the buffer
        */
        void readBlob(void* pDst, size_t offset, size_t size) const;

        /** Read the buffer data from the GPU.\n
            Note that it is possible to use this function to update only part of the CPU copy of the buffer. This might lead to inconsistencies between the GPU and CPU buffer, so make sure you know what you are doing.
            \param[in] offset Offset into the buffer to read from
            \param[in] size   Number of bytes to read. If this value is -1, will update the [Offset, EndOfBuffer] range.
        */
        void readFromGPU(size_t offset = 0, size_t size = -1) const;

        /** Set the GPUCopyDirty flag
        */
        void setGpuCopyDirty() const { mGpuCopyDirty = true; }

    private:
        ShaderStorageBuffer(const ProgramReflection::BufferReflection::SharedConstPtr& pReflector) : ConstantBuffer(pReflector) {}
        mutable bool mGpuCopyDirty = false;
    };
}