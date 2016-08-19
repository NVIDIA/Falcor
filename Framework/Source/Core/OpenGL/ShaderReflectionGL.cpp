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
#ifdef FALCOR_GL
#include "ShaderReflectionGL.h"
#include "Utils/StringUtils.h"

namespace Falcor
{
    namespace ShaderReflection
    {
        static bool reflectBufferByType(uint32_t programID, GLenum bufferType, ShaderReflection::BufferDescMap& descMap, std::string& log)
        {
            BufferDesc::Type falcorType;
            GLenum maxBlockType = GL_NONE;

            switch(bufferType)
            {
            case GL_UNIFORM_BLOCK:
                maxBlockType = GL_MAX_COMBINED_UNIFORM_BLOCKS;
                falcorType = BufferDesc::Type::Uniform;
                break;
            case GL_SHADER_STORAGE_BLOCK:
                maxBlockType = GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS;
                falcorType = BufferDesc::Type::ShaderStorage;
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

            std::vector<bool> usedBindings;
            usedBindings.assign(uniformBlockCount, false);

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
                GLenum props[] = {GL_BUFFER_DATA_SIZE, GL_BUFFER_BINDING, GL_NUM_ACTIVE_VARIABLES};
                uint32_t values[arraysize(props)];
                gl_call(glGetProgramResourceiv(programID, bufferType, blockIndex, arraysize(props), props, arraysize(props), nullptr, (int32_t*)values));

                // Fill the desc
                BufferDesc desc;
                desc.type = falcorType;
                desc.sizeInBytes = values[0];
                desc.bufferSlot = values[1];
                desc.variableCount = values[2];

                // Check that the binding index is unique
                if(usedBindings[desc.bufferSlot])
                {
                    log = "Uniform buffer \"" + name + "\" - binding index already used in program. Make sure you use explicit bindings - 'layout (binding = n)' - in your uniform buffer declarations.";
                    return false;
                }
                usedBindings[desc.bufferSlot] = true;
                descMap[name] = desc;
            }
            return true;
        }

        bool reflectBuffers(GLenum programID, ShaderReflection::BufferDescMap& descMap, std::string& log)
        {
            GLenum types[] = {GL_UNIFORM_BLOCK, GL_SHADER_STORAGE_BLOCK};
            bool bOK = true;
            for(uint32_t i = 0; i < arraysize(types) && bOK; i++)
            {
                bOK = reflectBufferByType(programID, types[i], descMap, log);
            }
            return true;
        }

        GLenum getVariableEnum(GLenum bufferType)
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

        VariableDesc::Type getFalcorVariableType(GLenum glType)
        {
            switch(glType)
            {
                case GL_BOOL:              
                    return VariableDesc::Type::Bool;
                case GL_BOOL_VEC2:         
                    return VariableDesc::Type::Bool2;
                case GL_BOOL_VEC3:         
                    return VariableDesc::Type::Bool3;
                case GL_BOOL_VEC4:         
                    return VariableDesc::Type::Bool4;
                case GL_UNSIGNED_INT:      
                    return VariableDesc::Type::Uint;
                case GL_UNSIGNED_INT_VEC2: 
                    return VariableDesc::Type::Uint2;
                case GL_UNSIGNED_INT_VEC3: 
                    return VariableDesc::Type::Uint3;
                case GL_UNSIGNED_INT_VEC4: 
                    return VariableDesc::Type::Uint4;
                case GL_INT:               
                    return VariableDesc::Type::Int;
                case GL_INT_VEC2:          
                    return VariableDesc::Type::Int2;
                case GL_INT_VEC3:          
                    return VariableDesc::Type::Int3;
                case GL_INT_VEC4:          
                    return VariableDesc::Type::Int4;
                case GL_FLOAT:             
                    return VariableDesc::Type::Float;
                case GL_FLOAT_VEC2:        
                    return VariableDesc::Type::Float2;
                case GL_FLOAT_VEC3:        
                    return VariableDesc::Type::Float3;
                case GL_FLOAT_VEC4:        
                    return VariableDesc::Type::Float4;
                case GL_FLOAT_MAT2:        
                    return VariableDesc::Type::Float2x2;
                case GL_FLOAT_MAT2x3:      
                    return VariableDesc::Type::Float2x3;
                case GL_FLOAT_MAT2x4:      
                    return VariableDesc::Type::Float2x4;
                case GL_FLOAT_MAT3:        
                    return VariableDesc::Type::Float3x3;
                case GL_FLOAT_MAT3x2:      
                    return VariableDesc::Type::Float3x2;
                case GL_FLOAT_MAT3x4:      
                    return VariableDesc::Type::Float3x4;
                case GL_FLOAT_MAT4:        
                    return VariableDesc::Type::Float4x4;
                case GL_FLOAT_MAT4x2:      
                    return VariableDesc::Type::Float4x2;
                case GL_FLOAT_MAT4x3:      
                    return VariableDesc::Type::Float4x3;
                case GL_GPU_ADDRESS_NV: 
                    return VariableDesc::Type::GpuPtr;
                case GL_INT64_NV:               
                    return VariableDesc::Type::Int64;
                case GL_INT64_VEC2_NV:          
                    return VariableDesc::Type::Int64_2;
                case GL_INT64_VEC3_NV:          
                    return VariableDesc::Type::Int64_3;
                case GL_INT64_VEC4_NV:          
                    return VariableDesc::Type::Int64_4;
                case GL_UNSIGNED_INT64_NV:      
                    return VariableDesc::Type::Uint64;
                case GL_UNSIGNED_INT64_VEC2_NV: 
                    return VariableDesc::Type::Uint64_2;
                case GL_UNSIGNED_INT64_VEC3_NV: 
                    return VariableDesc::Type::Uint64_3;
                case GL_UNSIGNED_INT64_VEC4_NV: 
                    return VariableDesc::Type::Uint64_4;
                /*case GL_UNSIGNED_INT8_NV:       return SVariableDesc::Type::; //TODO: add
                /*case GL_INT8_NV:                return SVariableDesc::Type::; //TODO: add
                case GL_INT8_VEC2_NV:           return SVariableDesc::Type::;
                case GL_INT8_VEC3_NV:           return SVariableDesc::Type::;
                case GL_INT8_VEC4_NV:           return SVariableDesc::Type::;
                case GL_INT16_NV:               return SVariableDesc::Type::;
                case GL_INT16_VEC2_NV:          return SVariableDesc::Type::;
                case GL_INT16_VEC3_NV:          return SVariableDesc::Type::;
                case GL_INT16_VEC4_NV:          return SVariableDesc::Type::;*/
                /*case GL_FLOAT16_NV:             return SVariableDesc::Type::; //TODO: add
                case GL_FLOAT16_VEC2_NV:        return SVariableDesc::Type::;
                case GL_FLOAT16_VEC3_NV:        return SVariableDesc::Type::;
                case GL_FLOAT16_VEC4_NV:        return SVariableDesc::Type::;
                case GL_UNSIGNED_INT8_VEC2_NV:  return SVariableDesc::Type::;
                case GL_UNSIGNED_INT8_VEC3_NV:  return SVariableDesc::Type::;
                case GL_UNSIGNED_INT8_VEC4_NV:  return SVariableDesc::Type::;
                case GL_UNSIGNED_INT16_NV:      return SVariableDesc::Type::;
                case GL_UNSIGNED_INT16_VEC2_NV: return SVariableDesc::Type::;
                case GL_UNSIGNED_INT16_VEC3_NV: return SVariableDesc::Type::;
                case GL_UNSIGNED_INT16_VEC4_NV: return SVariableDesc::Type::;*/
                default: 
                    return VariableDesc::Type::Unknown;
            }
        }

        static const std::map<GLenum, ShaderResourceDesc> kResourceDescMap =
        {
            {GL_SAMPLER_1D,                             {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_1D_SHADOW,                      {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_1D,                         {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_1D,                {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D,                             {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_SHADOW,                      {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_2D,                         {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_2D,                {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_3D,                             {ShaderResourceDesc::Dimensions::Texture3D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_3D,                         {ShaderResourceDesc::Dimensions::Texture3D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_3D,                {ShaderResourceDesc::Dimensions::Texture3D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_CUBE,                           {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_CUBE_SHADOW,                    {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_CUBE,                       {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_CUBE,              {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_1D_ARRAY,                       {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_1D_ARRAY_SHADOW,                {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_1D_ARRAY,                   {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,          {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_ARRAY,                       {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_ARRAY_SHADOW,                {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_2D_ARRAY,                   {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,          {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_CUBE_MAP_ARRAY,                 {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW,          {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_CUBE_MAP_ARRAY,             {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY,    {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_MULTISAMPLE,                 {ShaderResourceDesc::Dimensions::Texture2DMS,            ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_2D_MULTISAMPLE,             {ShaderResourceDesc::Dimensions::Texture2DMS,            ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,    {ShaderResourceDesc::Dimensions::Texture2DMS,            ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_MULTISAMPLE_ARRAY,           {ShaderResourceDesc::Dimensions::Texture2DMSArray,       ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,       {ShaderResourceDesc::Dimensions::Texture2DMSArray,       ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, {ShaderResourceDesc::Dimensions::Texture2DMSArray,    ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_BUFFER,                         {ShaderResourceDesc::Dimensions::TextureBuffer,          ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_BUFFER,                     {ShaderResourceDesc::Dimensions::TextureBuffer,          ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_BUFFER,            {ShaderResourceDesc::Dimensions::TextureBuffer,          ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_RECT,                        {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_SAMPLER_2D_RECT_SHADOW,                 {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Texture}},
            {GL_INT_SAMPLER_2D_RECT,                    {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Texture}},
            {GL_UNSIGNED_INT_SAMPLER_2D_RECT,           {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Texture}},
            
            {GL_IMAGE_1D,                               {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_1D,                           {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_1D,                  {ShaderResourceDesc::Dimensions::Texture1D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_2D,                               {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_2D,                           {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_2D,                  {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_3D,                               {ShaderResourceDesc::Dimensions::Texture3D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_3D,                           {ShaderResourceDesc::Dimensions::Texture3D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_3D,                  {ShaderResourceDesc::Dimensions::Texture3D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_2D_RECT,                          {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_2D_RECT,                      {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_2D_RECT,             {ShaderResourceDesc::Dimensions::Texture2D,              ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_CUBE,                             {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_CUBE,                         {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_CUBE,                {ShaderResourceDesc::Dimensions::TextureCube,            ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_BUFFER,                           {ShaderResourceDesc::Dimensions::TextureBuffer,          ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_BUFFER,                       {ShaderResourceDesc::Dimensions::TextureBuffer,          ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_BUFFER,              {ShaderResourceDesc::Dimensions::TextureBuffer,          ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_1D_ARRAY,                         {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_1D_ARRAY,                     {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_1D_ARRAY,            {ShaderResourceDesc::Dimensions::Texture1DArray,         ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_2D_ARRAY,                         {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_2D_ARRAY,                     {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_2D_ARRAY,            {ShaderResourceDesc::Dimensions::Texture2DArray,         ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_2D_MULTISAMPLE,                   {ShaderResourceDesc::Dimensions::Texture2DMS,            ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_2D_MULTISAMPLE,               {ShaderResourceDesc::Dimensions::Texture2DMS,            ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE,      {ShaderResourceDesc::Dimensions::Texture2DMS,            ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_2D_MULTISAMPLE_ARRAY,             {ShaderResourceDesc::Dimensions::Texture2DMSArray,       ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY,         {ShaderResourceDesc::Dimensions::Texture2DMSArray,       ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, {ShaderResourceDesc::Dimensions::Texture2DMSArray,      ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
            {GL_IMAGE_CUBE_MAP_ARRAY,                   {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Float, ShaderResourceDesc::ResourceType::Image}},
            {GL_INT_IMAGE_CUBE_MAP_ARRAY,               {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Int,   ShaderResourceDesc::ResourceType::Image}},
            {GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY,      {ShaderResourceDesc::Dimensions::TextureCubeArray,       ShaderResourceDesc::ReturnType::Uint,  ShaderResourceDesc::ResourceType::Image}},
        };

        const ShaderResourceDesc& getShaderResourceDesc(GLenum glType)
        {
            static const ShaderResourceDesc empty;
            const auto& it = kResourceDescMap.find(glType);
            if(it == kResourceDescMap.end())
            {
                return empty;
            }
            return it->second;
        }

        bool reflectBufferVariables(uint32_t programID, const std::string& bufferName, GLenum bufferType, size_t bufferSize, VariableDescMap& variableMap, ShaderResourceDescMap& resourceMap)
        {
            // Get the uniform block index
            uint32_t blockIndex = gl_call(glGetProgramResourceIndex(programID, bufferType, bufferName.c_str()));
            if(blockIndex == GL_INVALID_INDEX)
            {
                std::string err = "Error when reflecting uniform buffer. Can't find buffer " + bufferName + ".";
                Logger::log(Logger::Level::Error, err);
                return false;
            }

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

                VariableDesc desc;
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
                if(desc.type == VariableDesc::Type::Unknown)
                {
                    resourceMap[uniformName] = getShaderResourceDesc(values[1]);
                    assert(resourceMap[uniformName].type != ShaderResourceDesc::ResourceType::Unknown);
                    resourceMap[uniformName].arraySize = desc.arraySize;
                    resourceMap[uniformName].offset = desc.offset;
                    desc.type = VariableDesc::Type::Resource;
                }
                variableMap[uniformName] = desc;
            }
            return true;
        }
    }
}
#endif //#ifdef FALCOR_DX11
