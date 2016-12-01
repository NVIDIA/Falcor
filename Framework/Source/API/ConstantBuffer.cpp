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
#include "ConstantBuffer.h"
#include "ProgramVersion.h"
#include "buffer.h"
#include "glm/glm.hpp"
#include "texture.h"
#include "API/ProgramReflection.h"
#include "API/Device.h"

namespace Falcor
{
    ConstantBuffer::SharedPtr ConstantBuffer::create(const ProgramReflection::BufferReflection::SharedConstPtr& pReflector, size_t overrideSize)
    {
        SharedPtr pBuffer = SharedPtr(new ConstantBuffer(pReflector));
        if(pBuffer->init(overrideSize, true) == false)
        {
            pBuffer = nullptr;
        }

        return pBuffer;
    }

    ConstantBuffer::SharedPtr ConstantBuffer::create(Program::SharedPtr& pProgram, const std::string& name, size_t overrideSize)
    {
        auto& pProgReflector = pProgram->getActiveVersion()->getReflector();
        auto& pBufferReflector = pProgReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Constant);
        if(pBufferReflector)
        {
            return create(pBufferReflector, overrideSize);
        }
        return nullptr;
    }

    bool ConstantBuffer::init(size_t overrideSize, bool isConstantBuffer)
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
            mpBuffer = Buffer::create(mSize, Buffer::BindFlags::Constant, Buffer::CpuAccess::Write, mData.data());
        }
        
        return true;
    }
    
    size_t ConstantBuffer::getVariableOffset(const std::string& varName) const
    {
        size_t offset;
        mpReflector->getVariableData(varName, offset, true);
        return offset;
    }

    void ConstantBuffer::uploadToGPU(size_t offset, size_t size) const
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
            Logger::log(Logger::Level::Warning, "ConstantBuffer::uploadToGPU() - trying to upload more data than what the buffer contains. Call is ignored.");
            return;
        }

        uint8_t* pData = (uint8_t*)mpBuffer->map(Buffer::MapType::WriteDiscard);
        assert(pData);
        memcpy(pData + offset, mData.data() + offset, size);
        mpBuffer->unmap();
        mDirty = false;

        // Invalidate the view
        mResourceView = nullptr;
    }

    bool checkVariableType(ProgramReflection::Variable::Type callType, ProgramReflection::Variable::Type shaderType, const std::string& name, const std::string& bufferName)
    {
#if _LOG_ENABLED
        // Check that the types match
        if(callType != shaderType)
        {
            std::string msg("Error when setting variable \"");
            msg += name + "\" to buffer \"" + bufferName + "\".\n";
            msg += "Type mismatch.\nsetVariable() was called with Type " + to_string(callType) + ".\nVariable was declared with Type " + to_string(shaderType) + ".\n\n";
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
        // Find the variable
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
                        std::string Msg("Error when setting constant by offset. Found constant \"" + varName + "\" which is not an array, but trying to set more than 1 element");
                        Logger::log(Logger::Level::Error, Msg);
                        return false;
                    }
                }
                else if(arrayIndex + count > varDesc.arraySize)
                {
                    std::string Msg("Error when setting constant by offset. Found constant \"" + varName + "\" with array size " + std::to_string(varDesc.arraySize));
                    Msg += ". Trying to set " + std::to_string(count) + " elements, starting at index " + std::to_string(arrayIndex) + ", which will cause out-of-bound access. Ignoring call.";
                    Logger::log(Logger::Level::Error, Msg);
                    return false;
                }
                return checkVariableType(callType, varDesc.type, varName + "(Set by offset)", pBufferDesc->getName());
            }
        }
        std::string msg("Error when setting constant by offset. No constant found at offset ");
        msg += std::to_string(offset) + ". Ignoring call";
        Logger::log(Logger::Level::Error, msg);
        return false;
#else
        return true;
#endif
    }

#define set_constant_by_offset(_var_type, _c_type) \
    template<> void ConstantBuffer::setVariable(size_t offset, const _c_type& value)    \
    {                                                           \
        if(checkVariableByOffset(ProgramReflection::Variable::Type::_var_type, offset, 1, mpReflector.get())) \
        {                                                       \
            const uint8_t* pVar = mData.data() + offset;        \
            *(_c_type*)pVar = value;                            \
            mDirty = true;                                      \
        }                                                       \
    }

    set_constant_by_offset(Bool, bool);
    set_constant_by_offset(Bool2, glm::bvec2);
    set_constant_by_offset(Bool3, glm::bvec3);
    set_constant_by_offset(Bool4, glm::bvec4);

    set_constant_by_offset(Uint, uint32_t);
    set_constant_by_offset(Uint2, glm::uvec2);
    set_constant_by_offset(Uint3, glm::uvec3);
    set_constant_by_offset(Uint4, glm::uvec4);

    set_constant_by_offset(Int, int32_t);
    set_constant_by_offset(Int2, glm::ivec2);
    set_constant_by_offset(Int3, glm::ivec3);
    set_constant_by_offset(Int4, glm::ivec4);

    set_constant_by_offset(Float, float);
    set_constant_by_offset(Float2, glm::vec2);
    set_constant_by_offset(Float3, glm::vec3);
    set_constant_by_offset(Float4, glm::vec4);

    set_constant_by_offset(Float2x2, glm::mat2);
    set_constant_by_offset(Float2x3, glm::mat2x3);
    set_constant_by_offset(Float2x4, glm::mat2x4);

    set_constant_by_offset(Float3x3, glm::mat3);
    set_constant_by_offset(Float3x2, glm::mat3x2);
    set_constant_by_offset(Float3x4, glm::mat3x4);

    set_constant_by_offset(Float4x4, glm::mat4);
    set_constant_by_offset(Float4x2, glm::mat4x2);
    set_constant_by_offset(Float4x3, glm::mat4x3);

    set_constant_by_offset(GpuPtr, uint64_t);

#undef set_constant_by_offset

#define set_constant_by_name(_var_type, _c_type) \
    template<> void ConstantBuffer::setVariable(const std::string& name, const _c_type& value)    \
    {                                                           \
        size_t offset;                                          \
        const auto* pVar = mpReflector->getVariableData(name, offset, false); \
        bool valid = true;                                      \
        if((_LOG_ENABLED == 0) || (offset != ProgramReflection::kInvalidLocation && checkVariableType(ProgramReflection::Variable::Type::_var_type, pVar->type, name, mpReflector->getName()))) \
        {                                                       \
            setVariable(offset, value);                         \
        }                                                       \
    }

    set_constant_by_name(Bool, bool);
    set_constant_by_name(Bool2, glm::bvec2);
    set_constant_by_name(Bool3, glm::bvec3);
    set_constant_by_name(Bool4, glm::bvec4);

    set_constant_by_name(Uint, uint32_t);
    set_constant_by_name(Uint2, glm::uvec2);
    set_constant_by_name(Uint3, glm::uvec3);
    set_constant_by_name(Uint4, glm::uvec4);

    set_constant_by_name(Int, int32_t);
    set_constant_by_name(Int2, glm::ivec2);
    set_constant_by_name(Int3, glm::ivec3);
    set_constant_by_name(Int4, glm::ivec4);

    set_constant_by_name(Float, float);
    set_constant_by_name(Float2, glm::vec2);
    set_constant_by_name(Float3, glm::vec3);
    set_constant_by_name(Float4, glm::vec4);

    set_constant_by_name(Float2x2, glm::mat2);
    set_constant_by_name(Float2x3, glm::mat2x3);
    set_constant_by_name(Float2x4, glm::mat2x4);

    set_constant_by_name(Float3x3, glm::mat3);
    set_constant_by_name(Float3x2, glm::mat3x2);
    set_constant_by_name(Float3x4, glm::mat3x4);

    set_constant_by_name(Float4x4, glm::mat4);
    set_constant_by_name(Float4x2, glm::mat4x2);
    set_constant_by_name(Float4x3, glm::mat4x3);

    set_constant_by_name(GpuPtr, uint64_t);
#undef set_constant_by_name

#define set_constant_array_by_offset(_var_type, _c_type) \
    template<> void ConstantBuffer::setVariableArray(size_t offset, const _c_type* pValue, size_t count)             \
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

    set_constant_array_by_offset(Bool, bool);
    set_constant_array_by_offset(Bool2, glm::bvec2);
    set_constant_array_by_offset(Bool3, glm::bvec3);
    set_constant_array_by_offset(Bool4, glm::bvec4);

    set_constant_array_by_offset(Uint, uint32_t);
    set_constant_array_by_offset(Uint2, glm::uvec2);
    set_constant_array_by_offset(Uint3, glm::uvec3);
    set_constant_array_by_offset(Uint4, glm::uvec4);

    set_constant_array_by_offset(Int, int32_t);
    set_constant_array_by_offset(Int2, glm::ivec2);
    set_constant_array_by_offset(Int3, glm::ivec3);
    set_constant_array_by_offset(Int4, glm::ivec4);

    set_constant_array_by_offset(Float, float);
    set_constant_array_by_offset(Float2, glm::vec2);
    set_constant_array_by_offset(Float3, glm::vec3);
    set_constant_array_by_offset(Float4, glm::vec4);

    set_constant_array_by_offset(Float2x2, glm::mat2);
    set_constant_array_by_offset(Float2x3, glm::mat2x3);
    set_constant_array_by_offset(Float2x4, glm::mat2x4);

    set_constant_array_by_offset(Float3x3, glm::mat3);
    set_constant_array_by_offset(Float3x2, glm::mat3x2);
    set_constant_array_by_offset(Float3x4, glm::mat3x4);

    set_constant_array_by_offset(Float4x4, glm::mat4);
    set_constant_array_by_offset(Float4x2, glm::mat4x2);
    set_constant_array_by_offset(Float4x3, glm::mat4x3);

    set_constant_array_by_offset(GpuPtr, uint64_t);

#undef set_constant_array_by_offset

#define set_constant_array_by_string(_var_type, _c_type) \
    template<>                                      \
    void ConstantBuffer::setVariableArray(const std::string& name, const _c_type* pValue, size_t count)           \
    {                                                                                                             \
        size_t offset;                                                                                            \
        const auto& pVarDesc = mpReflector->getVariableData(name, offset, true);                                  \
        if( _LOG_ENABLED == 0 || (offset != ProgramReflection::kInvalidLocation && checkVariableType(ProgramReflection::Variable::Type::_var_type, pVarDesc->type, name, mpReflector->getName()))) \
        {                                                                                                         \
            setVariableArray(offset, pValue, count);                                                              \
        }                                                                                                         \
    }
    
    set_constant_array_by_string(Bool, bool);
    set_constant_array_by_string(Bool2, glm::bvec2);
    set_constant_array_by_string(Bool3, glm::bvec3);
    set_constant_array_by_string(Bool4, glm::bvec4);

    set_constant_array_by_string(Uint, uint32_t);
    set_constant_array_by_string(Uint2, glm::uvec2);
    set_constant_array_by_string(Uint3, glm::uvec3);
    set_constant_array_by_string(Uint4, glm::uvec4);

    set_constant_array_by_string(Int, int32_t);
    set_constant_array_by_string(Int2, glm::ivec2);
    set_constant_array_by_string(Int3, glm::ivec3);
    set_constant_array_by_string(Int4, glm::ivec4);

    set_constant_array_by_string(Float, float);
    set_constant_array_by_string(Float2, glm::vec2);
    set_constant_array_by_string(Float3, glm::vec3);
    set_constant_array_by_string(Float4, glm::vec4);

    set_constant_array_by_string(Float2x2, glm::mat2);
    set_constant_array_by_string(Float2x3, glm::mat2x3);
    set_constant_array_by_string(Float2x4, glm::mat2x4);

    set_constant_array_by_string(Float3x3, glm::mat3);
    set_constant_array_by_string(Float3x2, glm::mat3x2);
    set_constant_array_by_string(Float3x4, glm::mat3x4);

    set_constant_array_by_string(Float4x4, glm::mat4);
    set_constant_array_by_string(Float4x2, glm::mat4x2);
    set_constant_array_by_string(Float4x3, glm::mat4x3);

    set_constant_array_by_string(GpuPtr, uint64_t);

#undef set_constant_array_by_string

    void ConstantBuffer::setBlob(const void* pSrc, size_t offset, size_t size)
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
        bool imageMatch = bindAsImage ? (pResourceDesc->type == ProgramReflection::Resource::ResourceType::TextureUav) : (pResourceDesc->type == ProgramReflection::Resource::ResourceType::TextureSrv);

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
                msg += "Dimensions mismatch.\nTexture has Type " + to_string(texDim) + (isArray ? "Array" : "") + ".\nVariable has Type " + to_string(pResourceDesc->dims) + ".\n";
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
                msg += "Format mismatch.\nTexture has format Type " + to_string(texFormatType) + ".\nVariable has Type " + to_string(pResourceDesc->retType) + ".\n";
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

    void ConstantBuffer::setTexture(size_t offset, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage)
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
                std::string msg("Error when setting texture by offset. No varialbe found at offset ");
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

    void ConstantBuffer::setTexture(const std::string& name, const Texture* pTexture, const Sampler* pSampler, bool bindAsImage)
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

    void ConstantBuffer::setTextureArray(const std::string& name, const Texture* pTexture[], const Sampler* pSampler, size_t count, bool bindAsImage)
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

    DescriptorHeap::Entry ConstantBuffer::getResourceView() const
    {
        if(mResourceView == nullptr)
        {
            DescriptorHeap* pHeap = gpDevice->getSrvDescriptorHeap().get();

            mResourceView = pHeap->allocateEntry();
            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
            viewDesc.BufferLocation = mpBuffer->getGpuAddress();
            viewDesc.SizeInBytes = (uint32_t)mpBuffer->getSize();
            gpDevice->getApiHandle()->CreateConstantBufferView(&viewDesc, mResourceView->getCpuHandle());
        }

        return mResourceView;
    }
}