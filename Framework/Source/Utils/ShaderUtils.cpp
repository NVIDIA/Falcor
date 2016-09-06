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
#include <vector>
#include "Utils/ShaderPreprocessor.h"
#include "Core/Shader.h"
#include "Utils/OS.h"
#include <fstream>
#include <sstream>
#include "Utils/Process.h"

namespace Falcor
{
    const std::string getShaderNameFromType(ShaderType type)
    {
        switch(type)
        {
        case ShaderType::Vertex:
            return "Vertex";
        case ShaderType::Fragment:
            return "Pixel";
        case ShaderType::Hull:
            return "Hull";
        case ShaderType::Domain:
            return "Domain";
        case ShaderType::Geometry:
            return "Geometry";
        case ShaderType::Compute:
            return "Compute";
        default:
            should_not_get_here();
            return "";
        }
    }

    bool glslToHlslShader(std::string& shader)
    {
        const char* compiler = "crosscompiler.exe";
        std::string cmdLine;
        if(findFileInDataDirectories(compiler, cmdLine) == false)
        {
            Logger::log(Logger::Level::Fatal, "can't find the cross compiler. Quiting.");
            exit(1);
        }
        std::ofstream o("shader.glsl");
        o << shader;
        o.close();

        cmdLine += " -o shader.hlsl shader.glsl";
        std::unique_ptr<Process> pProcess = std::make_unique<Process>(cmdLine);
        if(pProcess == nullptr)
        {
            Logger::log(Logger::Level::Fatal, "can't start the cross compiler. Quiting.");
            exit(1);
        }

        if(pProcess->getRetVal() != 0)
        {
            std::string err;
            pProcess->readStdErr(err);
            Logger::log(Logger::Level::Error, "Error when trying to convert glsl to hlsl\n" + err);
            return false;
        }

        std::ifstream i("shader.hlsl");
        std::stringstream ss;
        ss << i.rdbuf();
        shader = std::move(ss.str());

        // delete the files
        std::remove("shader.hlsl");
        std::remove("shader.glsl");

        return true;
    }

    const Shader::SharedPtr createShaderFromString(const std::string& shaderString, ShaderType shaderType, const Program::DefineList& shaderDefines)
    {
        std::string shader = shaderString;
        std::string errorMsg;

        if(ShaderPreprocessor::parseShader("", shader, errorMsg, shaderDefines) == false)
        {
            std::string msg = std::string("Error when parsing shader from string. Code:\n") + shaderString + "\nError:\n" + errorMsg;
            Logger::log(Logger::Level::Fatal, msg);
            return nullptr;
        }
#ifdef FALCOR_DX11
        if(glslToHlslShader(shader) == false)
        {
            return nullptr;
        }
#endif
        std::string log;
        auto pShader = Shader::create(shader, shaderType, log);
        if(pShader == nullptr)
        {
            std::string msg = "Error when creating " + getShaderNameFromType(shaderType) + " shader from string\nError log:\n";
            msg += log;
            msg += "\nShader string:\n" + shaderString + "\n";
            Logger::log(Logger::Level::Fatal, msg);
        }
        return pShader;
    }

    const Shader::SharedPtr createShaderFromFile(const std::string& filename, ShaderType shaderType, const Program::DefineList& shaderDefines)
    {
        // New shader, look for the file
        std::string fullpath;
        if(findFileInDataDirectories(filename, fullpath) == false)
        {
            std::string err = std::string("Can't find shader file ") + filename;
            Logger::log(Logger::Level::Fatal, err);
            return nullptr;
        }

        while(1)
        {
            // Open the file
            std::string shader;
            readFileToString(fullpath, shader);

            // Preprocess
            std::string errorMsg;
            if(ShaderPreprocessor::parseShader(fullpath, shader, errorMsg, shaderDefines) == false)
            {
                std::string msg = std::string("Error when pre-processing shader ") + filename + "\n" + errorMsg;
                if(msgBox(msg, MsgBoxType::RetryCancel) == MsgBoxButton::Cancel)
                {
                    Logger::log(Logger::Level::Fatal, msg);
                    return nullptr;
                }
            }
            else
            {
                // Preprocessing is good
#ifdef FALCOR_DX11
                if(glslToHlslShader(shader) == false)
                {
                    if(msgBox("retry cross-compilation?", MsgBoxType::RetryCancel) == MsgBoxButton::Cancel)
                    {
                        return nullptr;
                    }
                    continue;
                }
#endif
                std::string errorLog;
                auto pShader = Shader::create(shader, shaderType, errorLog);

                if(pShader == nullptr)
                {
                    std::string error = std::string("Compilation of shader ") + filename + "\n\n";
                    error += errorLog;
                    MsgBoxButton mbButton = msgBox(error, MsgBoxType::RetryCancel);
                    if(mbButton == MsgBoxButton::Cancel)
                    {
                        Logger::log(Logger::Level::Fatal, error);
                        exit(1);
                    }
                }
                else
                {
                    return pShader;
                }
            }
        }
    }
}
