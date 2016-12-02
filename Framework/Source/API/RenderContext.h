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
#include "API/PipelineStateObject.h"
#include "API/ProgramVars.h"
#include "Graphics/PipelineState.h"
#include "API/LowLevel/CopyContext.h"

namespace Falcor
{

    /** The rendering context. Use it to bind state and dispatch calls to the GPU
    */
    class RenderContext : public std::enable_shared_from_this<RenderContext>
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

		/** Reset
		*/
		void reset();

        /** Clear an FBO
            \param[in] pFbo The FBO to clear
            \param[in] color The clear color for the bound render-targets
            \param[in] depth The depth clear value
            \param[in] stencil The stencil clear value
            \param[in] flags Optional. Which components of the FBO to clear. By default will clear all attached resource.
            If you'd like to clear a specific color target, you can use RenderContext#clearFboColorTarget().
        */
		void clearFbo(const Fbo* pFbo, const glm::vec4& color, float depth, uint8_t stencil, FboAttachmentType flags = FboAttachmentType::All);

        /** Clear a specific color buffer in an FBO
            \param[in] pFbo The FBO to clear
            \param[in] rtIndex The render-target index we need to clear
            \param[in] color The clear color for the bound render-targets
        */
        void clearFboColorTarget(const Fbo* pFbo, uint32_t rtIndex, const glm::vec4& color);

        /** Clear an FBO's attached depth-stencil buffer
            \param[in] pFbo The FBO to clear
            \param[in] depth The depth clear value
            \param[in] stencil The stencil clear value
            \param[in] clearDepth Optional. Controls whether or not to clear the depth channel
            \param[in] clearStencil Optional. Controls whether or not to clear the stencil channel
            */
        void clearFboDepthStencil(const Fbo* pFbo, float depth, uint8_t stencil, bool clearDepth = true, bool clearStencil = true);

        void clearUAV(Buffer::SharedPtr pBuffer, const vec4& clear);
        void clearUAV(Buffer::SharedPtr pBuffer, const uvec4& clear);
        void clearUAV(Texture::SharedPtr pBuffer, const vec4& clear);
        void clearUAV(Texture::SharedPtr pBuffer, const uvec4& clear);

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

        /** Set the program variables
        */
        void setProgramVariables(const ProgramVars::SharedPtr& pVars) { mpProgramVars = pVars; applyProgramVars(); }
        
        /** Get the bound program variables object
        */
        ProgramVars::SharedPtr getProgramVars() const { return mpProgramVars; }

        /** Push the current ProgramVars and sets a new one
        */
        void pushProgramVars(const ProgramVars::SharedPtr& pVars);

        /** Pops the last ProgramVars from the stack and sets it
        */
        void popProgramVars();

        /** Set a pipeline state
        */
        void setPipelineState(const PipelineState::SharedPtr& pState) { mpPipelineState = pState; applyPipelineState(); }
        
        /** Get the currently bound pipeline state
        */
        PipelineState::SharedPtr getPipelineState() const { return mpPipelineState; }

        /** Push the current PipelineState and sets a new one
        */
        void pushPipelineState(const PipelineState::SharedPtr& pState);

        /** Pops the last PipelineState from the stack and sets it
        */
        void popPipelineState();

        /** Flush the command list. This doesn't reset the command allocator, just submits the commands
            \param[in] wait If true, will block execution until the GPU finished processing the commands
        */
        void flush(bool wait = false);
                
        void updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset = 0, size_t size = 0);
        void updateTexture(const Texture* pTexture, const void* pData);
        void updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData);
        void updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData);

        GpuFence::SharedPtr getFence() const;
    private:
        RenderContext();

        ProgramVars::SharedPtr mpProgramVars;
        PipelineState::SharedPtr mpPipelineState;

        std::stack<PipelineState::SharedPtr> mPipelineStateStack;
        std::stack<ProgramVars::SharedPtr> mProgramVarsStack;

        // Internal functions used by the API layers
        void applyProgramVars();
        void applyPipelineState();
        void prepareForDraw();
        void prepareForDrawApi();
        void bindDescriptorHeaps();
		void* mpApiData = nullptr;
    };
}