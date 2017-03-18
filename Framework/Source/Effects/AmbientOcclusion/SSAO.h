/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
//#include "Graphics/GraphicsState.h"
#include "API/ProgramVars.h"
#include "Graphics/FullScreenPass.h"
#include "Data/Effects/SSAOData.h"
#include "Effects/Utils/GaussianBlur.h"
#include "Utils/Gui.h"

namespace Falcor
{
    class Gui;
    class Camera;

    class SSAO
    {
    public:
        using UniquePtr = std::unique_ptr<SSAO>;

        enum class SampleDistribution
        {
            Random,
            UniformHammersley,
            CosineHammersley
        };

        /** Create an SSAO pass object sampling with a hemisphere kernel by default.
            \param[in] aoMapWidth Width of the AO map texture
            \param[in] aoMapHeight Height of the AO map texture
            \param[in] kernelSize Number of samples in the AO kernel
            \param[in] blurSize Kernel size used for blurring the AO map
            \param[in] noiseWidth Width of the noise texture
            \param[in] noiseHeight Height of the noise texture
            \param[in] distribution Distribution of sample points when using a hemisphere kernel.
            \return SSAO pass object.
        */
        static UniquePtr create(uint32_t aoMapWidth, uint32_t aoMapHeight, uint32_t kernelSize = 16, uint32_t blurSize = 5, uint32_t noiseWidth = 16, uint32_t noiseHeight = 16, SampleDistribution distribution = SampleDistribution::UniformHammersley);

        /** Render GUI for tweaking SSAO settings
        */
        void renderGui(Gui* pGui);

        /** Generate the AO map
            \param[in] pContext Render context
            \param[in] pCamera Camera used to render the scene
            \param[in] pDepthTexture Scene depth buffer
            \param[in] pNormalTexture Scene normals buffer. If valid, AO is generated with hemisphere kernel, sphere kernel otherwise. Switching kernel shapes triggers shader recompile.
            \return AO map texture
        */
        Texture::SharedPtr generateAOMap(RenderContext* pContext, const Camera* pCamera, const Texture::SharedPtr& pDepthTexture, const Texture::SharedPtr& pNormalTexture = nullptr);

        /** Recreates the Gaussian blur pass with the desired blur kernel size.
            \param[in] size Blur kernel size
        */
        void setBlurKernelSize(uint32_t size) { mpBlur = GaussianBlur::create(size); }

        /** Recreate sampling kernel
            \param[in] kernelSize Number of samples
            \param[in] distribution Distribution of sample points within a hemisphere kernel. Parameter is ignored for sphere kernel generation, but is saved for use in future hemisphere kernels.
        */
        void setKernel(uint32_t kernelSize, SampleDistribution distribution = SampleDistribution::Random);

        /** Recreate noise texture
            \param[in] width Noise texture width
            \param[in] height Noise texture height
        */
        void setNoiseTexture(uint32_t width, uint32_t height);

    private:

        SSAO(uint32_t aoMapWidth, uint32_t aoMapHeight, uint32_t kernelSize, uint32_t blurSize, uint32_t noiseWidth, uint32_t noiseHeight, SampleDistribution distribution);

        void upload();

        enum class KernelShape
        {
            Sphere,
            Hemisphere
        };

        KernelShape mKernelShape = KernelShape::Hemisphere;

        void initShader();

        SSAOData mData;
        bool mDirty = false;

        Fbo::SharedPtr mpAOFbo;
        GraphicsState::SharedPtr mpSSAOState;
        Sampler::SharedPtr mpPointSampler;

        Texture::SharedPtr mpNoiseTexture;

        uint32_t mHemisphereDistribution = (uint32_t)SampleDistribution::Random;

        static const Gui::DropdownList kKernelDropdown;
        static const Gui::DropdownList kDistributionDropdown;

        FullScreenPass::UniquePtr mpSSAOPass;
        GraphicsVars::SharedPtr mpSSAOVars;

        bool mApplyBlur = true;
        GaussianBlur::UniquePtr mpBlur;
    };

}