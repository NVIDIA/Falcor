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
#include "Core/FBO.h"
#include "Core/Texture.h"

namespace Falcor
{
    static bool checkAttachmentParams(const Texture* pTexture, uint32_t mipLevel, uint32_t arraySlice, bool isDepthAttachment)
    {
        if(pTexture == nullptr)
        {
            return true;
        }

        if(mipLevel >= pTexture->getMipCount())
        {
            Logger::log(Logger::Level::Error, "Error when attaching texture to FBO. Requested mip-level is out-of-bound.");
            return false;
        }

        if(arraySlice != Fbo::kAttachEntireMipLevel)
        {
            if(pTexture->getType() == Texture::Type::Texture3D)
            {
                if(arraySlice >= pTexture->getDepth())
                {
                    Logger::log(Logger::Level::Error, "Error when attaching texture to FBO. Requested depth-index is out-of-bound.");
                    return false;
                }
            }
            else
            {
                if(arraySlice >= pTexture->getArraySize())
                {
                    Logger::log(Logger::Level::Error, "Error when attaching texture to FBO. Requested array index is out-of-bound.");
                    return false;
                }
            }
        }

        if(isDepthAttachment)
        {
            if(isDepthStencilFormat(pTexture->getFormat()) == false)
            {
                Logger::log(Logger::Level::Error, "Error when attaching texture to FBO. Attaching to depth-stencil target, but resource has color format.");
                return false;
            }
        }
        else
        {
            if(isDepthStencilFormat(pTexture->getFormat()))
            {
                Logger::log(Logger::Level::Error, "Error when attaching texture to FBO. Attaching to color target, but resource has depth-stencil format.");
                return false;
            }
        }

        return true;
    }

    Fbo::SharedPtr Fbo::create()
    {
        return SharedPtr(new Fbo(true));
    }

    Fbo::SharedPtr Fbo::getDefault()
    {
        static Fbo::SharedPtr pDefault;
        if(pDefault == nullptr)
        {
            pDefault = Fbo::SharedPtr(new Fbo(false));
        }
        return pDefault;
    }

    void Fbo::attachDepthStencilTarget(const Texture::SharedConstPtr& pDepthStencil, uint32_t mipLevel, uint32_t arraySlice)
    {
        if(checkAttachmentParams(pDepthStencil.get(), mipLevel, arraySlice, true))
        {
            mIsDirty = true;
            mDepthStencil.pTexture = pDepthStencil;
            mDepthStencil.mipLevel = mipLevel;
            mDepthStencil.arraySlice = arraySlice;
            applyDepthAttachment();
        }
    }

    void Fbo::attachColorTarget(const Texture::SharedConstPtr& pTexture, uint32_t rtIndex, uint32_t mipLevel, uint32_t arraySlice)
    {
        if(rtIndex >= mColorAttachments.size())
        {
            Logger::log(Logger::Level::Error, "Error when attaching texture to FBO. Requested color index " + std::to_string(rtIndex) + ", but context only supports " + std::to_string(mColorAttachments.size()) + " targets");
            return;
        }

        if(checkAttachmentParams(pTexture.get(), mipLevel, arraySlice, false))
        {
            mIsDirty = true;
            mColorAttachments[rtIndex].pTexture = pTexture;
            mColorAttachments[rtIndex].mipLevel = mipLevel;
            mColorAttachments[rtIndex].arraySlice = arraySlice;
            applyColorAttachment(rtIndex);
        }
    }

    bool Fbo::verifyAttachment(const Attachment& attachment) const
    {
        const Texture* pTexture = attachment.pTexture.get();
        if(pTexture)
        {
            // Calculate size
            if(mWidth == uint32_t(-1))
            {
                // First attachment in the FBO
                mSampleCount = pTexture->getSampleCount();
                mIsLayered = (attachment.arraySlice == kAttachEntireMipLevel);
            }

            mWidth = min(mWidth, pTexture->getWidth());
            mHeight = min(mHeight, pTexture->getHeight());
            mDepth = min(mDepth, pTexture->getDepth());

            {
				if ( (pTexture->getSampleCount() > mSampleCount) && isDepthStencilFormat(pTexture->getFormat()) )
				{
					// We're using target-independent raster (more depth samples than color samples).  This is OK.
					mSampleCount = pTexture->getSampleCount();
					return true;
				}

				if (mSampleCount != pTexture->getSampleCount())
				{
					Logger::log(Logger::Level::Error, "Error when validating FBO. Sifferent sample counts in attachments\n");
					return false;
				}

	
                if(mIsLayered != (attachment.arraySlice == kAttachEntireMipLevel))
                {
                    Logger::log(Logger::Level::Error, "Error when validating FBO. Can't bind both layered and non-layered textures\n");
                    return false;
                }
            }
        }
        return true;
    }

    bool Fbo::calcAndValidateProperties() const
    {
        mWidth = (uint32_t)-1;
        mHeight = (uint32_t)-1;
        mDepth = (uint32_t)-1;
        mSampleCount = (uint32_t)-1;
        mIsLayered = false;

        // Check color
        for(const auto& attachment : mColorAttachments)
        {
            if(verifyAttachment(attachment) == false)
            {
                return false;
            }
        }

        // Check depth
        return verifyAttachment(mDepthStencil);
    }

    void Fbo::clear(const glm::vec4& color, float depth, int32_t stencil, FboAttachmentType flags) const
    {
        if(flags == FboAttachmentType::None)
        {
            Logger::log(Logger::Level::Warning, "Hmmmm... you called CFbo::clear() with flags set to zero... Why would you do that?");
            return;
        }

        if(!checkStatus())
            return;

        bool clearDepth = (flags & FboAttachmentType::Depth) != FboAttachmentType::None;
        bool clearColor = (flags & FboAttachmentType::Color) != FboAttachmentType::None;
        bool clearStencil = (flags & FboAttachmentType::Stencil) != FboAttachmentType::None;

        if(clearDepth || clearStencil)
        {
            if(mDepthStencil.pTexture || mApiHandle == 0)
            {
                clearDepthStencil(depth, stencil, clearDepth, clearStencil);
            }
            else
            {
                Logger::log(Logger::Level::Warning, "CFbo::clear(). Requesting to clear depth-stencil buffer, but the buffer doesn't exist in the FBO. Ignoring call.");
            }
        }

        if(clearColor)
        {
            for(uint32_t rtIndex = 0; rtIndex < mColorAttachments.size(); rtIndex++)
            {
                if(mColorAttachments[rtIndex].pTexture || mApiHandle == 0)
                {
                    clearColorTarget(rtIndex, color);
                }
            }
        }
    }

    Texture::SharedConstPtr Fbo::getColorTexture(uint32_t index) const
    {
        if(index >= mColorAttachments.size())
        {
            Logger::log(Logger::Level::Error, "CFbo::getColorTexture(): Index is out of range. Requested " + std::to_string(index) + " but only " + std::to_string(mColorAttachments.size()) + " color slots are available.");
            return nullptr;
        }
        return mColorAttachments[index].pTexture;
    }

    Texture::SharedConstPtr Fbo::getDepthStencilTexture() const
    {
        return mDepthStencil.pTexture;
    }
}