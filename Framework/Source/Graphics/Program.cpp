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
            if(mOriginalShaderStrings[i].size())
            {
                desc += mOriginalShaderStrings[i] + "\n";
            }
        }
        return desc;
    }

    void Program::init(const std::string& VS, const std::string& FS, const std::string& GS, const std::string& HS, const std::string& DS, const DefineList& programDefines, bool createdFromFile)
    {
        mOriginalShaderStrings[(uint32_t)ShaderType::Vertex] = VS.size() ? VS : "DefaultVS.slang";
        mOriginalShaderStrings[(uint32_t)ShaderType::Pixel] = FS;
        mOriginalShaderStrings[(uint32_t)ShaderType::Geometry] = GS;
        mOriginalShaderStrings[(uint32_t)ShaderType::Hull] = HS;
        mOriginalShaderStrings[(uint32_t)ShaderType::Domain] = DS;
        mCreatedFromFile = createdFromFile;
        mDefineList = programDefines;
    }

    void Program::init(const std::string& cs, const DefineList& programDefines, bool createdFromFile)
    {
        mOriginalShaderStrings[(uint32_t)ShaderType::Compute] = cs;
        mCreatedFromFile = createdFromFile;
        mDefineList = programDefines;
    }

    void Program::addDefine(const std::string& name, const std::string& value)
    {
        // Make sure that it doesn't exist already
        if(mDefineList.find(name) != mDefineList.end())
        {
            if(mDefineList[name] == value)
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
        if(mDefineList.find(name) != mDefineList.end())
        {
            mLinkRequired = true;
            mDefineList.erase(name);
        }
    }

    bool Program::checkIfFilesChanged()
    {
        if(mpActiveProgram == nullptr)
        {
            // We never linked, so nothing really changed
            return false;
        }

        // Have any of the files we depend on changed?
        for(auto& entry : mFileTimeMap)
        {
            auto& path = entry.first;
            auto& modifiedTime = entry.second;

            if( modifiedTime != getFileModifiedTime(path) )
            {
                return true;
            }
        }
        return false;
    }

    ProgramVersion::SharedConstPtr Program::getActiveVersion() const
    {
        if(mLinkRequired)
        {
            const auto& it = mProgramVersions.find(mDefineList);
            ProgramVersion::SharedConstPtr pVersion = nullptr;
            if(it == mProgramVersions.end())
            {
                if(link() == false)
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

    SlangSession* getSlangSession()
    {
        // TODO: figure out a strategy for finalizing the Slang session, if desired

        static SlangSession* slangSession = spCreateSession(NULL);
        return slangSession;
    }

    void loadSlangBuiltins(char const* name, char const* text)
    {
        spAddBuiltins(getSlangSession(), name, text);
    }

    static const char* getSlangTargetString(ShaderType type)
    {
        // TODO: either pick these based on target API,
        // or invent some API-neutral target names
        switch (type)
        {
        case ShaderType::Vertex:
            return "vs_5_0";
        case ShaderType::Pixel:
            return "ps_5_0";
        case ShaderType::Hull:
            return "hs_5_0";
        case ShaderType::Domain:
            return "ds_5_0";
        case ShaderType::Geometry:
            return "gs_5_0";
        case ShaderType::Compute:
            return "cs_5_0";
        default:
            should_not_get_here();
            return "";
        }
    }

    ProgramVersion::SharedPtr Program::preprocessAndCreateProgramVersion(std::string& log) const
    {
        mFileTimeMap.clear();

        // Run all of the shaders through Slang, so that we can get final code,
        // reflection data, etc.
        //
        // Note that we provide all the shaders at once, so that automatically
        // generated bindings can be made consistent across the stages.

        SlangSession* slangSession = getSlangSession();

        // Start building a request for compilation
        SlangCompileRequest* slangRequest = spCreateCompileRequest(slangSession);

        // Add our media search paths as `#include` search paths for Slang.
        //
        // TODO: Slang should probably support a callback API for all file I/O,
        // rather than having us specify data directories to it...
        for (auto path : getDataDirectoriesList())
        {
            spAddSearchPath(slangRequest, path.c_str());
        }

        // Pass any `#define` flags along to Slang, since we aren't doing our
        // own preprocessing any more.
        for(auto shaderDefine : mDefineList)
        {
            spAddPreprocessorDefine(slangRequest, shaderDefine.first.c_str(), shaderDefine.second.c_str());
        }

        // Pick the right target based on the current graphics API
#if defined(FALCOR_GL)
        spSetCodeGenTarget(slangRequest, SLANG_GLSL);
        spAddPreprocessorDefine(slangRequest, "FALCOR_GLSL", "1");
        SlangSourceLanguage sourceLanguage = SLANG_SOURCE_LANGUAGE_GLSL;
#elif defined(FALCOR_D3D11) || defined(FALCOR_D3D12)
        // Note: we could compile Slang directly to DXBC (by having Slang invoke the MS compiler for us,
        // but that path seems to have more issues at present, so let's just go to HLSL instead...)
        spSetCodeGenTarget(slangRequest, SLANG_HLSL);
        spAddPreprocessorDefine(slangRequest, "FALCOR_HLSL", "1");
        SlangSourceLanguage sourceLanguage = SLANG_SOURCE_LANGUAGE_HLSL;
#else
#error unknown shader compilation target
#endif

        // Configure any flags for the Slang compilation step
        SlangCompileFlags slangFlags = 0;

        // Don't actually perform semantic checking: just pass through functions bodies to downstream compiler
        slangFlags |= SLANG_COMPILE_FLAG_NO_CHECKING;
        spSetCompileFlags(slangRequest, slangFlags);

        // Now lets add all our input shader code, one-by-one
        int translationUnitsAdded = 0;
        for (uint32_t i = 0; i < kShaderCount; i++)
        {
            if (!mOriginalShaderStrings[i].size())
                continue;

            int translationUnitIndex = spAddTranslationUnit(slangRequest, sourceLanguage, nullptr);
            assert(translationUnitIndex == translationUnitsAdded);
            translationUnitsAdded++;

            if (mCreatedFromFile)
            {
                std::string fullpath;
                findFileInDataDirectories(mOriginalShaderStrings[i], fullpath);
                spAddTranslationUnitSourceFile(slangRequest, translationUnitIndex, fullpath.c_str());
            }
            else
            {
                spAddTranslationUnitSourceString(slangRequest, translationUnitIndex, "", mOriginalShaderStrings[i].c_str());
            }

            spAddTranslationUnitEntryPoint(
                slangRequest,
                translationUnitIndex,
                "main", // TODO: allow customization of entry point name?
                spFindProfile(slangSession, getSlangTargetString(ShaderType(i))));
        }

        int anySlangErrors = spCompile(slangRequest);
        log += spGetDiagnosticOutput(slangRequest);
        if(anySlangErrors)
        {
            spDestroyCompileRequest(slangRequest);
            return nullptr;
        }

        // Extract the generated code for each stage
        int translationUnitsExtracted = 0;
        for (uint32_t i = 0; i < kShaderCount; i++)
        {
            if (!mOriginalShaderStrings[i].size())
                continue;

            int translationUnitIndex = translationUnitsExtracted++;
            assert(translationUnitIndex < translationUnitsAdded);

            mPreprocessedShaderStrings[i] = spGetTranslationUnitSource(slangRequest, translationUnitIndex);
        }
        assert(translationUnitsExtracted == translationUnitsAdded);

        // Extract the reflection data
        mPreprocessedReflector = ProgramReflection::create(slang::ShaderReflection::get(slangRequest), log);

        // Extract list of files referenced, for dependency-tracking purposes
        int depFileCount = spGetDependencyFileCount(slangRequest);
        for(int ii = 0; ii < depFileCount; ++ii)
        {
            std::string depFilePath = spGetDependencyFilePath(slangRequest, ii);
            mFileTimeMap[depFilePath] = getFileModifiedTime(depFilePath);
        }

        spDestroyCompileRequest(slangRequest);

        // Now that we've preprocessed things, dispatch to the actual program creation logic,
        // which may vary in subclasses of `Program`
        return createProgramVersion(log);
    }

    ProgramVersion::SharedPtr Program::createProgramVersion(std::string& log) const
    {
        // create the shaders
        Shader::SharedPtr shaders[kShaderCount] = {};
        for (uint32_t i = 0; i < kShaderCount; i++)
        {
            if (mPreprocessedShaderStrings[i].size())
            { 
                shaders[i] = createShaderFromString(mPreprocessedShaderStrings[i], ShaderType(i));
            }           
        }

        if (shaders[(uint32_t)ShaderType::Compute])
        {
            return ProgramVersion::create(
                mPreprocessedReflector,
                shaders[(uint32_t)ShaderType::Compute], log, getProgramDescString());
        }
        else
        {
            return ProgramVersion::create(
                mPreprocessedReflector,
                shaders[(uint32_t)ShaderType::Vertex],
                shaders[(uint32_t)ShaderType::Pixel],
                shaders[(uint32_t)ShaderType::Geometry],
                shaders[(uint32_t)ShaderType::Hull],
                shaders[(uint32_t)ShaderType::Domain],
                log,
                getProgramDescString());
        }
    }


    bool Program::link() const
    {
        while(1)
        {
            // create the program
            std::string log;
            ProgramVersion::SharedConstPtr pProgram = preprocessAndCreateProgramVersion(log);

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
