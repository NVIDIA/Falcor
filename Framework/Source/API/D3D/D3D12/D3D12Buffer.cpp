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
#include "API/Buffer.h"
#include "API/Device.h"
#include "Api/LowLevel/ResourceAllocator.h"
#include "D3D12Resource.h"

namespace Falcor
{
    struct BufferData
    {
        ResourceAllocator::AllocationData dynamicData;
        Buffer::SharedPtr pStagingResource; // For buffers that have both CPU read flag and can be used by the GPU
    };

    ID3D12ResourcePtr createBuffer(Buffer::State initState, size_t size, const D3D12_HEAP_PROPERTIES& heapProps, Buffer::BindFlags bindFlags)
    {
        ID3D12Device* pDevice = gpDevice->getApiHandle();

        // Create the buffer
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Alignment = 0;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Flags = getD3D12ResourceFlags(bindFlags);
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.Height = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.SampleDesc.Quality = 0;
        bufDesc.Width = size;

        D3D12_RESOURCE_STATES d3dState = getD3D12ResourceState(initState);
        ID3D12ResourcePtr pApiHandle;
        d3d_call(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, d3dState, nullptr, IID_PPV_ARGS(&pApiHandle)));

        // Map and upload data if needed
        return pApiHandle;
    }

    Buffer::~Buffer()
    {
        BufferData* pApiData = (BufferData*)mpApiData;
        gpDevice->getResourceAllocator()->release(pApiData->dynamicData);
        safe_delete(pApiData);
        gpDevice->releaseResource(mApiHandle);
    }

    size_t getDataAlignmentFromUsage(Buffer::BindFlags flags)
    {
        switch (flags)
        {
        case Buffer::BindFlags::Constant:
            return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        case Buffer::BindFlags::None:
            return D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
        default:
            return 1;
        }
    }

    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, CpuAccess cpuAccess, const void* pInitData)
    {
        Buffer::SharedPtr pBuffer = SharedPtr(new Buffer(size, usage, cpuAccess));
        return pBuffer->init(pInitData) ? pBuffer : nullptr;
    }

    bool Buffer::init(const void* pInitData)
    {
        if (mBindFlags == BindFlags::Constant)
        {
            mSize = align_to(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, mSize);
        }

        BufferData* pApiData = new BufferData;
        mpApiData = pApiData;
        if (mCpuAccess == CpuAccess::Write)
        {
            mState = Resource::State::GenericRead;
            pApiData->dynamicData = gpDevice->getResourceAllocator()->allocate(mSize, getDataAlignmentFromUsage(mBindFlags));
            mApiHandle = pApiData->dynamicData.pResourceHandle;
        }
        else if (mCpuAccess == CpuAccess::Read && mBindFlags == BindFlags::None)
        {
            mState = Resource::State::CopyDest;
            mApiHandle = createBuffer(mState, mSize, kReadbackHeapProps, mBindFlags);
        }
        else
        {
            mState = Resource::State::Common;
            mApiHandle = createBuffer(mState, mSize, kDefaultHeapProps, mBindFlags);
        }

        if (pInitData)
        {
            updateData(pInitData, 0, mSize);
        }

        return true;
    }

    void Buffer::copy(Buffer* pDst) const
    {
    }

    void Buffer::copy(Buffer* pDst, size_t srcOffset, size_t dstOffset, size_t count) const
    {
    }

    void Buffer::updateData(const void* pData, size_t offset, size_t size) const
    {
        // Clamp the offset and size
        if (adjustSizeOffsetParams(size, offset) == false)
        {
            logWarning("Buffer::updateData() - size and offset are invalid. Nothing to update.");
            return;
        }

        if (mCpuAccess == CpuAccess::Write)
        {
            uint8_t* pDst = (uint8_t*)map(MapType::WriteDiscard) + offset;
            memcpy(pDst, pData, size);
        }
        else
        {
            gpDevice->getRenderContext()->updateBuffer(this, pData, offset, size);
        }
    }

    void Buffer::readData(void* pData, size_t offset, size_t size) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::ReadData(). If you really need this, create the resource with CPU read flag, and use Buffer::Map()");
    }

    void* Buffer::map(MapType type) const
    {
        BufferData* pApiData = (BufferData*)mpApiData;

        if(type == MapType::WriteDiscard)
        {
            if (mCpuAccess != CpuAccess::Write)
            {
                logError("Trying to map a buffer for write, but it wasn't created with the write permissions");
                return nullptr;
            }

            // Allocate a new buffer
            gpDevice->getResourceAllocator()->release(pApiData->dynamicData);
            pApiData->dynamicData = gpDevice->getResourceAllocator()->allocate(mSize, getDataAlignmentFromUsage(mBindFlags));

            // I don't want to make mApiHandle mutable, so let's just const_cast here. This is D3D12 specific case
            const_cast<Buffer*>(this)->mApiHandle = pApiData->dynamicData.pResourceHandle;
            
            invalidateViews();
            return pApiData->dynamicData.pData;
        }
        else
        {
            assert(type == MapType::Read);

            if (mBindFlags == BindFlags::None)
            {
                D3D12_RANGE r{ 0, mSize };
                void* pData;
                d3d_call(mApiHandle->Map(0, &r, &pData));
                return pData;
            }
            else
            {
                logWarning("Buffer::map() performance warning - using staging resource which require us to flush the pipeline and wait for the GPU to finish its work");
                if (pApiData->pStagingResource == nullptr)
                {
                    pApiData->pStagingResource = Buffer::create(mSize, Buffer::BindFlags::None, Buffer::CpuAccess::Read, nullptr);
                }
                // Copy the buffer and flush the pipeline
                RenderContext* pContext = gpDevice->getRenderContext().get();
                pContext->resourceBarrier(this, Resource::State::CopySource);
                pContext->getLowLevelData()->getCommandList()->CopyResource(pApiData->pStagingResource->getApiHandle(), mApiHandle);
                pContext->flush(true);
                return pApiData->pStagingResource->map(MapType::Read);
            }
        }        
    }

    uint64_t Buffer::getGpuAddress() const
    {
        if (mCpuAccess == CpuAccess::Write)
        {
            BufferData* pApiData = (BufferData*)mpApiData;
            return pApiData->dynamicData.gpuAddress;
        }
        else
        {
            return mApiHandle->GetGPUVirtualAddress();
        }
    }

    void Buffer::unmap() const
    {
        // Only unmap read buffers
        BufferData* pApiData = (BufferData*)mpApiData;
        D3D12_RANGE r{};
        if (pApiData->pStagingResource)
        {
            pApiData->pStagingResource->mApiHandle->Unmap(0, &r);
        }
        else if (mCpuAccess == CpuAccess::Read)
        {
            mApiHandle->Unmap(0, &r);
        }
    }

    uint64_t Buffer::makeResident(Buffer::GpuAccessFlags flags) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::makeResident()");
        return 0;
    }

    void Buffer::evict() const
    {
        UNSUPPORTED_IN_D3D12("Buffer::evict()");
    }

    template<bool forClear>
    UavHandle getUavCommon(UavHandle& handle, size_t bufSize, Buffer::ApiHandle apiHandle)
    {
        if (handle == nullptr)
        {

            DescriptorHeap* pHeap = forClear ? gpDevice->getCpuUavDescriptorHeap().get() : gpDevice->getUavDescriptorHeap().get();
            handle = pHeap->allocateEntry();
            gpDevice->getApiHandle()->CreateUnorderedAccessView(apiHandle, nullptr, &desc, handle->getCpuHandle());
        }

        return handle;
    }
}
