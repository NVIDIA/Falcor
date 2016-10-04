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
#ifdef FALCOR_D3D12
#include "Framework.h"
#include "API/Buffer.h"
#include "API/Device.h"

namespace Falcor
{
    static D3D12_HEAP_PROPERTIES kUploadHeapProps =
    {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0,
    };

    // D3D12TODO - this in in texture
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

    D3D12_HEAP_TYPE getHeapType(Buffer::HeapType type)
    {
        switch (type)
        {
        case Buffer::HeapType::Default:
            return D3D12_HEAP_TYPE_DEFAULT;
        case Buffer::HeapType::Upload:
            return D3D12_HEAP_TYPE_UPLOAD;
        case Buffer::HeapType::Readback:
            return D3D12_HEAP_TYPE_READBACK;
        default:
            should_not_get_here();
            return D3D12_HEAP_TYPE(-1);
        }
    }

    const D3D12_HEAP_PROPERTIES& getHeapProps(Buffer::HeapType type)
    {
        static const D3D12_HEAP_PROPERTIES kEmptyDesc = {};
        switch (type)
        {
        case Buffer::HeapType::Default:
            return kDefaultHeapProps;
        case Buffer::HeapType::Upload:
            return kUploadHeapProps;
        case Buffer::HeapType::Readback:
            return kReadbackHeapProps;
        default:
            should_not_get_here();
            return kEmptyDesc;
        }
    }

    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, AccessFlags access, const void* pInitData)
    {
        return create(size, HeapType::Upload, pInitData);
    }

    Buffer::SharedPtr Buffer::create(size_t size, HeapType heapType, const void* pInitData)
    {
        Buffer::SharedPtr pBuffer = SharedPtr(new Buffer(size, BindFlags::None, AccessFlags::None, heapType));
        ID3D12Device* pDevice = gpDevice->getApiHandle();

        // Create the buffer
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Alignment = 0;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.Height = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.SampleDesc.Quality = 0;
        bufDesc.Width = size;

        const auto& heapProps = getHeapProps(heapType);
        D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;
        switch (heapType)
        {
        case Buffer::HeapType::Upload:
            initState = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        case Buffer::HeapType::Readback:
            initState = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
        }
        d3d_call(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer->mApiHandle)));

        // Map and upload data if needed
        if (heapType != HeapType::Default)
        {
            D3D12_RANGE readRange = (heapType == HeapType::Readback) ? D3D12_RANGE{ 0, size } : D3D12_RANGE{};
            d3d_call(pBuffer->mApiHandle->Map(0, &readRange, &pBuffer->mpMappedData));
        }

        if (pInitData)
        {
            pBuffer->updateData(pInitData, 0, size);
        }

        return pBuffer;
    }

    Buffer::~Buffer() = default;

    void Buffer::copy(Buffer* pDst) const
    {
    }

    void Buffer::copy(Buffer* pDst, size_t srcOffset, size_t dstOffset, size_t count) const
    {
    }

    void Buffer::updateData(const void* pData, size_t offset, size_t size, bool forceUpdate)
    {
        // Clamp the offset and size
        if (adjustSizeOffsetParams(size, offset) == false)
        {
            logWarning("Buffer::updateData() - size and offset are invalid. Nothing to update.");
            return;
        }


        if (mHeapType == HeapType::Default)
        {
            gpDevice->getCopyContext()->updateBuffer(this, pData, offset, size);
        }
        else
        {
            assert(mHeapType == HeapType::Upload);
            assert(mpMappedData);
            uint8_t* pDst = (uint8_t*)mpMappedData + offset;
            memcpy(pDst, pData, size);
        }
    }

    void Buffer::readData(void* pData, size_t offset, size_t size) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::ReadData(). If you really need this, create the resource with CPU read flag, and use Buffer::Map()");
    }

    uint64_t Buffer::getBindlessHandle()
    {
        UNSUPPORTED_IN_D3D12("D3D12 buffers don't have bindless handles.");
        return 0;
    }

    void* Buffer::map(MapType type)
    {
        return mpMappedData;
    }

    void Buffer::unmap()
    {

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
}
#endif // #ifdef FALCOR_D3D12
