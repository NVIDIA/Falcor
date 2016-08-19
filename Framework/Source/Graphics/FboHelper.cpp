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
#include "Core/Formats.h"
#include "FboHelper.h"
#include "Core/FBO.h"
#include "Core/Texture.h"

namespace Falcor
{
    namespace FboHelper
    {
        bool CheckParams(const std::string& Func, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t renderTargetCount, uint32_t mipLevels, uint32_t sampleCount)
        {
            std::string msg = "CFramebuffer::" + Func + "() - ";
            std::string Param;

            if(mipLevels == 0)
                Param = "mipLevels";
            else if(width == 0)
                Param = "width";
            else if(height == 0)
                Param = "height";
            else if(arraySize == 0)
                Param = "arraySize";
            else if(renderTargetCount == 0)
                Param = "renderTargetCount";
            else
            {
                if(sampleCount > 0 && mipLevels > 1)
                {
                    Logger::log(Logger::Level::Error, msg + "can't create multisampled texture with more than one mip-level. sampleCount = " + std::to_string(sampleCount) + ", mipLevels = " + std::to_string(mipLevels) + ".");
                    return false;
                }
                return true;
            }

            Logger::log(Logger::Level::Error, msg + Param + " can't be zero.");
            return false;
        }

        Fbo::SharedPtr create2D(uint32_t width, uint32_t height, const ResourceFormat formats[], uint32_t arraySize, uint32_t renderTargetCount, uint32_t sampleCount, uint32_t mipLevels)
        {
            if(CheckParams("Create2D", width, height, arraySize, renderTargetCount, mipLevels, sampleCount) == false)
            {
                return false;
            }

            Fbo::SharedPtr pFbo = Fbo::create();

            // create the color targets
            for(uint32_t i = 0; i < renderTargetCount; i++)
            {
                Texture::SharedPtr pTex;
                if(sampleCount > 0)
                {
                    pTex = Texture::create2DMS(width, height, formats[i], sampleCount, arraySize);
                }
                else
                {
                    pTex = Texture::create2D(width, height, formats[i], arraySize, mipLevels);
                }

                pFbo->attachColorTarget(pTex, i, 0, Fbo::kAttachEntireMipLevel);
            }

            return pFbo;
        }

        Fbo::SharedPtr create2DWithDepth(uint32_t width, uint32_t height, const ResourceFormat colorFormats[], ResourceFormat depthFormat, uint32_t arraySize, uint32_t renderTargetCount, uint32_t sampleCount, uint32_t mipLevels)
        {
            if(CheckParams("Create2DWithDepth", width, height, arraySize, renderTargetCount, mipLevels, sampleCount) == false)
            {
                return false;
            }

            Fbo::SharedPtr pFbo = create2D(width, height, colorFormats, arraySize, renderTargetCount, sampleCount, mipLevels);
            Texture::SharedPtr pDepth;

            if(sampleCount > 0)
            {
                pDepth = Texture::create2DMS(width, height, depthFormat, sampleCount, arraySize);
            }
            else
            {
                pDepth = Texture::create2D(width, height, depthFormat, arraySize, mipLevels);
            }

            pFbo->attachDepthStencilTarget(pDepth, 0, Fbo::kAttachEntireMipLevel);

            return pFbo;
        }

        Fbo::SharedPtr createCubemap(uint32_t width, uint32_t height, const ResourceFormat formats[], uint32_t arraySize, uint32_t renderTargetCount, uint32_t mipLevels)
        {
            if(CheckParams("CreateCubemap", width, height, arraySize, renderTargetCount, mipLevels, 0) == false)
            {
                return false;
            }

            Fbo::SharedPtr pFbo = Fbo::create();

            // create the color targets
            for(uint32_t i = 0; i < renderTargetCount; i++)
            {
                auto pTex = Texture::createCube(width, height, formats[i], arraySize, mipLevels);
                pFbo->attachColorTarget(pTex, i, 0, Fbo::kAttachEntireMipLevel);
            }

            return pFbo;
        }

        Fbo::SharedPtr createCubemapWithDepth(uint32_t width, uint32_t height, const ResourceFormat colorFormats[], ResourceFormat depthFormat, uint32_t arraySize, uint32_t renderTargetCount, uint32_t mipLevels)
        {
            if(CheckParams("CreateCubemapWithDepth", width, height, arraySize, renderTargetCount, mipLevels, 0) == false)
            {
                return false;
            }

            Fbo::SharedPtr pFbo = createCubemap(width, height, colorFormats, arraySize, renderTargetCount, mipLevels);
            auto pDepth = Texture::createCube(width, height, depthFormat, arraySize, mipLevels);
            pFbo->attachDepthStencilTarget(pDepth, 0, Fbo::kAttachEntireMipLevel);

            return pFbo;
        }

        Fbo::SharedPtr createDepthOnly(uint32_t width, uint32_t height, ResourceFormat depthFormat, uint32_t arraySize, uint32_t mipLevels)
        {
            if(CheckParams("CreateDepthOnly", width, height, arraySize, 1, mipLevels, 0) == false)
            {
                return false;
            }

            Fbo::SharedPtr pFbo = Fbo::create();
            auto pDepth = Texture::create2D(width, height, depthFormat, arraySize, mipLevels);
            pFbo->attachDepthStencilTarget(pDepth, 0, Fbo::kAttachEntireMipLevel);

            return pFbo;
        }
    }
}