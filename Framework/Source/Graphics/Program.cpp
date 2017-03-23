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
#include "API/Shader.h"
#include "API/ProgramVersion.h"
#include "API/Texture.h"
#include "API/Sampler.h"
#include "Utils/ShaderUtils.h"
#include "API/RenderContext.h"
#include "Utils/StringUtils.h"

#include "Utils/SpireSupport.h"

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
		if (mSpireEnv)
			ShaderRepository::Instance().UnloadSource(mSpireEnv);
		if (mSpireSink)
			spDestroyDiagnosticSink(mSpireSink);
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

    void Program::init(const std::string& VS, const std::string& FS, const std::string& GS, const std::string& HS, const std::string& DS, const DefineList& programDefines, bool createdFromFile)
    {
        mShaderStrings[(uint32_t)ShaderType::Vertex] = VS.size() ? VS : "DefaultVS.hlsl";
        mShaderStrings[(uint32_t)ShaderType::Pixel] = FS;
        mShaderStrings[(uint32_t)ShaderType::Geometry] = GS;
        mShaderStrings[(uint32_t)ShaderType::Hull] = HS;
        mShaderStrings[(uint32_t)ShaderType::Domain] = DS;
        mCreatedFromFile = createdFromFile;
        mDefineList = programDefines;
    }

    void Program::init(const std::string& cs, const DefineList& programDefines, bool createdFromFile)
    {
        mShaderStrings[(uint32_t)ShaderType::Compute] = cs;
        mCreatedFromFile = createdFromFile;
        mDefineList = programDefines;
    }

    static std::string getSpireErrors(SpireDiagnosticSink* sink)
    {
        int size = spGetDiagnosticOutput(sink, nullptr, 0);

        std::string result;
        result.resize(size-1);

        spGetDiagnosticOutput(sink, &result[0], size);

        spClearDiagnosticSink(sink);

        return result;
    }

    void Program::initFromSpire(const std::string& shader, bool createdFromFile)
    {
        mCreatedFromFile = createdFromFile;
        mIsSpire = true;
		mSpireSink = spCreateDiagnosticSink(ShaderRepository::Instance().GetContext());

        // load the code
        // TODO: loop so that the user can fix errors and retry
        {
            std::string name;
            std::string code;
            if (createdFromFile)
            {
                std::string fullpath;
                if (!findFileInDataDirectories(shader, fullpath))
                {
                    std::string err = std::string("Can't find shader file ") + shader;
                    logError(err);
                    return;
                }

                name = shader;
                readFileToString(fullpath, code);
            }
            else
            {
                name = "shader";
                code = shader;
            }

			SpireDiagnosticSink * spireSink = spCreateDiagnosticSink(ShaderRepository::Instance().GetContext());

			mSpireEnv = ShaderRepository::Instance().LoadSource(code.c_str(), name.c_str(), spireSink);

            // check for and report errors
            //
            // TODO: show a dialog box and allow user to iterat eon fixing errors
            if( spDiagnosticSinkHasAnyErrors(spireSink) )
            {
                std::string err = std::string("Errors compiling ") + name + ":\n"
                    + getSpireErrors(spireSink);
                logError(err);
                return;
            }

            // retrieve the last shader entry point in source

//            SpireShader* spireShader = spEnvGetShader(mSpireEnv, spEnvGetShaderCount(mSpireEnv) - 1);

            // HACK: pick first shader with non-zero number of parameters
            int shaderCount = spEnvGetShaderCount(mSpireEnv);
            SpireShader* spireShader = nullptr;
            for(int ss = 0; ss < shaderCount; ++ss)
            {
                spireShader = spEnvGetShader(mSpireEnv, ss);
                if(spShaderGetParameterCount(spireShader) > 0)
                    break;
            }

            int componentParameterCount = spShaderGetParameterCount(spireShader);
            mSpireComponentClassList.resize(componentParameterCount, nullptr);

            SpireModule* spireShaderParamsComponentClass = nullptr;
            if(componentParameterCount >= 1)
            {
                // We assume that the first component parameter represents the inputs
                // needed by the shader itself
                char const* componentTypeName = spShaderGetParameterType(spireShader, 0);

                spireShaderParamsComponentClass = spEnvFindModule(
                    mSpireEnv,
                    componentTypeName);
            }

            // HACK
            if( !spireShaderParamsComponentClass )
            {
                spireShaderParamsComponentClass = spEnvFindModule(
					mSpireEnv,
                    "PerFrameCB");
            }

            if( spireShaderParamsComponentClass )
            {
                mSpireComponentClassList[0] = spireShaderParamsComponentClass;
            }

            mSpireShader = spireShader;
            mSpireShaderParamsComponentClass = spireShaderParamsComponentClass;
        }
    }

    ProgramReflection::SharedConstPtr Program::getSpireReflector() const
    {
        if( !mSpireReflector )
        {
            std::string err;
            mSpireReflector = ProgramReflection::createFromSpire(mSpireEnv, mSpireShader, err);
            if( !err.empty() )
            {
                logError(err);
            }
        }
        return mSpireReflector;
    }

    void Program::setComponent(size_t index, SpireModule* componentClass)
    {
        assert(index < mSpireComponentClassList.size());

        // Don't change anything if the same class is already set
        if(mSpireComponentClassList[index] == componentClass)
            return;

        mSpireComponentClassList[index] = componentClass;
        mLinkRequired = true;
    }

    void Program::addDefine(const std::string& name, const std::string& value)
    {
        // Make sure that it doesn't exist already
        if (mDefineList.find(name) != mDefineList.end())
        {
            if (mDefineList[name] == value)
            {
                // Same define
                return;
            }
        }
        mLinkRequired = true;
        mDefineList[name] = value;
    }

    void Program::removeDefine(const std::string& name)
    {
        if (mDefineList.find(name) != mDefineList.end())
        {
            mLinkRequired = true;
            mDefineList.erase(name);
        }
    }

    bool Program::checkIfFilesChanged()
    {
        if (mpActiveProgram == nullptr)
        {
            // We never linked, so nothing really changed
            return false;
        }

        for (uint32_t i = 0; i < arraysize(mShaderStrings); i++)
        {
            const Shader* pShader = mpActiveProgram->getShader(ShaderType(i));
            if (pShader)
            {
                if (mCreatedFromFile)
                {
                    std::string fullpath;
                    findFileInDataDirectories(mShaderStrings[i], fullpath);
                    if (mFileTimeMap[fullpath] != getFileModifiedTime(fullpath))
                    {
                        return true;
                    }
                }

                // Loop over the shader's included files
                for (const auto& include : pShader->getIncludeList())
                {
                    if (mFileTimeMap[include] != getFileModifiedTime(include))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    ProgramVersion::SharedConstPtr Program::getActiveVersion() const
    {
        if (mLinkRequired)
        {
            const auto& it = mProgramVersions.find(mDefineList);
            ProgramVersion::SharedConstPtr pVersion = nullptr;
            if (it == mProgramVersions.end())
            {
                if (link() == false)
                {
                    return false;
                }
                else
                {
                    mProgramVersions[mDefineList] = mpActiveProgram;
                }
            }
            else
            {
                mpActiveProgram = mProgramVersions[mDefineList];
            }
        }

        return mpActiveProgram;
    }

    bool Program::link() const
    {
        mFileTimeMap.clear();

        while (1)
        {
            Shader::SharedPtr pShaders[kShaderCount];

            // create the shaders
            if( mIsSpire )
            {
                // we have a spire shader, so we need to link here for a particular version

                spClearDiagnosticSink(mSpireSink);

#if 0
                SpireModule* componentClasses[16];
                int componentClassCount = 0;

                if( mSpireShaderParamsComponentClass )
                {
                    componentClasses[componentClassCount++] = mSpireShaderParamsComponentClass;
                }
#else
                SpireModule** componentClasses = (SpireModule**) &mSpireComponentClassList[0];
                int componentClassCount = (int) mSpireComponentClassList.size();
#endif

                // TODO: this is where we'd need to enumerate the additional component
                // classes desired for the "active version"

                SpireCompilationResult* spireResult = spEnvCompileShader(
                    mSpireEnv,
                    mSpireShader,

                    // no arguments for now
                    &componentClasses[0],
                    componentClassCount,

                    // no additional source
                    "",
                    mSpireSink);

                // TODO: check the sink
                if( spDiagnosticSinkHasAnyErrors(mSpireSink) )
                {
                    std::string err = std::string("Errors compiling Spire shader:\n")
                        + getSpireErrors(mSpireSink);
                    logError(err);

                    // TDOO: use a message box and give the option to retry compilation
                    return false;
                }

                // Now extract the per-stage kernels from the compilation reuslt

                static const char* kSpireStageNames[] = {
                    "vs",
                    "fs",
                    "hs",
                    "ds",
                    "gs",
                    "cs",
                };

                for(uint32_t i = 0; i < kShaderCount; i++)
                {
                    char const* code = nullptr;
                    int codeLength = 0;
                    code = spGetShaderStageSource(
                        spireResult,
                        nullptr,
                        kSpireStageNames[i],
                        &codeLength);

                    if( code )
                    {
                        pShaders[i] = createShaderFromString(code, ShaderType(i), mDefineList);
                    }
                }
            }
            else
            for(uint32_t i = 0; i < kShaderCount; i++)
            {
                if(mShaderStrings[i].size())
                {
                    if(mCreatedFromFile)
                    {
                        pShaders[i] = createShaderFromFile(mShaderStrings[i], ShaderType(i), mDefineList);
                        if(pShaders[i])
                        {
                            std::string fullpath;
                            findFileInDataDirectories(mShaderStrings[i], fullpath);
                            mFileTimeMap[fullpath] = getFileModifiedTime(fullpath);
                        }
                    }
                    else
                    {
                        pShaders[i] = createShaderFromString(mShaderStrings[i], ShaderType(i), mDefineList);
                    }

                    if(pShaders[i])
                    {
                        for(const auto& include : pShaders[i]->getIncludeList())
                        {
                            mFileTimeMap[include] = getFileModifiedTime(include);
                        }
                    }
                }
            }

            // create the program
            std::string log;
            ProgramVersion::SharedConstPtr pProgram;
            if (pShaders[(uint32_t)ShaderType::Compute])
            {
                pProgram = ProgramVersion::create(pShaders[(uint32_t)ShaderType::Compute], log, getProgramDescString());
            }
            else
            {
                pProgram = ProgramVersion::create(pShaders[(uint32_t)ShaderType::Vertex],
                    pShaders[(uint32_t)ShaderType::Pixel],
                    pShaders[(uint32_t)ShaderType::Geometry],
                    pShaders[(uint32_t)ShaderType::Hull],
                    pShaders[(uint32_t)ShaderType::Domain],
                    log,
                    getProgramDescString());
            }

            if(pProgram == nullptr)
            {
                std::string error = std::string("Program Linkage failed.\n\n");
                error += getProgramDescString() + "\n";
                error += log;

                if(msgBox(error, MsgBoxType::RetryCancel) == MsgBoxButton::Cancel)
                {
                    logError(error);
                    return false;
                }
            }
            else
            {
                mpActiveProgram = pProgram;
                return true;
            }
        }
    }

    void Program::reset()
    {
        mpActiveProgram = nullptr;
        mProgramVersions.clear();
        mFileTimeMap.clear();
        mLinkRequired = true;
    }

    void Program::reloadAllPrograms()
    {
        for(auto& pProgram : sPrograms)
        {
            if(pProgram->checkIfFilesChanged())
            {
                pProgram->reset();
            }
        }
    }

}
