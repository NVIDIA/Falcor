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
#include "GraphicsProgram.h"

namespace Falcor
{
    GraphicsProgram::SharedPtr GraphicsProgram::createFromFile(const std::string& vertexFile, const std::string& fragmentFile, const Program::DefineList& programDefines)
    {
        return createFromFile(vertexFile, fragmentFile, "", "", "", programDefines);
    }

    GraphicsProgram::SharedPtr GraphicsProgram::createFromFile(const std::string& vertexFile, const std::string& fragmentFile, const std::string& geometryFile, const std::string& hullFile, const std::string& domainFile, const DefineList& programDefines)
    {
        SharedPtr pProg = SharedPtr(new GraphicsProgram);
        pProg->init(vertexFile, fragmentFile, geometryFile, hullFile, domainFile, programDefines, true);
        return pProg;
    }

    GraphicsProgram::SharedPtr GraphicsProgram::createFromString(const std::string& vertexShader, const std::string& fragmentShader, const DefineList& programDefines)
    {
        return createFromString(vertexShader, fragmentShader, "", "", "", programDefines);
    }

    GraphicsProgram::SharedPtr GraphicsProgram::createFromString(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::string& hullShader, const std::string& domainShader, const DefineList& programDefines)
    {
        SharedPtr pProg = SharedPtr(new GraphicsProgram);
        pProg->init(vertexShader, fragmentShader, geometryShader, hullShader, domainShader, programDefines, false);
        return pProg;
    }
}