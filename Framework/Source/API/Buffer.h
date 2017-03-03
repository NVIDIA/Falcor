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
#include "Resource.h"

namespace Falcor
{
    /** Low-level buffer object
        This class abstracts the API's buffer creation and management
    */
    class Buffer : public Resource, public inherit_shared_from_this<Resource, Buffer>
    {
    public:
        using SharedPtr = std::shared_ptr<Buffer>;
        using WeakPtr = std::weak_ptr<Buffer>;
        using SharedConstPtr = std::shared_ptr<const Buffer>;
        using inherit_shared_from_this<Resource, Buffer>::shared_from_this;

        /** Buffer access flags.
            These flags are hints the driver how the buffer will be used.
        */
        enum class CpuAccess
        {
            None,    ///< The CPU can't access the buffer's content. The buffer can be updated using Buffer#updateData()
            Write,   ///< The buffer can be mapped for CPU writes
            Read,    ///< The buffer can be mapped for CPU reads
        };

        /** Buffer GPU access flags.
            These flags are hints to the driver about how the buffer will 
            be used from the GPU.
        */
        enum class GpuAccessFlags
        {
            ReadOnly  = 0, ///< Buffer will mapped for GPU read only.
            ReadWrite = 1, ///< Buffer will mapped for GPU read-write.
            WriteOnly = 2, ///< Buffer will mapped for GPU write only.
        };

        enum class MapType
        {
            Read,               ///< Map the buffer for read access. Buffer had to be created with AccessFlags#MapWrite flag.
            WriteDiscard,       ///< Map the buffer for write access, discarding the previous content of the entire buffer. Buffer had to be created with AccessFlags#MapWrite flag.
        };
        
        ~Buffer();

        /** create a new buffer.
            \param[in] size The size in bytes of the buffer.
            \param[in] bind Buffer bind flags
            \param[in] update Flags indicating how the buffer can be updated
            \param[in] pInitData Optional parameter. Initial buffer data. Pointed buffer size should be at least Size bytes.
            \return A pointer to a new buffer object, or nullptr if creation failed.
        */
        static SharedPtr create(size_t size, Resource::BindFlags bind, CpuAccess cpuAccess, const void* pInitData);
        
        /** Copy the contents of this buffer to the given buffer.\nThe entire buffer will be copied, so the destination buffer must have the *same* size as the source buffer.
            \param pDst The destination buffer.
        */
        void copy(Buffer* pDst) const;
        /** Copy the contents of this buffer to the given buffer.
            \param pDst The destination buffer.
            \param srcOffset The offset within this buffer to start copying from.
            \param dstOffset The offset within the destination buffer to start copying from.
            \param count The number of bytes to copy.
        */
        void copy(Buffer* pDst, size_t srcOffset, size_t dstOffset, size_t count) const;

        /** Update the buffer's data
            \param[in] pData Pointer to the source data.
            \param[in] offset Byte offset into the destination buffer, indicating where to start copy into.
            \param[in] size Number of bytes to copy.
            If offset and size will cause an out-of-bound access to the buffer, an error will be logged and the update will fail.
        */
        void updateData(const void* pData, size_t offset, size_t size) const;

        /** Read the buffer's data to a user supplied buffer
            \param[in] pData Pointer to the destination buffer.
            \param[in] offset Byte offset into the destination buffer, indicating where to start copy into.
            \param[in] size Number of bytes to copy. Destination buffer should be at least Size bytes.
            If offset and size will cause an out-of-bound access to the buffer, an error will be logged and the read will fail.
        */
        void readData(void* pData, size_t offset, size_t size) const;

        /** Get the GPU address
        */
        uint64_t getGpuAddress() const;

        /** Get the size of the buffer
        */
        size_t getSize() const { return mSize; }

        /** Map the buffer
        */
        void* map(MapType Type) const;

        /** Unmap the buffer
        */
        void unmap() const;

        /** Load the buffer to the GPU memory.
            \return The GPU address, which can be used as a pointer in shaders.
        */
        uint64_t makeResident(Buffer::GpuAccessFlags flags = Buffer::GpuAccessFlags::ReadOnly) const;

        /** Unload the texture to the GPU memory. This function is only valid after makeResident() call was made with a matching sample. If makeResident() wasn't called, the evict() will be silently ignored.
        */
        void evict() const;

        /** Get safe offset and size values
        */
        bool adjustSizeOffsetParams(size_t& size, size_t& offset) const
        {
            if (offset >= mSize)
            {
                logWarning("Buffer::adjustSizeOffsetParams() - offset is larger than the buffer size.");
                return false;
            }

            if (offset + size > mSize)
            {
                logWarning("Buffer::adjustSizeOffsetParams() - offset + size will cause an OOB access. Clamping size");
                size = mSize - offset;
            }
            return true;
        }

    protected:
        bool init(const void* pInitData);
        Buffer(size_t size, BindFlags bind, CpuAccess update) : Resource(Type::Buffer, bind), mSize(size), mCpuAccess(update){}
        uint64_t mBindlessHandle = 0;
        size_t mSize = 0;
        CpuAccess mCpuAccess;
        void* mpApiData = nullptr;
    };
}