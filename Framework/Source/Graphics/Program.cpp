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
#include "Program.h"
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Graphics/TextureHelper.h"
#include "Utils/OS.h"
#include "Core/Shader.h"
#include "Core/ProgramVersion.h"
#include "Core/Texture.h"
#include "Core/Sampler.h"
#include "Utils/ShaderUtils.h"
#include "Core/RenderContext.h"
#include "Utils/StringUtils.h"

namespace Falcor
{
    std::vector<Program*> Program::sPrograms;

    Program::Program()
    {
        sPrograms.push_back(this);
    }

    Program::~Program()
    {
        // Remove the current program from the program vector
        for(auto it = sPrograms.begin() ; it != sPrograms.end() ; it++)
        {
            if(*it == this)
            {
                sPrograms.erase(it);
                break;;
            }
        }
    }

    std::string Program::getProgramDescString() const
    {
        std::string desc = "Program with Shaders:\n";

        for(uint32_t i = 0; i < kShaderCount; i++)
        {
            if(mShaderStrings[i].size())
            {
                desc += mShaderStrings[i] + "\n";
            }
        }
        return desc;
    }

    Program::SharedPtr Program::createFromFile(const std::string& vertexFile, const std::string& fragmentFile, const std::string& shaderDefines)
    {
        std::string empty;
        return createFromFile(vertexFile, fragmentFile, empty, empty, empty, shaderDefines);
    }

    Program::SharedPtr Program::createFromFile(const std::string& vertexFile, const std::string& fragmentFile, const std::string& geometryFile, const std::string& hullFile, const std::string& domainFile, const std::string& shaderDefines)
    {
        return createInternal(vertexFile, fragmentFile, geometryFile, hullFile, domainFile, shaderDefines, true);
    }

    Program::SharedPtr Program::createFromString(const std::string& vertexShader, const std::string& fragmentShader, const std::string& shaderDefines)
    {
        std::string empty;
        return createFromString(vertexShader, fragmentShader, empty, empty, empty, shaderDefines);
    }

    Program::SharedPtr Program::createFromString(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::string& hullShader, const std::string& domainShader, const std::string& shaderDefines)
    {
        return createInternal(vertexShader, fragmentShader, geometryShader, hullShader, domainShader, shaderDefines, false);
    }

    Program::SharedPtr Program::createInternal(const std::string& VS, const std::string& FS, const std::string& GS, const std::string& HS, const std::string& DS, const std::string& shaderDefines, bool createdFromFile)
    {
        SharedPtr pProgram = SharedPtr(new Program);
        pProgram->mShaderStrings[(uint32_t)ShaderType::Vertex] = VS.size() ? VS : "DefaultVS.vs";
        pProgram->mShaderStrings[(uint32_t)ShaderType::Fragment] = FS;
        pProgram->mShaderStrings[(uint32_t)ShaderType::Geometry] = GS;
        pProgram->mShaderStrings[(uint32_t)ShaderType::Hull] = HS;
        pProgram->mShaderStrings[(uint32_t)ShaderType::Domain] = DS;
        pProgram->mCreatedFromFile = createdFromFile;
        pProgram->setInitialProgramDefines(shaderDefines);

        return pProgram;
    }

    void Program::setInitialProgramDefines(const std::string& defines)
    {
        if(defines.size())
        {
            std::vector<std::string> definesVec = splitString(defines, "\n");

            for(auto def : definesVec)
            {
                if(def.size())
                {
                    // Find the first space
                    size_t space = def.find(' ');
                    std::string val;
                    if(space != std::string::npos)
                    {
                        val = def.substr(space + 1);
                        def = def.substr(0, space);
                    }
                    addDefine(def, val);
                }
            }
        }
    }

    void Program::addDefine(const std::string& name, const std::string& value)
    {
        removeDefine(name);
        mDefineSet.insert(name);
        mActiveDefineString += name;
        if(value.size())
        {
            mActiveDefineString += ' ' + value;
        }
        mActiveDefineString += '\n';
        mLinkRequired = true;
    }

    void Program::removeDefine(const std::string& name)
    {
        auto& def = mDefineSet.find(name);
        if(def != mDefineSet.end())
        {
            mDefineSet.erase(def);
            size_t start = mActiveDefineString.find(name);
            size_t end = mActiveDefineString.find('\n', start);
            mActiveDefineString.erase(start, end-start+1);
            mLinkRequired = true;
        }
    }

    ProgramVersion::SharedConstPtr Program::getActiveProgramVersion() const
    {
        if(mLinkRequired)
        {
            const auto& it = mProgramVersions.find(mActiveDefineString);
            ProgramVersion::SharedConstPtr pVersion = nullptr;
            if(it == mProgramVersions.end())
            {
                // New version
                pVersion = link(mActiveDefineString);

                if(pVersion == nullptr)
                {
                    return false;
                }
                else
                {
                    mProgramVersions[mActiveDefineString] = std::move(pVersion);
                }
            }
            mpActiveProgram = mProgramVersions[mActiveDefineString];
            mLinkRequired = false;
        }

        return mpActiveProgram;
    }

    ProgramVersion::SharedConstPtr Program::link(const std::string& defines) const
    {
        mUboMap.clear();
        while(1)
        {
            Shader::SharedPtr pShaders[kShaderCount];

            // create the shaders
            for(uint32_t i = 0; i < kShaderCount; i++)
            {
                if(mShaderStrings[i].size())
                {
                    if(mCreatedFromFile)
                    {
                        pShaders[i] = createShaderFromFile(mShaderStrings[i], ShaderType(i), defines);
                    }
                    else
                    {
                        pShaders[i] = createShaderFromString(mShaderStrings[i], ShaderType(i));
                    }
                }
            }

            // create the program
            std::string log;
            ProgramVersion::SharedConstPtr pProgram = ProgramVersion::create(pShaders[(uint32_t)ShaderType::Vertex],
                pShaders[(uint32_t)ShaderType::Fragment],
                pShaders[(uint32_t)ShaderType::Geometry],
                pShaders[(uint32_t)ShaderType::Hull],
                pShaders[(uint32_t)ShaderType::Domain],
                log, 
                getProgramDescString());

            if(pProgram == nullptr)
            {
                std::string error = std::string("Program Linkage failed.\n\n");
                error += getProgramDescString() + "\n";
                error += log;

                if(msgBox(error, MsgBoxType::RetryCancel) == MsgBoxButton::Cancel)
                {
                    Logger::log(Logger::Level::Fatal, error);
                    return nullptr;
                }
            }
            else
            {
                return pProgram;
            }
        }
    }

    void Program::reloadAllPrograms()
    {
        for(auto& pProgram : sPrograms)
        {
			pProgram->mProgramVersions.clear();
            pProgram->mLinkRequired = true;
        }
    }

    const Shader* Program::getShader(ShaderType Type) const
    {
        return getActiveProgramVersion()->getShader(Type);
    }
    
    int32_t Program::getAttributeLocation(const std::string& Attribute) const
    {
        return getActiveProgramVersion()->getAttributeLocation(Attribute);
    }

    uint32_t Program::getUniformBufferBinding(const std::string& Name) const
    {
        return getActiveProgramVersion()->getUniformBufferBinding(Name);
    }

    UniformBuffer::SharedPtr Program::getUniformBuffer(const std::string& bufName)
    {
        // Check to see if this UBO has previously been accessed
        UniformBuffer::SharedPtr pUbo = mUboMap[bufName];
        if(pUbo == nullptr)
        {
            pUbo = UniformBuffer::create(getActiveProgramVersion().get(), bufName);
            mUboMap[bufName] = pUbo;
        }
        return pUbo;
    }

    void Program::bindUniformBuffer(const std::string& bufName, UniformBuffer::SharedPtr& pUbo)
    {
        if(getUniformBufferBinding(bufName) != ProgramVersion::kInvalidLocation)
        {
            mUboMap[bufName] = pUbo;
        }
        else
        {
            Logger::log(Logger::Level::Error, "Can't bind UBO to program. UBO " + bufName + " wasn't found.");
        }
    }

    void Program::setUniformBuffersIntoContext(RenderContext* pContext)
    {
        for(auto& i = mUboMap.begin(); i != mUboMap.end(); i++)
        {
            uint32_t loc = getUniformBufferBinding(i->first);
            assert(loc != ProgramVersion::kInvalidLocation);
            pContext->setUniformBuffer(loc, i->second);
        }
    }
}