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
#ifdef FALCOR_GL
#include "Framework.h"
#include "../ProgramReflection.h"
#include "../ProgramVersion.h"
#include "Utils/StringUtils.h"

namespace Falcor
{
    static GLenum getVariableEnum(GLenum bufferType)
    {
        switch(bufferType)
        {
        case GL_UNIFORM_BLOCK:
            return GL_UNIFORM;
        case GL_SHADER_STORAGE_BLOCK:
            return GL_BUFFER_VARIABLE;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    static ProgramReflection::Variable::Type getFalcorVariableType(GLenum glType)
    {
        switch(glType)
        {
        case GL_BOOL:
            return ProgramReflection::Variable::Type::Bool;
        case GL_BOOL_VEC2:
            return ProgramReflection::Variable::Type::Bool2;
        case GL_BOOL_VEC3:
            return ProgramReflection::Variable::Type::Bool3;
        case GL_BOOL_VEC4:
            return ProgramReflection::Variable::Type::Bool4;
        case GL_UNSIGNED_INT:
            return ProgramReflection::Variable::Type::Uint;
        case GL_UNSIGNED_INT_VEC2:
            return ProgramReflection::Variable::Type::Uint2;
        case GL_UNSIGNED_INT_VEC3:
            return ProgramReflection::Variable::Type::Uint3;
        case GL_UNSIGNED_INT_VEC4:
            return ProgramReflection::Variable::Type::Uint4;
        case GL_INT:
            return ProgramReflection::Variable::Type::Int;
        case GL_INT_VEC2:
            return ProgramReflection::Variable::Type::Int2;
        case GL_INT_VEC3:
            return ProgramReflection::Variable::Type::Int3;
        case GL_INT_VEC4:
            return ProgramReflection::Variable::Type::Int4;
        case GL_FLOAT:
            return ProgramReflection::Variable::Type::Float;
        case GL_FLOAT_VEC2:
            return ProgramReflection::Variable::Type::Float2;
        case GL_FLOAT_VEC3:
            return ProgramReflection::Variable::Type::Float3;
        case GL_FLOAT_VEC4:
            return ProgramReflection::Variable::Type::Float4;
        case GL_FLOAT_MAT2:
            return ProgramReflection::Variable::Type::Float2x2;
        case GL_FLOAT_MAT2x3:
            return ProgramReflection::Variable::Type::Float2x3;
        case GL_FLOAT_MAT2x4:
            return ProgramReflection::Variable::Type::Float2x4;
        case GL_FLOAT_MAT3:
            return ProgramReflection::Variable::Type::Float3x3;
        case GL_FLOAT_MAT3x2:
            return ProgramReflection::Variable::Type::Float3x2;
        case GL_FLOAT_MAT3x4:
            return ProgramReflection::Variable::Type::Float3x4;
        case GL_FLOAT_MAT4:
            return ProgramReflection::Variable::Type::Float4x4;
        case GL_FLOAT_MAT4x2:
            return ProgramReflection::Variable::Type::Float4x2;
        case GL_FLOAT_MAT4x3:
            return ProgramReflection::Variable::Type::Float4x3;
        case GL_GPU_ADDRESS_NV:
            return ProgramReflection::Variable::Type::GpuPtr;
        case GL_INT64_NV:
            return ProgramReflection::Variable::Type::Int64;
        case GL_INT64_VEC2_NV:
            return ProgramReflection::Variable::Type::Int64_2;
        case GL_INT64_VEC3_NV:
            return ProgramReflection::Variable::Type::Int64_3;
        case GL_INT64_VEC4_NV:
            return ProgramReflection::Variable::Type::Int64_4;
        case GL_UNSIGNED_INT64_NV:
            return ProgramReflection::Variable::Type::Uint64;
        case GL_UNSIGNED_INT64_VEC2_NV:
            return ProgramReflection::Variable::Type::Uint64_2;
        case GL_UNSIGNED_INT64_VEC3_NV:
            return ProgramReflection::Variable::Type::Uint64_3;
        case GL_UNSIGNED_INT64_VEC4_NV:
            return ProgramReflection::Variable::Type::Uint64_4;
            /*case GL_UNSIGNED_INT8_NV:       return SProgramReflection::Variable::Type::; //TODO: add
            /*case GL_INT8_NV:                return SProgramReflection::Variable::Type::; //TODO: add
            case GL_INT8_VEC2_NV:           return SProgramReflection::Variable::Type::;
            case GL_INT8_VEC3_NV:           return SProgramReflection::Variable::Type::;
            case GL_INT8_VEC4_NV:           return SProgramReflection::Variable::Type::;
            case GL_INT16_NV:               return SProgramReflection::Variable::Type::;
            case GL_INT16_VEC2_NV:          return SProgramReflection::Variable::Type::;
            case GL_INT16_VEC3_NV:          return SProgramReflection::Variable::Type::;
            case GL_INT16_VEC4_NV:          return SProgramReflection::Variable::Type::;*/
            /*case GL_FLOAT16_NV:             return SProgramReflection::Variable::Type::; //TODO: add
            case GL_FLOAT16_VEC2_NV:        return SProgramReflection::Variable::Type::;
            case GL_FLOAT16_VEC3_NV:        return SProgramReflection::Variable::Type::;
            case GL_FLOAT16_VEC4_NV:        return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT8_VEC2_NV:  return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT8_VEC3_NV:  return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT8_VEC4_NV:  return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT16_NV:      return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT16_VEC2_NV: return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT16_VEC3_NV: return SProgramReflection::Variable::Type::;
            case GL_UNSIGNED_INT16_VEC4_NV: return SProgramReflection::Variable::Type::;*/
        default:
            return ProgramReflection::Variable::Type::Unknown;
        }
    }

    static const std::map<GLenum, ProgramReflection::Resource> kResourceDescMap =
    {
        {GL_SAMPLER_1D,                             {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_1D_SHADOW,                      {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_1D,                         {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_1D,                {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D,                             {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_SHADOW,                      {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_2D,                         {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_2D,                {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_3D,                             {ProgramReflection::Resource::Dimensions::Texture3D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_3D,                         {ProgramReflection::Resource::Dimensions::Texture3D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_3D,                {ProgramReflection::Resource::Dimensions::Texture3D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_CUBE,                           {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_CUBE_SHADOW,                    {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_CUBE,                       {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_CUBE,              {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_1D_ARRAY,                       {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_1D_ARRAY_SHADOW,                {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_1D_ARRAY,                   {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,          {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_ARRAY,                       {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_ARRAY_SHADOW,                {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_2D_ARRAY,                   {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,          {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_CUBE_MAP_ARRAY,                 {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW,          {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_CUBE_MAP_ARRAY,             {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY,    {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_MULTISAMPLE,                 {ProgramReflection::Resource::Dimensions::Texture2DMS,            ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_2D_MULTISAMPLE,             {ProgramReflection::Resource::Dimensions::Texture2DMS,            ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,    {ProgramReflection::Resource::Dimensions::Texture2DMS,            ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_MULTISAMPLE_ARRAY,           {ProgramReflection::Resource::Dimensions::Texture2DMSArray,       ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,       {ProgramReflection::Resource::Dimensions::Texture2DMSArray,       ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, {ProgramReflection::Resource::Dimensions::Texture2DMSArray,    ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_BUFFER,                         {ProgramReflection::Resource::Dimensions::TextureBuffer,          ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_BUFFER,                     {ProgramReflection::Resource::Dimensions::TextureBuffer,          ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_BUFFER,            {ProgramReflection::Resource::Dimensions::TextureBuffer,          ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_RECT,                        {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_SAMPLER_2D_RECT_SHADOW,                 {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Texture}},
        {GL_INT_SAMPLER_2D_RECT,                    {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Texture}},
        {GL_UNSIGNED_INT_SAMPLER_2D_RECT,           {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Texture}},
            
        {GL_IMAGE_1D,                               {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_1D,                           {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_1D,                  {ProgramReflection::Resource::Dimensions::Texture1D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_2D,                               {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_2D,                           {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_2D,                  {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_3D,                               {ProgramReflection::Resource::Dimensions::Texture3D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_3D,                           {ProgramReflection::Resource::Dimensions::Texture3D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_3D,                  {ProgramReflection::Resource::Dimensions::Texture3D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_2D_RECT,                          {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_2D_RECT,                      {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_2D_RECT,             {ProgramReflection::Resource::Dimensions::Texture2D,              ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_CUBE,                             {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_CUBE,                         {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_CUBE,                {ProgramReflection::Resource::Dimensions::TextureCube,            ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_BUFFER,                           {ProgramReflection::Resource::Dimensions::TextureBuffer,          ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_BUFFER,                       {ProgramReflection::Resource::Dimensions::TextureBuffer,          ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_BUFFER,              {ProgramReflection::Resource::Dimensions::TextureBuffer,          ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_1D_ARRAY,                         {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_1D_ARRAY,                     {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_1D_ARRAY,            {ProgramReflection::Resource::Dimensions::Texture1DArray,         ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_2D_ARRAY,                         {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_2D_ARRAY,                     {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_2D_ARRAY,            {ProgramReflection::Resource::Dimensions::Texture2DArray,         ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_2D_MULTISAMPLE,                   {ProgramReflection::Resource::Dimensions::Texture2DMS,            ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_2D_MULTISAMPLE,               {ProgramReflection::Resource::Dimensions::Texture2DMS,            ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE,      {ProgramReflection::Resource::Dimensions::Texture2DMS,            ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_2D_MULTISAMPLE_ARRAY,             {ProgramReflection::Resource::Dimensions::Texture2DMSArray,       ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY,         {ProgramReflection::Resource::Dimensions::Texture2DMSArray,       ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, {ProgramReflection::Resource::Dimensions::Texture2DMSArray,      ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
        {GL_IMAGE_CUBE_MAP_ARRAY,                   {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Float, ProgramReflection::Resource::ResourceType::Image}},
        {GL_INT_IMAGE_CUBE_MAP_ARRAY,               {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Int,   ProgramReflection::Resource::ResourceType::Image}},
        {GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY,      {ProgramReflection::Resource::Dimensions::TextureCubeArray,       ProgramReflection::Resource::ReturnType::Uint,  ProgramReflection::Resource::ResourceType::Image}},
    };

    static const ProgramReflection::Resource& getResourceDesc(GLenum glType)
    {
        static const ProgramReflection::Resource empty;
        const auto& it = kResourceDescMap.find(glType);
        if(it == kResourceDescMap.end())
        {
            return empty;
        }
        return it->second;
    }

    static bool reflectBufferVariables(uint32_t programID, uint32_t blockIndex, const std::string& bufferName, GLenum bufferType, size_t bufferSize, ProgramReflection::VariableMap& variableMap, ProgramReflection::ResourceMap& resourceMap)
    {
        // Get the names and offsets of the fields
        int32_t count;
        GLenum varCountEnum = GL_NUM_ACTIVE_VARIABLES;
        gl_call(glGetProgramResourceiv(programID, bufferType, blockIndex, 1, &varCountEnum, 1, nullptr, &count));
        std::vector<int32_t> indices(count);
        GLenum varEnum = GL_ACTIVE_VARIABLES;
        gl_call(glGetProgramResourceiv(programID, bufferType, blockIndex, 1, &varEnum, count, nullptr, indices.data()));

        for(int32_t i = 0; i < count; i++)
        {
            GLenum interfaceEnum = getVariableEnum(bufferType);

            GLenum uniformDataEnum[] = {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET, GL_ARRAY_SIZE, GL_ARRAY_STRIDE};
            int32_t values[arraysize(uniformDataEnum)];
            gl_call(glGetProgramResourceiv(programID, interfaceEnum, indices[i], arraysize(uniformDataEnum), uniformDataEnum, arraysize(uniformDataEnum), nullptr, (int32_t*)&values));

            ProgramReflection::Variable desc;
            desc.type = getFalcorVariableType(values[1]);
            desc.offset = values[2];
            desc.arrayStride = values[4];
            desc.arraySize = 0; // OpenGL reports array size of 1 for non-array variables, so set it to zero
            if(desc.arrayStride)
            {
                // For arrays with unspecified array size (Like 'SomeArray[]'), OpenGL reports array-size of zero.
                // There can only be a single array like this for each buffer, it must be the last element in the buffer and the array uses the rest of the buffer space.
                desc.arraySize = values[3] ? values[3] : ((uint32_t)(bufferSize - desc.offset) / desc.arrayStride);
            }

            std::vector<char> nameStorage(values[0]);

            // Get the uniform name
            gl_call(glGetProgramResourceName(programID, interfaceEnum, indices[i], values[0], nullptr, nameStorage.data()));
            std::string uniformName(nameStorage.data());
            if(uniformName.find(bufferName + ".") == 0)
            {
                uniformName.erase(0, bufferName.length() + 1);
            }

            if(desc.arrayStride > 0 && hasSuffix(uniformName, "[0]"))
            {
                // Some implementations report the name of the variable with [0]. We remove it.
                uniformName = uniformName.erase(uniformName.length() - 3);
            }

            // If the var-Type is unknown, this is a resource
            if(desc.type == ProgramReflection::Variable::Type::Unknown)
            {
                resourceMap[uniformName] = getResourceDesc(values[1]);
                assert(resourceMap[uniformName].type != ProgramReflection::Resource::ResourceType::Unknown);
                resourceMap[uniformName].arraySize = desc.arraySize;
                resourceMap[uniformName].offset = desc.offset;
                desc.type = ProgramReflection::Variable::Type::Resource;
            }
            variableMap[uniformName] = desc;
        }
        return true;
    }

    static bool reflectBuffersByType(uint32_t programID, GLenum bufferType, ProgramReflection::BufferMap& descMap, ProgramReflection::string_2_uint_map& bufferNameMap, std::string& log)
    {
        GLenum maxBlockType = GL_NONE;
        ProgramReflection::BufferDesc::Type falcorType;
        switch(bufferType)
        {
        case GL_UNIFORM_BLOCK:
            maxBlockType = GL_MAX_COMBINED_UNIFORM_BLOCKS;
            falcorType = ProgramReflection::BufferDesc::Type::Uniform;
            break;
        case GL_SHADER_STORAGE_BLOCK:
            maxBlockType = GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS;
            falcorType = ProgramReflection::BufferDesc::Type::ShaderStorage;
            break;
        default:
            should_not_get_here();
            return false;
        }

        // Find all uniform buffers
        int32_t numBlocks;
        gl_call(glGetProgramInterfaceiv(programID, bufferType, GL_ACTIVE_RESOURCES, &numBlocks));

        // Get the max length name and allocate space for it
        int32_t maxLength;
        gl_call(glGetProgramInterfaceiv(programID, bufferType, GL_MAX_NAME_LENGTH, &maxLength));
        std::vector<char> chars(maxLength + 1);

        int uniformBlockCount;
        gl_call(glGetIntegerv(maxBlockType, &uniformBlockCount));

        for(int i = 0; i < numBlocks; i++)
        {
            // Get the block name
            gl_call(glGetProgramResourceName(programID, bufferType, i, maxLength, nullptr, &chars[0]));
            std::string name(&chars[0]);

            // Get the index
            uint32_t blockIndex = gl_call(glGetProgramResourceIndex(programID, bufferType, name.c_str()));
            if(blockIndex >= (uint32_t)uniformBlockCount)
            {
                log = "Uniform buffer \"" + name + "\" - location is larger than available HW slots. Falcor can't function correctly";
                return false;
            }

            // Get the block size, bind location and variable count
            GLenum props[] = {GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES};
            uint32_t values[arraysize(props)];
            gl_call(glGetProgramResourceiv(programID, bufferType, blockIndex, arraysize(props), props, arraysize(props), nullptr, (int32_t*)values));

            // Check that the binding index is unique
            if(descMap.find(values[0]) != descMap.end())
            {
                log = "Uniform buffer \"" + name + "\" - binding index already used in program. Make sure you use explicit bindings - 'layout (binding = n)' - in your uniform buffer declarations.";
                return false;
            }
            
            // Reflect the buffer
            ProgramReflection::VariableMap varMap;
            ProgramReflection::ResourceMap resourceMap;
            reflectBufferVariables(programID, blockIndex, name, bufferType, values[1], varMap, resourceMap);

            // Initialize the maps
            descMap[values[0]] = ProgramReflection::BufferDesc::create(name, falcorType, values[1], values[2], varMap, resourceMap);
            bufferNameMap[name] = values[0];
        }
        return true;
    }

    bool ProgramReflection::reflectBuffers(const ProgramVersion* pProgVer, std::string& log)
    {        
        GLenum bufferTypes[] = {GL_UNIFORM_BLOCK, GL_SHADER_STORAGE_BLOCK};
        bool bOK = true;
        for(auto t : bufferTypes)
        {
            bOK = reflectBuffersByType(pProgVer->getApiHandle(), t, mUniformBuffers.descMap, mUniformBuffers.nameMap, log);
        }
        return true;
    }
}
#endif