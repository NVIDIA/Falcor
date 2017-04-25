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
#include <string>
#include "Graphics/Program.h"
#include "Utils/OS.h"

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    /** create a new shader from file. The shader will be processed using the shader pre-processor before creating the hardware object. See CShaderPreprocessor reference to see its supported directives.
    \param[in] filename Shader filename. It will search for the shader in the common directory structure. Shaders should usually be placed under Media/Shaders.
    \param[in] type Shader Type
    \param[in] shaderDefines A string containing macro definitions to be patched into the shaders. Defines are separated by newline.
    \return A pointer to a new object if compilation was successful, otherwise nullptr.
    In case of compilation error, a message box will appear with the log, allowing quick shader fixes without having to restart the program.
    */
    Shader::SharedPtr createShaderFromFile(const std::string& filename, ShaderType type, const Program::DefineList& shaderDefines = Program::DefineList());

    /** create a new shader from a string. The shader will be processed using the shader pre-processor before creating the hardware object. See CShaderPreprocessor reference to see its supported directives.
    \param[in] shaderString The shader.
    \param[in] type Shader Type
    \param[in] shaderDefines A string containing macro definitions to be patched into the shaders. Defines are separated by newline.
    \return A pointer to a new object if compilation was successful, otherwise nullptr.
    */
    Shader::SharedPtr createShaderFromString(const std::string& shaderString, ShaderType type, const Program::DefineList& shaderDefines = Program::DefineList());
       
    template<typename ObjectType, typename EnumType>
    typename ObjectType::SharedPtr createShaderFromFile(const std::string& filename, EnumType shaderType, const Program::DefineList& shaderDefines)
    {
        // New shader, look for the file
        std::string fullpath;
        if (findFileInDataDirectories(filename, fullpath) == false)
        {
            std::string err = std::string("Can't find shader file ") + filename;
            logError(err);
            return nullptr;
        }

        while (1)
        {
            // Open the file
            std::string shader;
            readFileToString(fullpath, shader);

            // Preprocess
            std::string errorMsg;
            Shader::unordered_string_set includeList;
            {
                // Preprocessing is good
                std::string errorLog;
                auto pShader = ObjectType::create(shader, shaderType, errorLog);

                if (pShader == nullptr)
                {
                    std::string error = std::string("Compilation of shader ") + filename + "\n\n";
                    error += errorLog;
                    MsgBoxButton mbButton = msgBox(error, MsgBoxType::RetryCancel);
                    if (mbButton == MsgBoxButton::Cancel)
                    {
                        logError(error);
                        exit(1);
                    }
                }
                else
                {
                    pShader->setIncludeList(includeList);
                    return pShader;
                }
            }
        }
    }
}