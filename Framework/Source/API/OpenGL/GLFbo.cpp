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
#include "API/FBO.h"
#include "API/Texture.h"
#include "API/Window.h"
#include "glm/gtc/type_ptr.hpp"
#include "Utils/Bitmap.h"

namespace Falcor
{
    // Used for glDrawBuffer. Static ordering of the render-targets
    static const GLenum kGlColorAttachments[] = 
    {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7,
        GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
        GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};

    Fbo::Fbo(bool initApiHandle)
    {
        mColorAttachments.resize(getMaxColorTargetCount());
        if(initApiHandle)
        {
            gl_call(glCreateFramebuffers(1, &mApiHandle));
            gl_call(glNamedFramebufferDrawBuffers(mApiHandle, getMaxColorTargetCount(), kGlColorAttachments));
        }
    }

    Fbo::~Fbo()
    {
        glDeleteFramebuffers(1, &mApiHandle);
    }

    uint32_t Fbo::getApiHandle() const
    {
        return mApiHandle;
    }

    uint32_t Fbo::getMaxColorTargetCount()
    {
        static uint32_t colorTargetCount = 0;
        if(colorTargetCount == 0)
        {
            glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, (GLint*)&colorTargetCount);
        }
        return colorTargetCount;
    }

    void bindTexture(const Texture* pTexture, GLenum attachment, uint32_t fboID, uint32_t mipLevel, uint32_t arraySlice)
    {
        if(pTexture)
        {
            if(arraySlice == Fbo::kAttachEntireMipLevel)
            {
                gl_call(glNamedFramebufferTexture(fboID, attachment, pTexture->getApiHandle(), mipLevel));
            }
            else
            {
                if((pTexture->getArraySize() > 1) || (pTexture->getType() == Texture::Type::TextureCube) || (pTexture->getType() == Texture::Type::Texture3D))
                {
                    gl_call(glNamedFramebufferTextureLayer(fboID, attachment, pTexture->getApiHandle(), mipLevel, arraySlice));
                }
                else
                {
                    switch(pTexture->getType())
                    {
                    case Texture::Type::Texture1D:
                        gl_call(glNamedFramebufferTexture1DEXT(fboID, attachment, GL_TEXTURE_1D, pTexture->getApiHandle(), mipLevel));
                        break;
                    case Texture::Type::Texture2D:
                        gl_call(glNamedFramebufferTexture2DEXT(fboID, attachment, GL_TEXTURE_2D, pTexture->getApiHandle(), mipLevel));
                        break;
                    case Texture::Type::Texture2DMultisample:
                        gl_call(glNamedFramebufferTexture2DEXT(fboID, attachment, GL_TEXTURE_2D_MULTISAMPLE, pTexture->getApiHandle(), mipLevel));
                        break;
                    default:
                        should_not_get_here();
                    }
                }
            }
        }
        else
        {
            // No texture
            gl_call(glNamedFramebufferTexture(fboID, attachment, 0, 0));
        }
    }

    void Fbo::applyColorAttachment(uint32_t rtIndex)
    {
        if(mApiHandle != 0)
        {
            bindTexture(mColorAttachments[rtIndex].pTexture.get(), GL_COLOR_ATTACHMENT0 + rtIndex, mApiHandle, mColorAttachments[rtIndex].mipLevel, mColorAttachments[rtIndex].arraySlice);
        }
        else if(mColorAttachments[rtIndex].pTexture && mColorAttachments[rtIndex].pTexture->getApiHandle() != 0)
        {
            logError("Can't attach a texture to the default FBO. Ignoring call");
        }
    }

    void Fbo::applyDepthAttachment()
    {
        if(mApiHandle != 0)
        {
            // Attach depth-stencil
            GLenum depthAttachment = GL_DEPTH_STENCIL_ATTACHMENT;
            if(mDepthStencil.pTexture && (isStencilFormat(mDepthStencil.pTexture->getFormat()) == false))
            {
                depthAttachment = GL_DEPTH_ATTACHMENT;
            }

            bindTexture(mDepthStencil.pTexture.get(), depthAttachment, mApiHandle, mDepthStencil.mipLevel, mDepthStencil.arraySlice);
        }
        else if(mDepthStencil.pTexture && mDepthStencil.pTexture->getApiHandle() != 0)
        {
            logError("Can't attach a texture to the default FBO. Ignoring call");
        }
        mHasDepthAttachment = (mDepthStencil.pTexture != nullptr);
    }

    bool Fbo::checkStatus() const
    {
        if(mpDesc == nullptr)
        {
            if(calcAndValidateProperties() == false)
            {
                return false;
            }
            
            mIsDirty = false;

            // Check the status
            if(mApiHandle != 0)
            {
                GLenum FbStatus;
                gl_call(FbStatus = glCheckNamedFramebufferStatus(mApiHandle, GL_DRAW_FRAMEBUFFER));
                if(FbStatus != GL_FRAMEBUFFER_COMPLETE)
                {
                    logError("Incomplete framebuffer object");
                    return false;
                }
            }
        }
        return true;
    }
    
    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::vec4& color) const
    {
        if(!checkStatus())
        {
            return;
        }

        bool isDefaultFbo = (mApiHandle == 0);
        if(mColorAttachments[rtIndex].pTexture)
        {
            FormatType Type = getFormatType(mColorAttachments[rtIndex].pTexture->getFormat());
            bool isValidType = (Type == FormatType::Float) || (Type == FormatType::Snorm) || (Type == FormatType::Unorm) || (Type == FormatType::UnormSrgb);
            if(isValidType == false)
            {
                logWarning("CFbo::clearColorTarget() - Trying to clear a non-normalized, non-floating-point render-target with a floating point value. This is undefined and so the call is ignored");
                return;
            }
        }
        else if(mApiHandle != 0)
        {
            logWarning("Trying to clear a color render-target, but the texture does not exist in CFbo.");
            return;
        }

        gl_call(glClearNamedFramebufferfv(mApiHandle, GL_COLOR, rtIndex, (GLfloat*)glm::value_ptr(color)));
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::uvec4& color) const
    {
        if(!checkStatus())
        {
            return;
        }

        if(mColorAttachments[rtIndex].pTexture)
        {
            FormatType Type = getFormatType(mColorAttachments[rtIndex].pTexture->getFormat());
            if(Type != FormatType::Uint)
            {
                logWarning("CFbo::clearColorTarget() - Trying to clear a non-unsigned-integer render-target with a uint value. This is undefined and so the call is ignored");
                return;
            }
        }
        else if(mApiHandle != 0)
        {
            logWarning("Trying to clear a color render-target, but the texture does not exist in CFbo.");
            return;
        }

        gl_call(glClearNamedFramebufferuiv(mApiHandle, GL_COLOR, rtIndex, glm::value_ptr(color)));
    }

    void Fbo::clearColorTarget(uint32_t rtIndex, const glm::ivec4& color) const
    {
        if(!checkStatus())
        {
            return;
        }

        if(mColorAttachments[rtIndex].pTexture)
        {
            FormatType Type = getFormatType(mColorAttachments[rtIndex].pTexture->getFormat());
            if(Type == FormatType::Sint)
            {
                logWarning("CFbo::clearColorTarget() - Trying to clear a non-signed-integer render-target with an int value. This is undefined and so the call is ignored");
                return;
            }
        }
        else if(mApiHandle != 0)
        {
            logWarning("Trying to clear a color render-target, but the texture does not exist in CFbo.");
            return;
        }

        gl_call(glClearNamedFramebufferiv(mApiHandle, GL_COLOR, rtIndex, glm::value_ptr(color)));
    }

    void Fbo::clearDepthStencil(float depth, uint8_t stencil, bool ClearDepth, bool ClearStencil) const
    {
        if(!checkStatus())
        {
            return;
        }

        if(mHasDepthAttachment)
        {
            bool useSingleCall = (mApiHandle != 0) && (ClearDepth && ClearStencil);
            if(useSingleCall)
            {
                // Clear both with one API call
                gl_call(glClearNamedFramebufferfi(mApiHandle, GL_DEPTH_STENCIL, 0, depth, stencil));
            }
            else
            {
                // Check if we need to clear depth or stencil independently
                if(ClearDepth)
                {
                    gl_call(glClearNamedFramebufferfv(mApiHandle, GL_DEPTH, 0, &depth));
                }

                if(ClearStencil)
                {
                    GLint S = (GLint)stencil;
                    gl_call(glClearNamedFramebufferiv(mApiHandle, GL_STENCIL, 0, &S));
                }
            }
        }
    }

    void Fbo::captureToFile( uint32_t rtIndex, const std::string& filename, Bitmap::FileFormat fileFormat, uint32_t exportFlags )
    {
        // Figure out the size of the buffer we'll need to pass to GL to store our buffer (and send to FreeImage)
        ResourceFormat format = mColorAttachments[rtIndex].pTexture->getFormat();
        uint32_t bytesPerPixel = getFormatBytesPerBlock(format);

        uint32_t mipWidth, mipHeight;
        mColorAttachments[rtIndex].pTexture->getMipLevelImageSize(mColorAttachments[rtIndex].mipLevel, mipWidth, mipHeight);
        
        uint32_t bufferSize = mipWidth * mipHeight * bytesPerPixel;
        std::vector<uint8_t> data(bufferSize);

        // Grab the frame from GL memory (equiv to ScreenCapture::captureToMemory())
        gl_call(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        gl_call(glNamedFramebufferReadBuffer(getApiHandle(), GL_COLOR_ATTACHMENT0 + rtIndex));
        GLenum glFormat = getGlBaseFormat(format);
        GLenum glType = getGlFormatType(format);

        // Store the current read FB
        GLint boundFB;
        gl_call(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundFB));

        // Capture
        gl_call(glBindFramebuffer(GL_READ_FRAMEBUFFER, getApiHandle()));
        gl_call( glReadPixels( 0, 0, mipWidth, mipHeight, glFormat, glType, data.data() ) );
        
        // Restore the read FB
        gl_call(glBindFramebuffer(GL_READ_FRAMEBUFFER, boundFB));

        Bitmap::saveImage(filename, mipWidth, mipHeight, fileFormat, exportFlags, bytesPerPixel, false, data.data());
    }


	void Fbo::setZeroAttachments(uint32_t width, uint32_t height, uint32_t layers, uint32_t samples, bool fixedSampleLocs){
		if (mApiHandle != 0)
		{
			assert(Falcor::checkExtensionSupport("GL_ARB_framebuffer_no_attachments"));
			
			//Todo: detach everything else !!
			gl_call(glNamedFramebufferParameteriEXT(mApiHandle, GL_FRAMEBUFFER_DEFAULT_WIDTH, width));
			gl_call(glNamedFramebufferParameteriEXT(mApiHandle, GL_FRAMEBUFFER_DEFAULT_HEIGHT, height));

			gl_call(glNamedFramebufferParameteriEXT(mApiHandle, GL_FRAMEBUFFER_DEFAULT_LAYERS, layers));
			gl_call(glNamedFramebufferParameteriEXT(mApiHandle, GL_FRAMEBUFFER_DEFAULT_SAMPLES, samples));

			gl_call(glNamedFramebufferParameteriEXT(mApiHandle, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, int(fixedSampleLocs)));


			mWidth = width;
			mHeight = height;
			mDepth = layers;
			if (layers > 1)
				mIsLayered = true;
			mSampleCount = samples;


			mIsDirty = true;
			mIsZeroAttachment = true;
		}
		else
		{
			logError("Can't create zero-attachment with the default FBO. Ignoring call");
		}

	}

}
#endif //#ifdef FALCOR_GL
