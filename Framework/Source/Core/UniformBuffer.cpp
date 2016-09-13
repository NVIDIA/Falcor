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
#include "UniformBuffer.h"
#include "ProgramVersion.h"
#include "buffer.h"
#include "glm/glm.hpp"
#include "texture.h"

namespace Falcor
{
    using namespace ShaderReflection;

    inline const std::string removeLastArrayIndex(const std::string& name)
    {
        size_t dot = name.find_last_of(".");
        size_t bracket = name.find_last_of("[");

        if(bracket != std::string::npos)
        {
            // Ignore cases where the last index is an array of struct index (SomeStruct[1].v should be ignored)
            if((dot == std::string::npos) || (bracket > dot))
            {
                return name.substr(0, bracket);
            }
        }
        
        return name;
    }

    UniformBuffer::SharedPtr UniformBuffer::create(const ProgramVersion* pProgram, const std::string& bufferName, size_t overrideSize)
    {
        SharedPtr pBuffer = SharedPtr(new UniformBuffer(bufferName));
        if(pBuffer->init(pProgram, bufferName, overrideSize, true) == false)
        {
            pBuffer = nullptr;
        }

        return pBuffer;
    }

    bool UniformBuffer::init(const ProgramVersion* pProgram, const std::string& bufferName, size_t overrideSize, bool isUniformBuffer)
    {
        if(apiInit(pProgram, bufferName, isUniformBuffer) == false)
        {
            return false;
        }

        // create the internal data
        if(overrideSize != 0)
        {
            mSize = overrideSize;
        }

        if(mSize)
        {
            mData.assign(mSize, 0);
            mpBuffer = Buffer::create(mSize, Buffer::BindFlags::Uniform, Buffer::AccessFlags::MapWrite, mData.data());
        }
        
        return true;
    }
    
    UniformBuffer::UniformBuffer(const std::string& bufferName) : mName(bufferName)
    {

    }

    void UniformBuffer::uploadToGPU(size_t offset, size_t size) const
    {
        if(mDirty == false)
        {
            return;
        }

        if(mpBuffer == nullptr)
        {
            return;     // Can happen in DX11, if the buffer only contained textures
        }

        if(size == -1)
        {
            size = mSize - offset;
        }

        if(size + offset > mSize)
        {
            Logger::log(Logger::Level::Warning, "UniformBuffer::uploadToGPU() - trying to upload more data than what the buffer contains. Call is ignored.");
            return;
        }

        Buffer::MapType mapType = Buffer::MapType::Write;
        if((offset == 0) && (size = mSize))
        {
            mapType = Buffer::MapType::WriteDiscard; // Updating the entire buffer
        }

        uint8_t* pData = (uint8_t*)mpBuffer->map(mapType);
        assert(pData);
        memcpy(pData + offset, mData.data() + offset, size);
        mpBuffer->unmap();
        mDirty = false;
    }

    template<bool ExpectArrayIndex>
    __forceinline const VariableDesc* UniformBuffer::getVariableData(const std::string& name, size_t& offset) const
    {
        const std::string msg = "Error when getting uniform data\"" + name + "\" from uniform buffer \"" + mName + "\".\n";
        uint32_t arrayIndex = 0;

        // Look for the uniform
        auto& var = mVariables.find(name);

#ifdef FALCOR_D3D11
        if(var == mVariables.end())
        {
            // Textures might come from our struct. Try again.
            std::string texName = name + ".t";
            var = mVariables.find(texName);
        }
#endif
        if(var == mVariables.end())
        {
            // The name might contain an array index. Remove the last array index and search again
            std::string nameV2 = removeLastArrayIndex(name);
            var = mVariables.find(nameV2);

            if(var == mVariables.end())
            {
                Logger::log(Logger::Level::Error, msg + "Uniform not found.");
                return nullptr;
            }

            const auto& data = var->second;
            if(data.arraySize == 0)
            {
                // Not an array, so can't have an array index
                Logger::log(Logger::Level::Error, msg + "Uniform is not an array, so name can't include an array index.");
                return nullptr;
            }

            // We know we have an array index. Make sure it's in range
            std::string indexStr = name.substr(nameV2.length() + 1);
            char* pEndPtr;
            arrayIndex = strtol(indexStr.c_str(), &pEndPtr, 0);
            if(*pEndPtr != ']')
            {
                Logger::log(Logger::Level::Error, msg + "Array index must be a literal number (no whitespace are allowed)");
                return nullptr;
            }

            if(arrayIndex >= data.arraySize)
            {
                Logger::log(Logger::Level::Error, msg + "Array index (" + std::to_string(arrayIndex) + ") out-of-range. Array size == " + std::to_string(data.arraySize) + ".");
                return nullptr;
            }
        }
        else if(ExpectArrayIndex && var->second.arraySize > 0)
        {
            // Variable name should contain an explicit array index (for N-dim arrays, N indices), but the index was missing
            Logger::log(Logger::Level::Error, msg + "Expecting to find explicit array index in uniform name (for N-dimensional array, N indices must be specified).");
            return nullptr;
        }

        const auto* pData = &var->second;
        offset = pData->offset + pData->arrayStride * arrayIndex;
        return pData;
    }

    bool checkVariableType(VariableDesc::Type callType, VariableDesc::Type shaderType, const std::string& name, const std::string& bufferName)
    {
#if _LOG_ENABLED
        // Check that the types match
        if(callType != shaderType)
        {
            std::string msg("Error when setting uniform \"");
            msg += name + "\" to buffer \"" + bufferName + "\".\n";
            msg += "Type mismatch.\nsetUniform() was called with Type " + to_string(callType) + ".\nVariable was declared with Type " + to_string(shaderType) + ".\n\n";
            Logger::log(Logger::Level::Error, msg);
            assert(0);
            return false;
        }
#endif
        return true;
    }

    bool checkVariableByOffset(VariableDesc::Type callType, size_t offset, size_t count, const VariableDescMap& uniforms, const std::string& bufferName)
    {
#if _LOG_ENABLED
        // Find the uniform
        for(const auto& a : uniforms)
        {
            const auto& Data = a.second;
            size_t ArrayIndex = 0;
            bool bCheck = (Data.offset == offset);

            // If this is an array, check if we set an element inside it
            if(Data.arrayStride > 0 && offset > Data.offset)
            {
                size_t Stride = offset - Data.offset;
                if((Stride % Data.arrayStride) == 0)
                {
                    ArrayIndex = Stride / Data.arrayStride;
                    if(ArrayIndex < Data.arraySize)
                    {
                        bCheck = true;
                    }
                }
            }

            if(bCheck)
            {
                if(Data.arraySize == 0)
                {
                    if(count > 1)
                    {
                        std::string Msg("Error when setting uniform by offset. Found uniform \"" + a.first + "\" which is not an array, but trying to set more than 1 element");
                        Logger::log(Logger::Level::Error, Msg);
                        return false;
                    }
                }
                else if(ArrayIndex + count > Data.arraySize)
                {
                    std::string Msg("Error when setting uniform by offset. Found uniform \"" + a.first + "\" with array size " + std::to_string(a.second.arraySize));
                    Msg += ". Trying to set " + std::to_string(count) + " elements, starting at index " + std::to_string(ArrayIndex) + ", which will cause out-of-bound access. Ignoring call.";
                    Logger::log(Logger::Level::Error, Msg);
                    return false;
                }
                return checkVariableType(callType, a.second.type, a.first + "(Set by offset)", bufferName);
            }
        }
        std::string msg("Error when setting uniform by offset. No uniform found at offset ");
        msg += std::to_string(offset) + ". Ignoring call";
        Logger::log(Logger::Level::Error, msg);
        return false;
#else
        return true;
#endif
    }

#define set_uniform_offset(_var_type, _c_type) \
    template<> void UniformBuffer::setVariable(size_t offset, const _c_type& value)    \
    {                                                           \
        if(checkVariableByOffset(VariableDesc::Type::_var_type, offset, 1, mVariables, mName)) \
        {                                                       \
            const uint8_t* pVar = mData.data() + offset;        \
            *(_c_type*)pVar = value;                            \
            mDirty = true;                                      \
        }                                                       \
    }

    set_uniform_offset(Bool, bool);
    set_uniform_offset(Bool2, glm::bvec2);
    set_uniform_offset(Bool3, glm::bvec3);
    set_uniform_offset(Bool4, glm::bvec4);

    set_uniform_offset(Uint, uint32_t);
    set_uniform_offset(Uint2, glm::uvec2);
    set_uniform_offset(Uint3, glm::uvec3);
    set_uniform_offset(Uint4, glm::uvec4);

    set_uniform_offset(Int, int32_t);
    set_uniform_offset(Int2, glm::ivec2);
    set_uniform_offset(Int3, glm::ivec3);
    set_uniform_offset(Int4, glm::ivec4);

    set_uniform_offset(Float, float);
    set_uniform_offset(Float2, glm::vec2);
    set_uniform_offset(Float3, glm::vec3);
    set_uniform_offset(Float4, glm::vec4);

    set_uniform_offset(Float2x2, glm::mat2);
    set_uniform_offset(Float2x3, glm::mat2x3);
    set_uniform_offset(Float2x4, glm::mat2x4);

    set_uniform_offset(Float3x3, glm::mat3);
    set_uniform_offset(Float3x2, glm::mat3x2);
    set_uniform_offset(Float3x4, glm::mat3x4);

    set_uniform_offset(Float4x4, glm::mat4);
    set_uniform_offset(Float4x2, glm::mat4x2);
    set_uniform_offset(Float4x3, glm::mat4x3);

    set_uniform_offset(GpuPtr, uint64_t);

#undef set_uniform_offset

#define set_uniform_string(_var_type, _c_type) \
    template<> void UniformBuffer::setVariable(const std::string& name, const _c_type& value)    \
    {                                                           \
        size_t offset;                                    \
        const auto* pUniform = getVariableData<true>(name, offset);    \
        assert(pUniform);                                       \
        if((_LOG_ENABLED == 0) || (pUniform && checkVariableType(VariableDesc::Type::_var_type, pUniform->type, name, mName))) \
        {                                                       \
            setVariable(offset, value);                         \
        }                                                       \
    }

    set_uniform_string(Bool, bool);
    set_uniform_string(Bool2, glm::bvec2);
    set_uniform_string(Bool3, glm::bvec3);
    set_uniform_string(Bool4, glm::bvec4);

    set_uniform_string(Uint, uint32_t);
    set_uniform_string(Uint2, glm::uvec2);
    set_uniform_string(Uint3, glm::uvec3);
    set_uniform_string(Uint4, glm::uvec4);

    set_uniform_string(Int, int32_t);
    set_uniform_string(Int2, glm::ivec2);
    set_uniform_string(Int3, glm::ivec3);
    set_uniform_string(Int4, glm::ivec4);

    set_uniform_string(Float, float);
    set_uniform_string(Float2, glm::vec2);
    set_uniform_string(Float3, glm::vec3);
    set_uniform_string(Float4, glm::vec4);

    set_uniform_string(Float2x2, glm::mat2);
    set_uniform_string(Float2x3, glm::mat2x3);
    set_uniform_string(Float2x4, glm::mat2x4);

    set_uniform_string(Float3x3, glm::mat3);
    set_uniform_string(Float3x2, glm::mat3x2);
    set_uniform_string(Float3x4, glm::mat3x4);

    set_uniform_string(Float4x4, glm::mat4);
    set_uniform_string(Float4x2, glm::mat4x2);
    set_uniform_string(Float4x3, glm::mat4x3);

    set_uniform_string(GpuPtr, uint64_t);
#undef set_uniform_string

#define set_uniform_array_offset(_var_type, _c_type) \
    template<> void UniformBuffer::setVariableArray(size_t offset, const _c_type* pValue, size_t count)             \
    {                                                                                                               \
        if(checkVariableByOffset(VariableDesc::Type::_var_type, offset, count, mVariables, mName))                  \
        {                                                                                                           \
            const uint8_t* pVar = mData.data() + offset;                                                            \
            _c_type* pData = (_c_type*)pVar;                                                                        \
            for(size_t i = 0; i < count; i++)                                                                       \
            {                                                                                                       \
                pData[i] = pValue[i];                                                                               \
            }                                                                                                       \
            mDirty = true;                                                                                          \
        }                                                                                                           \
    }

    set_uniform_array_offset(Bool, bool);
    set_uniform_array_offset(Bool2, glm::bvec2);
    set_uniform_array_offset(Bool3, glm::bvec3);
    set_uniform_array_offset(Bool4, glm::bvec4);

    set_uniform_array_offset(Uint, uint32_t);
    set_uniform_array_offset(Uint2, glm::uvec2);
    set_uniform_array_offset(Uint3, glm::uvec3);
    set_uniform_array_offset(Uint4, glm::uvec4);

    set_uniform_array_offset(Int, int32_t);
    set_uniform_array_offset(Int2, glm::ivec2);
    set_uniform_array_offset(Int3, glm::ivec3);
    set_uniform_array_offset(Int4, glm::ivec4);

    set_uniform_array_offset(Float, float);
    set_uniform_array_offset(Float2, glm::vec2);
    set_uniform_array_offset(Float3, glm::vec3);
    set_uniform_array_offset(Float4, glm::vec4);

    set_uniform_array_offset(Float2x2, glm::mat2);
    set_uniform_array_offset(Float2x3, glm::mat2x3);
    set_uniform_array_offset(Float2x4, glm::mat2x4);

    set_uniform_array_offset(Float3x3, glm::mat3);
    set_uniform_array_offset(Float3x2, glm::mat3x2);
    set_uniform_array_offset(Float3x4, glm::mat3x4);

    set_uniform_array_offset(Float4x4, glm::mat4);
    set_uniform_array_offset(Float4x2, glm::mat4x2);
    set_uniform_array_offset(Float4x3, glm::mat4x3);

    set_uniform_array_offset(GpuPtr, uint64_t);

#undef set_uniform_array_offset

#define set_uniform_array_string(_var_type, _c_type) \
    template<>                                      \
    void UniformBuffer::setVariableArray(const std::string& name, const _c_type* pValue, size_t count)            \
    {                                                                                                             \
        size_t offset;                                                                                            \
        const auto pUniform = getVariableData<false>(name, offset);                                               \
        assert(pUniform);                                                                                         \
        if(pUniform && checkVariableType(VariableDesc::Type::_var_type, pUniform->type, name, mName))             \
        {                                                                                                         \
            setVariableArray(offset, pValue, count);                                                              \
        }                                                                                                         \
    }
    
    set_uniform_array_string(Bool, bool);
    set_uniform_array_string(Bool2, glm::bvec2);
    set_uniform_array_string(Bool3, glm::bvec3);
    set_uniform_array_string(Bool4, glm::bvec4);

    set_uniform_array_string(Uint, uint32_t);
    set_uniform_array_string(Uint2, glm::uvec2);
    set_uniform_array_string(Uint3, glm::uvec3);
    set_uniform_array_string(Uint4, glm::uvec4);

    set_uniform_array_string(Int, int32_t);
    set_uniform_array_string(Int2, glm::ivec2);
    set_uniform_array_string(Int3, glm::ivec3);
    set_uniform_array_string(Int4, glm::ivec4);

    set_uniform_array_string(Float, float);
    set_uniform_array_string(Float2, glm::vec2);
    set_uniform_array_string(Float3, glm::vec3);
    set_uniform_array_string(Float4, glm::vec4);

    set_uniform_array_string(Float2x2, glm::mat2);
    set_uniform_array_string(Float2x3, glm::mat2x3);
    set_uniform_array_string(Float2x4, glm::mat2x4);

    set_uniform_array_string(Float3x3, glm::mat3);
    set_uniform_array_string(Float3x2, glm::mat3x2);
    set_uniform_array_string(Float3x4, glm::mat3x4);

    set_uniform_array_string(Float4x4, glm::mat4);
    set_uniform_array_string(Float4x2, glm::mat4x2);
    set_uniform_array_string(Float4x3, glm::mat4x3);

    set_uniform_array_string(GpuPtr, uint64_t);

#undef set_uniform_array_string

    size_t UniformBuffer::getVariableOffset(const std::string& varName) const
    {
        size_t offset;
        const auto* pData = getVariableData<false>(varName, offset);
        return pData ? offset : kInvalidUniformOffset;
    }

    void UniformBuffer::setBlob(const void* pSrc, size_t offset, size_t size)
    {
        if((_LOG_ENABLED != 0) && (offset + size > mSize))
        {
            std::string Msg("Error when setting blob to buffer\"");
            Msg += mName + "\". Blob to large and will result in overflow. Ignoring call.";
            Logger::log(Logger::Level::Error, Msg);
            return;
        }
        memcpy(mData.data() + offset, pSrc, size);
        mDirty = true;
    }

    bool checkResourceDimension(const Texture* pTexture, const ShaderResourceDesc& shaderDesc, bool bindAsImage, const std::string& name, const std::string& bufferName)
    {
#if _LOG_ENABLED

        bool dimsMatch = false;
        bool formatMatch = false;
        bool imageMatch = bindAsImage ? (shaderDesc.type == ShaderResourceDesc::ResourceType::Image) : (shaderDesc.type == ShaderResourceDesc::ResourceType::Texture);

        Texture::Type texDim = pTexture->getType();
        bool isArray = pTexture->getArraySize() > 1;

        // Check if the dimensions match
        switch(shaderDesc.dims)
        {
        case ShaderResourceDesc::Dimensions::Texture1D:
            dimsMatch = (texDim == Texture::Type::Texture1D) && (isArray == false);
            break;
        case ShaderResourceDesc::Dimensions::Texture2D:
            dimsMatch = (texDim == Texture::Type::Texture2D) && (isArray == false);
            break;
        case ShaderResourceDesc::Dimensions::Texture3D:
            dimsMatch = (texDim == Texture::Type::Texture3D) && (isArray == false);
            break;
        case ShaderResourceDesc::Dimensions::TextureCube:
            dimsMatch = (texDim == Texture::Type::TextureCube) && (isArray == false);
            break;
        case ShaderResourceDesc::Dimensions::Texture1DArray:
            dimsMatch = (texDim == Texture::Type::Texture1D) && isArray;
            break;
        case ShaderResourceDesc::Dimensions::Texture2DArray:
            dimsMatch = (texDim == Texture::Type::Texture2D);
            break;
        case ShaderResourceDesc::Dimensions::Texture2DMS:
            dimsMatch = (texDim == Texture::Type::Texture2DMultisample) && (isArray == false);
            break;
        case ShaderResourceDesc::Dimensions::Texture2DMSArray:
            dimsMatch = (texDim == Texture::Type::Texture2DMultisample);
            break;
        case ShaderResourceDesc::Dimensions::TextureCubeArray:
            dimsMatch = (texDim == Texture::Type::TextureCube) && isArray;
            break;
        case ShaderResourceDesc::Dimensions::TextureBuffer:
            break;
        default:
            should_not_get_here();
        }

        // Check if the resource Type match
        FormatType texFormatType = getFormatType(pTexture->getFormat());

        switch(shaderDesc.retType)
        {
        case ShaderResourceDesc::ReturnType::Float:
            formatMatch = (texFormatType == FormatType::Float) || (texFormatType == FormatType::Snorm) || (texFormatType == FormatType::Unorm) || (texFormatType == FormatType::UnormSrgb);
            break;
        case ShaderResourceDesc::ReturnType::Int:
            formatMatch = (texFormatType == FormatType::Sint);
            break;
        case ShaderResourceDesc::ReturnType::Uint:
            formatMatch = (texFormatType == FormatType::Uint);
            break;
        default:
            should_not_get_here();
        }
        if((imageMatch && dimsMatch && formatMatch) == false)
        {
            std::string msg("Error when setting texture \"");
            msg += name + "\".\n";
            if(dimsMatch == false)
            {
                msg += "Dimensions mismatch.\nTexture has Type " + to_string(texDim) + (isArray ? "Array" : "") + ".\nUniform has Type " + to_string(shaderDesc.dims) + ".\n";
            }

            if(imageMatch == false)
            {
                msg += "Using ";
                msg += (bindAsImage ? "Image" : "Texture");
                msg += " method to bind ";
                msg += ((!bindAsImage) ? "Image" : "Texture") + std::string(".\n");
            }

            if(formatMatch == false)
            {
                msg += "Format mismatch.\nTexture has format Type " + to_string(texFormatType) + ".\nUniform has Type " + to_string(shaderDesc.retType) + ".\n";
            }

            msg += "\nError when setting resource to buffer " + bufferName;
            Logger::log(Logger::Level::Error, msg);
            return false;
        }
#endif
        return true;
    }

    ShaderReflection::ShaderResourceDescMap::const_iterator getResourceDescIt(const std::string& name, const ShaderReflection::ShaderResourceDescMap& descMap)
    {
        auto& it = descMap.find(name);
#ifdef FALCOR_D3D11
        // If it's not found and this is DX, search for out internal struct
        if(it == descMap.end())
        {
            it = descMap.find(name + ".t");
        }
#endif
        return it;
    }

    void UniformBuffer::setTexture(size_t offset, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage)
    {
        bool bOK = true;
#if _LOG_ENABLED
        // Debug checks
        if(pTexture)
        {
            for(const auto& a : mVariables)
            {
                if(a.second.type == VariableDesc::Type::Resource)
                {
                    size_t ArrayIndex = 0;
                    bool bCheck = (a.second.offset == offset);

                    // Check arrays
                    if(a.second.arrayStride > 0 && offset > a.second.offset)
                    {
                        size_t Stride = offset - a.second.offset;
                        if((Stride % a.second.arrayStride) == 0)
                        {
                            ArrayIndex = Stride / a.second.arrayStride;
                            if(ArrayIndex < a.second.arraySize)
                            {
                                bCheck = true;
                            }
                        }
                    }

                    if(bCheck)
                    {
                        const auto& it = getResourceDescIt(a.first, mResources);
                        assert(it != mResources.end());
                        const auto& desc = it->second;                      
                        if(desc.type != ShaderResourceDesc::ResourceType::Sampler)
                        {
                            bOK = checkResourceDimension(pTexture, desc, bindAsImage, a.first, mName);
                            break;
                        }
                    }
                }
            }

            if(bOK == false)
            {
                std::string msg("Error when setting texture by offset. No uniform found at offset ");
                msg += std::to_string(offset) + ". Ignoring call";
                Logger::log(Logger::Level::Error, msg);
            }
        }
#endif

        if(bOK)
        {
            mDirty = true;
            setTextureInternal(offset, pTexture, pSampler);
        }
    }

    void UniformBuffer::setTexture(const std::string& name, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage)
    {
        size_t Offset;
        const auto pUniform = getVariableData<true>(name, Offset);
        if(pUniform)
        {
            bool bOK = true;
#if _LOG_ENABLED == 1
            if(pTexture != nullptr)
            {
                const auto& it = getResourceDescIt(name, mResources);
                bOK = (it != mResources.end()) && checkResourceDimension(pTexture, it->second, bindAsImage, name, mName);
            }
#endif
            if(bOK)
            {
                setTexture(Offset, pTexture, pSampler, bindAsImage);
            }
        }
    }

    void UniformBuffer::setTextureArray(const std::string& name, const Texture* pTexture[], const Sampler* pSampler, size_t count, bool bindAsImage)
    {
        size_t Offset;
        const auto pUniform = getVariableData<false>(name, Offset);

        if(pUniform->arraySize < count)
        {
            Logger::log(Logger::Level::Warning, "Error when setting textures array. Count is larger than the array size. Ignoring out-of-bound elements");
            count = pUniform->arraySize;
        }

        if(pUniform)
        {
            for(uint32_t i = 0; i < count; i++)
            {
                bool bOK = true;
#if _LOG_ENABLED == 1
                if(pTexture[i] != nullptr)
                {
                    const auto& it = getResourceDescIt(name, mResources);
                    bOK = (it != mResources.end()) && checkResourceDimension(pTexture[i], it->second, bindAsImage, name, mName);
                }
#endif
                if(bOK)
                {
                    setTexture(Offset + i * sizeof(uint64_t), pTexture[i], pSampler, bindAsImage);
                }
            }
        }
    }
}