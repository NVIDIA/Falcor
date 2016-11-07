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
#include "API/PipelineStateObject.h"
#include "Graphics/Program.h"
#include "API/VAO.h"
#include "API/FBO.h"
#include "API/RasterizerState.h"
#include "API/DepthStencilState.h"
#include "API/BlendState.h"

namespace Falcor
{
    /** Pipeline state.
        This class contains the entire state required by a single draw-call. It's not an immutable object - you can change it dynamically during rendering.
        The recommended way to use it is to create multiple PipelineState objects (ideally, a single object per render-pass)
    */
    class PipelineState
    {
    public:
        using SharedPtr = std::shared_ptr<PipelineState>;
        using SharedConstPtr = std::shared_ptr<const PipelineState>;
        
        struct Viewport
        {
            float originX = 0;
            float originY = 0;
            float width = 0;
            float height = 0;
            float minDepth = 0;
            float maxDepth = 1;
        };

        struct Scissor
        {
            Scissor() = default;
            Scissor(int32_t x, int32_t y, int32_t w, int32_t h) : originX(x), originY(y), width(w), height(h) {}
            int32_t originX = 0;
            int32_t originY = 0;
            int32_t width = 0;
            int32_t height = 0;
        };

        /** Create a new object
        */
        static SharedPtr create() { return SharedPtr(new PipelineState); }

        /** Copy constructor. Useful if you need to make minor changes to an already existing object
        */
        SharedPtr operator=(const SharedPtr& other);

        /** Get current FBO.
        */
        Fbo::SharedConstPtr getFbo() const { return mpFbo; }

        /** Set a new FBO. This function doesn't store the current FBO state.
        \param[in] pFbo - a new FBO object. If nullptr is used, will detach the current FBO
        */
        PipelineState& setFbo(const Fbo::SharedConstPtr& pFbo) { mpFbo = pFbo; return *this; }
        
        /** Set a new vertex array object. By default, no VAO is bound.
        \param[in] pVao The Vao object to bind. If this is nullptr, will unbind the current VAO.
        */
        PipelineState& setVao(const Vao::SharedConstPtr& pVao) { mpVao = pVao; return *this; }

        /** Get the currently bound VAO
        */
        Vao::SharedConstPtr getVao() const { return mpVao; }

        /** Set the stencil reference value
        */
        PipelineState& setStencilRef(uint8_t refValue) { mStencilRef = refValue;}

        /** Get the current stencil ref
        */
        uint8_t getStencilRef() const { return mStencilRef; }

        /** Set a viewport.
        \param[in] index Viewport index
        \param[in] vp Viewport to set
        */
        void setViewport(uint32_t index, const Viewport& vp);

        /** Get a viewport.
        \param[in] index Viewport index
        */
        const Viewport& getViewport(uint32_t index) const;

        /** Set a scissor.
        \param[in] index Scissor index
        \param[in] sc Scissor to set
        */
        void setScissor(uint32_t index, const Scissor& sc);

        /** Get a Scissor.
        \param[in] index scissor index
        */
        const Scissor& getScissor(uint32_t index) const;

        /** Bind a program to the pipeline
        */
        PipelineState& setProgram(const Program::SharedPtr& pProgram) { mpProgram = pProgram; return *this; }

        /** Get the currently bound program
        */
        Program::SharedPtr getProgram() const { return mpProgram; }

        /** Set a blend-state
        */
        PipelineState& setBlendState(BlendState::SharedPtr pBlendState) { mDesc.setBlendState(pBlendState); return *this; }

        /** Get the currently bound blend-state
        */
        BlendState::SharedPtr getBlendState() const { return mDesc.getBlendState(); }

        /** Set a rasterizer-state
        */
        PipelineState& setRasterizerState(RasterizerState::SharedPtr pRasterizerState) { mDesc.setRasterizerState(pRasterizerState); return *this; }

        /** Get the currently bound rasterizer-state
        */
        RasterizerState::SharedPtr getRasterizerState() const { return mDesc.getRasterizerState(); }

        /** Set a depth-stencil state
        */
        PipelineState& setDepthStencilState(DepthStencilState::SharedPtr pDepthStencilState) { mDesc.setDepthStencilState(pDepthStencilState); return *this; }

        /** Get the currently bound depth-stencil state
        */
        DepthStencilState::SharedPtr getDepthStencilState() const { return mDesc.getDepthStencilState(); }

        /** Set the sample mask
        */
        PipelineState& setSampleMask(uint32_t sampleMask) { mDesc.setSampleMask(sampleMask); return *this; }

        /** Get the current sample mask
        */
        uint32_t getSampleMask() const { return mDesc.getSampleMask(); }

        /** Set the primitive topology
        */
        PipelineState& setRootSignature(RootSignature::SharedPtr pSignature) { mpRootSignature = pSignature; mCachedData.isUserRootSignature = (mpRootSignature == nullptr); }

        /** Get the active PSO
        */
        PipelineStateObject::SharedPtr getPSO();

        /** Get the current root-signature object
        */
        RootSignature::SharedPtr getRootSignature() const { return mpRootSignature; }
        
    private:
        PipelineState();
        Vao::SharedConstPtr mpVao;
        Fbo::SharedConstPtr mpFbo;
        Program::SharedPtr mpProgram;
        RootSignature::SharedPtr mpRootSignature;
        PipelineStateObject::Desc mDesc;
        uint8_t mStencilRef = 0;

        struct CachedData
        {
            const ProgramVersion* pProgramVersion = nullptr;
            bool isUserRootSignature = false;
        };
        CachedData mCachedData;
    };
}