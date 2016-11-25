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
#include <stack>

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
            Viewport() = default;
            Viewport(float x, float y, float w, float h, float minZ, float maxZ) : originX(x), originY(y), width(w), height(h), minDepth(minZ), maxDepth(maxZ) {}
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
        static SharedPtr create() { return SharedPtr(new PipelineState()); }

        /** Copy constructor. Useful if you need to make minor changes to an already existing object
        */
        SharedPtr operator=(const SharedPtr& other);

        /** Get current FBO.
        */
        Fbo::SharedConstPtr getFbo() const { return mpFbo; }

        /** Set a new FBO. This function doesn't store the current FBO state.
        \param[in] pFbo - a new FBO object. If nullptr is used, will detach the current FBO
        \param[in] setVp0Sc0 If true, will set the viewport 0 and scissor 0 to match the FBO dimensions
        */
        PipelineState& setFbo(const Fbo::SharedConstPtr& pFbo, bool setVp0Sc0 = true);
        
        /** Set a new FBO and store the current FBO into a stack. Useful for multi-pass effects.
            \param[in] pFbo - a new FBO object. If nullptr is used, will bind an empty framebuffer object
            \param[in] setVp0Sc0 If true, will set the viewport 0 and scissor 0 to match the FBO dimensions
            */
        void pushFbo(const Fbo::SharedPtr& pFbo, bool setVp0Sc0 = true);
        
        /** Restore the last FBO pushed into the FBO stack. If the stack is empty, will log an error.
        \param[in] setVp0Sc0 If true, will set the viewport 0 and scissor 0 to match the FBO dimensions
        */
        void popFbo(bool setVp0Sc0 = true);

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
        \param[in] setScissors If true, will set the corresponding scissors entry
        */
        void setViewport(uint32_t index, const Viewport& vp, bool setScissors = true);

        /** Get a viewport.
        \param[in] index Viewport index
        */
        const Viewport& getViewport(uint32_t index) const { return mViewports[index]; }

        /** Push the current viewport and sets a new one
        \param[in] index Viewport index
        \param[in] vp Viewport to set
        \param[in] setScissors If true, will set the corresponding scissors entry
        */
        void pushViewport(uint32_t index, const Viewport& vp, bool setScissors = true);

        /** Pops the last viewport from the stack and sets it
        \param[in] index Viewport index
        \param[in] setScissors If true, will set the corresponding scissors entry
        */
        void popViewport(uint32_t index, bool setScissors = true);

        /** Set a scissor.
        \param[in] index Scissor index
        \param[in] sc Scissor to set
        */
        void setScissors(uint32_t index, const Scissor& sc);

        /** Get a Scissor.
        \param[in] index scissor index
        */
        const Scissor& getScissors(uint32_t index) const { return mScissors[index]; }

        /** Push the current Scissor and sets a new one
        */
        void pushScissors(uint32_t index, const Scissor& sc);

        /** Pops the last Scissor from the stack and sets it
        */
        void popScissors(uint32_t index);

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
        std::vector<Viewport> mViewports;
        std::vector<Scissor> mScissors;

        std::stack<Fbo::SharedConstPtr> mFboStack;
        std::vector<std::stack<Viewport>> mVpStack;
        std::vector<std::stack<Scissor>> mScStack;

        struct CachedData
        {
            const ProgramVersion* pProgramVersion = nullptr;
            bool isUserRootSignature = false;
        };
        CachedData mCachedData;
    };
}