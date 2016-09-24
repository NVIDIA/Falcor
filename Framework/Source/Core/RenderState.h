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
#include "Core/VertexLayout.h"
#include "Core/FBO.h"
#include "Core/ProgramVersion.h"
#include "Core/RasterizerState.h"
#include "Core/DepthStencilState.h"
#include "Core/BlendState.h"

namespace Falcor
{
    class RenderContext;

    class RenderState
    {
    public:
        using SharedPtr = std::shared_ptr<RenderState>;
        using SharedConstPtr = std::shared_ptr<const RenderState>;
        using ApiHandle = RenderStateHandle;

        /** Primitive topology
        */
        enum class PrimitiveType
        {
            Undefined,
            Point,
            Line,
            Triangle,
            Patch,
        };

        class Desc
        {
        public:
            Desc& setVertexLayout(VertexLayout::SharedConstPtr pLayout) { mpLayout = pLayout; return *this; }
            Desc& setFbo(Fbo::SharedConstPtr pFbo) { mpFbo = pFbo; return *this; }
            Desc& setProgramVersion(ProgramVersion::SharedConstPtr pProgram) { mpProgram = pProgram; return *this; }
            Desc& setBlendState(BlendState::SharedConstPtr pBlendState) { mpBlendState = pBlendState; return *this; }
            Desc& setRasterizerState(RasterizerState::SharedConstPtr pRasterizerState) { mpRasterizerState = pRasterizerState; return *this; }
            Desc& setDepthStencilState(DepthStencilState::SharedConstPtr pDepthStencilState) { mpDepthStencilState = pDepthStencilState; return *this; }
            Desc& setSampleMask(uint32_t sampleMask) { mSampleMask = sampleMask; return *this; }
            Desc& setPrimitiveType(PrimitiveType type) { mPrimType = type; return *this; }
        private:
            friend class RenderState;
            VertexLayout::SharedConstPtr mpLayout;
            Fbo::SharedConstPtr mpFbo;
            ProgramVersion::SharedConstPtr mpProgram;
            RasterizerState::SharedConstPtr mpRasterizerState;
            DepthStencilState::SharedConstPtr mpDepthStencilState;
            BlendState::SharedConstPtr mpBlendState;
            uint32_t mSampleMask = uint32_t(-1);
            PrimitiveType mPrimType = PrimitiveType::Undefined;
        };

        static SharedPtr create(const Desc& desc);

        ApiHandle getApiHandle() { return mApiHandle; }

        const Desc& getDesc() const { return mDesc; }

        bool bindToContext(RenderContext* pContext);
    private:
        RenderState(const Desc& desc) : mDesc(desc) {}
        Desc mDesc;
        ApiHandle mApiHandle;

        // Default state objects
        static BlendState::SharedPtr spDefaultBlendState;
        static RasterizerState::SharedPtr spDefaultRasterizerState;
        static DepthStencilState::SharedPtr spDefaultDepthStencilState;

        bool apiInit();
    };
}