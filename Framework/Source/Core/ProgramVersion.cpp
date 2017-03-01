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
#include "Core/ProgramVersion.h"
#include "Graphics/Material/MaterialSystem.h"

namespace Falcor
{
    ProgramVersion::ProgramVersion(const Shader::SharedPtr& pVS, const Shader::SharedPtr& pFS, const Shader::SharedPtr& pGS, const Shader::SharedPtr& pHS, const Shader::SharedPtr& pDS, const Shader::SharedPtr& pCS, const std::string& name) : mName(name)
    {
        mpShaders[(uint32_t)ShaderType::Vertex] = pVS;
        mpShaders[(uint32_t)ShaderType::Fragment] = pFS;
        mpShaders[(uint32_t)ShaderType::Geometry] = pGS;
        mpShaders[(uint32_t)ShaderType::Domain] = pDS;
        mpShaders[(uint32_t)ShaderType::Hull] = pHS;
        mpShaders[(uint32_t)ShaderType::Hull] = pCS;
    }

    ProgramVersion::~ProgramVersion()
    {
        MaterialSystem::removeProgramVersion(this);
        deleteApiHandle();
    }

    uint32_t ProgramVersion::getUniformBufferBinding(const std::string& name) const
    {
        const auto& it = mBuffersDesc.find(name);
        if(it == mBuffersDesc.end())
        {
            std::string Msg("Error when getting uniform block binding index.\n");
            Msg += "Block \"" + name + "\" not found in program \"" + mName + "\".\n";
            Logger::log(Logger::Level::Error, Msg);
            return kInvalidLocation;
        }
        return it->second.bufferSlot;
    }
    ProgramVersion::SharedConstPtr ProgramVersion::create(const Shader::SharedPtr& pVS,
        const Shader::SharedPtr& pFS,
        const Shader::SharedPtr& pGS,
        const Shader::SharedPtr& pHS,
        const Shader::SharedPtr& pDS,
        std::string& log,
        const std::string& name)
    {
        return ProgramVersion::createInternal(pVS, pFS, pGS, pHS, pDS, nullptr, log, name);
    }

    ProgramVersion::SharedConstPtr ProgramVersion::create(const Shader::SharedPtr& pCS, std::string& log, const std::string& name)
    {
        return ProgramVersion::createInternal(nullptr, nullptr, nullptr, nullptr, nullptr, pCS, log, name);
    }

}
