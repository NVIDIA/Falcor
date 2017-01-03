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
#include "API/StructuredBuffer.h"
#include "API/Texture.h"
#include "Framework.h"
#include "API/GraphicsStateObject.h"
#include "API/ProgramVars.h"
#include "Graphics/GraphicsState.h"
#include "API/LowLevel/CopyContext.h"
#include "Graphics/ComputeState.h"
#include "API/LowLevel/CopyContext.h"

namespace Falcor
{

    /** The rendering context. Use it to bind state and dispatch calls to the GPU
    */
    class RenderContext : public CopyContext, public inherit_shared_from_this<CopyContext, RenderContext>
    {
    public:
        using SharedPtr = std::shared_ptr<RenderContext>;
        using SharedConstPtr = std::shared_ptr<const RenderContext>;

        /** create a new object
        */
        static SharedPtr create(uint32_t allocatorsCount = 1);

		/** Get the list API handle
		*/
		CommandListHandle getCommandListApiHandle() const;

        CommandQueueHandle getCommandQueue() const;

        /** Clear an FBO
            \param[in] pFbo The FBO to clear
            \param[in] color The clear color for the bound render-targets
            \param[in] depth The depth clear value
            \param[in] stencil The stencil clear value
            \param[in] flags Optional. Which components of the FBO to clear. By default will clear all attached resource.
            If you'd like to clear a specific color target, you can use RenderContext#clearFboColorTarget().
        */
		void clearFbo(const Fbo* pFbo, const glm::vec4& color, float depth, uint8_t stencil, FboAttachmentType flags = FboAttachmentType::All);

        /** Clear a render-target view
            \param[in] pRtv The RTV to clear
            \param[in] color The clear color
        */
        void clearRtv(const RenderTargetView* pRtv, const glm::vec4& color);

        /** Clear a depth-stencil view
            \param[in] pDsv The DSV to clear
            \param[in] depth The depth clear value
            \param[in] stencil The stencil clear value
            \param[in] clearDepth Optional. Controls whether or not to clear the depth channel
            \param[in] clearStencil Optional. Controls whether or not to clear the stencil channel
            */
        void clearDsv(const DepthStencilView* pDsv, float depth, uint8_t stencil, bool clearDepth = true, bool clearStencil = true);

        /** Clear an unordered-access view
            \param[in] pUav The UAV to clear
            \param[in] value The clear value
        */
        void clearUAV(const UnorderedAccessView* pUav, const vec4& value);

        /** Clear an unordered-access view
            \param[in] pUav The UAV to clear
            \param[in] value The clear value
        */
        void clearUAV(const UnorderedAccessView* pUav, const uvec4& value);

        void resourceBarrier(const Resource* pResource, Resource::State newState);

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

        /** Set the program variables for graphics
        */
        void setGraphicsVars(const GraphicsVars::SharedPtr& pVars) { mpGraphicsVars = pVars; applyProgramVars(); }
        
        /** Get the bound graphics program variables object
        */
        GraphicsVars::SharedPtr getGraphicsVars() const { return mpGraphicsVars; }

        /** Push the current graphics vars and sets a new one
        */
        void pushGraphicsVars(const GraphicsVars::SharedPtr& pVars);

        /** Pops the last ProgramVars from the stack and sets it
        */
        void popGraphicsVars();

        /** Set a graphics state
        */
        void setGraphicsState(const GraphicsState::SharedPtr& pState) { mpGraphicsState = pState; applyGraphicsState(); }
        
        /** Get the currently bound graphics state
        */
        GraphicsState::SharedPtr getGraphicsState() const { return mpGraphicsState; }

        /** Push the current graphics state and sets a new one
        */
        void pushGraphicsState(const GraphicsState::SharedPtr& pState);

        /** Pops the last graphics state from the stack and sets it
        */
        void popGraphicsState();

        /** Set the compute variables
        */
        void setComputeVars(const ComputeVars::SharedPtr& pVars) { mpComputeVars = pVars; applyComputeVars(); }

        /** Get the bound program variables object
        */
        ComputeVars::SharedPtr getComputeVars() const { return mpComputeVars; }

        /** Push the current compute vars and sets a new one
        */
        void pushComputeVars(const ComputeVars::SharedPtr& pVars);

        /** Pops the last ProgramVars from the stack and sets it
        */
        void popComputeVars();

        /** Set a compute state
        */
        void setComputeState(const ComputeState::SharedPtr& pState) { mpComputeState = pState; applyComputeState(); }

        /** Get the currently bound compute state
        */
        ComputeState::SharedPtr getComputeState() const { return mpComputeState; }

        /** Push the current compute state and sets a new one
        */
        void pushComputeState(const ComputeState::SharedPtr& pState);

        /** Pops the last PipelineState from the stack and sets it
        */
        void popComputeState();
        
        /** Dispatch a compute task
        */
        void dispatch(uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ);

        void copyResource(const Resource* pDst, const Resource* pSrc);

        GpuFence::SharedPtr getFence() const;
    private:
        RenderContext();
        GraphicsVars::SharedPtr mpGraphicsVars;
        GraphicsState::SharedPtr mpGraphicsState;

        ComputeVars::SharedPtr mpComputeVars;
        ComputeState::SharedPtr mpComputeState;

        std::stack<GraphicsState::SharedPtr> mPipelineStateStack;
        std::stack<GraphicsVars::SharedPtr> mpGraphicsVarsStack;
        std::stack<ComputeState::SharedPtr> mpComputeStateStack;
        std::stack<ComputeVars::SharedPtr> mpComputeVarsStack;

        // Internal functions used by the API layers
        void applyProgramVars();
        void applyGraphicsState();
        void applyComputeState();
        void applyComputeVars();
        void prepareForDraw();
        void prepareForDispatch();
    };
}