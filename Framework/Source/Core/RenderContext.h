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
#include "Core/Sampler.h"
#include "Core/DepthStencilState.h"
#include "Core/FBO.h"
#include "Core/ProgramVersion.h"
#include "Core/RasterizerState.h"
#include "Core/BlendState.h"
#include "Core/VAO.h"
#include "Core/UniformBuffer.h"
#include "Core/ShaderStorageBuffer.h"
#include "Core/Texture.h"
#include "Framework.h"

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
            int32_t originX = 0;
            int32_t originY = 0;
            int32_t width = 0;
            int32_t height = 0;
        };

        /** create a new object
        */
        static SharedPtr create();
        /** Destructor
        */
        ~RenderContext();

        /** Ordered draw call
            \param[in] vertexCount Number of vertices to draw
            \param[in] startVertexLocation The location of the first vertex to read from the vertex buffers (offset in vertices)
        */
        void draw(uint32_t vertexCount, uint32_t startVertexLocation);
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

        /** Dispatch a compute shader
        \param[in] numGroups Dimensions of the grid of groups to dispatch.
        */
        void dispatch(glm::u32vec3 numGroups);

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
            \param[in] pVao The CVao object to bind. If this is nullptr, will unbind the current VAO.
        */
        void setVao(const Vao::SharedConstPtr& pVao);
        /** Set the active primitive topology. Be default, this is set to Topology#TriangleList.
            \param[in] Topology The primitive topology
        */
        void setTopology(Topology topology);
        /** Set a new rasterizer state. The default rasterizer state is solid with backface culling enabled.
            \param[in] pRastState The rasterizer state to set. If this is nullptr, will set the default rasterizer state.
        */
        void setRasterizerState(const RasterizerState::SharedConstPtr& pRastState);

        /** Get the current rasterizer state
        */
        const RasterizerState::SharedConstPtr& getRasterizerState() const { return mState.pRastState; }

        /** Set a depth stencil state. The default depth stencil state is depth enabled, stencil disabled and depth function is less.
            \param[in] pDepthStencil The depth stencil state to set. If this is nullptr, will set the default depth stencil state.
            \param[in] stencilRef The reference value to use when performing stencil test.
        */
        void setDepthStencilState(const DepthStencilState::SharedConstPtr& pDepthStencil, uint32_t stencilRef);

        /** Get the current depth-stencil state
        */
        const DepthStencilState::SharedConstPtr& getDepthStencilState() const { return mState.pDsState; }

        /** Get the current stencil ref
        */
        const uint32_t getStencilRef() const { return mState.stencilRef; }

        static const uint32_t kSampleMaskAll = -1;
        /** Set a blend state. By default, blending is disabled for all render-targets.
            \param[in] pBlendState The blend state. If this is nullptr, will use the default blend-state.
            \param[in] sampleMask Sample coverage write mask. Use SampleMaskAll to enable output of all samples.
        */
        void setBlendState(const BlendState::SharedConstPtr& pBlendState, uint32_t sampleMask = kSampleMaskAll);

        /** Get the current blend state
        */
        const BlendState::SharedConstPtr& getBlendState() const { return mState.pBlendState; }

        /** Get the current sample mask
        */
        uint32_t getSampleMask() const { return mState.sampleMask; }

        /** Set a uniform-buffer into the state. By default no buffers are set.
            \param[in] index The uniform-buffer slot index to set the buffer into.
            \param[in] pBuffer The uniform-buffer to set. nullptr can be used to unbind a buffer from a slot.
        */
        void setUniformBuffer(uint32_t index, const UniformBuffer::SharedConstPtr& pBuffer);

        /** Set a shader storage buffer into the state. By default no buffers are set.
        \param[in] index The shader storage buffer slot index to set the buffer into.
        \param[in] pBuffer The shader storage buffer to set. nullptr can be used to unbind a buffer from a slot.
        */
        void setShaderStorageBuffer(uint32_t index, const ShaderStorageBuffer::SharedConstPtr& pBuffer);

        /** Set a program object. By default, no program is bound.
            \param[in] pProgram The program object. nullptr can be used to unbind a program.
        */
        void setProgram(const ProgramVersion::SharedConstPtr& pProgram);

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
    private:
        RenderContext(uint32_t viewportCount);

        struct State
        {
            Fbo::SharedPtr pFbo;
            Vao::SharedConstPtr pVao = nullptr;
            Topology topology = Topology::TriangleList;
            RasterizerState::SharedConstPtr pRastState = nullptr;
            DepthStencilState::SharedConstPtr pDsState = nullptr;
            uint32_t stencilRef = 0;
            BlendState::SharedConstPtr pBlendState = nullptr;
            uint32_t sampleMask = -1;
            std::vector<UniformBuffer::SharedConstPtr> pUniformBuffers;
            std::vector<ShaderStorageBuffer::SharedConstPtr> pShaderStorageBuffers;
            std::vector<Viewport> viewports;
            std::vector<Scissor> scissors;
            ProgramVersion::SharedConstPtr pProgram = nullptr;
        };

        State mState;
        std::stack<State> mStateStack;
        std::stack<Fbo::SharedPtr> mFboStack;
        std::vector<std::stack<Viewport>> mVpStack;
        std::vector<std::stack<Scissor>> mScStack;

        // Default state objects
        RasterizerState::SharedConstPtr mpDefaultRastState;
        BlendState::SharedConstPtr mpDefaultBlendState;
        DepthStencilState::SharedConstPtr mpDefaultDepthStencilState;
        Fbo::SharedPtr mpEmptyFBO;

        // Internal functions used by the API layers
        void applyViewport(uint32_t index) const;
        void applyScissor(uint32_t index) const;
        void applyRasterizerState() const;
        void applyDepthStencilState() const;
        void applyBlendState() const;
        void applyProgram() const;
        void applyVao() const;
        void applyFbo() const;
        void applyUniformBuffer(uint32_t Index) const;
        void applyShaderStorageBuffer(uint32_t Index) const;
        void applyTopology() const;
        void prepareForDraw() const;
        void prepareForDrawApi() const;
    };
}