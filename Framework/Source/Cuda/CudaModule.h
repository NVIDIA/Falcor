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

#include <Falcor.h>

#include "CudaContext.h"


#include <string>
#include <vector>
#include <cuda.h>
#include <cudaGL.h> //Driver API
#include <vector_types.h>
#include <helper_cuda_drvapi.h>


namespace Falcor {
namespace Cuda {


	/** Kernel abstraction for CUDA. 
		Encapsulates kernel loading at runtime and execution.
		Uses the CUDA driver API.
	*/
	class CudaModule : public std::enable_shared_from_this<CudaModule>
	{
    public:
        using SharedPtr = std::shared_ptr<CudaModule>;
        using SharedConstPtr = std::shared_ptr<const CudaModule>;

		~CudaModule();

		/** Construct a new CudaTexture from a registered OpenGL texture.
		*/
		static CudaModule::SharedPtr create(const std::string &filename);

		/** Launch a CUDA kernel contained in the module.
		*/
		template<typename ... Types>
		bool launchKernel(const std::string &kernelName, dim3 blockSize, dim3 gridSize, Types& ... params)
        {
			//Build arguments list
			mArgsVector.clear();
			buildArgsList(mArgsVector, params...);
			void **arr = (void **)mArgsVector.data();

			CUfunction kernel_addr;
			checkFalcorCudaErrors(cuModuleGetFunction(&kernel_addr, mpModule, kernelName.c_str()));

			checkFalcorCudaErrors(cuLaunchKernel(kernel_addr,
						gridSize.x, gridSize.y, gridSize.z,			/* grid dim */
						blockSize.x, blockSize.y, blockSize.z,		/* block dim */
						0, 0,										/* shared mem, stream */
						arr,										/* arguments */
						0));

			checkFalcorCudaErrors(cuCtxSynchronize());
			
			return true;
		}

		/** Return the CUDA module structure.
		*/
		CUmodule &get()    { return mpModule; }

		/** Return a (GPU) pointer to a global __constant__ or __device__ variable declared in the module.
		*/
		CUdeviceptr getGlobalVariablePtr(const std::string &varName, size_t &varSize)
        {
			CUdeviceptr ptr;
			checkFalcorCudaErrors( cuModuleGetGlobal(&ptr, &varSize, mpModule, varName.c_str()) );
			return ptr;
		}

    protected:
        CudaModule() { }

        CudaModule(const std::string &filename);

        static void compileToPTX(char *filename, int argc, const char **argv,
            char **ptxResult, size_t *ptxResultSize);

        //Build list of arguments using variadic parameters and recursion
        void buildArgsList(std::vector<void*> &argVector) { }

        template<typename T, typename ... Types>
        void buildArgsList(std::vector<void*> &argVector, T& first, Types& ... rest)
        {
            argVector.push_back((void *)&(first));
            buildArgsList(argVector, rest...);
        }

        CUmodule					mpModule = nullptr;

        std::vector<void*>		mArgsVector;

        bool						mIncludePathVectorInitOK = false;
        std::vector<std::string>	mIncludePathVectorStr;
        std::vector<const char*>	mIncludePathVector;
	};
}
}