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
#include <unordered_map>

namespace Falcor {namespace Reflection
{
    class BaseType
    {
    public:
        virtual ~BaseType() = 0;
        using SharedPtr = std::shared_ptr<BaseType>;
        template<typename T> 
        const T* as() const { return dynamic_cast<const T*>(this); }
    protected:        
    };

    class NumericType : public BaseType
    {
    public:
        using SharedPtr = std::shared_ptr<NumericType>;
        virtual ~NumericType() = default;

        enum class Type
        {
            Unknown,
            Bool,
            Bool2,
            Bool3,
            Bool4,
            Uint,
            Uint2,
            Uint3,
            Uint4,
            Uint64,
            Uint64_2,
            Uint64_3,
            Uint64_4,
            Int,
            Int2,
            Int3,
            Int4,
            Int64,
            Int64_2,
            Int64_3,
            Int64_4,
            Float,
            Float2,
            Float3,
            Float4,
            Float2x2,
            Float2x3,
            Float2x4,
            Float3x2,
            Float3x3,
            Float3x4,
            Float4x2,
            Float4x3,
            Float4x4
        };

        static SharedPtr create(Type t) { return SharedPtr(new NumericType(t)); }
        Type getType() const { return mType; }
    private:
        NumericType(Type t) : mType(t) {}
        Type mType;
    };

    class StructType : public BaseType
    {
    public:
        using SharedPtr = std::shared_ptr<StructType>;
        virtual ~StructType() = default;

          static SharedPtr create(const std::string& name, uint32_t size) { return SharedPtr(new StructType(name, size)); }

        void addField(const std::string& name, const BaseType::SharedPtr& pType, uint32_t offset);

        const uint32_t getFieldOffset(const std::string& name) { return mFields.at(name).offset; }
        const BaseType* getFieldType(const std::string& name) { return mFields.at(name).pType.get(); }
    private:
        StructType(const std::string& name, uint32_t size) : mName(name), mSize(size) {}
        std::string mName;
        uint32_t mSize;
        struct Field
        {
            Field(const BaseType::SharedPtr& pT, uint32_t o) : pType(pT), offset(o) {} 
            BaseType::SharedPtr pType;
            uint32_t offset;
        };
        std::unordered_map<std::string, Field> mFields;
    };

    class ArrayType : public BaseType
    {
    public:
        using SharedPtr = std::shared_ptr<ArrayType>;
        ~ArrayType() = default;

        static SharedPtr create(uint32_t elementCount, uint32_t stride, const BaseType::SharedPtr& pType)
            { return SharedPtr(new ArrayType(pType, elementCount, stride)); }

        uint32_t getElementCount() const { return mElementCount; }
        uint32_t getStride() const { return mStride; }

        const BaseType* getType() const { return mpType.get(); }
    private:
        ArrayType(const BaseType::SharedPtr& pType, uint32_t elementCount, uint32_t stride) : mpType(pType), mElementCount(elementCount), mStride(stride) {}
        uint32_t mElementCount;
        uint32_t mStride;
        BaseType::SharedPtr mpType;
    };

    class ResourceType
    {
    public:
        using SharedPtr = std::shared_ptr<ResourceType>;
        ~ResourceType() = default;

        enum class Type
        {
            Unknown,
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
            Texture1DArray,
            Texture2DArray,
            Texture2DMS,
            Texture2DMSArray,
            TextureCubeArray,
            ConstantBuffer,
            StructuredBuffer,
            TypedBuffer,
            RawBuffer,
            Sampler
        };

        enum class ShaderAccess
        {
            Undefined,
            Read,
            ReadWrite
        };

        static SharedPtr create(Type type, ShaderAccess access, const BaseType::SharedPtr& pBaseType)
            { return SharedPtr(new ResourceType(type, acess, pBaseType)); }

        Type getType() const { return mType; }
        ShaderAccess getShaderAccess() const { return mShaderAccess; }
        const BaseType::SharedPtr& getBaseType() const { return mpBaseType; }
    private:
        ResourceType(Type type, ShaderAccess access, const BaseType::SharedPtr& pBaseType) : mType(type), mShaderAccess(access), mpBaseType(pBaseType) {}
        Type mType;
        ShaderAccess mShaderAccess;
        BaseType::SharedPtr mpBaseType;
    };

    class Variable
    {
    public:
        Variable(uint32_t offset, const BaseType::SharedPtr& pType) : mOffset(offset), mpType(pType) {}
        Variable operator[](const std::string& field) const;
        Variable operator[](uint32_t arrayIndex) const;
    private:
        uint32_t mSetIndex;
        uint32_t mBindLocation;
        uint32_t mArrayIndex;
        BaseType::SharedPtr mpType;
    };

    class Reflector : public std::enable_shared_from_this<Reflector>
    {
    public:
        using SharedPtr = std::shared_ptr<Reflector>;
        static SharedPtr create();

    private:
        Reflector() = default;
    };
}}