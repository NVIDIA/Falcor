/***************************************************************************
# Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
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
#include <memory>
#include "API/Sampler.h"
#include "API/Texture.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Program.h"
#include "API/ConstantBuffer.h"
#include "API/DepthStencilState.h"
#include "API/RasterizerState.h"
#include "API/BlendState.h"

namespace Falcor
{
    class RenderContext;

    class SkyBox
    {
    public:
        using UniquePtr = std::unique_ptr<SkyBox>;
        static UniquePtr create(Texture::SharedPtr& pTexture, Sampler::SharedPtr pSampler = nullptr, bool renderStereo = false);
        static UniquePtr createFromTexture(const std::string& textureName, bool loadAsSrgb = true, Sampler::SharedPtr pSampler = nullptr, bool renderStereo = false);

        void render(RenderContext* pRenderCtx, Camera* pCamera);
        void setSampler(Sampler::SharedPtr pSampler);

        Texture::SharedPtr getTexture() const;
        Sampler::SharedPtr getSampler() const;

        void setScale(float scale) { mScale = scale; }
        float getScale() const { return mScale; }
    private:
        SkyBox() = default;
        bool createResources(Texture::SharedPtr& pTexture, Sampler::SharedPtr pSampler, bool renderStereo);

        size_t mMatOffset;
        size_t mScaleOffset;

        float mScale = 1;
        Model::SharedPtr mpCubeModel;
        GraphicsProgram::SharedPtr mpProgram;
        GraphicsVars::SharedPtr mpVars;
        GraphicsState::SharedPtr mpState;
    };
}
