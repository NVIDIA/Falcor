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
#include "API/LowLevel/DescriptorPool.h"
#include "API/Device.h"
#include "glm/gtc/type_ptr.hpp"

namespace Falcor
{
    VkImageAspectFlags getAspectFlagsFromFormat(ResourceFormat format);

    RenderContext::SharedPtr RenderContext::create(CommandQueueHandle queue)
    {
        SharedPtr pCtx = SharedPtr(new RenderContext());
        pCtx->mpLowLevelData = LowLevelContextData::create(LowLevelContextData::CommandQueueType::Direct, queue);
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

    RenderContext::~RenderContext() = default;

    void RenderContext::clearRtv(const RenderTargetView* pRtv, const glm::vec4& color)
    {
        resourceBarrier(pRtv->getResource(), Resource::State::CopyDest);

        VkClearColorValue colVal;
        memcpy_s(colVal.float32, sizeof(colVal.float32), &color, sizeof(color));
        VkImageSubresourceRange range;
        const auto& viewInfo = pRtv->getViewInfo();
        range.baseArrayLayer = viewInfo.firstArraySlice;
        range.baseMipLevel = viewInfo.mostDetailedMip;
        range.layerCount = viewInfo.arraySize;
        range.levelCount = viewInfo.mipCount;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkCmdClearColorImage(mpLowLevelData->getCommandList(), pRtv->getResource()->getApiHandle().getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colVal, 1, &range);
        mCommandsPending = true;
    }

    void RenderContext::clearDsv(const DepthStencilView* pDsv, float depth, uint8_t stencil, bool clearDepth, bool clearStencil)
    {
        resourceBarrier(pDsv->getResource(), Resource::State::CopyDest);

        VkClearDepthStencilValue val;
        val.depth = depth;
        val.stencil = stencil;

        VkImageSubresourceRange range;
        const auto& viewInfo = pDsv->getViewInfo();
        range.baseArrayLayer = viewInfo.firstArraySlice;
        range.baseMipLevel = viewInfo.mostDetailedMip;
        range.layerCount = viewInfo.arraySize;
        range.levelCount = viewInfo.mipCount;
        range.aspectMask = clearDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
        range.aspectMask |= clearStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;

        vkCmdClearDepthStencilImage(mpLowLevelData->getCommandList(), pDsv->getResource()->getApiHandle().getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &val, 1, &range);
        mCommandsPending = true;
    }

    void setViewports(CommandListHandle cmdList, const std::vector<GraphicsState::Viewport>& viewports)
    {
        static_assert(offsetof(GraphicsState::Viewport, originX) == offsetof(VkViewport, x), "VP originX offset");
        static_assert(offsetof(GraphicsState::Viewport, originY) == offsetof(VkViewport, y), "VP originY offset");
        static_assert(offsetof(GraphicsState::Viewport, width) == offsetof(VkViewport, width), "VP width offset");
        static_assert(offsetof(GraphicsState::Viewport, height) == offsetof(VkViewport, height), "VP height offset");
        static_assert(offsetof(GraphicsState::Viewport, minDepth) == offsetof(VkViewport, minDepth), "VP minDepth offset");
        static_assert(offsetof(GraphicsState::Viewport, maxDepth) == offsetof(VkViewport, maxDepth), "VP maxDepth offset");

        vkCmdSetViewport(cmdList, 0, (uint32_t)viewports.size(), (VkViewport*)viewports.data());
    }

    void setScissors(CommandListHandle cmdList, const std::vector<GraphicsState::Scissor>& scissors)
    {
        std::vector<VkRect2D> vkScissors(scissors.size());
        for (size_t i = 0; i < scissors.size(); i++)
        {
            vkScissors[i].offset.x = scissors[i].left;
            vkScissors[i].offset.y = scissors[i].top;
            vkScissors[i].extent.width = scissors[i].right - scissors[i].left;
            vkScissors[i].extent.height = scissors[i].bottom - scissors[i].top;
        }
        vkCmdSetScissor(cmdList, 0, (uint32_t)scissors.size(), vkScissors.data());
    }

    void setVao(CommandListHandle cmdList, const Vao* pVao)
    {
        // #VKTODO setVao() - Implement me properly
        VkDeviceSize offset = 0;
        VkBuffer vertexbuffer = pVao->getVertexBuffer(0)->getApiHandle().getBuffer();
        vkCmdBindVertexBuffers(cmdList, 0, 1, &vertexbuffer, &offset);
    }

    void beginRenderPass(CommandListHandle cmdList, const Fbo* pFbo, VkRenderPass renderPass)
    {
        // #VKTODO Where should we make and store the frame buffer handle? It requires both the renderPass and FBO views.

        //
        // Create and bind frame buffer
        //

        // #VKFRAMEBUFFER beginRenderPass()

        std::vector<VkImageView> attachments(1/*Fbo::getMaxColorTargetCount()*/ + 1);
        for (uint32_t i = 0; i < 1/*Fbo::getMaxColorTargetCount()*/; i++)
        {
            attachments[i] = pFbo->getRenderTargetView(i)->getApiHandle();
        }

        attachments.back() = pFbo->getDepthStencilView()->getApiHandle();

        VkFramebufferCreateInfo frameBufferInfo = {};
        frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferInfo.renderPass = renderPass;
        frameBufferInfo.attachmentCount = (uint32_t)attachments.size();
        frameBufferInfo.pAttachments = attachments.data();
        frameBufferInfo.width = pFbo->getWidth();
        frameBufferInfo.height = pFbo->getHeight();
        frameBufferInfo.layers = 1; // #VKTODO what are frame buffer "layers?"

        VkFramebuffer frameBuffer;
        vkCreateFramebuffer(gpDevice->getApiHandle(), &frameBufferInfo, nullptr, &frameBuffer);

        //
        // Begin Render Pass
        //
        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = frameBuffer;
        beginInfo.renderArea.offset = { 0, 0 };
        beginInfo.renderArea.extent = { pFbo->getWidth(), pFbo->getHeight() };

        // Only needed if attachments use VK_ATTACHMENT_LOAD_OP_CLEAR
        beginInfo.clearValueCount = 0;
        beginInfo.pClearValues = nullptr;

        vkCmdBeginRenderPass(cmdList, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void RenderContext::prepareForDraw()
    {
        GraphicsStateObject::SharedPtr pGSO = mpGraphicsState->getGSO(mpGraphicsVars.get());
        vkCmdBindPipeline(mpLowLevelData->getCommandList(), VK_PIPELINE_BIND_POINT_GRAPHICS, pGSO->getApiHandle());

        setViewports(mpLowLevelData->getCommandList(), mpGraphicsState->getViewports());
        setScissors(mpLowLevelData->getCommandList(), mpGraphicsState->getScissors());

        setVao(mpLowLevelData->getCommandList(), pGSO->getDesc().getVao().get());
        
        beginRenderPass(mpLowLevelData->getCommandList(), mpGraphicsState->getFbo().get(), pGSO->getRenderPass());
    }

    void RenderContext::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
    {
        resourceBarrier(mpGraphicsState->getFbo()->getDepthStencilTexture().get(), Resource::State::DepthStencil);
        resourceBarrier(mpGraphicsState->getFbo()->getColorTexture(0).get(), Resource::State::RenderTarget);
        prepareForDraw();
        vkCmdDraw(mpLowLevelData->getCommandList(), vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
        vkCmdEndRenderPass(mpLowLevelData->getCommandList());
    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        drawInstanced(vertexCount, 1, startVertexLocation, 0);
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        vkCmdDrawIndexed(mpLowLevelData->getCommandList(), indexCount, instanceCount, startIndexLocation, baseVertexLocation, startIndexLocation);
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation)
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

    template<typename ViewType>
    void initBlitData(const ViewType* pView, const uvec4& rect, VkImageSubresourceLayers& layer, VkOffset3D offset[2])
    {
        const Texture* pTex = dynamic_cast<const Texture*>(pView->getResource());

        layer.aspectMask = getAspectFlagsFromFormat(pTex->getFormat());
        const auto& viewInfo = pView->getViewInfo();
        layer.baseArrayLayer = viewInfo.firstArraySlice;
        layer.layerCount = viewInfo.arraySize;
        layer.mipLevel = viewInfo.mostDetailedMip;
        assert(pTex->getDepth(viewInfo.mostDetailedMip) == 1);

        offset[0].x =  (rect.x == -1) ? 0 : rect.x;
        offset[0].y = (rect.y == -1) ? 0 : rect.y;
        offset[0].z = 0;

        offset[1].x = (rect.z == -1) ? pTex->getWidth(viewInfo.mostDetailedMip) : rect.z;
        offset[1].y = (rect.w == -1) ? pTex->getHeight(viewInfo.mostDetailedMip) : rect.w;
        offset[1].z = 1;
    }

    void RenderContext::blit(ShaderResourceView::SharedPtr pSrc, RenderTargetView::SharedPtr pDst, const uvec4& srcRect, const uvec4& dstRect, Sampler::Filter filter)
    {
        VkImageBlit blt;
        initBlitData(pSrc.get(), srcRect, blt.srcSubresource, blt.srcOffsets);
        initBlitData(pDst.get(), dstRect, blt.dstSubresource, blt.dstOffsets);

        resourceBarrier(pSrc->getResource(), Resource::State::CopySource);
        resourceBarrier(pDst->getResource(), Resource::State::CopyDest);
        vkCmdBlitImage(mpLowLevelData->getCommandList(), pSrc->getResource()->getApiHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pDst->getResource()->getApiHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blt, VK_FILTER_NEAREST);
    }
    
    void RenderContext::applyProgramVars() {}
    void RenderContext::applyGraphicsState() {}
}
