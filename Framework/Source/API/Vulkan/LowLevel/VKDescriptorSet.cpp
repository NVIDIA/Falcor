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
#include "Framework.h"
#include "API/DescriptorSet.h"
#include "VKDescriptorData.h"
#include "API/Device.h"
#include "API/Buffer.h"

namespace Falcor
{
    VkDescriptorSetLayout createDescriptorSetLayout(const DescriptorSet::Layout& layout);

    bool DescriptorSet::apiInit()
    {
        auto layout = createDescriptorSetLayout(mLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mpPool->getApiHandle(0);
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;
        vk_call(vkAllocateDescriptorSets(gpDevice->getApiHandle(), &allocInfo, &mApiHandle));
        mpApiData = mpApiData = std::make_shared<DescriptorSetApiData>(layout, mpPool->getApiHandle(0), mApiHandle);

        return true;
    }

    DescriptorSet::CpuHandle DescriptorSet::getCpuHandle(uint32_t rangeIndex, uint32_t descInRange) const
    {
        UNSUPPORTED_IN_VULKAN("DescriptorSet::getCpuHandle");
        return nullptr;
    }

    DescriptorSet::GpuHandle DescriptorSet::getGpuHandle(uint32_t rangeIndex, uint32_t descInRange) const
    {
        UNSUPPORTED_IN_VULKAN("DescriptorSet::getGpuHandle");
        return nullptr;
    }

    template<bool isUav, typename ViewType>
    static void setSrvUavCommon(VkDescriptorSet set, uint32_t regIndex, const ViewType* pView)
    {
        VkWriteDescriptorSet write = {};
        VkDescriptorImageInfo image;
        VkDescriptorBufferInfo buffer;
        ViewType::ApiHandle handle = pView->getApiHandle();

        if (handle.getType() == VkResourceType::Buffer)
        {
            const Buffer* pBuffer = dynamic_cast<const Buffer*>(pView->getResource());
            buffer.buffer = pBuffer->getApiHandle();
            buffer.offset = pBuffer->getGpuAddressOffset();
            buffer.range = pBuffer->getSize();

            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &buffer;
        }
        else
        {
            assert(handle.getType() == VkResourceType::Image);
            image.imageLayout = isUav ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image.imageView = handle;
            image.sampler = nullptr;
            write.pImageInfo = &image;
            write.descriptorType = isUav ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        }

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = regIndex;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;

        vkUpdateDescriptorSets(gpDevice->getApiHandle(), 1, &write, 0, nullptr);
    }

    void DescriptorSet::setSrv(uint32_t rangeIndex, uint32_t descIndex, uint32_t regIndex, const ShaderResourceView* pSrv)
    {
        setSrvUavCommon<false>(mApiHandle, regIndex, pSrv);
    }

    void DescriptorSet::setUav(uint32_t rangeIndex, uint32_t descIndex, uint32_t regIndex, const UnorderedAccessView* pUav)
    {
        setSrvUavCommon<true>(mApiHandle, regIndex, pUav);
    }

    void DescriptorSet::setSampler(uint32_t rangeIndex, uint32_t descIndex, uint32_t regIndex, const Sampler* pSampler)
    {
        VkDescriptorImageInfo info;
        info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        info.imageView = nullptr;
        info.sampler = pSampler->getApiHandle();

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = mApiHandle;
        write.dstBinding = regIndex;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &info;

        vkUpdateDescriptorSets(gpDevice->getApiHandle(), 1, &write, 0, nullptr);
    }

    void DescriptorSet::setCb(uint32_t rangeIndex, uint32_t descIndex, uint32_t regIndex, const Buffer* pBuffer)
    {
        VkDescriptorBufferInfo info;
        info.buffer = pBuffer->getApiHandle();
        info.offset = pBuffer->getGpuAddressOffset();
        info.range = pBuffer->getSize();

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = mApiHandle;
        write.dstBinding = regIndex;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &info;
        vkUpdateDescriptorSets(gpDevice->getApiHandle(), 1, &write, 0, nullptr);
    }

    template<bool forGraphics>
    static void bindCommon(DescriptorSet::ApiHandle set, CopyContext* pCtx, const RootSignature* pRootSig, uint32_t bindLocation)
    {
        VkPipelineBindPoint bindPoint = forGraphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
        vkCmdBindDescriptorSets(pCtx->getLowLevelData()->getCommandList(), bindPoint, pRootSig->getApiHandle(), bindLocation, 1, &set, 0, nullptr);
    }

    void DescriptorSet::bindForGraphics(CopyContext* pCtx, const RootSignature* pRootSig, uint32_t rootIndex)
    {
        bindCommon<true>(mApiHandle, pCtx, pRootSig, mLayout.getRange(0).regSpace);
    }

    void DescriptorSet::bindForCompute(CopyContext* pCtx, const RootSignature* pRootSig, uint32_t rootIndex)
    {
        bindCommon<false>(mApiHandle, pCtx, pRootSig, mLayout.getRange(0).regSpace);
    }
}