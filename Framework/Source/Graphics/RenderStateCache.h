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
#include "Core/RenderState.h"
#include "Graphics/Program.h"
#include "Core/VAO.h"
#include "Core/FBO.h"
#include "Core/RasterizerState.h"
#include "Core/DepthStencilState.h"
#include "Core/BlendState.h"

namespace Falcor
{
    class RenderStateCache
    {
    public:
        using SharedPtr = std::shared_ptr<RenderStateCache>;
        using SharedConstPtr = std::shared_ptr<const RenderStateCache>;
        
        static SharedPtr create() { return SharedPtr(new RenderStateCache); }
        RenderStateCache& SetVao(const Vao::SharedConstPtr& pVao) { mpVao = pVao; return *this; }
        RenderStateCache& setFbo(const Fbo::SharedConstPtr& pFbo) { mpFbo = pFbo; return *this; }
        RenderStateCache& setProgram(const Program::SharedConstPtr& pProgram) { mpProgram = pProgram; return *this; }
        RenderStateCache& setBlendState(BlendState::SharedConstPtr pBlendState) { mDesc.setBlendState(pBlendState); return *this; }
        RenderStateCache& setRasterizerState(RasterizerState::SharedConstPtr pRasterizerState) { mDesc.setRasterizerState(pRasterizerState); return *this; }
        RenderStateCache& setDepthStencilState(DepthStencilState::SharedConstPtr pDepthStencilState) { mDesc.setDepthStencilState(pDepthStencilState); return *this; }
        RenderStateCache& setSampleMask(uint32_t sampleMask) { mDesc.setSampleMask(sampleMask); return *this; }
        RenderStateCache& setPrimitiveType(RenderState::PrimitiveType type) { mDesc.setPrimitiveType(type); return *this; }

        RenderState::SharedPtr getRenderState();

        Vao::SharedConstPtr getVao() const { return mpVao; }
        Fbo::SharedConstPtr getFbo() const { return mpFbo; }
        Program::SharedConstPtr getProgram() const { return mpProgram; }
        BlendState::SharedConstPtr getBlendState() const { return mDesc.getBlendState(); }
        RasterizerState::SharedConstPtr getRasterizerState() const { return mDesc.getRasterizerState(); }
        DepthStencilState::SharedConstPtr getDepthStencilState() const { return mDesc.getDepthStencilState(); }
        uint32_t getSampleMask() const { return mDesc.getSampleMask(); }
        RenderState::PrimitiveType getPrimitiveType() const { return mDesc.getPrimitiveType(); }
    private:
        RenderStateCache() = default;
        Vao::SharedConstPtr mpVao;
        Fbo::SharedConstPtr mpFbo;
        Program::SharedConstPtr mpProgram;

        RenderState::Desc mDesc;
    };
}