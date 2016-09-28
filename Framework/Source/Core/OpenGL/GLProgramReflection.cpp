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
#error Shader usage flags
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

    static const ProgramReflection::Resource& getResourceDescFromGl(GLenum glType)
    {
        static const ProgramReflection::Resource empty;
        const auto& it = kResourceDescMap.find(glType);
        if(it == kResourceDescMap.end())
        {
            return empty;
        }
        return it->second;
    }

    static ProgramReflection::Variable createVarDesc(uint32_t programID, GLenum interfaceEnum, uint32_t index, std::string& varName, GLenum locationType, bool queryArrayStride)
    {
        GLenum uniformDataEnum[] = {GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, locationType};
        int32_t values[arraysize(uniformDataEnum)];
        gl_call(glGetProgramResourceiv(programID, interfaceEnum, index, arraysize(uniformDataEnum), uniformDataEnum, arraysize(uniformDataEnum), nullptr, (int32_t*)&values));

        ProgramReflection::Variable desc;
        desc.type = getFalcorVariableType(values[1]);
        desc.location = values[3];
        desc.arrayStride = 0;
        desc.arraySize = 0; // OpenGL reports array size of 1 for non-array variables, so set it to zero
        std::vector<char> nameStorage(values[0]);

        if(queryArrayStride)
        {
            GLenum strideEnum = GL_ARRAY_STRIDE;
            gl_call(glGetProgramResourceiv(programID, interfaceEnum, index, 1, &strideEnum, 1, nullptr, (int32_t*)&desc.arrayStride));
            if(desc.arrayStride)
            {
                desc.arraySize = values[2];
            }
        }

        // Get the variable name
        gl_call(glGetProgramResourceName(programID, interfaceEnum, index, values[0], nullptr, nameStorage.data()));
        varName = std::string(nameStorage.data());
        
        if(desc.arrayStride > 0 && hasSuffix(varName, "[0]"))
        {
            // Some implementations report the name of the variable with [0]. We remove it.
            varName = varName.erase(varName.length() - 3);
        }

        return desc;
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

            std::string varName;
            ProgramReflection::Variable varDesc = createVarDesc(programID, interfaceEnum, indices[i], varName, GL_OFFSET, true);

            // Remove the buffer name if it is part of the variable name
            if(varName.find(bufferName + ".") == 0)
            {
                varName.erase(0, bufferName.length() + 1);
            }

            if(varDesc.arrayStride && (varDesc.arrayStride == 0))
            {
                // For arrays with unspecified array size (Like 'SomeArray[]'), OpenGL reports array-size of zero.
                // There can only be a single array like this for each buffer, it must be the last element in the buffer and the array uses the rest of the buffer space.
                varDesc.arraySize = (uint32_t)(bufferSize - varDesc.location) / varDesc.arrayStride;
            }

            // If the var-Type is unknown, this is a resource
            if(varDesc.type == ProgramReflection::Variable::Type::Unknown)
            {
                GLenum uniformDataEnum = GL_TYPE;
                int32_t glType;
                gl_call(glGetProgramResourceiv(programID, interfaceEnum, indices[i], 1, &uniformDataEnum, 1, nullptr, (int32_t*)&glType));

                resourceMap[varName] = getResourceDescFromGl(glType);
                assert(resourceMap[varName].type != ProgramReflection::Resource::ResourceType::Unknown);
                resourceMap[varName].arraySize = varDesc.arraySize;
                resourceMap[varName].location = varDesc.location;
                varDesc.type = ProgramReflection::Variable::Type::Resource;
            }
            variableMap[varName] = varDesc;
        }
        return true;
    }

    static bool reflectBuffersByType(uint32_t programID, ProgramReflection::BufferReflection::Type type, ProgramReflection::BufferMap& descMap, ProgramReflection::string_2_uint_map& bufferNameMap, std::string& log)
    {
        GLenum maxBlockType = GL_NONE;
        GLenum glType = GL_NONE;
        switch(type)
        {
        case ProgramReflection::BufferReflection::Type::Uniform:
            maxBlockType = GL_MAX_COMBINED_UNIFORM_BLOCKS;
            glType = GL_UNIFORM_BLOCK;
            break;
        case ProgramReflection::BufferReflection::Type::ShaderStorage:
            maxBlockType = GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS;
            glType = GL_SHADER_STORAGE_BLOCK;
            break;
        default:
            should_not_get_here();
            return false;
        }

        // Find all uniform buffers
        int32_t numBlocks;
        gl_call(glGetProgramInterfaceiv(programID, glType, GL_ACTIVE_RESOURCES, &numBlocks));

        // Get the max length name and allocate space for it
        int32_t maxLength;
        gl_call(glGetProgramInterfaceiv(programID, glType, GL_MAX_NAME_LENGTH, &maxLength));
        std::vector<char> chars(maxLength + 1);

        int uniformBlockCount;
        gl_call(glGetIntegerv(maxBlockType, &uniformBlockCount));

        for(int i = 0; i < numBlocks; i++)
        {
            // Get the block name
            gl_call(glGetProgramResourceName(programID, glType, i, maxLength, nullptr, &chars[0]));
            std::string name(&chars[0]);

            // Get the index
            uint32_t blockIndex = gl_call(glGetProgramResourceIndex(programID, glType, name.c_str()));
            if(blockIndex >= (uint32_t)uniformBlockCount)
            {
                log = "Uniform buffer \"" + name + "\" - location is larger than available HW slots. Falcor can't function correctly";
                return false;
            }

            // Get the block size, bind location and variable count
            GLenum props[] = {GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES};
            uint32_t values[arraysize(props)];
            gl_call(glGetProgramResourceiv(programID, glType, blockIndex, arraysize(props), props, arraysize(props), nullptr, (int32_t*)values));

            // Check that the binding index is unique
            if(descMap.find(values[0]) != descMap.end())
            {
                log = "Uniform buffer \"" + name + "\" - binding index already used in program. Make sure you use explicit bindings - 'layout (binding = n)' - in your uniform buffer declarations.";
                return false;
            }
            
            // Reflect the buffer
            ProgramReflection::VariableMap varMap;
            ProgramReflection::ResourceMap resourceMap;
            reflectBufferVariables(programID, blockIndex, name, glType, values[1], varMap, resourceMap);

            // Initialize the maps
            descMap[values[0]] = ProgramReflection::BufferReflection::create(name, type, values[1], values[2], varMap, resourceMap);
            bufferNameMap[name] = values[0];
        }
        return true;
    }

    template<ShaderType shaderType, GLenum interfaceEnum>
    bool reflectShaderIO(const ProgramVersion* pProgVer, std::string& log, ProgramReflection::VariableMap& varMap)
    {
        if(pProgVer->getShader(shaderType) == nullptr)
        {
            // Nothing to do
            return true;
        }

        uint32_t programID = pProgVer->getApiHandle();

        // Get the number of outputs
        int32_t numVars;
        gl_call(glGetProgramInterfaceiv(programID, interfaceEnum, GL_ACTIVE_RESOURCES, &numVars));

        // Get the max length name and allocate space for it
        int32_t maxLength;
        gl_call(glGetProgramInterfaceiv(programID, interfaceEnum, GL_MAX_NAME_LENGTH, &maxLength));
        std::vector<char> chars(maxLength + 1);

        for(int i = 0; i < numVars; i++)
        {
            // Get the block name
            gl_call(glGetProgramResourceName(programID, interfaceEnum, i, maxLength, nullptr, &chars[0]));
            std::string name(&chars[0]);

            // Get the index
            uint32_t varIndex = gl_call(glGetProgramResourceIndex(programID, interfaceEnum, name.c_str()));

            // Get the data type and array size
            std::string varName;
            ProgramReflection::Variable varDesc = createVarDesc(programID, interfaceEnum, varIndex, varName, GL_LOCATION, false);
            varDesc.location = varIndex;
            varMap[varName] = varDesc;
        }

        return true;
    }

    bool ProgramReflection::reflectFragmentOutputs(const ProgramVersion* pProgVer, std::string& log)
    {
        return reflectShaderIO<ShaderType::Fragment, GL_PROGRAM_OUTPUT>(pProgVer, log, mFragOut);
    }

    bool ProgramReflection::reflectVertexAttributes(const ProgramVersion* pProgVer, std::string& log)
    {
        return reflectShaderIO<ShaderType::Vertex, GL_PROGRAM_INPUT>(pProgVer, log, mVertAttr);
    }

    bool ProgramReflection::reflectBuffers(const ProgramVersion* pProgVer, std::string& log)
    {        
        bool bOK = true;
        for(uint32_t i = 0 ; i < BufferReflection::kTypeCount ; i++)
        {
            bOK = reflectBuffersByType(pProgVer->getApiHandle(), (BufferReflection::Type)i, mBuffers[i].descMap, mBuffers[i].nameMap, log);
        }
        return bOK;
    }


    bool ProgramReflection::reflectResources(const ProgramVersion* pProgVer, std::string& log)
    {
        uint32_t programID = pProgVer->getApiHandle();
        // Get the number of uniforms.
        int32_t numUniforms = 0;
        gl_call(glGetProgramInterfaceiv(programID, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms));

        // Loop over all of the uniforms
        for(int i = 0; i < numUniforms; i++)
        {
            const GLenum props[] = {GL_BLOCK_INDEX, GL_TYPE, GL_NAME_LENGTH, GL_LOCATION, GL_ARRAY_STRIDE, GL_ARRAY_SIZE};
            uint32_t values[arraysize(props)];
            gl_call(glGetProgramResourceiv(programID, GL_UNIFORM, i, arraysize(props), props, arraysize(props), nullptr, (int32_t*)values));

            // Check if this variable is declared inside a uniform buffer
            if(values[0] != -1)
            {
                continue;;
            }

            // Get the variable name
            std::vector<char> nameChar(values[2] + 1);
            gl_call(glGetProgramResourceName(programID, GL_UNIFORM, i, (GLsizei)nameChar.size(), NULL, nameChar.data()));
            std::string name(nameChar.data());

            // Make sure this is actually a resource. We do not allow normal variables in a global scope
            Variable::Type type = getFalcorVariableType(props[1]);
            if(type != Variable::Type::Unknown)
            {
                log += "Error. Variable '" + name + "' is defined in the global namespace but is not a resources.";
                return false;
            }

            // OK, this is a resource. Create the resource desc
            mResources[name] = getResourceDescFromGl(values[1]);
            assert(mResources[name].type != ProgramReflection::Resource::ResourceType::Unknown);
            mResources[name].arraySize = values[4] ? values[5] : 0;
            mResources[name].location = values[3];
        }
        return true;
    }
}
#endif