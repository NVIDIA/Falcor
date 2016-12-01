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

namespace Falcor
{
    struct BufferData
    {
        ResourceAllocator::AllocationData dynamicData;
        Buffer::SharedPtr pStagingResource; // For buffers that have both CPU read flag and can be used by the GPU
    };

    // FIXME D3D12 - this in in texture
    static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
    {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };

    static const D3D12_HEAP_PROPERTIES kReadbackHeapProps =
    {
        D3D12_HEAP_TYPE_READBACK,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };

    D3D12_RESOURCE_FLAGS getResourceFlags(Buffer::BindFlags bindFlags)
    {
#define is_set(_a) ((bindFlags & _a) != Buffer::BindFlags::None)

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        if (is_set(Buffer::BindFlags::UnorderedAccess))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        if (is_set(Buffer::BindFlags::ShaderResource))
        {
            flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        return flags;
#undef is_set
    }

    ID3D12ResourcePtr createBuffer(size_t size, const D3D12_HEAP_PROPERTIES& heapProps, Buffer::BindFlags bindFlags)
    {
        ID3D12Device* pDevice = gpDevice->getApiHandle();

        // Create the buffer
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Alignment = 0;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Flags = getResourceFlags(bindFlags);
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.Height = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.SampleDesc.Quality = 0;
        bufDesc.Width = size;

        D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;
        switch (heapProps.Type)
        {
        case D3D12_HEAP_TYPE_UPLOAD:
            initState = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        case D3D12_HEAP_TYPE_READBACK:
            initState = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
        }
        ID3D12ResourcePtr pApiHandle;
        d3d_call(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pApiHandle)));

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
        case Buffer::BindFlags::Staging:
            return D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
        default:
            return 1;
        }
    }

    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, CpuAccess cpuAccess, const void* pInitData)
    {
        if (usage == BindFlags::Constant)
        {
            size = align_to(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, size);
        }

        Buffer::SharedPtr pBuffer = SharedPtr(new Buffer(size, usage, cpuAccess));

        BufferData* pApiData = new BufferData;
        pBuffer->mpApiData = pApiData;
        if (cpuAccess == CpuAccess::Write)
        {
            pApiData->dynamicData = gpDevice->getResourceAllocator()->allocate(size, getDataAlignmentFromUsage(usage));
            pBuffer->mApiHandle = pApiData->dynamicData.pResourceHandle;
        }
        else if (cpuAccess == CpuAccess::Read && usage == BindFlags::None)
        {
            pBuffer->mApiHandle = createBuffer(size, kReadbackHeapProps, usage);
        }
        else
        {
            pBuffer->mApiHandle = createBuffer(size, kDefaultHeapProps, usage);
        }

        if (pInitData)
        {
            pBuffer->updateData(pInitData, 0, size);
        }

        return pBuffer;
    }

    void Buffer::copy(Buffer* pDst) const
    {
    }

    void Buffer::copy(Buffer* pDst, size_t srcOffset, size_t dstOffset, size_t count) const
    {
    }

    void Buffer::updateData(const void* pData, size_t offset, size_t size)
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
            // FIXME D3D12 handle case where buffer is mapped for read
            gpDevice->getRenderContext()->updateBuffer(this, pData, offset, size);
        }
    }

    void Buffer::readData(void* pData, size_t offset, size_t size) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::ReadData(). If you really need this, create the resource with CPU read flag, and use Buffer::Map()");
    }

    void* Buffer::map(MapType type)
    {
        BufferData* pApiData = (BufferData*)mpApiData;

        if(type == MapType::WriteDiscard)
        {
            if (mCpuAccess != CpuAccess::Write)
            {
                logError("Trying to map a buffer for write, but it wasn't created with the write access type");
                return nullptr;
            }

            // Allocate a new buffer
            gpDevice->getResourceAllocator()->release(pApiData->dynamicData);
            pApiData->dynamicData = gpDevice->getResourceAllocator()->allocate(mSize, getDataAlignmentFromUsage(mBindFlags));
            mApiHandle = pApiData->dynamicData.pResourceHandle;
            
            // Invalidate the views
            mSrv = nullptr;
            mUav = nullptr;
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
                if (pApiData->pStagingResource == nullptr)
                {
                    pApiData->pStagingResource = Buffer::create(mSize, Buffer::BindFlags::None, Buffer::CpuAccess::Read, nullptr);

                    // Copy the buffer and flush the pipeline
                    RenderContext* pContext = gpDevice->getRenderContext().get();
                    pContext->getCommandListApiHandle()->CopyResource(pApiData->pStagingResource->getApiHandle(), mApiHandle);
                    pContext->flush(true);
                }
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

    void Buffer::unmap()
    {
        // Only unmap read buffers
        if (mCpuAccess == CpuAccess::Read)
        {
            BufferData* pApiData = (BufferData*)mpApiData;
            D3D12_RANGE r{};
            if (pApiData->pStagingResource)
            {
                pApiData->pStagingResource->mApiHandle->Unmap(0, &r);
                pApiData->pStagingResource = nullptr;
            }
            else
            {
                mApiHandle->Unmap(0, &r);
            }
        }
    }

    uint64_t Buffer::makeResident(Buffer::GpuAccessFlags flags/* = Buffer::GpuAccessFlags::ReadOnly*/) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::makeResident()");
        return 0;
    }

    void Buffer::evict() const
    {
        UNSUPPORTED_IN_D3D12("Buffer::evict()");
    }

    SrvHandle Buffer::getSRV()
    {
        if(mSrv == 0)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (uint32_t)mSize / sizeof(float);
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Format = DXGI_FORMAT_R32_TYPELESS;

            mSrv = gpDevice->getSrvDescriptorHeap()->allocateEntry();
            gpDevice->getApiHandle()->CreateShaderResourceView(mApiHandle, &desc, mSrv->getCpuHandle());
        }

        return mSrv;
    }

    UavHandle Buffer::getUAV()
    {
        if (mUav == 0)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Format = DXGI_FORMAT_R32_TYPELESS;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
            desc.Buffer.NumElements = (uint32_t)mSize / sizeof(float);

            mUav = gpDevice->getUavDescriptorHeap()->allocateEntry();
            gpDevice->getApiHandle()->CreateUnorderedAccessView(mApiHandle, nullptr, &desc, mUav->getCpuHandle());
        }

        return mUav;
    }
}
