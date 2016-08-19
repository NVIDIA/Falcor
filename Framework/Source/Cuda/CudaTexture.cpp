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
#include "CudaTexture.h"

namespace Falcor {
namespace Cuda {

	CudaTexture::CudaTexture(CUgraphicsResource &cuGraphicsResource) 
    {
		cuGraphicsMapResources(1, &cuGraphicsResource, 0);
		mpCUGraphicsResource = cuGraphicsResource;

		cuGraphicsSubResourceGetMappedArray(&mpCUArray, cuGraphicsResource, 0, 0);

		memset(&mCUImgRessource, 0, sizeof(CUDA_RESOURCE_DESC));
		mCUImgRessource.resType = CUresourcetype::CU_RESOURCE_TYPE_ARRAY;
		mCUImgRessource.res.array.hArray = mpCUArray;
	}

	CudaTexture::~CudaTexture()
    {
		if (mpCUGraphicsResource != nullptr){
			if (mCUTexObjMap.size() > 0){
				//cuTexObjectDestroy(_cuTexObj);
				for (auto it = mCUTexObjMap.begin(); it != mCUTexObjMap.end(); ++it)
					cuTexObjectDestroy( it->second );
			}

			if (mIsCUSurfObjCreated)
				cuSurfObjectDestroy(mCUSurfObj);

			cuGraphicsUnmapResources(1, &mpCUGraphicsResource, 0);
		}
	}

	CudaTexture::SharedPtr CudaTexture::create(CUgraphicsResource &cuGraphicsResource)
    {
		CudaTexture::SharedPtr cudaTex(new CudaTexture(cuGraphicsResource));
		return cudaTex;
	}

	CUsurfObject &CudaTexture::getSurfaceObject()
    {
		if (!mIsCUSurfObjCreated)
        {
			cuSurfObjectCreate(&mCUSurfObj, &mCUImgRessource);
			mIsCUSurfObjCreated = true;
		}

		return mCUSurfObj;
	}

	CUtexObject &CudaTexture::getTextureObject(bool accessUseNormalizedCoords, bool linearFiltering, Sampler::AddressMode addressMode)
    {
		uint32_t hashVal = hashCUTexObject(accessUseNormalizedCoords, linearFiltering, addressMode);
		
		if ( mCUTexObjMap.find(hashVal) == mCUTexObjMap.end() )
        {
			CUDA_TEXTURE_DESC		cu_texDescr;
			memset(&cu_texDescr, 0, sizeof(CUDA_TEXTURE_DESC));

			cu_texDescr.flags = accessUseNormalizedCoords ? CU_TRSF_NORMALIZED_COORDINATES : 0;

			cu_texDescr.filterMode = linearFiltering ? CU_TR_FILTER_MODE_LINEAR : CU_TR_FILTER_MODE_POINT;  //cudaFilterModeLinear

			CUaddress_mode addressmode = CU_TR_ADDRESS_MODE_CLAMP;
			switch (addressMode){
			case Sampler::AddressMode::Clamp:
				addressmode = CU_TR_ADDRESS_MODE_CLAMP;
				break;
			case Sampler::AddressMode::Wrap:
				addressmode = CU_TR_ADDRESS_MODE_WRAP;
				break;
			case Sampler::AddressMode::Mirror:
				addressmode = CU_TR_ADDRESS_MODE_MIRROR;
				break;
			case Sampler::AddressMode::Border:
				addressmode = CU_TR_ADDRESS_MODE_BORDER;
				break;

			default:
				assert(!"MirrorClampToEdge address mode unsupported !");
				break;
			}

			cu_texDescr.addressMode[0] = addressmode;
			cu_texDescr.addressMode[1] = addressmode;
			cu_texDescr.addressMode[2] = addressmode;

			CUtexObject	cuTexObj;
			cuTexObjectCreate(&cuTexObj, &mCUImgRessource, &cu_texDescr, NULL);

			mCUTexObjMap[hashVal] = cuTexObj;
		}

		return mCUTexObjMap[hashVal];
	}
}
}