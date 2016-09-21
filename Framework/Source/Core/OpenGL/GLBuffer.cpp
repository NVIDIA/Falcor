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
#ifdef FALCOR_GL
#include "Core/Buffer.h"
#include "Utils/OS.h"

namespace Falcor
{
    GLenum getGlUsageFlags(Buffer::AccessFlags flags)
    {
        GLenum glFlags = 0;
        glFlags |= ((flags & Buffer::AccessFlags::Dynamic) != Buffer::AccessFlags::None) ? GL_DYNAMIC_STORAGE_BIT : 0;
        glFlags |= ((flags & Buffer::AccessFlags::MapRead) != Buffer::AccessFlags::None) ? GL_MAP_READ_BIT : 0;
        glFlags |= ((flags & Buffer::AccessFlags::MapWrite) != Buffer::AccessFlags::None) ? GL_MAP_WRITE_BIT : 0;

        return glFlags;
    }

    Buffer::~Buffer()
    {
        glDeleteBuffers(1, &mApiHandle);
    }

    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, AccessFlags access, const void* pInitData)
    {
        auto pBuffer = SharedPtr(new Buffer(size, usage, access));
        gl_call(glCreateBuffers(1, &pBuffer->mApiHandle));
        gl_call(glNamedBufferStorage(pBuffer->mApiHandle, size, pInitData, getGlUsageFlags(access)));
        return pBuffer;
    }

    void Buffer::copy(Buffer* pDst) const
    {
        if(mSize != pDst->mSize)
        {
            Logger::log(Logger::Level::Error, "Error in Buffer::copy().\nSource buffer size is " + std::to_string(mSize) + ", Destination buffer size is " + std::to_string(pDst->mSize) + ".\nBuffers should have the same size.");
            return;
        }
        gl_call(glCopyNamedBufferSubData(mApiHandle, pDst->mApiHandle, 0, 0, mSize));
    }

    void Buffer::copy(Buffer* pDst, size_t srcOffset, size_t dstOffset, size_t count) const
    {
        if(mSize < srcOffset+count || pDst->mSize < dstOffset+count)
        {
            Logger::log(Logger::Level::Error, "Error in Buffer::copy().\nSource buffer size is " + std::to_string(mSize) + ", Destination buffer size is " + std::to_string(pDst->mSize) + ", Copy offsets are " + std::to_string(srcOffset) + ", " + std::to_string(dstOffset) + ", Copy size is " + std::to_string(count) + ".\nBuffers are too small to perform copy.");
            return;
        }
        gl_call(glCopyNamedBufferSubData(mApiHandle, pDst->mApiHandle, srcOffset, dstOffset, count));
    }

    void Buffer::updateData(const void* pData, size_t offset, size_t size, bool forceUpdate)
    {
        if((size + offset) > mSize)
        {
            std::string Error = "Buffer::updateData() called with data larger then the buffer size. Buffer size = " + std::to_string(mSize) + " , Data size = " + std::to_string(size) + ".";
            Logger::log(Logger::Level::Error, Error);
        }

        if((mAccessFlags & Buffer::AccessFlags::Dynamic) != Buffer::AccessFlags::None)
        {
            gl_call(glNamedBufferSubData(mApiHandle, offset, size, pData));
        }
        else if(forceUpdate)
        {
            Logger::log(Logger::Level::Warning, "Buffer::updateData() - Updating buffer using a staging resource. If you care about the performance implications consider creating the buffer with AccessFlags::Dynamic.");
            auto pStaging = create(mSize, mBindFlags, Buffer::AccessFlags::None, pData);
            pStaging->copy(this);            
        }
        else
        {
            Logger::log(Logger::Level::Error, "Buffer::updateData() - The buffer wasn't created with AccessFlags::Dynamic flag. Can't update the buffer.");
        }
    }

    void Buffer::readData(void* pData, size_t offset, size_t size) const
    {
        if((size + offset) > mSize)
        {
            std::string Error = "Buffer::readData called with data larger then the buffer size. Buffer size = " + std::to_string(mSize) + " , Data size = " + std::to_string(size) + ".";
            Logger::log(Logger::Level::Error, Error);
        }
        gl_call(glGetNamedBufferSubData(mApiHandle, offset, size, pData));
    }

    uint64_t Buffer::getBindlessHandle()
    {
        if(mBindlessHandle == 0)
        {
            gl_call(glGetNamedBufferParameterui64vNV(mApiHandle, GL_BUFFER_GPU_ADDRESS_NV, &mBindlessHandle));
            gl_call(glMakeNamedBufferResidentNV(mApiHandle, GL_READ_ONLY));
        }
        return mBindlessHandle;
    }

    void* Buffer::map(MapType Type)
    {
        if(mIsMapped)
        {
            Logger::log(Logger::Level::Error, "Buffer::map() error. Buffer is already mapped");
            return nullptr;
        }

        GLenum flags = 0;
        switch(Type)
        {
        case MapType::Read:
            flags = GL_MAP_READ_BIT;
            break;
        case MapType::Write:
        case MapType::WriteNoOverwrite:
            flags = GL_MAP_WRITE_BIT;
            break;
        case MapType::ReadWrite:
            flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
            break;
        case MapType::WriteDiscard:
            flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
            break;
        default:
            should_not_get_here();
            return nullptr;
        }

        void* pData = gl_call(glMapNamedBufferRange(mApiHandle, 0, mSize, flags));
        mIsMapped = true;
        return pData;
    }

    void Buffer::unmap()
    {
        if(mIsMapped == false)
        {
            Logger::log(Logger::Level::Error, "Buffer::unmap() error. Buffer is not mapped");
            return;
        }

        gl_call(glUnmapNamedBuffer(mApiHandle));
        mIsMapped = false;
    }

    uint64_t Buffer::makeResident(Buffer::GpuAccessFlags flags) const
    {
        if(mGpuPtr == 0)
        {
            gl_call(glGetNamedBufferParameterui64vNV(mApiHandle, GL_BUFFER_GPU_ADDRESS_NV, &mGpuPtr));
            switch (flags)
            {
                case Buffer::GpuAccessFlags::ReadOnly:  gl_call(glMakeNamedBufferResidentNV(mApiHandle, GL_READ_ONLY));  break;
                case Buffer::GpuAccessFlags::ReadWrite: gl_call(glMakeNamedBufferResidentNV(mApiHandle, GL_READ_WRITE)); break;
                case Buffer::GpuAccessFlags::WriteOnly: gl_call(glMakeNamedBufferResidentNV(mApiHandle, GL_WRITE_ONLY)); break;
            };
        }
        return mGpuPtr;
    }

    void Buffer::evict() const
    {
        if(mGpuPtr)
        {
            gl_call(glMakeNamedBufferNonResidentNV(mApiHandle));
            mGpuPtr = 0;
        }
    }
}
#endif // #ifdef FALCOR_GL
