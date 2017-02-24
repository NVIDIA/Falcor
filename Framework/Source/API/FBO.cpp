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
#include "API/FBO.h"
#include "API/Texture.h"

namespace Falcor
{
    Fbo::Desc::Desc()
    {
        mColorTargets.resize(Fbo::getMaxColorTargetCount());
    }

    static bool checkAttachmentParams(const Texture* pTexture, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize, bool isDepthAttachment)
    {
        if(pTexture == nullptr)
        {
            return true;
        }

        if(mipLevel >= pTexture->getMipCount())
        {
            logError("Error when attaching texture to FBO. Requested mip-level is out-of-bound.");
            return false;
        }

        if(arraySize != Fbo::kAttachEntireMipLevel)
        {
            if(pTexture->getType() == Texture::Type::Texture3D)
            {
                if(arraySize + firstArraySlice >= pTexture->getDepth())
                {
                    logError("Error when attaching texture to FBO. Requested depth-index is out-of-bound.");
                    return false;
                }
            }
            else
            {
                //Why was this >=? shouldnt array size = texture's array size be fine?
                if(arraySize + firstArraySlice > pTexture->getArraySize())
                {
                    logError("Error when attaching texture to FBO. Requested array index is out-of-bound.");
                    return false;
                }
            }
        }

        if(isDepthAttachment)
        {
            if(isDepthStencilFormat(pTexture->getFormat()) == false)
            {
                logError("Error when attaching texture to FBO. Attaching to depth-stencil target, but resource has color format.");
                return false;
            }

            if ((pTexture->getBindFlags() & Texture::BindFlags::DepthStencil) == Texture::BindFlags::None)
            {
                logError("Error when attaching texture to FBO. Attaching to depth-stencil target, the texture wasn't create with the DepthStencil bind flag");
                return false;

            }
        }
        else
        {
            if(isDepthStencilFormat(pTexture->getFormat()))
            {
                logError("Error when attaching texture to FBO. Attaching to color target, but resource has depth-stencil format.");
                return false;
            }

            if ((pTexture->getBindFlags() & Texture::BindFlags::RenderTarget) == Texture::BindFlags::None)
            {
                logError("Error when attaching texture to FBO. Attaching to color target, the texture wasn't create with the RenderTarget bind flag");
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

    void Fbo::attachDepthStencilTarget(const Texture::SharedPtr& pDepthStencil, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        if(checkAttachmentParams(pDepthStencil.get(), mipLevel, firstArraySlice, arraySize, true))
        {
            mIsDirty = true;
            mDepthStencil.pTexture = pDepthStencil;
            mDepthStencil.mipLevel = mipLevel;
            mDepthStencil.firstArraySlice = firstArraySlice;
            mDepthStencil.arraySize = arraySize;
            bool allowUav = false;
            if (pDepthStencil)
            {
                allowUav = ((pDepthStencil->getBindFlags() & Texture::BindFlags::UnorderedAccess) != Texture::BindFlags::None);
            }

            mDesc.setDepthStencilTarget(pDepthStencil ? pDepthStencil->getFormat() : ResourceFormat::Unknown, allowUav);
            applyDepthAttachment();
        }
    }

    void Fbo::attachColorTarget(const Texture::SharedPtr& pTexture, uint32_t rtIndex, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        if(rtIndex >= mColorAttachments.size())
        {
            logError("Error when attaching texture to FBO. Requested color index " + std::to_string(rtIndex) + ", but context only supports " + std::to_string(mColorAttachments.size()) + " targets");
            return;
        }

        if(checkAttachmentParams(pTexture.get(), mipLevel, firstArraySlice, arraySize, false))
        {
            mIsDirty = true;
            mColorAttachments[rtIndex].pTexture = pTexture;
            mColorAttachments[rtIndex].mipLevel = mipLevel;
            mColorAttachments[rtIndex].firstArraySlice = firstArraySlice;
            mColorAttachments[rtIndex].arraySize = arraySize;
            bool allowUav = false;
            if(pTexture)
            {
                allowUav = ((pTexture->getBindFlags() & Texture::BindFlags::UnorderedAccess) != Texture::BindFlags::None);
            }

            mDesc.setColorTarget(rtIndex, pTexture ? pTexture->getFormat() : ResourceFormat::Unknown, allowUav);
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
                mDesc.setSampleCount(pTexture->getSampleCount());
                mIsLayered = (attachment.arraySize > 1);
            }

            mWidth = min(mWidth, pTexture->getWidth(attachment.mipLevel));
            mHeight = min(mHeight, pTexture->getHeight(attachment.mipLevel));
            mDepth = min(mDepth, pTexture->getDepth(attachment.mipLevel));

            {
				if ( (pTexture->getSampleCount() > mDesc.getSampleCount()) && isDepthStencilFormat(pTexture->getFormat()) )
				{
					// We're using target-independent raster (more depth samples than color samples).  This is OK.
					mDesc.setSampleCount(pTexture->getSampleCount());
					return true;
				}

				if (mDesc.getSampleCount() != pTexture->getSampleCount())
				{
                    logError("Error when validating FBO. Different sample counts in attachments\n");
					return false;
				}

	
                if(mIsLayered != (attachment.arraySize > 1))
                {
                    logError("Error when validating FBO. Can't bind both layered and non-layered textures\n");
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
        mDesc.setSampleCount(uint32_t(-1));
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

    Texture::SharedPtr Fbo::getColorTexture(uint32_t index) const
    {
        if(index >= mColorAttachments.size())
        {
            logError("CFbo::getColorTexture(): Index is out of range. Requested " + std::to_string(index) + " but only " + std::to_string(mColorAttachments.size()) + " color slots are available.");
            return nullptr;
        }
        return mColorAttachments[index].pTexture;
    }

    Texture::SharedPtr Fbo::getDepthStencilTexture() const
    {
        return mDepthStencil.pTexture;
    }
}