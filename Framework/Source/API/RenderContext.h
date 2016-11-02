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
#include <stack>
#include <vector>
#include "API/Sampler.h"
#include "API/FBO.h"
#include "API/VAO.h"
#include "API/ShaderStorageBuffer.h"
#include "API/Texture.h"
#include "Framework.h"
#include "API/PipelineState.h"
#include "API/ProgramVars.h"

namespace Falcor
{

    /** The rendering context. State object binding and drawing commands are issued through the rendering context. \n
        This class also helps with state management. Unlike the default API state objects, it is initialized with a renderable state. See state objects assigments functions for the defaults.
    */
    class RenderContext : public std::enable_shared_from_this<RenderContext>
    {
    public:
        using SharedPtr = std::shared_ptr<RenderContext>;
        using SharedConstPtr = std::shared_ptr<const RenderContext>;

        /** Primitive topology
        */
        enum class Topology
        {
            PointList,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip
        };
        
        struct Viewport
        {
            float originX  = 0;
            float originY  = 0;
            float width    = 0;
            float height   = 0;
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

        /** create a new object
        */
        static SharedPtr create(uint32_t allocatorsCount = 1);

		/** Get the list API handle
		*/
		CommandListHandle getCommandListApiHandle() const;

		/** Reset
		*/
		void reset();

		void clearFbo(const Fbo* pFbo, const glm::vec4& color, float depth, uint8_t stencil, FboAttachmentType flags);
        void resourceBarrier(const Texture* pTexture, D3D12_RESOURCE_STATES state);

        /** Destructor
        */
        ~RenderContext();

        /** Ordered draw call
            \param[in] vertexCount Number of vertices to draw
            \param[in] startVertexLocation The location of the first vertex to read from the vertex buffers (offset in vertices)
        */
        void draw(uint32_t vertexCount, uint32_t startVertexLocation);

        /** Ordered instanced draw call
        \param[in] vertexCount Number of vertices to draw
        \param[in] instanceCount Number of instances to draw
        \param[in] startVertexLocation The location of the first vertex to read from the vertex buffers (offset in vertices)
        \param[in] startInstanceLocation A value which is added to each index before reading per-instance data from the vertex buffer
        */
        void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation);

        /** Indexed draw call
            \param[in] indexCount Number of indices to draw
            \param[in] startIndexLocation The location of the first index to read from the index buffer (offset in indices)
            \param[in] baseVertexLocation A value which is added to each index before reading a vertex from the vertex buffer
        */
        void drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation);

        /** Indexed instanced draw call
            \param[in] indexCount Number of indices to draw per instance
            \param[in] instanceCount Number of instances to draw
            \param[in] startIndexLocation The location of the first index to read from the index buffer (offset in indices)
            \param[in] baseVertexLocation A value which is added to each index before reading a vertex from the vertex buffer
            \param[in] startInstanceLocation A value which is added to each index before reading per-instance data from the vertex buffer
        */
        void drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation);

        /** Get current FBO.
        */
        Fbo::SharedConstPtr getFbo() const;

        /** Set a new FBO. This function doesn't store the current FBO state.
            \param[in] pFbo - a new FBO object. If nullptr is used, will detach the current FBO
        */
        void setFbo(const Fbo::SharedPtr& pFbo);
        /** Set a new FBO and store the current FBO into a stack. Useful for multi-pass effects.
            \param[in] pFbo - a new FBO object. If nullptr is used, will bind an empty framebuffer object
        */
        void pushFbo(const Fbo::SharedPtr& pFbo);
        /** Restore the last FBO pushed into the FBO stack. If the stack is empty, will log an error.
        */
        void popFbo();

        /** Blits (low-level copy) a source region of one FBO into the target region of another FBO. Supports only one-RT FBOs, only near and linear filtering.
            \param[in] pSource Source FBO to copy from
            \param[in] pTarget Target FBO to copy to
            \param[in] srcRegion Source region to copy from (format: x0, y0, x1, y1)
            \param[in] dstRegion Target region to copy to (format: x0, y0, x1, y1)
            \param[in] useLinearFiltering Choice between linear (true) and nearest (false) filtering for resampling
            \param[in] copyFlags indicate what surfaces of an FBO should be copied (options: color buffer, depth, and stencil buffers)
            \param[in] srcIdx index of a source color attachment to copy
            \param[in] dstIdx index of a destination color attachment to copy
        */
        void blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, bool useLinearFiltering = false, FboAttachmentType copyFlags = FboAttachmentType::Color, uint32_t srcIdx = 0, uint32_t dstIdx = 0);

        /** Store the entire rendering-state, including the FBO, into a stack.
        */
        void pushState();
        /** Restore the last state saved into the stack. If the stack is empty will log an error.\n
            Using this function might lead to undesirable performance degradation. Use with care.
        */
        void popState();

        /** Set a new vertex array object. By default, no VAO is bound.
            \param[in] pVao The Vao object to bind. If this is nullptr, will unbind the current VAO.
        */
        void setVao(const Vao::SharedConstPtr& pVao);

        /** Set the active primitive topology. Be default, this is set to Topology#TriangleList.
            \param[in] Topology The primitive topology
        */
        void setTopology(Topology topology);

        /** Set the stencil reference value
        */
        uint8_t setStencilRef(uint8_t refValue) { mState.stencilRef = refValue; applyStencilRef(); }
        /** Get the current stencil ref
        */
        uint8_t getStencilRef() const { return mState.stencilRef; }

        /** Set a viewport.
        \param[in] index Viewport index
        \param[in] vp Viewport to set
        */
        void setViewport(uint32_t index, const Viewport& vp);

        /** Get a viewport.
        \param[in] index Viewport index
        */
        const Viewport& getViewport(uint32_t index) const;

        /** Push the current viewport and sets a new one
        */
        void pushViewport(uint32_t index, const Viewport& vp);

        /** Pops the last viewport from the stack and sets it
        */
        void popViewport(uint32_t index);

        /** Set a scissor.
        \param[in] index Scissor index
        \param[in] sc Scissor to set
        */
        void setScissor(uint32_t index, const Scissor& sc);

        /** Get a Scissor.
        \param[in] index scissor index
        */
        const Scissor& getScissor(uint32_t index) const;

        /** Push the current Scissor and sets a new one
        */
        void pushScissor(uint32_t index, const Scissor& sc);

        /** Pops the last Scissor from the stack and sets it
        */
        void popScissor(uint32_t index);

        /** Set the render state
        */
        void setPipelineState(const PipelineState::SharedPtr& pState);

        /** Get the bound render state
        */
        PipelineState::SharedConstPtr getRenderState() const { return mState.pRenderState; }

        /** Set the program variables
        */
        void setProgramVariables(const ProgramVars::SharedPtr& pVars) { mState.pProgramVars = pVars; applyProgramVars(); }
        
        /** Get the bound program variables object
        */
        ProgramVars::SharedPtr getProgramVars() const { return mState.pProgramVars; }

        void setPipelineStateCache(const PipelineStateCache::SharedPtr& pPsoCache) { mState.pPsoCache = pPsoCache; }
        PipelineStateCache::SharedPtr getPipelineStateCache() const { return mState.pPsoCache; }
    private:
        RenderContext() = default;
        void initCommon(uint32_t viewportCount);

        struct State
        {
            Fbo::SharedPtr pFbo;
            Vao::SharedConstPtr pVao = nullptr;
            Topology topology = Topology::TriangleList;
            uint8_t stencilRef = 0;
            ProgramVars::SharedPtr pProgramVars;
            std::vector<ShaderStorageBuffer::SharedConstPtr> pShaderStorageBuffers;
            std::vector<Viewport> viewports;
            std::vector<Scissor> scissors;
            PipelineState::SharedPtr pRenderState;
            PipelineStateCache::SharedPtr pPsoCache;
        };

        State mState;
        std::stack<State> mStateStack;
        std::stack<Fbo::SharedPtr> mFboStack;
        std::vector<std::stack<Viewport>> mVpStack;
        std::vector<std::stack<Scissor>> mScStack;

        // Default state objects
        Fbo::SharedPtr mpEmptyFBO;

        // Internal functions used by the API layers
        void applyViewport(uint32_t index);
        void applyScissor(uint32_t index);
        void applyPipelineState();
        void applyStencilRef();
        void applyVao();
        void applyFbo();
        void applyProgramVars();
        void applyTopology();
        void prepareForDraw();
        void prepareForDrawApi();

		void* mpApiData = nullptr;
    };
}