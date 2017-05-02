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

        for(uint32_t i = 0; i < arraysize(mShaderStrings); i++)
        {
            const Shader* pShader = mpActiveProgram->getShader(ShaderType(i));
            if(pShader)
            {
                if(mCreatedFromFile)
                {
                    std::string fullpath;
                    findFileInDataDirectories(mShaderStrings[i], fullpath);
                    if(mFileTimeMap[fullpath] != getFileModifiedTime(fullpath))
                    {
                        return true;
                    }
                }

                // Loop over the shader's included files
                for(const auto& include : pShader->getIncludeList())
                {
                    if(mFileTimeMap[include] != getFileModifiedTime(include))
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

    ProgramVersion::SharedPtr Program::createProgramVersion(std::string& log) const
    {
        mFileTimeMap.clear();

        // Run all of the shaders through Spire, so that we can get final code,
        // reflection data, etc.
        //
        // Note that we provide all the shaders at once, so that automatically
        // generated bindings can be made consistent across the stages.

        Shader::SharedPtr shaders[kShaderCount] = {};
        ProgramReflection::SharedPtr pReflector;

        std::string translatedShaderStrings[kShaderCount];
        {
            // Create a Spire compilation context, and a sink for it to dump errors into.
            SpireCompilationContext* spireContext = spCreateCompilationContext(NULL);
            SpireDiagnosticSink* spireSink = spCreateDiagnosticSink(spireContext);

            // Add our media search paths as `#include` search paths for Spire.
            //
            // TODO: Spire should probably support a callback API for all file I/O,
            // rather than having us specify data directories to it...
            for (auto path : getDataDirectoriesList())
            {
                spAddSearchPath(spireContext, path.c_str());
            }

            // Pass any `#define` flags along to Spire, since we aren't doing our
            // own preprocessing any more.
            for(auto shaderDefine : mDefineList)
            {
                spAddPreprocessorDefine(spireContext, shaderDefine.first.c_str(), shaderDefine.second.c_str());
            }

            // Pick the right target based on the current graphics API
#if defined(FALCOR_GL)
            spSetCodeGenTarget(spireContext, SPIRE_GLSL);
            spAddPreprocessorDefine(spireContext, "FALCOR_GLSL", "1");
#elif defined(FALCOR_D3D11) || defined(FALCOR_D3D12)
            // Note: we could compile Spire directly to DXBC (by having Spire invoke the MS compiler for us,
            // but that path seems to have more issues at present, so let's just go to HLSL instead...)
            spSetCodeGenTarget(spireContext, SPIRE_HLSL);
            spAddPreprocessorDefine(spireContext, "FALCOR_HLSL", "1");
#else
#error unknown shader compilation target
#endif

            // Configure any flags for the Spire compilation step
            SpireCompileFlags spireFlags = 0;

            // Don't actually perform semantic checking: just pass through functions bodies to downstream compiler
            spireFlags |= SPIRE_COMPILE_FLAG_NO_CHECKING;
            spSetCompileFlags(spireContext, spireFlags);

            // Now lets add all our input shader code, one-by-one
            for (uint32_t i = 0; i < kShaderCount; i++)
            {
                if (!mShaderStrings[i].size())
                    continue;

                spAddTranslationUnit(spireContext, nullptr);

                if (mCreatedFromFile)
                {
                    std::string fullpath;
                    findFileInDataDirectories(mShaderStrings[i], fullpath);
                    spAddTranslationUnitSourceFile(spireContext, i, fullpath.c_str());
                }
                else
                {
                    spAddTranslationUnitSourceString(spireContext, i, "", mShaderStrings[i].c_str());
                }
            }

            SpireCompilationResult* result = spCompile(spireContext, spireSink);
            int diagnosticsSize = spGetDiagnosticOutput(spireSink, NULL, 0);
            if (diagnosticsSize != 0)
            {
                char* diagnostics = (char*)malloc(diagnosticsSize);
                spGetDiagnosticOutput(spireSink, diagnostics, diagnosticsSize);
                log += diagnostics;
                free(diagnostics);
            }
            if(spDiagnosticSinkHasAnyErrors(spireSink) != 0)
            {
                spDestroyDiagnosticSink(spireSink);
                spDestroyCompilationContext(spireContext);
                return nullptr;
            }

            // Extract the generated code for each stage
            for (uint32_t i = 0; i < kShaderCount; i++)
            {
                if (!mShaderStrings[i].size())
                    continue;

                translatedShaderStrings[i] = spGetTranslationUnitSource(result, int(i));
            }

            // Extract the reflection data
            pReflector = ProgramReflection::create(spire::ShaderReflection::get(result), log);

            spDestroyDiagnosticSink(spireSink);
            spDestroyCompilationContext(spireContext);
        }



        // create the shaders
        for (uint32_t i = 0; i < kShaderCount; i++)
        {
            if (translatedShaderStrings[i].size())
            {
                shaders[i] = createShaderFromString(translatedShaderStrings[i], ShaderType(i));

                if(shaders[i])
                {
                    // TODO(tfoley): Need to get dependency info from Spire...
#if 0
                    for(const auto& include : shaders[i]->getIncludeList())
                    {
                        mFileTimeMap[include] = getFileModifiedTime(include);
                    }
#endif
                }
            }           
        }


        if (shaders[(uint32_t)ShaderType::Compute])
        {
            return ProgramVersion::create(
                pReflector,
                shaders[(uint32_t)ShaderType::Compute], log, getProgramDescString());
        }
        else
        {
            return ProgramVersion::create(
                pReflector,
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
            ProgramVersion::SharedConstPtr pProgram = createProgramVersion(log);

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
