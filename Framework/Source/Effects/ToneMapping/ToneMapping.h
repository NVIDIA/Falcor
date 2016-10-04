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
#include "Graphics/FullScreenPass.h"
#include "API/UniformBuffer.h"
#include "API/FBO.h"
#include "API/Sampler.h"
#include "Utils/Gui.h"

namespace Falcor
{
    /** Tone-mapping effect
    */
    class ToneMapping
    {
    public:
        using UniquePtr = std::unique_ptr<ToneMapping>;
        /** Destructor
        */
        ~ToneMapping();

        /** The tone-mapping operator to use
        */
        enum class Operator
        {
            Clamp,              ///< Clamp to [0, 1]. Just like LDR
            Linear,             ///< Linear mapping
            Reinhard,           ///< Reinhard operator
            ReinhardModified,   ///< Reinhard operator with maximum white intensity
            HejiHableAlu,       ///< John Hable's ALU approximation of Jim Heji's filmic operator
            HableUc2,           ///< John Hable's filmic tone-mapping used in Uncharted 2
        };

        /** Create a new object
        */
        static UniquePtr create(Operator op);

        /** Set UI elements into a give GUI and UI group
        */
        void setUiElements(Gui* pGui, const std::string& uiGroup);

        /** Run the tone-mapping program
            \param pRenderContext Render-context to use
            \param pSrc The source FBO
            \param pDst The destination FBO
        */
        void execute(RenderContext* pRenderContext, Fbo::SharedPtr pSrc, Fbo::SharedPtr pDst);

        /** Set a new operator
        */
        void setOperator(Operator op);

        /** Sets the middle-gray luminance used for normalizing each pixel's luminance. 
            Middle gray is usually in the range of[0.045, 0.72].
            Lower values maximize contrast. Useful for night scenes.
            Higher values minimize contrast, resulting in brightly lit objects.
        */
        void setMiddleGray(float middleGray);

        /** Sets the maximal luminance to be consider as pure white.
            Only valid if the operator is ReinhardModified
        */
        void setWhiteMaxLuminance(float maxLuminance);

        /** Sets the luminance texture LOD to use when fetching average luminance values.
            Lower values will result in a more localized effect
        */
        void setLuminanceLod(float lod);

        /** Sets the white-scale used in Uncharted 2 tone mapping.
        */
        void setWhiteScale(float whiteScale);

    private:
        ToneMapping(Operator op);
        void createLuminanceFbo(Fbo::SharedPtr pSrcFbo);

        Operator mOperator;
        FullScreenPass::UniquePtr mpToneMapPass;
        FullScreenPass::UniquePtr mpLuminancePass;
        Fbo::SharedPtr mpLuminanceFbo;

        UniformBuffer::SharedPtr mpUbo;
        Sampler::SharedPtr mpPointSampler;
        Sampler::SharedPtr mpLinearSampler;

        struct  
        {
            size_t colorTex;
            size_t luminanceTex;
            size_t middleGray;
            size_t maxWhiteLuminance;
            size_t luminanceLod;
            size_t whiteScale;
        } mUboOffsets;

        void createToneMapPass(Operator op);
        void createLuminancePass();
        float mMiddleGray = 0.18f;
        float mWhiteMaxLuminance = 1.0f;
        float mLuminanceLod = 16; // Max possible LOD, will result in global operation
        float mWhiteScale = 11.2f;

        // UI callback
        static void GUI_CALL getToneMapOperator(void* pVal, void* pThis);
        static void GUI_CALL setToneMapOperator(const void* pVal, void* pThis);
    };
}