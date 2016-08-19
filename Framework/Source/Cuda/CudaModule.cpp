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
#include "CudaModule.h"
#include <nvrtc_helper.h>
#include <vector_types.h>

namespace Falcor {
namespace Cuda {
	CudaModule::CudaModule(const std::string &filename) 
    {
		if(!mIncludePathVectorInitOK)
        {
			for (auto& dir : getDataDirectoriesList())
            {
				std::string optionLine = std::string("-I") + dir;
				mIncludePathVectorStr.push_back(optionLine);
			}

			for (auto& dir : mIncludePathVectorStr)
            {
				mIncludePathVector.push_back(dir.c_str());
			}

            mIncludePathVectorInitOK = true;
		}

		std::string fullpath;
		if (Falcor::findFileInDataDirectories(filename, fullpath) == false) 
        {
			Logger::log(Logger::Level::Fatal, std::string("Can't find CUDA file ") + filename);
			return;
		}

		char *ptx;
		size_t ptxSize;
		compileToPTX((char*)fullpath.c_str(), (int)mIncludePathVector.size(), mIncludePathVector.data(),	&ptx, &ptxSize);
        checkCudaErrors(cuModuleLoadDataEx(&mpModule, ptx, 0, 0, 0));
	}

	CudaModule::~CudaModule()
    {
	}

	CudaModule::SharedPtr CudaModule::create(const std::string &filename) 
    {
		CudaModule::SharedPtr cudaTex(new CudaModule(filename));
		return cudaTex;
	}

	void CudaModule::compileToPTX(char *filename, int argc, const char **argv,
		char **ptxResult, size_t *ptxResultSize)
	{
		std::ifstream inputFile(filename, std::ios::in | std::ios::binary |
			std::ios::ate);

		if (!inputFile.is_open())
		{
            Logger::log(Logger::Level::Fatal, std::string("Can't open CUDA file for reading") + filename);
			return;
		}

		std::streampos pos = inputFile.tellg();
		size_t inputSize = (size_t)pos;
		char * memBlock = new char[inputSize + 1];

		inputFile.seekg(0, std::ios::beg);
		inputFile.read(memBlock, inputSize);
		inputFile.close();
		memBlock[inputSize] = '\x0';

		// compile
		nvrtcProgram prog;
		NVRTC_SAFE_CALL("nvrtcCreateProgram", nvrtcCreateProgram(&prog, memBlock,
			filename, 0, NULL, NULL));
		nvrtcResult res = nvrtcCompileProgram(prog, argc, argv);

		// dump log
		size_t logSize;
		NVRTC_SAFE_CALL("nvrtcGetProgramLogSize", nvrtcGetProgramLogSize(prog, &logSize));
		char *log = (char *)malloc(sizeof(char) * logSize + 1);
		NVRTC_SAFE_CALL("nvrtcGetProgramLog", nvrtcGetProgramLog(prog, log));
		log[logSize] = '\x0';
        if(logSize > 1)
            Logger::log(Logger::Level::Fatal, std::string("Can't compile CUDA kernel ") + filename + "\n" + log);
		free(log);

		NVRTC_SAFE_CALL("nvrtcCompileProgram", res);
		// fetch PTX
		size_t ptxSize;
		NVRTC_SAFE_CALL("nvrtcGetPTXSize", nvrtcGetPTXSize(prog, &ptxSize));
		char *ptx = (char *)malloc(sizeof(char) * ptxSize);
		NVRTC_SAFE_CALL("nvrtcGetPTX", nvrtcGetPTX(prog, ptx));
		NVRTC_SAFE_CALL("nvrtcDestroyProgram", nvrtcDestroyProgram(&prog));
		*ptxResult = ptx;
		*ptxResultSize = ptxSize;
	}
}
}