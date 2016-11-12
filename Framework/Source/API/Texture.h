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
#include <map>
#include "API/Formats.h"
#include <unordered_map>

namespace Falcor
{
    class Sampler;
    class Device;

    /** Abstracts the API texture objects
    */
    class Texture : public std::enable_shared_from_this<Texture>
    {
    public:
        using SharedPtr = std::shared_ptr<Texture>;
        using SharedConstPtr = std::shared_ptr<const Texture>;
        using ApiHandle = TextureHandle;

        /** Texture types. Notice there are no array types. Array are controlled using the array size parameter on texture creation.
        */
        enum class Type
        {
            Texture1D,                  ///< 1D texture. Can be bound as render-target, shader-resource and image
            Texture2D,                  ///< 2D texture. Can be bound as render-target, shader-resource and image
            Texture3D,                  ///< 3D texture. Can be bound as render-target, shader-resource and image
            TextureCube,                ///< Texture-cube. Can be bound as render-target, shader-resource and image
            Texture2DMultisample,       ///< 2D multi-sampled texture. Can be bound as render-target, shader-resource and image
        };

        /** Get the API handle
        */
        TextureHandle getApiHandle() const { return mApiHandle; }

        /** Load the texture to the GPU memory.
            \params[in] pSampler If not null, will get a pointer to the combination of the texture/sampler.
            \return The GPU address, which can be used as a pointer in shaders.
        */
        uint64_t makeResident(const Sampler* pSampler) const;

        /** Evict the texture from the GPU memory. This function is only valid after makeResident() call was made with a matching sample. If makeResident() wasn't called, the evict() will be silently ignored.
            \params[in] pSampler The sampler object used in a matching makeResident() call.
        */
        void evict(const Sampler* pSampler) const;

        ~Texture();

        /** Get the texture width
        */
        uint32_t getWidth() const {return mWidth;}
        /** Get the texture height
        */
        uint32_t getHeight() const { return mHeight; }
        /** Get the texture depth
        */
        uint32_t getDepth() const { return mDepth; }
        /** Get the number of mip-levels
        */
        uint32_t getMipCount() const {return mMipLevels;}
        /** Get the sample count
        */
        uint32_t getSampleCount() const { return mSampleCount; }
        /** Get the array size
        */
        uint32_t getArraySize() const { return mArraySize; }

        /** Get the array index of a subresource
        */
        uint32_t getSubresourceArraySlice(uint32_t subresource) const { return subresource / mMipLevels; }

        /** Get the mip-level of a subresource
        */
        uint32_t getSubresourceMipLevel(uint32_t subresource) const { return subresource % mMipLevels; }

        /** Get the subresource index
        */
        uint32_t getSubresourceIndex(uint32_t arraySlice, uint32_t mipLevel) const { return mipLevel + arraySlice * mMipLevels; }

        /** Get the resource format
        */
        ResourceFormat getFormat() const { return mFormat; }
        /** Get the resource Type
        */
        Type getType() const { return mType; }
        /** For multi-sampled resources, check if the texture uses fixed sample locations
        */
        bool hasFixedSampleLocations() const { return mHasFixedSampleLocations; }

        /** Value used in create*() methods to signal an entire mip-chain is required
        */
        static const uint32_t kEntireMipChain = uint32_t(-1);

        /** create a 1D texture
            \param Width The width of the texture.
            \param Format The format of the texture.
            \param ArraySize The array size of the texture.
            \param mipLevels if equal to kEntireMipChain then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
            \param pInitData Optional. If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create1D(uint32_t width, ResourceFormat format, uint32_t arraySize = 1, uint32_t mipLevels = kEntireMipChain, const void* pInitData = nullptr);
        /** create a 2D texture
            \param width The width of the texture.
            \param height The height of the texture.
            \param Format The format of the texture.
            \param arraySize The array size of the texture.
			\param mipLevels if equal to kEntireMipChain then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
			\param pInitData Optional. If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create2D(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize = 1, uint32_t mipLevels = kEntireMipChain, const void* pInitData = nullptr);
        /** create a 3D texture
            \param width The width of the texture.
            \param height The height of the texture.
            \param depth The depth of the texture.
            \param format The format of the texture.
			\param mipLevels if equal to kEntireMipChain then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
			\param pInitData Optional. If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create3D(uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels = kEntireMipChain, const void* pInitData = nullptr, bool isSparse=false);
        /** create a texture-cube
            \param width The width of the texture.
            \param height The height of the texture.
            \param format The format of the texture.
            \param arraySize The array size of the texture.
			\param mipLevels if equal to kEntireMipChain then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
			\param pInitData Optional. If different than nullptr, pointer to a buffer containing data to initialize the texture with.
			\param isSparse Optional. If true, the texture is created as a sparse texture (GL_ARB_sparse_texture) without any physical memory allocated. pInitData must be null in this case.
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr createCube(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize = 1, uint32_t mipLevels = kEntireMipChain, const void* pInitData = nullptr);
        /** create a multi-sampled 2D texture
            \param width The width of the texture.
            \param height The height of the texture.
            \param format The format of the texture.
            \param sampleCount Requested sample count of the texture.
            \param useFixedSampleLocations Controls weather or not the texture uses fixed or variable sample locations
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create2DMS(uint32_t width, uint32_t height, ResourceFormat format, uint32_t sampleCount, uint32_t arraySize = 1, bool useFixedSampleLocations = true);
        /** create from existing 2D texture view; this is necessary to interface with some lower-level APIs.  TODO: maybe factor this
            functionality out into a separate CTextureView class?
            \param apiHandle The ID of the pre-existing texture view object.
            \param width The width of the texture.
            \param height The height of the texture.
            \param format The format of the texture.
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create2DFromView(uint32_t apiHandle, uint32_t width, uint32_t height, ResourceFormat format);

        /** Get the image size for a single array slice in a mip-level
        */
        void getMipLevelImageSize(uint32_t mipLevel, uint32_t& width, uint32_t& height, uint32_t& depth = tempDefaultUint) const;

        /** Get the required buffer size for a single array slice in a mip-level
        */
        uint32_t getMipLevelDataSize(uint32_t mipLevel) const;

        /** Read the data from one a for the texture's sub-resources
            \param pData Buffer to write the data to.
            \param dataSize Size of buffer pointed to by pData. DataSize must be equal to the value returned by Texture#GetMipLevelDataSize().
            \param mipLevel Requested mip-level
            \param arraySlice Requested array-slice
        */
        void readSubresourceData(void* pData, uint32_t dataSize, uint32_t mipLevel, uint32_t arraySlice) const;
        
        /** Uploads the data from host into the texture's sub-resource level
            \param pData Buffer to read the data from.
            \param dataSize Size of buffer pointed to by pData. DataSize must be equal to the value returned by Texture#GetMipLevelDataSize().
            \param mipLevel Mip-level to update
            \param arraySlice Array-slice to update
        */
        void uploadSubresourceData(const void* pData, uint32_t dataSize, uint32_t mipLevel = 0, uint32_t arraySlice = 0);

        /** Capture the texture to a PNG image.\n
        \param[in] mipLevel Requested mip-level
        \param[in] arraySlice Requested array-slice
        \param[in] filename Name of the PNG file to save.
        */
        void captureToPng(uint32_t mipLevel, uint32_t arraySlice, const std::string& filename) const;

        void compress2DTexture();
		
        /** Generates mipmaps for a specified texture object.
        */
        void generateMips() const;

        /** Name the texture
        */
        void setName(const std::string& name) { mName = name; }

        /** Get the texture name
        */
        const std::string& getName() const { return mName; }

        /** In case the texture was loaded from a file, use this to set the filename
        */
        void setSourceFilename(const std::string& filename) { mSourceFilename = filename; }

        /** In case the texture was loaded from a file, get the source filename
        */
        const std::string& getSourceFilename() const { return mSourceFilename; }

        void copySubresource(const Texture* pDst, uint32_t srcMipLevel, uint32_t srcArraySlice, uint32_t dstMipLevel, uint32_t dstArraySlice) const;
        Texture::SharedPtr createView(uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount) const;

		/** If the texture has been created as using sparse storage, makes individual physically pages resident and non-resident.
		  * Use page index.
        */
		void setSparseResidencyPageIndex(bool isResident, uint32_t mipLevel,  uint32_t pageX, uint32_t pageY, uint32_t pageZ, uint32_t width=1, uint32_t height=1, uint32_t depth=1);

        D3D12_RESOURCE_STATES getResourceState() const { return mResourceState; }
        void setResourceState(D3D12_RESOURCE_STATES state) const { mResourceState = state; }

        SrvHandle getResourceView(uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount) const;
        SrvHandle getWholeResourceView() const;
    protected:
        friend class Device;
        
        struct ViewInfo
        {
            uint32_t firstArraySlice;
            uint32_t arraySize;
            uint32_t mostDetailedMip;
            uint32_t mipCount;

            bool operator==(const ViewInfo& other) const
            {
                return (firstArraySlice == other.firstArraySlice) && (arraySize == other.arraySize) && (mipCount == other.mipCount) && (mostDetailedMip == other.mostDetailedMip);
            }
        };

        struct ViewInfoHasher
        {
            std::size_t operator()(const ViewInfo& v) const
            {
                return ((std::hash<uint32_t>()(v.firstArraySlice)
                    ^ (std::hash<uint32_t>()(v.arraySize) << 1)) >> 1)
                    ^ (std::hash<uint32_t>()(v.mipCount) << 1)
                    ^ (std::hash<uint32_t>()(v.mostDetailedMip) << 3);
            }
        };

        mutable std::unordered_map<ViewInfo, SrvHandle, ViewInfoHasher> mSrvs;
        mutable SrvHandle mWholeResourceView = { 0 };

		static uint32_t tempDefaultUint;

        std::string mName;
        std::string mSourceFilename;

        Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, uint32_t sampleCount, ResourceFormat format, Type Type);
        
        TextureHandle mApiHandle = 0;
        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        uint32_t mDepth = 0;
        uint32_t mMipLevels = 0;
        uint32_t mSampleCount = 0;
        uint32_t mArraySize = 0;
        Type mType;
        ResourceFormat mFormat = ResourceFormat::Unknown;
        bool mHasFixedSampleLocations = true;
		bool mIsSparse = false;
		int32_t mSparsePageWidth = 0;
		int32_t mSparsePageHeight = 0;
		int32_t mSparsePageDepth = 0;

        mutable std::map<uint32_t, uint64_t> mBindlessTextureHandle;
        mutable D3D12_RESOURCE_STATES mResourceState = D3D12_RESOURCE_STATE_COMMON;
    };

    const std::string to_string(Texture::Type Type);
}