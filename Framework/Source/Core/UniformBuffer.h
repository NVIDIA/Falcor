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
#include "ShaderReflection.h"
#include "Texture.h"
#include "Buffer.h"

namespace Falcor
{
    class ProgramVersion;
    class Sampler;

    /** Variable naming rules are very similar to OpenGL uniform naming rules.\n
        When accessing a variable by name, you can only use a name which points to a basic Type, or an array of basic Type (so if you want the start of a structure, ask for the first field in the struct).\n
        Note that Falcor has 2 flavors of setting uniform by names - SetVariable() and SetVariableArray(). Naming rules for N-dimensional arrays of a basic Type are a little different between the two.\n
        SetVariable() must include N indices. SetVariableArray() can include N indices, or N-1 indices (implicit [0] as last index).\n\n
    */
    class UniformBuffer : public std::enable_shared_from_this<UniformBuffer>
    {
    public:
        using SharedPtr = std::shared_ptr<UniformBuffer>;
        using SharedConstPtr = std::shared_ptr<const UniformBuffer>;

        /** create a new uniform buffer.\n
            Even though the buffer is created with a specific program, it can be used with other programs as long as the buffer declarations are the same across programs.
            \param[in] pProgram A program object with the uniform buffer declared
            \param[in] bufferName The name of the buffer in the program
            \param[in] overrideSize - if 0, will use the buffer size as declared in the shader. Otherwise, will use this value as the buffer size. Useful when using buffers with dynamic arrays.
            \return A new buffer object if the operation was successful, otherwise nullptr
        */
        static SharedPtr create(const ProgramVersion* pProgram, const std::string& bufferName, size_t overrideSize = 0);

        ~UniformBuffer();

        /** Set a uniform into the buffer.
        The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] name The uniform name. See notes about naming in the UniformBuffer class description.
        \param[in] value Value to set
        */
        template<typename T>
        void setVariable(const std::string& name, const T& value);

        /** Set a uniform array in the buffer.
        The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] offset The uniform byte offset inside the buffer
        \param[in] pValue Pointer to an array of values to set
        \param[in] count pValue array size
        */
        template<typename T>
        void setVariableArray(size_t offset, const T* pValue, size_t count);

        /** Set a uniform into the buffer.
        The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] offset The uniform byte offset inside the buffer
        \param[in] value Value to set
        */
        template<typename T>
        void setVariable(size_t offset, const T& value);

        /** Set a uniform array in the buffer.
        The function will validate that the value Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] name The uniform name. See notes about naming in the UniformBuffer class description.
        \param[in] pValue Pointer to an array of values to set
        \param[in] count pValue array size
        */
        template<typename T>
        void setVariableArray(const std::string& name, const T* pValue, size_t count);

        /** Set a texture or image.
        The function will validate that the resource Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] name The uniform name in the program. See notes about naming in the UniformBuffer class description.
        \param[in] pTexture The resource to bind. If bBindAsImage is set, binds as image.
        \param[in] pSampler The sampler to use for filtering. If this is nullptr, the default sampler will be used
        \param[in] BindAsImage Whether pTexture should be bound as an image
        */
        void setTexture(const std::string& name, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage = false);

        /** Set a texture or image.
        The function will validate that the resource Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] name The uniform name in the program. See notes about naming in the UniformBuffer class description.
        \param[in] pTexture The resource to bind
        \param[in] pSampler The sampler to use for filtering. If this is nullptr, the default sampler will be used
        \param[in] count Number of textures to bind
        \param[in] BindAsImage Whether pTexture should be bound as an image
        */
        void setTextureArray(const std::string& name, const Texture* pTexture[], const Sampler* pSampler, size_t count, bool bindAsImage = false);

        /** Set a texture or image.
        The function will validate that the resource Type matches the declaration in the shader. If there's a mismatch, an error will be logged and the call will be ignored.
        \param[in] offset The uniform byte offset inside the buffer
        \param[in] pTexture The resource to bind. If bBindAsImage is set, binds as image.
        \param[in] pSampler The sampler to use for filtering. If this is nullptr, the default sampler will be used
        \param[in] BindAsImage Whether pTexture should be bound as an image
        */
        void setTexture(size_t Offset, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage = false);

        /** Apply the changes to the actual GPU buffer.
            Note that it is possible to use this function to update only part of the GPU copy of the buffer. This might lead to inconsistencies between the GPU and CPU buffer, so make sure you know what you are doing.
            \param[in] offset Offset into the buffer to write to
            \param[in] size   Number of bytes to upload. If this value is -1, will update the [Offset, EndOfBuffer] range.
        */
        void uploadToGPU(size_t offset = 0, size_t size = -1) const;

        /** Get the internal buffer object
        */
        Buffer::SharedPtr getBuffer() const { return mpBuffer; }

        /** Get uniform offset inside the buffer. See notes about naming in the UniformBuffer class description. Uniform name can be provided with an implicit array-index, similar to UniformBuffer#SetVariableArray.
        */
        size_t getVariableOffset(const std::string& varName) const;

        /** Set a block of data into the uniform buffer.\n
            If Offset + Size will result in buffer overflow, the call will be ignored and log an error.
            \param[in] pSrc Pointer to the source data.
            \param[in] offset Destination offset inside the buffer.
            \param[in] size Number of bytes in the source data.
        */
        void setBlob(const void* pSrc, size_t offset, size_t size);

        static const size_t kInvalidUniformOffset = (size_t)-1;
    protected:
        bool init(const ProgramVersion* pProgram, const std::string& bufferName, size_t overrideSize, bool isUniformBuffer);
        bool apiInit(const ProgramVersion* pProgram, const std::string& bufferName, bool isUniformBuffer);
        void setTextureInternal(size_t offset, const Texture* pTexture, const Sampler* pSampler);

        UniformBuffer(const std::string& bufferName);
        Buffer::SharedPtr mpBuffer = nullptr;
        const std::string mName;
        std::vector<uint8_t> mData;
        size_t mSize = 0;
        mutable bool mDirty = true;

        ShaderReflection::VariableDescMap mVariables;
        ShaderReflection::ShaderResourceDescMap mResources;

        template<bool ExpectArrayIndex>
        const ShaderReflection::VariableDesc* getVariableData(const std::string& name, size_t& offset) const;

#ifdef FALCOR_DX11
        friend class RenderContext;
        std::map<uint32_t, ID3D11ShaderResourceViewPtr>* mAssignedResourcesMap;
        std::map<uint32_t, ID3D11SamplerStatePtr>* mAssignedSamplersMap;
        const std::map<uint32_t, ID3D11ShaderResourceViewPtr>& getAssignedResourcesMap() const { return *mAssignedResourcesMap; }
        const std::map<uint32_t, ID3D11SamplerStatePtr>& getAssignedSamplersMap() const { return *mAssignedSamplersMap; }
#endif
    };
}