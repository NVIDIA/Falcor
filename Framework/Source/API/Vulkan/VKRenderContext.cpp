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


    void RenderContext::prepareForDraw()
    {
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
