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
#include "API/FBO.h"
#include "Graphics/FullScreenPass.h"
#include "API/Sampler.h"
#include "API/ProgramVars.h"
#include <memory>

namespace Falcor
{
    class RenderContext;
    class Texture;
    class Fbo;

    /** Gaussian-blur technique
    */
    class GaussianBlur
    {
    public:
        using UniquePtr = std::unique_ptr<GaussianBlur>;
        /** Destructor
        */
        ~GaussianBlur();

        /** Create a new object
        */
        static UniquePtr create(uint32_t kernelSize);

        /** Run the tone-mapping program
            \param pRenderContext Render-context to use
            \param pSrc The source FBO
            \param pDst The destination FBO
        */
        void execute(RenderContext* pRenderContext, Texture::SharedPtr pSrc, Fbo::SharedPtr pDst);

        uint32_t getKernelSize() const { return mKernelSize; }

        void setViewportMask(uint32_t mask);
    private:
        GaussianBlur(uint32_t kernelSize);
        uint32_t mKernelSize;
        uint32_t vpMask;
        void createTmpFbo(const Texture* pSrc);
        void createProgram();

        FullScreenPass::UniquePtr mpHorizontalBlur;
        FullScreenPass::UniquePtr mpVerticalBlur;
        Fbo::SharedPtr mpTmpFbo;
        Sampler::SharedPtr mpSampler;

        GraphicsVars::SharedPtr mpVars;
    };
}