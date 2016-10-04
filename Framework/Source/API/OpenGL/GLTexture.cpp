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
#include "API/Texture.h"
#include "API/Sampler.h"
#include "API/Window.h"
#include "Utils/Bitmap.h"

namespace Falcor
{
    GLenum convertTexTypeToGL(Texture::Type type, uint32_t arraySize)
    {
        bool isArray = arraySize > 1;
        switch(type)
        {
        case Texture::Type::Texture1D:
            return isArray ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D;
        case Texture::Type::Texture2D:
            return isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        case Texture::Type::Texture3D:
            return GL_TEXTURE_3D;
        case Texture::Type::TextureCube:
            return isArray ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
        case Texture::Type::Texture2DMultisample:
            return isArray ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    Texture::~Texture()
    {
        for(const auto& a : mBindlessTextureHandle)
        {
            glMakeTextureHandleNonResidentARB(a.second);
        }

        glDeleteTextures(1, &mApiHandle);
    }
        
    uint64_t Texture::makeResident(const Sampler* pSampler) const
    {
        uint32_t samplerID = pSampler ? pSampler->getApiHandle() : 0;

        const auto& handleIt = mBindlessTextureHandle.find(samplerID);
        if(handleIt == mBindlessTextureHandle.end())
        {
            // New sampler
            uint64_t handle;
            if(samplerID != 0)
            {
                handle = gl_call(glGetTextureSamplerHandleARB(mApiHandle, samplerID));
            }
            else
            {
                handle = gl_call(glGetTextureHandleARB(mApiHandle));
            }
            gl_call(glMakeTextureHandleResidentARB(handle));
            mBindlessTextureHandle[samplerID] = handle;
            return handle;
        }
        return handleIt->second;
    }

    void Texture::evict(const Sampler* pSampler) const
    {
        uint32_t SamplerID = pSampler ? pSampler->getApiHandle() : 0;

        const auto& handleIt = mBindlessTextureHandle.find(SamplerID);
        if(handleIt == mBindlessTextureHandle.end())
        {
            return;
        }
        gl_call(glMakeTextureHandleNonResidentARB(handleIt->second));
        mBindlessTextureHandle.erase(handleIt);
    }

	uint32_t init1DTexture(GLenum Type, uint32_t width, ResourceFormat format, uint32_t mipLevels, const void* pData, bool autoGenerateMipMaps)
    {
        uint32_t apiHandle;
        gl_call(glCreateTextures(Type, 1, &apiHandle));

        GLenum glFormat = getGlSizedFormat(format);
        gl_call(glTextureStorage1D(apiHandle, mipLevels, glFormat, width));
        if(pData)
        { 				
			gl_call(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

			GLenum baseFormat = getGlBaseFormat(format);
			GLenum baseType = getGlFormatType(format);
			gl_call(glTextureSubImage1D(apiHandle, 0, 0, width, baseFormat, baseType, pData));
			
			if (autoGenerateMipMaps)
			{	
				gl_call(glGenerateTextureMipmap(apiHandle));
			}
			else
			{
				uint8_t *data = (uint8_t*)pData;
				for (uint32_t i = 1; i < mipLevels; ++i) 
				{
					data += getFormatBytesPerBlock(format) * (width >> (i - 1)) / getFormatPixelsPerBlock(format);
					gl_call(glTextureSubImage1D(apiHandle, i, 0, width, baseFormat, baseType, data));
				}
			}
        }

        return apiHandle;
    }

    uint32_t init2DTextureStorage(GLenum Type, uint32_t width, uint32_t height, ResourceFormat format, uint32_t mipLevels)
    {
        uint32_t apiHandle;
        gl_call(glCreateTextures(Type, 1, &apiHandle));
        GLenum glFormat = getGlSizedFormat(format);
        gl_call(glTextureStorage2D(apiHandle, mipLevels, glFormat, width, height));
        return apiHandle;
    }

    void init2DTextureData(uint32_t apiHandle, uint32_t width, uint32_t height, ResourceFormat format, uint32_t mipLevels, const void* pData, bool autoGenerateMipMaps)
    {
        if(pData)
        {
            gl_call(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

            if(isCompressedFormat(format))
            {
				GLenum glFormat = getGlSizedFormat(format);
				uint32_t requiredSize;
				uint8_t *data = (uint8_t*)pData;
				for (uint32_t i = 0; i < mipLevels; ++i) 
				{
					gl_call(glGetTextureLevelParameteriv(apiHandle, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (int*)&requiredSize));
					glCompressedTextureSubImage2D(apiHandle, i, 0, 0, width, height, glFormat, requiredSize, data);
					uint32_t offset = getFormatBytesPerBlock(format) * (width >> i) * (height >> i) / getFormatPixelsPerBlock(format);
					data += offset;
					if (autoGenerateMipMaps)
					{
						break;
					}
				}
            }
            else
            {
				GLenum baseFormat = getGlBaseFormat(format);
				GLenum baseType = getGlFormatType(format);
				uint8_t *data = (uint8_t*)pData;
				for (uint32_t i = 0; i < mipLevels; ++i)
				{
					gl_call(glTextureSubImage2D(apiHandle, i, 0, 0, width, height, baseFormat, baseType, data));
					uint32_t offset = getFormatBytesPerBlock(format) * (width >> i) * (height >> i) / getFormatPixelsPerBlock(format);
					data += offset;
					if (autoGenerateMipMaps)
					{
						break;
					}
				}
            }

            if(autoGenerateMipMaps)
            {
                gl_call(glGenerateTextureMipmap(apiHandle));
            }
        }
    }

    uint32_t init2DTexture(GLenum Type, uint32_t width, uint32_t height, ResourceFormat format, uint32_t mipLevels, const void* pData, ResourceFormat dataFormat, bool autoGenerateMipMaps)
    {
        uint32_t apiHandle = init2DTextureStorage(Type, width, height, format, mipLevels);
		init2DTextureData(apiHandle, width, height, dataFormat, mipLevels, pData, autoGenerateMipMaps);
        return apiHandle;
    }

    uint32_t init3DTextureStorage(GLenum Type, uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels, bool isSparse = false)
    {
        uint32_t apiHandle;
        gl_call(glCreateTextures(Type, 1, &apiHandle));

		if (isSparse){
			assert(Falcor::checkExtensionSupport("GL_ARB_sparse_texture"));
			
			gl_call(glTextureParameteri(apiHandle, GL_TEXTURE_SPARSE_ARB, GL_TRUE));
		}

        GLenum glFormat = getGlSizedFormat(format);
        gl_call(glTextureStorage3D(apiHandle, mipLevels, glFormat, width, height, depth));

        return apiHandle;
    }


	void init3DTextureData(uint32_t apiHandle, uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels, const void* pData, bool autoGenerateMipMaps)
    {
        if(pData)
        {
            gl_call(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
			if (isCompressedFormat(format))
			{
				GLenum glFormat = getGlSizedFormat(format);
				uint32_t requiredSize;
				uint8_t *data = (uint8_t*)pData;
				for (uint32_t i = 0; i < mipLevels; ++i)
				{
					gl_call(glGetTextureLevelParameteriv(apiHandle, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (int*)&requiredSize));
					glCompressedTextureSubImage3D(apiHandle, i, 0, 0, 0, width, height, depth, glFormat, requiredSize, data);
					data += getFormatBytesPerBlock(format) * (width >> i) * (height >> i) * (depth >> i) / getFormatPixelsPerBlock(format);
					if (autoGenerateMipMaps) 
					{
						break;
					}
				}

			}
			else
			{
				GLenum baseFormat = getGlBaseFormat(format);
				GLenum baseType = getGlFormatType(format);
				uint8_t *data = (uint8_t*)pData;
				for (uint32_t i = 0; i < mipLevels; ++i)
				{
					gl_call(glTextureSubImage3D(apiHandle, i, 0, 0, 0, width, height, depth, baseFormat, baseType, data));
					data += getFormatBytesPerBlock(format) * (width >> i) * (height >> i) * (depth >> i) / getFormatPixelsPerBlock(format);
					if (autoGenerateMipMaps)
					{
						break;
					}
				}
			}
            if(autoGenerateMipMaps)
            {
                gl_call(glGenerateTextureMipmap(apiHandle));
            }
        }
    }

    uint32_t init3DTexture(GLenum Type, uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels, const void* pData, bool autoGenerateMipMaps, bool isSparse=false)
    {
        uint32_t apiHandle = init3DTextureStorage(Type, width, height, depth, format, mipLevels, isSparse);
		init3DTextureData(apiHandle, width, height, depth, format, mipLevels, pData, autoGenerateMipMaps);
        return apiHandle;
    }

    uint32_t init2DMultisample(uint32_t width, uint32_t height, ResourceFormat format, uint32_t sampleCount, bool useFixedSampleLocations)
    {
        uint32_t apiHandle;
        gl_call(glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &apiHandle));

        GLenum glFormat = getGlSizedFormat(format);
        gl_call(glTextureStorage2DMultisample(apiHandle, sampleCount, glFormat, width, height, useFixedSampleLocations ? GL_TRUE : GL_FALSE));
        return apiHandle;
    }

    uint32_t init2DMultisampleArray(uint32_t width, uint32_t height, uint32_t arraySize, ResourceFormat format, uint32_t sampleCount, bool useFixedSampleLocations)
    {
        uint32_t apiHandle;
        gl_call(glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, &apiHandle));

        GLenum glFormat = getGlSizedFormat(format);
        gl_call(glTextureStorage3DMultisample(apiHandle, sampleCount, glFormat, width, height, arraySize, useFixedSampleLocations ? GL_TRUE : GL_FALSE));
        return apiHandle;
    }

    Texture::SharedPtr Texture::create1D(uint32_t width, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData)
    {
        auto pResource = SharedPtr(new Texture(width, 1, 1, arraySize, mipLevels, 1, format, Texture::Type::Texture1D));
        if(pResource->mArraySize > 1)
        {
            pResource->mApiHandle = init2DTexture(GL_TEXTURE_1D_ARRAY, pResource->mWidth, pResource->mArraySize, pResource->mFormat, pResource->mMipLevels, pData, format, mipLevels == kEntireMipChain);
        }
        else
        {
            pResource->mApiHandle = init1DTexture(GL_TEXTURE_1D, pResource->mWidth, pResource->mFormat, pResource->mMipLevels, pData, mipLevels == kEntireMipChain);
        }
    
        return pResource;
    }
    
    Texture::SharedPtr Texture::create2D(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData)
    {
        auto pResource = SharedPtr(new Texture(width, height, 1, arraySize, mipLevels, 1, format, Texture::Type::Texture2D));

        if(pResource->mArraySize > 1)
        {
            pResource->mApiHandle = init3DTexture(GL_TEXTURE_2D_ARRAY, pResource->mWidth, pResource->mHeight, pResource->mArraySize, pResource->mFormat, pResource->mMipLevels, pData, mipLevels == kEntireMipChain);
        }
        else
        {
            pResource->mApiHandle = init2DTexture(GL_TEXTURE_2D, pResource->mWidth, pResource->mHeight, pResource->mFormat, pResource->mMipLevels, pData, format, mipLevels == kEntireMipChain);
        }

        return pResource;
    }

    Texture::SharedPtr Texture::create3D(uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels, const void* pData, bool isSparse)
    {
        auto pResource = SharedPtr(new Texture(width, height, depth, 1, mipLevels, 1, format, Texture::Type::Texture3D));

		pResource->mIsSparse = isSparse;
        pResource->mApiHandle = init3DTexture(GL_TEXTURE_3D, pResource->mWidth, pResource->mHeight, pResource->mDepth, pResource->mFormat, pResource->mMipLevels, pData, mipLevels == kEntireMipChain, isSparse);
    
        return pResource;
    }

    // Texture Cube
    Texture::SharedPtr Texture::createCube(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize, uint32_t mipLevels, const void* pData)
    {
        auto pResource = SharedPtr(new Texture(width, height, 1, arraySize, mipLevels, 1, format, Texture::Type::TextureCube));

        if(pResource->mArraySize > 1)
        {
            pResource->mApiHandle = init3DTextureStorage(
                GL_TEXTURE_CUBE_MAP_ARRAY,
                pResource->mWidth,
                pResource->mHeight,
                6 * pResource->mArraySize,
                pResource->mFormat,
                pResource->mMipLevels);
        }
        else
        {
            pResource->mApiHandle = init2DTextureStorage(GL_TEXTURE_CUBE_MAP, pResource->mWidth, pResource->mHeight, pResource->mFormat, pResource->mMipLevels);
        }
        init3DTextureData(
            pResource->mApiHandle,
            pResource->mWidth,
            pResource->mHeight,
            6 * pResource->mArraySize,
            pResource->mFormat,
            pResource->mMipLevels, 
            pData,
			mipLevels == kEntireMipChain);

        return pResource;
    }

    Texture::SharedPtr Texture::create2DMS(uint32_t width, uint32_t height, ResourceFormat format, uint32_t sampleCount, uint32_t arraySize, bool useFixedSampleLocations)
    {
        if(sampleCount <= 1)
        {
            Logger::log(Logger::Level::Warning, "Creating Texture2DMS with a single sample. Are you sure that's what you meant? Why not create a Texture2D?");
        }
        auto pResource = SharedPtr(new Texture(width, height, 1, arraySize, 1, sampleCount, format, Texture::Type::Texture2DMultisample));
        pResource->mHasFixedSampleLocations = useFixedSampleLocations;

        if(pResource->mArraySize > 1)
        {
            pResource->mApiHandle = init2DMultisampleArray(pResource->mWidth, pResource->mHeight, pResource->mArraySize, pResource->mFormat, sampleCount, pResource->mHasFixedSampleLocations);
        }
        else
        {
            pResource->mApiHandle = init2DMultisample(pResource->mWidth, pResource->mHeight, pResource->mFormat, sampleCount, pResource->mHasFixedSampleLocations);
        }
  
        return pResource;
    }

    Texture::SharedPtr Texture::create2DFromView(uint32_t apiHandle, uint32_t width, uint32_t height, ResourceFormat format)
    {
        auto pResource = SharedPtr(new Texture(width, height, 1u, 1u, 1u, 1u, format, Texture::Type::Texture2D));
        pResource->mApiHandle = apiHandle;
        return pResource;
    }

    void Texture::getMipLevelImageSize(uint32_t mipLevel, uint32_t& width, uint32_t& height, uint32_t& depth) const
    {
        if (mipLevel >= mMipLevels)
        {
            Logger::log(Logger::Level::Error, "Texture::getMipLevelImageSize() - Requested mip level " + std::to_string(mipLevel) + " is out-of-bound. Texture has " + std::to_string(mMipLevels) + " mip-levels.");
        }

        width = mWidth;
        height = mHeight;
		depth = mDepth;

        for (auto mLevelIter = 0u; mLevelIter < mipLevel; ++mLevelIter)
        {
            width = max(1U, width >> 1);
            height = max(1U, height >> 1);
			depth = max(1U, depth >> 1);
        }
    }

    uint32_t Texture::getMipLevelDataSize(uint32_t mipLevel) const
    {
        if(mipLevel >= mMipLevels)
        {
            Logger::log(Logger::Level::Error, "Texture::getMipLevelDataSize() - Requested mip level " + std::to_string(mipLevel) + " is out-of-bound. Texture has " + std::to_string(mMipLevels) + " mip-levels.");
            return 0;
        }

        uint32_t requiredSize;
        if(isCompressedFormat(mFormat))
        {
            gl_call(glGetTextureLevelParameteriv(mApiHandle, mipLevel, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (int*)&requiredSize));
            requiredSize /= mArraySize;
        }
        else
        {
            uint32_t width = mWidth;
            uint32_t height = mHeight;
			uint32_t depth = mDepth;

            while(mipLevel)
            {
                width = max(1U, width >> 1);
                height = max(1U, height >> 1);
				depth = max(1U, depth >> 1);

                mipLevel--;
            }

            requiredSize = width * height * depth * getFormatBytesPerBlock(mFormat)/getFormatPixelsPerBlock(mFormat);
        }

        return requiredSize;
    }

    void Texture::readSubresourceData(void* pData, uint32_t dataSize, uint32_t mipLevel, uint32_t arraySlice) const
    {
        if(mipLevel >= mMipLevels)
        {
            Logger::log(Logger::Level::Error, "Texture::readSubresourceData() - Requested mip level " + std::to_string(mipLevel) + " is out-of-bound. Texture has " + std::to_string(mMipLevels) + "mip-levels. Ignoring call.");
            return;
        }

        if(arraySlice >= mArraySize)
        {
            Logger::log(Logger::Level::Error, "Texture::readSubresourceData() - Requested array slice " + std::to_string(arraySlice) + " is out-of-bound. Texture has " + std::to_string(mArraySize) + "array slices. Ignoring call.");
            return;
        }

        // Check that there is enough data in the buffer.
        // We check for equality, since we want to make sure that the user understands what he is doing
#if _LOG_ENABLED
        if(dataSize != getMipLevelDataSize(mipLevel))
        {
            Logger::log(Logger::Level::Error, "Error when reading texture data. Buffer size should be equal to Texture::getMipLevelDataSize(). Ignoring call.");
            return;
        }
#endif

        // The opengl functions return the entire mip-slice, but our function works on a single array slice.
        // If the array size is larger than 1, allocate temporary storage, then copy the data from it to the user's buffer
        std::vector<uint8_t> data(0);
        void* pTempData = pData;
        if(mArraySize > 1)
        {
            data.resize(dataSize);
            pTempData = data.data();
        }

        gl_call(glPixelStorei(GL_PACK_ALIGNMENT, 1));

        if(isCompressedFormat(mFormat))
        {
            // Compressed formats return the entire 
            gl_call(glGetCompressedTextureImage(mApiHandle, mipLevel, dataSize * mArraySize, pTempData));
        }
        else
        {
			GLenum glFormat = getGlBaseFormat(mFormat);
			GLenum glType  = getGlFormatType(mFormat);
            gl_call(glGetTextureImage(mApiHandle, mipLevel, glFormat, glType, dataSize*mArraySize, pTempData));
        }
        
        // If we allocated temporary storage, copy the data to the user's buffer and release the memory
        if(pTempData != pData)
        {
            uint32_t offset = dataSize * arraySlice;
            uint8_t* pSrc = ((uint8_t*)pTempData) + offset;
            memcpy(pData, pSrc, dataSize);
        }
    }

    void Texture::uploadSubresourceData(const void* pData, uint32_t dataSize, uint32_t mipLevel, uint32_t arraySlice)
    {
        if(mipLevel >= mMipLevels)
        {
            Logger::log(Logger::Level::Error, "Texture::uploadSubresourceData() - Requested mip level " + std::to_string(mipLevel) + " is out-of-bound. Texture has " + std::to_string(mMipLevels) + "mip-levels. Ignoring call.");
            return;
        }

        if(arraySlice >= mArraySize)
        {
            Logger::log(Logger::Level::Error, "Texture::uploadSubresourceData() - Requested array slice " + std::to_string(arraySlice) + " is out-of-bound. Texture has " + std::to_string(mArraySize) + "array slices. Ignoring call.");
            return;
        }

        // Check that there is enough data in the buffer.
        // We check for equality, since we want to make sure that the user understands what he is doing
#if _LOG_ENABLED
        if(dataSize != getMipLevelDataSize(mipLevel))
        {
            Logger::log(Logger::Level::Error, "Error when uploading texture data. Buffer size should be equal to Texture::getMipLevelDataSize(). Ignoring call.");
            return;
        }
#endif

        if( (mType != Type::Texture2D) && (mType != Type::Texture3D) )
        {
            Logger::log(Logger::Level::Error, "Texture::uploadSubresourceData() - Only 2D textures are supported for now.");
            return;
        }

        if(!pData)
        {
            Logger::log(Logger::Level::Error, "Texture::uploadSubresourceData() - Data is not provided.");
            return;
        }

        gl_call(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        uint32_t width, height, depth;

        getMipLevelImageSize(mipLevel, width, height, depth);

		if (isCompressedFormat(mFormat))
		{
			GLenum glFormat = getGlSizedFormat(mFormat);
			uint32_t requiredSize;
			gl_call(glGetTextureLevelParameteriv(mApiHandle, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (int*)&requiredSize));

			if (mType == Type::Texture3D)
			{
				gl_call(glCompressedTextureSubImage3D(mApiHandle, mipLevel, 0, 0, 0, width, height, depth, glFormat, requiredSize, pData));
			}
			else
			{
				gl_call(glCompressedTextureSubImage2D(mApiHandle, mipLevel, 0, 0, width, height, glFormat, requiredSize, pData));
			}
		}
		else
		{
			GLenum baseFormat = getGlBaseFormat(mFormat);
			GLenum baseType = getGlFormatType(mFormat);
			if (mType == Type::Texture3D)
			{
				gl_call(glTextureSubImage3D(mApiHandle, mipLevel, 0, 0, 0, width, height, depth, baseFormat, baseType, pData));
			}
			else
			{
				gl_call(glTextureSubImage2D(mApiHandle, mipLevel, 0, 0, width, height, baseFormat, baseType, pData));
			}
		}
    }

    void Texture::captureToPng(uint32_t mipLevel, uint32_t arraySlice, const std::string& filename) const
    {
        auto dataSize = getMipLevelDataSize(mipLevel);
        uint32_t bytesPerPixel = getFormatBytesPerBlock(mFormat);

        uint32_t mipWidth, mipHeight;
        getMipLevelImageSize(mipLevel, mipWidth, mipHeight);

        uint32_t bufferSize = mipWidth * mipHeight * bytesPerPixel;
        std::vector<uint8_t> data(bufferSize);

        readSubresourceData(data.data(), bufferSize, mipLevel, arraySlice);

        Bitmap::saveImage(filename, mipWidth, mipHeight, Bitmap::FileFormat::PngFile, bytesPerPixel, false, data.data());

    }

    void Texture::compress2DTexture()
    {
        if(mType != Type::Texture2D)
        {
            Logger::log(Logger::Level::Error, "Texture::compress2DTexture() only supports 2D texture compression\n");
            return;
        }

        if(mArraySize > 1)
        {
            Logger::log(Logger::Level::Error, "Texture::compress2DTexture() only supports 2D texture compression with a single array slice\n");
            return;
        }

        if(isDepthStencilFormat(mFormat))
        {
            Logger::log(Logger::Level::Error, "Texture::compress2DTexture(): Can't compress depth-stencil resource\n");
            return;
        }

        if(isCompressedFormat(mFormat))
        {
            // Already compressed
            return;
        }

        // Read the old data
        std::vector<uint8_t> data;
        uint32_t requiredSize = getMipLevelDataSize(0);
        data.resize(requiredSize);

        readSubresourceData(data.data(), requiredSize, 0, 0);

        // Select format
        ResourceFormat compressedFormat = ResourceFormat::Unknown;
        bool isSrgb = isSrgbFormat(mFormat);

        switch(getFormatChannelCount(mFormat))
        {
        case 1:
            assert(isSrgb == false);
            compressedFormat = ResourceFormat::BC4Unorm;
            break;
        case 2:
            assert(isSrgb == false);
            compressedFormat = ResourceFormat::BC5Unorm;
            break;
        case 3:
            compressedFormat = isSrgb ? ResourceFormat::BC1UnormSrgb : ResourceFormat::BC1Unorm;
            break;
        case 4:
            compressedFormat = isSrgb ? ResourceFormat::BC3UnormSrgb : ResourceFormat::BC3Unorm;
            break;
        default:
            should_not_get_here();
        }

        // Delete the old resource
        gl_call(glDeleteTextures(1, &mApiHandle));
        
        // create a new texture
        mApiHandle = init2DTexture(GL_TEXTURE_2D, mWidth, mHeight, compressedFormat, mMipLevels, data.data(), mFormat, true);
        mFormat = compressedFormat;
    }

	void Texture::generateMips() const
	{
		if(getMipCount() <= 1)
		{
			Logger::log(Logger::Level::Error, "Texture::generateMips() - no mip levels to generate.");
			return;
		}
		gl_call(glGenerateTextureMipmap(getApiHandle()));
	}

    Texture::SharedPtr Texture::createView(uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount) const
    {
        if(firstArraySlice >= mArraySize)
        {
            Logger::log(Logger::Level::Error, "Texture::createView() - firstArraySlice larger than texture array size");
            return nullptr;
        }

        if(firstArraySlice + arraySize > mArraySize)
        {
            Logger::log(Logger::Level::Error, "Texture::createView() - (firstArraySlice + arraySize) larger than texture array size");
            return nullptr;
        }

        if(mostDetailedMip >= mMipLevels)
        {
            Logger::log(Logger::Level::Error, "Texture::createView() - mostDetailedMip larger than texture mip levels");
            return nullptr;
        }

        if(mostDetailedMip + mipCount > mMipLevels)
        {
            Logger::log(Logger::Level::Error, "Texture::createView() - (mostDetailedMip + mpiCount) larger than texture mip levels");
            return nullptr;
        }

        SharedPtr pTexture = SharedPtr(new Texture(mWidth, mHeight, mDepth, arraySize, mMipLevels, mSampleCount, mFormat, mType));
        gl_call(glGenTextures(1, &pTexture->mApiHandle));
        gl_call(glTextureView(pTexture->mApiHandle, convertTexTypeToGL(mType, arraySize), mApiHandle, getGlSizedFormat(mFormat), mostDetailedMip, mipCount, firstArraySlice, arraySize));

        return pTexture;
    }

    void Texture::copySubresource(const Texture* pDst, uint32_t srcMipLevel, uint32_t srcArraySlice, uint32_t dstMipLevel, uint32_t dstArraySlice) const
    {
        gl_call(glCopyImageSubData(mApiHandle, convertTexTypeToGL(mType, mArraySize), srcMipLevel, 0, 0, srcArraySlice, pDst->mApiHandle, convertTexTypeToGL(pDst->mType, pDst->mArraySize), dstMipLevel, 0, 0, dstArraySlice, mWidth, mHeight, mDepth));
    }


	void Texture::setSparseResidencyPageIndex(bool isResident, uint32_t mipLevel,  uint32_t pageX, uint32_t pageY, uint32_t pageZ, uint32_t width, uint32_t height, uint32_t depth)
	{
		assert(mIsSparse);

		if(mSparsePageWidth==0)
		{
			glGetInternalformativ(GL_TEXTURE_3D, getGlSizedFormat(mFormat), GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &mSparsePageWidth);
			glGetInternalformativ(GL_TEXTURE_3D, getGlSizedFormat(mFormat), GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &mSparsePageHeight);
			glGetInternalformativ(GL_TEXTURE_3D, getGlSizedFormat(mFormat), GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &mSparsePageDepth);
		}
		

		glTexturePageCommitmentEXT(getApiHandle(), mipLevel, static_cast<GLsizei>(mSparsePageWidth * pageX), static_cast<GLsizei>(mSparsePageHeight * pageY), static_cast<GLsizei>(mSparsePageDepth * pageZ),
			static_cast<GLsizei>(mSparsePageWidth*width), static_cast<GLsizei>(mSparsePageHeight*height), static_cast<GLsizei>(mSparsePageDepth*depth),
			isResident);
	}
}
#endif //#ifdef FALCOR_GL
