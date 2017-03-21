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
#include "API/ProgramVersion.h"
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Utils/OS.h"
#include "../ConstantBuffer.h"
#include "../Buffer.h"
#include <fstream>

namespace Falcor
{
    void ProgramVersion::deleteApiHandle()
    {
        glDeleteProgram(mApiHandle);
    }

    bool ProgramVersion::init(std::string& log, const std::string& name)
    {
        mApiHandle = gl_call(glCreateProgram());

        // Attach all shaders
        for(uint32_t i = 0; i < arraysize(mpShaders); i++)
        {
            const Shader* pShader = mpShaders[i].get();
            if(pShader)
            {
                gl_call(glAttachShader(mApiHandle, pShader->getApiHandle<GLenum>()));
            }
        }

        // Link the program
        gl_call(glLinkProgram(mApiHandle));

        // Check for errors
        GLint success;
        gl_call(glGetProgramiv(mApiHandle, GL_LINK_STATUS, &success));
        if(success == 0)
        {
            GLint logSize;
            gl_call(glGetProgramiv(mApiHandle, GL_INFO_LOG_LENGTH, &logSize));
            log.resize(logSize + 1);
            gl_call(glGetProgramInfoLog(mApiHandle, logSize + 1, nullptr, &log[0]));
            return false;
        }

        return true;
    }

    ProgramHandle ProgramVersion::getApiHandle() const
    {
        return mApiHandle;
    }

    void ProgramVersion::dumpProgramBinaryToFile(const std::string& filename) const
    {
        std::ofstream outfile(filename);
        if(outfile.good() == false)
        {
            logError("ProgramVersion::dumpShaderAssembly() - can't open output file \"" + filename + "\"\n");
            return;
        }
        int32_t progSize;
        gl_call(glGetProgramiv(mApiHandle, GL_PROGRAM_BINARY_LENGTH, &progSize));
        std::string progBinary(progSize + 1, 0);
        GLenum binaryFormat;
        gl_call(glGetProgramBinary(mApiHandle, progSize, nullptr, &binaryFormat, (void*)progBinary.data()));
        outfile << progBinary;
        outfile.close();
    }
}

#endif //#ifdef FALCOR_GL
