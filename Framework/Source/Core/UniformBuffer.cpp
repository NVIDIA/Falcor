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
#include "Core/ProgramReflection.h"

namespace Falcor
{
    UniformBuffer::SharedPtr UniformBuffer::create(const ProgramReflection::BufferReflection::SharedConstPtr& pReflector, size_t overrideSize)
    {
        SharedPtr pBuffer = SharedPtr(new UniformBuffer(pReflector));
        if(pBuffer->init(overrideSize, true) == false)
        {
            pBuffer = nullptr;
        }

        return pBuffer;
    }

    UniformBuffer::SharedPtr UniformBuffer::create(Program::SharedPtr& pProgram, const std::string& name, size_t overrideSize)
    {
        auto& pProgReflector = pProgram->getActiveVersion()->getReflector();
        auto& pBufferReflector = pProgReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Uniform);
        if(pBufferReflector)
        {
            return create(pBufferReflector, overrideSize);
        }
        return nullptr;
    }

    bool UniformBuffer::init(size_t overrideSize, bool isUniformBuffer)
    {
        mSize = mpReflector->getRequiredSize();

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
    
    size_t UniformBuffer::getVariableOffset(const std::string& varName) const
    {
        size_t offset;
        mpReflector->getVariableData(varName, offset, true);
        return offset;
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

    bool checkVariableType(ProgramReflection::Variable::Type callType, ProgramReflection::Variable::Type shaderType, const std::string& name, const std::string& bufferName)
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

    bool checkVariableByOffset(ProgramReflection::Variable::Type callType, size_t offset, size_t count, const ProgramReflection::BufferReflection* pBufferDesc)
    {
#if _LOG_ENABLED
        // Find the uniform
        for(auto& a = pBufferDesc->varBegin() ; a != pBufferDesc->varEnd() ; a++)
        {
            const auto& varDesc = a->second;
            const auto& varName = a->first;
            size_t arrayIndex = 0;
            bool checkThis = (varDesc.location == offset);

            // If this is an array, check if we set an element inside it
            if(varDesc.arrayStride > 0 && offset > varDesc.location)
            {
                size_t stride = offset - varDesc.location;
                if((stride % varDesc.arrayStride) == 0)
                {
                    arrayIndex = stride / varDesc.arrayStride;
                    if(arrayIndex < varDesc.arraySize)
                    {
                        checkThis = true;
                    }
                }
            }

            if(checkThis)
            {
                if(varDesc.arraySize == 0)
                {
                    if(count > 1)
                    {
                        std::string Msg("Error when setting uniform by offset. Found uniform \"" + varName + "\" which is not an array, but trying to set more than 1 element");
                        Logger::log(Logger::Level::Error, Msg);
                        return false;
                    }
                }
                else if(arrayIndex + count > varDesc.arraySize)
                {
                    std::string Msg("Error when setting uniform by offset. Found uniform \"" + varName + "\" with array size " + std::to_string(varDesc.arraySize));
                    Msg += ". Trying to set " + std::to_string(count) + " elements, starting at index " + std::to_string(arrayIndex) + ", which will cause out-of-bound access. Ignoring call.";
                    Logger::log(Logger::Level::Error, Msg);
                    return false;
                }
                return checkVariableType(callType, varDesc.type, varName + "(Set by offset)", pBufferDesc->getName());
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
        if(checkVariableByOffset(ProgramReflection::Variable::Type::_var_type, offset, 1, mpReflector.get())) \
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
        size_t offset;                                          \
        const auto* pUniform = mpReflector->getVariableData(name, offset, false); \
        bool valid = true;                                      \
        if((_LOG_ENABLED == 0) || (offset != ProgramReflection::kInvalidLocation && checkVariableType(ProgramReflection::Variable::Type::_var_type, pUniform->type, name, mpReflector->getName()))) \
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
        if(checkVariableByOffset(ProgramReflection::Variable::Type::_var_type, offset, count, mpReflector.get()))   \
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
        const auto& pVarDesc = mpReflector->getVariableData(name, offset, true);                                  \
        if( _LOG_ENABLED == 0 || (offset != ProgramReflection::kInvalidLocation && checkVariableType(ProgramReflection::Variable::Type::_var_type, pVarDesc->type, name, mpReflector->getName()))) \
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

    void UniformBuffer::setBlob(const void* pSrc, size_t offset, size_t size)
    {
        if((_LOG_ENABLED != 0) && (offset + size > mSize))
        {
            std::string Msg("Error when setting blob to buffer\"");
            Msg += mpReflector->getName() + "\". Blob to large and will result in overflow. Ignoring call.";
            Logger::log(Logger::Level::Error, Msg);
            return;
        }
        memcpy(mData.data() + offset, pSrc, size);
        mDirty = true;
    }

    bool checkResourceDimension(const Texture* pTexture, const ProgramReflection::Resource* pResourceDesc, bool bindAsImage, const std::string& name, const std::string& bufferName)
    {
#if _LOG_ENABLED

        bool dimsMatch = false;
        bool formatMatch = false;
        bool imageMatch = bindAsImage ? (pResourceDesc->type == ProgramReflection::Resource::ResourceType::Image) : (pResourceDesc->type == ProgramReflection::Resource::ResourceType::Texture);

        Texture::Type texDim = pTexture->getType();
        bool isArray = pTexture->getArraySize() > 1;

        // Check if the dimensions match
        switch(pResourceDesc->dims)
        {
        case ProgramReflection::Resource::Dimensions::Texture1D:
            dimsMatch = (texDim == Texture::Type::Texture1D) && (isArray == false);
            break;
        case ProgramReflection::Resource::Dimensions::Texture2D:
            dimsMatch = (texDim == Texture::Type::Texture2D) && (isArray == false);
            break;
        case ProgramReflection::Resource::Dimensions::Texture3D:
            dimsMatch = (texDim == Texture::Type::Texture3D) && (isArray == false);
            break;
        case ProgramReflection::Resource::Dimensions::TextureCube:
            dimsMatch = (texDim == Texture::Type::TextureCube) && (isArray == false);
            break;
        case ProgramReflection::Resource::Dimensions::Texture1DArray:
            dimsMatch = (texDim == Texture::Type::Texture1D) && isArray;
            break;
        case ProgramReflection::Resource::Dimensions::Texture2DArray:
            dimsMatch = (texDim == Texture::Type::Texture2D);
            break;
        case ProgramReflection::Resource::Dimensions::Texture2DMS:
            dimsMatch = (texDim == Texture::Type::Texture2DMultisample) && (isArray == false);
            break;
        case ProgramReflection::Resource::Dimensions::Texture2DMSArray:
            dimsMatch = (texDim == Texture::Type::Texture2DMultisample);
            break;
        case ProgramReflection::Resource::Dimensions::TextureCubeArray:
            dimsMatch = (texDim == Texture::Type::TextureCube) && isArray;
            break;
        case ProgramReflection::Resource::Dimensions::TextureBuffer:
            break;
        default:
            should_not_get_here();
        }

        // Check if the resource Type match
        FormatType texFormatType = getFormatType(pTexture->getFormat());

        switch(pResourceDesc->retType)
        {
        case ProgramReflection::Resource::ReturnType::Float:
            formatMatch = (texFormatType == FormatType::Float) || (texFormatType == FormatType::Snorm) || (texFormatType == FormatType::Unorm) || (texFormatType == FormatType::UnormSrgb);
            break;
        case ProgramReflection::Resource::ReturnType::Int:
            formatMatch = (texFormatType == FormatType::Sint);
            break;
        case ProgramReflection::Resource::ReturnType::Uint:
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
                msg += "Dimensions mismatch.\nTexture has Type " + to_string(texDim) + (isArray ? "Array" : "") + ".\nUniform has Type " + to_string(pResourceDesc->dims) + ".\n";
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
                msg += "Format mismatch.\nTexture has format Type " + to_string(texFormatType) + ".\nUniform has Type " + to_string(pResourceDesc->retType) + ".\n";
            }

            msg += "\nError when setting resource to buffer " + bufferName;
            Logger::log(Logger::Level::Error, msg);
            return false;
        }
#endif
        return true;
    }

    const ProgramReflection::Resource* getResourceDesc(const std::string& name, const ProgramReflection::BufferReflection* pReflector)
    {
        auto pResource = pReflector->getResourceData(name);
#ifdef FALCOR_D3D11
        // If it's not found and this is DX, search for out internal struct
        if(pResource = nullptr)
        {
            pResource = pReflector->getResourceData(name + ".t");
        }
#endif
        return pResource;
    }

    void UniformBuffer::setTexture(size_t offset, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage)
    {
        bool bOK = true;
#if _LOG_ENABLED
        // Debug checks
        if(pTexture)
        {
            for(auto& a = mpReflector->varBegin() ; a != mpReflector->varEnd() ; a++)
            {
                const auto& varDesc = a->second;
                const auto& varName = a->first;
                if(varDesc.type == ProgramReflection::Variable::Type::Resource)
                {
                    size_t ArrayIndex = 0;
                    bool bCheck = (varDesc.location == offset);

                    // Check arrays
                    if(varDesc.arrayStride > 0 && offset > varDesc.location)
                    {
                        size_t Stride = offset - varDesc.location;
                        if((Stride % varDesc.arrayStride) == 0)
                        {
                            ArrayIndex = Stride / varDesc.arrayStride;
                            if(ArrayIndex < varDesc.arraySize)
                            {
                                bCheck = true;
                            }
                        }
                    }

                    if(bCheck)
                    {
                        const auto& pResource = getResourceDesc(varName, mpReflector.get());
                        assert(pResource != nullptr);                
                        if(pResource->type != ProgramReflection::Resource::ResourceType::Sampler)
                        {
                            bOK = checkResourceDimension(pTexture, pResource, bindAsImage, varName, mpReflector->getName());
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
        size_t offset;
        const auto& pVarDesc = mpReflector->getVariableData(name, offset, false);
        if(offset != ProgramReflection::kInvalidLocation)
        {
            bool bOK = true;
#if _LOG_ENABLED == 1
            if(pTexture != nullptr)
            {
                const auto& pDesc = getResourceDesc(name, mpReflector.get());
                bOK = (pDesc != nullptr) && checkResourceDimension(pTexture, pDesc, bindAsImage, name, mpReflector->getName());
            }
#endif
            if(bOK)
            {
                setTexture(offset, pTexture, pSampler, bindAsImage);
            }
        }
    }

    void UniformBuffer::setTextureArray(const std::string& name, const Texture* pTexture[], const Sampler* pSampler, size_t count, bool bindAsImage)
    {
        size_t offset;
        const auto& pVarDesc = mpReflector->getVariableData(name, offset, true);

        if(pVarDesc)
        {
            if(pVarDesc->arraySize < count)
            {
                Logger::log(Logger::Level::Warning, "Error when setting textures array. 'count' is larger than the array size. Ignoring out-of-bound elements");
                count = pVarDesc->arraySize;
            }

            for(uint32_t i = 0; i < count; i++)
            {
                bool bOK = true;
#if _LOG_ENABLED == 1
                if(pTexture[i] != nullptr)
                {
                    const auto& pDesc = getResourceDesc(name, mpReflector.get());
                    bOK = (pDesc) && checkResourceDimension(pTexture[i], pDesc, bindAsImage, name, mpReflector->getName());
                }
#endif
                if(bOK)
                {
                    setTexture(offset + i * sizeof(uint64_t), pTexture[i], pSampler, bindAsImage);
                }
            }
        }
    }
}