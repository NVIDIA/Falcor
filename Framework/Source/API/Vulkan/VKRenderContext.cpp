/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
#include "Framework.h"
#include "API/RenderContext.h"
#include "API/LowLevel/DescriptorHeap.h"
#include "API/Device.h"
#include "glm/gtc/type_ptr.hpp"

namespace Falcor
{

    // Helper class to neatly record commands. The commands enclosed within the scope of the object of this class
    // will be the ones recorded for that particular command buffer.
    class ScopedCmdBufferRecorder
    {
    public:
        ScopedCmdBufferRecorder(VkDevice dev, VkCommandBuffer cmdBuffer)
        {
            assert(cmdBuffer == VK_NULL_HANDLE);

            mCmdBuffer = cmdBuffer;

            static const VkCommandBufferBeginInfo beginInfo =
            {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                nullptr,
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                nullptr
            };

            vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        }

        bool recordViewportCommands()
        {
            VkViewport viewport = { 1920.f, 1080.f, 0.f, 1.f };
            vkCmdSetViewport(mCmdBuffer, 0, 1, &viewport);
            
            return true;
        }

        bool recordScissorCommands()
        {
            VkRect2D scissor = { 1920  , 1080  ,   0, 0 };
            vkCmdSetScissor(mCmdBuffer, 0, 1, &scissor);

            return true;
        }

        ~ScopedCmdBufferRecorder()
        {
            vkEndCommandBuffer(mCmdBuffer);
        }

    private:
        VkDevice        mDevice;
        VkCommandBuffer mCmdBuffer;
    };

    RenderContext::SharedPtr RenderContext::create()
    {
        SharedPtr pCtx = SharedPtr(new RenderContext());
        pCtx->mpLowLevelData = LowLevelContextData::create(LowLevelContextData::CommandListType::Direct);
        if (pCtx->mpLowLevelData == nullptr)
        {
            return nullptr;
        }

        pCtx->bindDescriptorHeaps();

        if (spDrawCommandSig == nullptr)
        {
            initDrawCommandSignatures();
        }

        return pCtx;
    }
    
    void RenderContext::clearFbo(const Fbo* pFbo, const glm::vec4& color, float depth, uint8_t stencil, FboAttachmentType flags)
    {
        bool clearDepth = (flags & FboAttachmentType::Depth) != FboAttachmentType::None;
        bool clearColor = (flags & FboAttachmentType::Color) != FboAttachmentType::None;
        bool clearStencil = (flags & FboAttachmentType::Stencil) != FboAttachmentType::None;

        if(clearColor)
        {
            for(uint32_t i = 0 ; i < Fbo::getMaxColorTargetCount() ; i++)
            {
                if(pFbo->getColorTexture(i))
                {
                    //clearRtv(pFbo->getRenderTargetView(i).get(), color);
                }
            }
        }

        if(clearDepth | clearStencil)
        {
            //clearDsv(pFbo->getDepthStencilView().get(), depth, stencil, clearDepth, clearStencil);
        }
    }

    void RenderContext::clearRtv(const RenderTargetView* pRtv, const glm::vec4& color)
    {
    }

    void RenderContext::clearDsv(const DepthStencilView* pDsv, float depth, uint8_t stencil, bool clearDepth, bool clearStencil)
    {
    }


    void RenderContext::prepareForDraw()
    {
        // Record some commands. 
        // TODO: This step is done at 'init' time and not every frame! This is just for experiment
        {
            ScopedCmdBufferRecorder recorder(gpDevice->getApiHandle(), mpLowLevelData->getCommandList());
            recorder.recordViewportCommands();
            recorder.recordScissorCommands();
        }

        // The flow in Vulkan would be something like this:-
        // * Init:
        //   - Create render passes, set all necessary static states, etc.
        //   - Record command buffers
        // 
        // * Draw:
        //   - Acquire the next drawable swapchain image
        //   - Bind the right command buffer.
        //   - Submit the command buffer. Store the semaphores that'll be signalled.
        //   - Present the swapchain to display. 
    }

    void RenderContext::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        //mpLowLevelData->getCommandList()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        drawInstanced(vertexCount, 1, startVertexLocation, 0);
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        //mpLowLevelData->getCommandList()->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        drawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
    }

    void RenderContext::drawIndirect(const Buffer* argBuffer, uint64_t argBufferOffset)
    {
        prepareForDraw();
        resourceBarrier(argBuffer, Resource::State::IndirectArg);
        //mpLowLevelData->getCommandList()->ExecuteIndirect(spDrawCommandSig, 1, argBuffer->getApiHandle(), argBufferOffset, nullptr, 0);
    }

    void RenderContext::drawIndexedIndirect(const Buffer* argBuffer, uint64_t argBufferOffset)
    {
        prepareForDraw();
        resourceBarrier(argBuffer, Resource::State::IndirectArg);
        //mpLowLevelData->getCommandList()->ExecuteIndirect(spDrawIndexCommandSig, 1, argBuffer->getApiHandle(), argBufferOffset, nullptr, 0);
    }

    void RenderContext::initDrawCommandSignatures()
    {
    }

    void RenderContext::applyProgramVars() {}
    void RenderContext::applyGraphicsState() {}
}
