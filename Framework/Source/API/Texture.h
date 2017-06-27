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
#include "Resource.h"
#include "Utils/Bitmap.h"

namespace Falcor
{
    class Sampler;
    class Device;

    /** Abstracts the API texture objects
    */
    class Texture : public Resource, public inherit_shared_from_this<Resource, Texture>
    {
    public:
        using SharedPtr = std::shared_ptr<Texture>;
        using SharedConstPtr = std::shared_ptr<const Texture>;
        using WeakPtr = std::weak_ptr<Texture>;
        using WeakConstPtr = std::weak_ptr<const Texture>;
        using inherit_shared_from_this<Resource, Texture>::shared_from_this;

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
        uint32_t getWidth(uint32_t mipLevel = 0) const { return (mipLevel < mMipLevels) ? max(1U, mWidth >> mipLevel) : 0; }

        /** Get a mip-level height
        */
        uint32_t getHeight(uint32_t mipLevel = 0) const { return (mipLevel < mMipLevels) ? max(1U, mHeight >> mipLevel) : 0; }

        /** Get a mip-level depth
        */
        uint32_t getDepth(uint32_t mipLevel = 0) const { return (mipLevel < mMipLevels) ? max(1U, mDepth >> mipLevel) : 0; }

        /** Get the number of mip-levels
        */
        uint32_t getMipCount() const { return mMipLevels; }

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

        static SharedPtr create3D(uint32_t width, uint32_t height, uint32_t depth, ResourceFormat format, uint32_t mipLevels = kMaxPossible, const void* pInitData = nullptr, BindFlags bindFlags = BindFlags::ShaderResource, bool isSparse = false);
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

        /** Get the required buffer size for a single array slice in a mip-level
        */
        uint32_t getMipLevelDataSize(uint32_t mipLevel) const;

        /** Get the required buffer size for the full mip-map pyramid
        */
        uint32_t getDataSize() const;

        /** Capture the texture to a PNG image.\n
            \param[in] mipLevel Requested mip-level
            \param[in] arraySlice Requested array-slice
            \param[in] filename Name of the PNG file to save.
            \param[in] fileFormat Destination image file format (e.g., PNG, PFM, etc.)
            \param[in] exportFlags Save flags, see Bitmap::ExportFlags
        */
        void captureToFile(uint32_t mipLevel, uint32_t arraySlice, const std::string& filename, Bitmap::FileFormat format = Bitmap::FileFormat::PngFile, Bitmap::ExportFlags exportFlags = Bitmap::ExportFlags::None) const;

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

        /** If the texture has been created as using sparse storage, makes individual physically pages resident and non-resident.
          * Use page index.
        */
        void setSparseResidencyPageIndex(bool isResident, uint32_t mipLevel, uint32_t pageX, uint32_t pageY, uint32_t pageZ, uint32_t width = 1, uint32_t height = 1, uint32_t depth = 1);

        /** If the texture has been created as using sparse storage, makes individual physically pages resident and non-resident.
        */
        glm::i32vec3 getSparseResidencyPageSize();

    protected:
        friend class Device;
        void apinit(const void* pData, bool autoGenMips);
        void uploadInitData(const void* pData, bool autoGenMips);

        static RtvHandle spNullRTV;
        static DsvHandle spNullDSV;

        static uint32_t tempDefaultUint;

        std::string mName;
        std::string mSourceFilename;

        Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, uint32_t sampleCount, ResourceFormat format, Type Type, BindFlags bindFlags);

        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        uint32_t mDepth = 0;
        uint32_t mMipLevels = 0;
        uint32_t mSampleCount = 0;
        uint32_t mArraySize = 0;
        ResourceFormat mFormat = ResourceFormat::Unknown;
        bool mIsSparse = false;
        glm::i32vec3 mSparsePageRes = glm::i32vec3(0);
    };
}
