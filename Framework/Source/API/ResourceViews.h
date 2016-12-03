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
#include <vector>

namespace Falcor
{
    class Resource;
    using ResourceSharedPtr = std::shared_ptr<const Resource>;

    template<typename ApiHandleType>
    class ResourceView
    {
    public:
        using ApiHandle = typename ApiHandleType;
        static const uint32_t kMaxPossible = -1;
        struct ViewInfo
        {
            ViewInfo(uint32_t mostDetailedMip_, uint32_t mipCount_, uint32_t firstArraySlice_, uint32_t arraySize_) : mostDetailedMip(mostDetailedMip_), mipCount(mipCount), firstArraySlice(firstArraySlice_), arraySize(arraySize_) {}
            uint32_t mostDetailedMip;
            uint32_t mipCount;
            uint32_t firstArraySlice;
            uint32_t arraySize;

            bool operator==(const ViewInfo& other) const
            {
                return (firstArraySlice == other.firstArraySlice) && (arraySize == other.arraySize) && (mipCount == other.mipCount) && (mostDetailedMip == other.mostDetailedMip);
            }
        };

        ResourceView(ResourceSharedPtr& pResource, ApiHandle handle, uint32_t mostDetailedMip_, uint32_t mipCount_, uint32_t firstArraySlice_, uint32_t arraySize_) 
            : mApiHandle(handle), mpResource(pResource), mViewInfo(mostDetailedMip_, mipCount_, firstArraySlice_, arraySize_) {}

        ApiHandle getApiHandle() const { return mApiHandle; }
        const ViewInfo& getViewInfo() const { return mViewInfo; }
        const ResourceSharedPtr& getResource() const { return mpResource; }
    private:
        ApiHandle mApiHandle;
        ViewInfo mViewInfo;
        ResourceSharedPtr mpResource;
    };

    class ShaderResourceView : public ResourceView<SrvHandle>
    {
    public:
        using SharedPtr = std::shared_ptr<ShaderResourceView>;
        using SharedConstPtr = std::shared_ptr<const ShaderResourceView>;
        
        static SharedPtr create(ResourceSharedPtr pResource, uint32_t mostDetailedMip = 0, uint32_t mipCount = kMaxPossible, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible);
        static SharedPtr getNullView();
    private:
        ShaderResourceView(ResourceSharedPtr& pResource, ApiHandle handle, uint32_t mostDetailedMip_, uint32_t mipCount_, uint32_t firstArraySlice_, uint32_t arraySize_) :
            ResourceView(pResource, handle, mostDetailedMip_, mipCount_, firstArraySlice_, arraySize_) {}
        static SharedPtr sNullView;
    };

    class DepthStencilView : public ResourceView<DsvHandle>
    {
    public:
        using SharedPtr = std::shared_ptr<DepthStencilView>;
        using SharedConstPtr = std::shared_ptr<const DepthStencilView>;

        static SharedPtr create(ResourceSharedPtr pResource, uint32_t mipLevel, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible);
        static SharedPtr getNullView();
    private:
        DepthStencilView(ResourceSharedPtr& pResource, ApiHandle handle, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize) :
            ResourceView(pResource, handle, mipLevel, 1, firstArraySlice, arraySize) {}
        static SharedPtr sNullView;
    };

    class UnorderedAccessView : public ResourceView<UavHandle>
    {
    public:
        using SharedPtr = std::shared_ptr<UnorderedAccessView>;
        using SharedConstPtr = std::shared_ptr<const UnorderedAccessView>;
        static SharedPtr create(ResourceSharedPtr pResource, uint32_t mipLevel, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible);
        static SharedPtr getNullView();
#ifdef FALCOR_D3D12
        UavHandle getHandleForClear() const { return mViewForClear; }
    private:
        UavHandle mViewForClear;
#endif
    private:
        UnorderedAccessView(ResourceSharedPtr& pResource, ApiHandle handle, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize) :
            ResourceView(pResource, handle, mipLevel, 1, firstArraySlice, arraySize) {}

        static SharedPtr sNullView;
    };

    class RenderTargetView : public ResourceView<UavHandle>
    {
    public:
        using SharedPtr = std::shared_ptr<RenderTargetView>;
        using SharedConstPtr = std::shared_ptr<const RenderTargetView>;
        static SharedPtr create(ResourceSharedPtr pResource, uint32_t mipLevel, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible);
        static SharedPtr getNullView();
    private:
        RenderTargetView(ResourceSharedPtr& pResource, ApiHandle handle, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize) :
            ResourceView(pResource, handle, mipLevel, 1, firstArraySlice, arraySize) {}

        static SharedPtr sNullView;
    };
}