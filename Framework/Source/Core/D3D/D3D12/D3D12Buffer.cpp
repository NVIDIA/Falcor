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
#include "Core/Buffer.h"

namespace Falcor
{
    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, AccessFlags access, const void* pInitData)
    {
        return nullptr;
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
        return nullptr;
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
