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
#include "Framework.h"

namespace Falcor
{
    class ProgramVersion;

    /** This class holds all of the data required to reflect a program, including inputs, outputs, uniforms, textures and samplers declarations
    */
    class ProgramReflection
    {
    public:
        using SharedPtr = std::shared_ptr<ProgramReflection>;
        using SharedConstPtr = std::shared_ptr<const ProgramReflection>;

        struct BufferDesc
        {
            std::string name;
            size_t sizeInBytes = 0;
            size_t variableCount = 0;
        };

        /** Invalid location of uniform and attributes
        */
        static const int32_t kInvalidLocation = -1;

        /** Create a new object
        */
        SharedPtr create(const ProgramVersion* pProgramVersion);

        /** Get a uniform buffer binding index
        \param[in] name The uniform buffer name in the program
        \return The bind location of the UBO if it is found, otherwise ProgramVersion#kInvalidLocation
        */
        uint32_t getUniformBufferBinding(const std::string& name) const;

        using BufferDescMap = std::unordered_map<uint32_t, BufferDesc>;

        /** Get the uniform-buffer list
        */
        const BufferDescMap& getUniformBufferMap() const { return mUniformBuffer.descMap; }

        /** Get a uniform-buffer descriptor
            \param[in] bindLocation The bindLocation of the requested buffer
            \return Reference to the buffer descriptor. If the location is not used, returns the default BufferDesc
        */
        const BufferDesc& getUniformBufferDesc(uint32_t bindLocation) const
        {
            auto& desc = mUniformBuffer.descMap.find(bindLocation);
            if(desc == mUniformBuffer.descMap.end())
            {
                static BufferDesc defaultDesc;
                return defaultDesc;
            }
            return desc->second;
        }
    private:
        using string_2_uint_map = std::unordered_map<std::string, uint32_t>;
        struct
        {
            BufferDescMap descMap;
            string_2_uint_map nameMap;
        } mUniformBuffer;
    };
}