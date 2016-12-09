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
#include "API/Sampler.h"
#include "glm/gtc/type_ptr.hpp"

namespace Falcor
{
    GLenum getGlMagFilterMode(Sampler::Filter mag)
    {
        switch(mag)
        {
        case Sampler::Filter::Point:
            return GL_NEAREST;
        case Sampler::Filter::Linear:
            return GL_LINEAR;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    GLenum getGlMinFilterMode(Sampler::Filter min, Sampler::Filter mip)
    {
        switch(min)
        {
        case Falcor::Sampler::Filter::Point:
            switch(mip)
            {
            case Falcor::Sampler::Filter::Point:
                return GL_NEAREST_MIPMAP_NEAREST;
            case Falcor::Sampler::Filter::Linear:
                return GL_NEAREST_MIPMAP_LINEAR;
            }
            break;
        case Falcor::Sampler::Filter::Linear:
            switch(mip)
            {
            case Falcor::Sampler::Filter::Point:
                return GL_LINEAR_MIPMAP_NEAREST;
            case Falcor::Sampler::Filter::Linear:
                return GL_LINEAR_MIPMAP_LINEAR;
            }
            break;
        }
        should_not_get_here();
        return GL_NONE;
    }

    GLenum getGlComparisonMode(Sampler::ComparisonMode mode)
    {
        switch(mode)
        {
        case Sampler::ComparisonMode::NoComparison:
            return GL_NONE;
        case Sampler::ComparisonMode::LessEqual:
            return GL_LEQUAL;
        case Sampler::ComparisonMode::GreaterEqual:
            return GL_GEQUAL;
        case Sampler::ComparisonMode::Less:
            return GL_LESS;
        case Sampler::ComparisonMode::Greater:
            return GL_GREATER;
        case Sampler::ComparisonMode::Equal:
            return GL_EQUAL;
        case Sampler::ComparisonMode::NotEqual:
            return GL_NOTEQUAL;
        case Sampler::ComparisonMode::Always:
            return GL_ALWAYS;
        case Sampler::ComparisonMode::Never:
            return GL_NEVER;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    GLenum getGlAddressMode(Sampler::AddressMode mode)
    {
        switch(mode)
        {
        case Sampler::AddressMode::Wrap:
            return GL_REPEAT;
        case Sampler::AddressMode::Mirror:
            return GL_MIRRORED_REPEAT;
        case Sampler::AddressMode::Clamp:
            return GL_CLAMP_TO_EDGE;
        case Sampler::AddressMode::Border:
            return GL_CLAMP_TO_BORDER;
        case Sampler::AddressMode::MirrorOnce:
            return  GL_MIRROR_CLAMP_TO_EDGE;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    Sampler::~Sampler()
    {
        glDeleteSamplers(1, &mApiHandle);
    }

    uint32_t Sampler::getApiMaxAnisotropy()
    {
        float f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &f);
        return (uint32_t)f;
    }

    Sampler::SharedPtr Sampler::create(const Desc& desc)
    {
        auto pSampler = SharedPtr(new Sampler(desc));
        gl_call(glCreateSamplers(1, &pSampler->mApiHandle));
        uint32_t ApiHandle = pSampler->mApiHandle;

        // Set filter mode
        if((desc.mMagFilter != Filter::Linear) && (desc.mMagFilter != Filter::Point))
        {
            std::string Err = "Error in Sampler::create() - MagFilter should be either Nearest_NoMipmap or Linear_NoMipmap\n";
            logError(Err);
            return nullptr;
        }
        GLenum glMag = getGlMagFilterMode(desc.mMagFilter);
        gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_MAG_FILTER, glMag));

        GLenum glMin = getGlMinFilterMode(desc.mMinFilter, desc.mMipFilter);
        gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_MIN_FILTER, glMin));

        // Set max anisotropy
        if(desc.mMaxAnisotropy < 1 || getApiMaxAnisotropy() < desc.mMaxAnisotropy)
        {
            std::string err = "Error in Sampler::create() - MaxAnisotropy should be in range [1, " + std::to_string(getApiMaxAnisotropy()) + "]. " + std::to_string(desc.mMaxAnisotropy) + " provided)\n";
            logError(err);
            return nullptr;
        }
        gl_call(glSamplerParameterf(ApiHandle, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)desc.mMaxAnisotropy));

        // Set LOD params
        gl_call(glSamplerParameterf(ApiHandle, GL_TEXTURE_MIN_LOD, desc.mMinLod));
        gl_call(glSamplerParameterf(ApiHandle, GL_TEXTURE_MAX_LOD, desc.mMaxLod));
        gl_call(glSamplerParameterf(ApiHandle, GL_TEXTURE_LOD_BIAS, desc.mLodBias));

        // Set comparison mode
        if(desc.mComparisonMode == ComparisonMode::NoComparison)
        {
            gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_COMPARE_MODE, GL_NONE));
        }
        else
        {
            gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
            GLenum GlMode = getGlComparisonMode(desc.mComparisonMode);
            gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_COMPARE_FUNC, GlMode));
        }

        // Addressing Mode
        GLenum modeU = getGlAddressMode(desc.mModeU);
        gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_WRAP_S, modeU));

        GLenum modeV = getGlAddressMode(desc.mModeV);
        gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_WRAP_T, modeV));

        GLenum modeW = getGlAddressMode(desc.mModeW);
        gl_call(glSamplerParameteri(ApiHandle, GL_TEXTURE_WRAP_R, modeW));

        // Border color
        gl_call(glSamplerParameterfv(ApiHandle, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(desc.mBorderColor)));    

        return pSampler;
    }
}
#endif //#ifdef FALCOR_GL
