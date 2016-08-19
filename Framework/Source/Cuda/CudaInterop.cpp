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

#include "CudaInterop.h"
#include "CudaContext.h"

#include <helper_cuda_drvapi.h>

namespace Falcor {
namespace Cuda {
    Falcor::Cuda::CudaInterop& CudaInterop::get()
    {
        static CudaInterop instance;
        return instance;
    }

    std::shared_ptr<Cuda::CudaTexture> CudaInterop::getMappedCudaTexture(const Falcor::Texture::SharedConstPtr& tex)
    {
		if (mTextureMap.find((size_t)tex.get()) == mTextureMap.end())
        {
			CuInteropMapVal<Falcor::Texture, Cuda::CudaTexture> interopVal;
			GLenum glSizedFormat = getGlSizedFormat(tex->getFormat());
			GLenum glTarget = getGlTextureTarget((int)(tex->getType()));

			checkFalcorCudaErrors(
				cuGraphicsGLRegisterImage(&(interopVal.cudaGraphicsResource), tex->getApiHandle(), glTarget, CU_GRAPHICS_REGISTER_FLAGS_NONE));  //CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST

			mTextureMap[(size_t)tex.get()] = interopVal;
		}

        auto& resource = mTextureMap[(size_t)tex.get()];
		if (resource.pFalcorCudaGraphicsResource == nullptr)
        {
			resource.pFalcorCudaGraphicsResource = Cuda::CudaTexture::create(resource.cudaGraphicsResource);
		}

		return mTextureMap[(size_t)tex.get()].pFalcorCudaGraphicsResource;
	}

    void CudaInterop::unmapCudaTexture(const Falcor::Texture::SharedConstPtr& tex)
    {
		//mTextureMap.erase((size_t)tex.get());
		if (mTextureMap.find((size_t)tex.get()) != mTextureMap.end())
        {
			mTextureMap[(size_t)tex.get()].pFalcorCudaGraphicsResource = nullptr;  //force deletion
		}
	}

    std::shared_ptr<Cuda::CudaBuffer> CudaInterop::getMappedCudaBuffer(const Falcor::Buffer::SharedConstPtr& buff)
    {
		if (mBufferMap.find((size_t)buff.get()) == mBufferMap.end())
        {
			CuInteropMapVal<Falcor::Buffer, Cuda::CudaBuffer> interopVal;
			checkFalcorCudaErrors(cuGraphicsGLRegisterBuffer(&(interopVal.cudaGraphicsResource), buff->getApiHandle(), CU_GRAPHICS_REGISTER_FLAGS_NONE));
			mBufferMap[(size_t)buff.get()] = interopVal;
		}

		if (mBufferMap[(size_t)buff.get()].pFalcorCudaGraphicsResource == nullptr) 
        {
			mBufferMap[(size_t)buff.get()].pFalcorCudaGraphicsResource = Cuda::CudaBuffer::create(mBufferMap[(size_t)buff.get()].cudaGraphicsResource);
		}

		return mBufferMap[(size_t)buff.get()].pFalcorCudaGraphicsResource;
		////HERE////
	}

    void CudaInterop::unmapCudaBuffer(const Falcor::Buffer::SharedConstPtr& buff)
    {
		//mTextureMap.erase((size_t)tex.get());
		if (mBufferMap.find((size_t)buff.get()) != mBufferMap.end())
        {
			mBufferMap[(size_t)buff.get()].pFalcorCudaGraphicsResource = nullptr;  //force deletion
		}
	}
}}
