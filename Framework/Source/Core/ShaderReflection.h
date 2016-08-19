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

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */
    namespace ShaderReflection
    {
        struct BufferDesc
        {
            enum class Type
            {
                Uniform,
                ShaderStorage
            };
            size_t sizeInBytes   = 0;
            size_t variableCount = 0;
            uint32_t bufferSlot  = 0;
            Type type;
        };

        using BufferDescMap = std::unordered_map<std::string, BufferDesc>;

        struct VariableDesc
        {
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
                Float4x4,
                GpuPtr,
                Resource
            };
            size_t offset        = 0;
            uint32_t arraySize   = 0;             // 0 if not an array
            uint32_t arrayStride = 0;               // 0 If not an array
            bool isRowMajor      = false;
            Type type            = Type::Unknown;
        };

        using VariableDescMap = std::unordered_map<std::string, VariableDesc>;

        inline const std::string to_string(VariableDesc::Type type)
        {
#define type_2_string(a) case ShaderReflection::VariableDesc::Type::a: return #a;
            switch(type)
            {
            type_2_string(Unknown);
            type_2_string(Bool);
            type_2_string(Bool2);
            type_2_string(Bool3);
            type_2_string(Bool4);
            type_2_string(Uint);
            type_2_string(Uint2);
            type_2_string(Uint3);
            type_2_string(Uint4);
            type_2_string(Int);
            type_2_string(Int2);
            type_2_string(Int3);
            type_2_string(Int4);
            type_2_string(Float);
            type_2_string(Float2);
            type_2_string(Float3);
            type_2_string(Float4);
            type_2_string(Float2x2);
            type_2_string(Float2x3);
            type_2_string(Float2x4);
            type_2_string(Float3x2);
            type_2_string(Float3x3);
            type_2_string(Float3x4);
            type_2_string(Float4x2);
            type_2_string(Float4x3);
            type_2_string(Float4x4);
            type_2_string(GpuPtr);
            default:
                should_not_get_here();
                return "";
            }
#undef type_2_string
        }

        struct ShaderResourceDesc
        {
            enum class ReturnType
            {
                Unknown,
                Float,
                Double,
                Int,
                Uint
            };

            enum class Dimensions
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
                TextureBuffer,
            };

            enum class ResourceType
            {
                Unknown,
                Texture,        // Read-only
                Image,          // Read-write
                Sampler         // Sampler state
            };

            Dimensions dims   = Dimensions::Unknown;
            ReturnType retType = ReturnType::Unknown;
            ResourceType type = ResourceType::Unknown;
            size_t offset = -1;
            uint32_t arraySize = 0;
            ShaderResourceDesc(Dimensions d, ReturnType r, ResourceType t) : dims(d), retType(r), type(t) {}
            ShaderResourceDesc() {}
        };

        using ShaderResourceDescMap = std::map < std::string, ShaderResourceDesc > ;
    }

    inline const std::string to_string(ShaderReflection::ShaderResourceDesc::ResourceType access)
    {
#define type_2_string(a) case ShaderReflection::ShaderResourceDesc::ResourceType::a: return #a;
        switch(access)
        {
            type_2_string(Unknown);
            type_2_string(Texture);
            type_2_string(Image);
        default:
            should_not_get_here();
            return "";
        }
#undef type_2_string
    }

    inline const std::string to_string(ShaderReflection::ShaderResourceDesc::ReturnType retType)
    {
#define type_2_string(a) case ShaderReflection::ShaderResourceDesc::ReturnType::a: return #a;
        switch(retType)
        {
            type_2_string(Unknown);
            type_2_string(Float);
            type_2_string(Uint);
            type_2_string(Int);
        default:
            should_not_get_here();
            return "";
        }
#undef type_2_string
    }

    inline const std::string to_string(ShaderReflection::ShaderResourceDesc::Dimensions resource)
    {
#define type_2_string(a) case ShaderReflection::ShaderResourceDesc::Dimensions::a: return #a;
        switch(resource)
        {
            type_2_string(Unknown);
            type_2_string(Texture1D);
            type_2_string(Texture2D);
            type_2_string(Texture3D);
            type_2_string(TextureCube);
            type_2_string(Texture1DArray);
            type_2_string(Texture2DArray);
            type_2_string(Texture2DMS);
            type_2_string(Texture2DMSArray);
            type_2_string(TextureCubeArray);
            type_2_string(TextureBuffer);
        default:
            should_not_get_here();
            return "";
        }
#undef type_2_string
    }

    /*! @} */
}
