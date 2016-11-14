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
#include "glm/vec4.hpp"
#include "Texture.h"
#include "Utils/Bitmap.h"

namespace Falcor
{
    enum class FboAttachmentType;
   
    /** Low level framebuffer object.
        This class abstracts the API's framebuffer creation and management.
    */
    class Fbo : public std::enable_shared_from_this<Fbo>
    {
    public:
        using SharedPtr = std::shared_ptr<Fbo>;
        using SharedConstPtr = std::shared_ptr<const Fbo>;

        class Desc
        {
        public:
            Desc();
            Desc& setColorFormat(uint32_t rtIndex, ResourceFormat format);
            Desc& setDepthStencilFormat(ResourceFormat format) { mDepthStencilFormat = format; return *this; }
            Desc& setSampleCount(uint32_t sampleCount) { mSampleCount = sampleCount ? sampleCount : 1; return *this; }

            uint32_t getColorFormatCount() const { return mRtCount; }
            ResourceFormat getColorFormat(uint32_t rtIndex) const { return mColorFormats[rtIndex]; }
            ResourceFormat getDepthStencilFormat() const { return mDepthStencilFormat; }
            uint32_t getSampleCount() const { return mSampleCount; }
        private:
            std::vector<ResourceFormat> mColorFormats;
            ResourceFormat mDepthStencilFormat = ResourceFormat::Unknown;
            uint32_t mSampleCount = 1;
            uint32_t mRtCount = 0;
        };

        /** Used to tell some functions to attach all array slices of a specific mip-level.
        */
        static const uint32_t kAttachEntireMipLevel = uint32_t(-1);

        /** Destructor. Releases the API object
        */
        ~Fbo();

        /** Get a FBO representing the default framebuffer object
        */
        static SharedPtr getDefault();

        /** create a new empty FBO.
        */
        static SharedPtr create();

        /** Attach a depth-stencil texture.
            \param pDepthStencil The depth-stencil texture.
            \param mipLevel The selected mip-level to attach.
            \param arraySlice The selected array-slice to attach, or CFbo#AttachEntireMipLevel to attach the all array-slices.
        */
        void attachDepthStencilTarget(const Texture::SharedConstPtr& pDepthStencil, uint32_t mipLevel = 0, uint32_t arraySlice = 0);
        /** Attach a color texture.
            \param pColorTexture The depth-stencil texture.
            \param rtIndex The render-target index to attach the texture to.
            \param mipLevel The selected mip-level to attach.
            \param arraySlice The selected array-slice to attach, or CFbo#k_attachEntireMipLevel to attach the all array-slices.
        */
        void attachColorTarget(const Texture::SharedConstPtr& pColorTexture, uint32_t rtIndex, uint32_t mipLevel = 0, uint32_t arraySlice = 0);

        /** Get the object's API handle.      
        */
        uint32_t getApiHandle() const;

        /** Capture a buffer in the FBO to a PNG image.\n
        \param[in] rtIndex The render-target index to capture.
        \param[in] filename Name of the PNG file to save.
        \param[in] fileFormat Destination image file format (e.g., PNG, PFM, etc.)
        */
        void captureToFile(uint32_t rtIndex, const std::string& filename, Bitmap::FileFormat fileFormat);

        /** Get the maximum number of color targets
        */
        static uint32_t getMaxColorTargetCount();

        /** Get an attached color texture. If no texture is attached will return nullptr.
        */
        Texture::SharedConstPtr getColorTexture(uint32_t index) const;

        /** Get the attached depth-stencil texture, or nullptr if no texture is attached.
        */
        Texture::SharedConstPtr getDepthStencilTexture() const;

        /** Validates that the framebuffer attachments are OK. This function causes the actual HW resources to be generated (RTV/DSV in DX, FBO attachment calls in GL).
        */
        bool checkStatus() const;

        /** Get the width of the FBO
        */
        uint32_t getWidth() const { checkStatus(); return mWidth; }
        /** Get the height of the FBO
        */
        uint32_t getHeight() const { checkStatus(); return mHeight; }
        /** Get the sample-count of the FBO
        */
        uint32_t getSampleCount() const { checkStatus(); return mDesc.getSampleCount(); }

		/** Force the FBO to have zero attachment (no texture attached) and use a virtual resolution.
		*/
		void setZeroAttachments(uint32_t width, uint32_t height, uint32_t layers=1, uint32_t samples=1, bool fixedSampleLocs=false);

        /** Get the FBO format descriptor
        */
        const Desc& getDesc() const { checkStatus();  return mDesc; }
#ifdef FALCOR_D3D
        DsvHandle getDepthStencilView() const;
        RtvHandle getRenderTargetView(uint32_t rtIndex) const;
        void resetViews();
#endif
        struct Attachment
        {
            Texture::SharedConstPtr pTexture = nullptr;
            uint32_t mipLevel = 0;
            uint32_t arraySlice = 0;

            bool operator==(const Attachment& other) const
            {
                return (pTexture == other.pTexture) && (mipLevel == other.mipLevel) && (arraySlice == other.arraySlice);
            }
        };
    private:

        bool verifyAttachment(const Attachment& attachment) const;
        bool calcAndValidateProperties() const;

        void applyColorAttachment(uint32_t rtIndex);
        void applyDepthAttachment();

        Fbo(bool initApiHandle = true);
        std::vector<Attachment> mColorAttachments;
        Attachment mDepthStencil;

        mutable Desc mDesc;
        mutable bool mIsDirty = false;
        mutable uint32_t mWidth  = (uint32_t)-1;
        mutable uint32_t mHeight = (uint32_t)-1;
        mutable uint32_t mDepth = (uint32_t)-1;
        mutable bool mHasDepthAttachment = false;
        mutable bool mIsLayered = false;
		mutable bool mIsZeroAttachment = false;

        uint32_t mApiHandle = 0;
        void* mpPrivateData = nullptr;
    };
}