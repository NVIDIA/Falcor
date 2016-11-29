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

        /** BindFlags flags
        */
        enum class BindFlags
        {
            None                =   0x0,    ///< Here for completion. Trying to create a resource with this flag will result in an error
            ShaderResource      =   0x1,    ///< Shader resource view
            UnorderedAccess     =   0x2,    ///< Unrodered access view
            RenderTarget        =   0x4,    ///< Render-target view
            DepthStencil        =   0x8,    ///< Depth-stencil view
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

        /** Get a mip-level width
        */
        uint32_t getWidth(uint32_t mipLevel = 0) const {return (mipLevel < mMipLevels) ? max(1U, mWidth >> mipLevel) : 0;}
        /** Get a mip-level height
        */
        uint32_t getHeight(uint32_t mipLevel = 0) const { return (mipLevel < mMipLevels) ? max(1U, mHeight >> mipLevel) : 0; }
        /** Get a mip-level depth
        */
        uint32_t getDepth(uint32_t mipLevel = 0) const { return (mipLevel < mMipLevels) ? max(1U, mDepth >> mipLevel) : 0; }
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
        static const uint32_t kMaxPossible = uint32_t(-1);

        /** create a 1D texture
            \param Width The width of the texture.
            \param Format The format of the texture.
            \param ArraySize The array size of the texture.
            \param mipLevels if equal to kMaxPossible then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
            \param pInitData If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \param bindFlags The requested bind flags for the resource
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create1D(uint32_t width, ResourceFormat format, uint32_t arraySize = 1, uint32_t mipLevels = kMaxPossible, const void* pInitData = nullptr, BindFlags bindFlags = BindFlags::ShaderResource);
        /** create a 2D texture
            \param width The width of the texture.
            \param height The height of the texture.
            \param Format The format of the texture.
            \param arraySize The array size of the texture.
			\param mipLevels if equal to kMaxPossible then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
            \param pInitData If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \param bindFlags The requested bind flags for the resource
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create2D(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize = 1, uint32_t mipLevels = kMaxPossible, const void* pInitData = nullptr, BindFlags bindFlags = BindFlags::ShaderResource);
        /** create a 3D texture
            \param width The width of the texture.
            \param height The height of the texture.
            \param depth The depth of the texture.
            \param format The format of the texture.
			\param mipLevels if equal to kMaxPossible then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
            \param pInitData If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \param bindFlags The requested bind flags for the resource
            \param isSparse If true, the texture is created as a sparse texture (GL_ARB_sparse_texture) without any physical memory allocated. pInitData must be null in this case.
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create3D(uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels = kMaxPossible, const void* pInitData = nullptr, BindFlags bindFlags = BindFlags::ShaderResource, bool isSparse=false);
        /** create a texture-cube
            \param width The width of the texture.
            \param height The height of the texture.
            \param format The format of the texture.
            \param arraySize The array size of the texture.
			\param mipLevels if equal to kMaxPossible then an entire mip chain will be generated from mip level 0. If any other value is given then the data for at least that number of miplevels must be provided.
			\param pInitData If different than nullptr, pointer to a buffer containing data to initialize the texture with.
            \param bindFlags The requested bind flags for the resource
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr createCube(uint32_t width, uint32_t height, ResourceFormat format, uint32_t arraySize = 1, uint32_t mipLevels = kMaxPossible, const void* pInitData = nullptr, BindFlags bindFlags = BindFlags::ShaderResource);
        /** create a multi-sampled 2D texture
            \param width The width of the texture.
            \param height The height of the texture.
            \param format The format of the texture.
            \param sampleCount Requested sample count of the texture.
            \param bindFlags The requested bind flags for the resource
            \return A pointer to a new texture, or nullptr if creation failed
        */
        static SharedPtr create2DMS(uint32_t width, uint32_t height, ResourceFormat format, uint32_t sampleCount, uint32_t arraySize = 1, BindFlags bindFlags = BindFlags::ShaderResource);

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

        /** Get a shader-resource view.
            \param[in] firstArraySlice The first array slice of the view
            \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will create a view ranging from firstArraySlice to the texture's array size
            \param[in] mostDetailedMip The most detailed mip level of the view
            \param[in] mipCount The number of mip-levels to bind. If this is equal to Texture#kMaxPossible, will create a view ranging from mostDetailedMip to the texture's mip levels count
        */
        SrvHandle getSRV(uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible, uint32_t mostDetailedMip = 0, uint32_t mipCount = kMaxPossible) const;

        /** Get a render-target view.
        \param[in] mipLevel The requested mip-level
        \param[in] firstArraySlice The first array slice of the view
        \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will create a view ranging from firstArraySlice to the texture's array size
        */
        RtvHandle getRTV(uint32_t mipLevel = 0, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible) const;

        /** Get a depth stencil view.
        \param[in] mipLevel The requested mip-level
        \param[in] firstArraySlice The first array slice of the view
        \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will create a view ranging from firstArraySlice to the texture's array size
        */
        DsvHandle getDSV(uint32_t mipLevel = 0, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible) const;

        /** Get the bind flags
        */
        BindFlags getBindFlags() const { return mBindFlags; }
        static RtvHandle getNullRtv() { return sNullRTV; }
        static DsvHandle getNullDsv() { return sNullDSV; }

        
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
        
    protected:
        friend class Device;
        mutable std::unordered_map<ViewInfo, SrvHandle, ViewInfoHasher> mSrvs;
        mutable std::unordered_map<ViewInfo, RtvHandle, ViewInfoHasher> mRtvs;
        mutable std::unordered_map<ViewInfo, DsvHandle, ViewInfoHasher> mDsvs;

        static RtvHandle sNullRTV;
        static DsvHandle sNullDSV;

		static uint32_t tempDefaultUint;

        std::string mName;
        std::string mSourceFilename;

        Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, uint32_t sampleCount, ResourceFormat format, Type Type, BindFlags bindFlags);
        
        TextureHandle mApiHandle = 0;
        BindFlags mBindFlags = BindFlags::None;
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

        static void createNullViews();
    };

    enum_class_operators(Texture::BindFlags);
    const std::string to_string(Texture::Type Type);
}