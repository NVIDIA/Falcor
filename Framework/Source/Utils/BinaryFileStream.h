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
#include <fstream>

namespace Falcor
{    
    class BinaryFileStream
    {
    public:
        enum class Mode
        {
            Read    = 0x1,
            Write   = 0x2,

            ReadWrite = 0x3
        };

        BinaryFileStream() {};
        BinaryFileStream(const std::string& filename, Mode mode = Mode::ReadWrite)
        {
            open(filename, mode);
        }

        ~BinaryFileStream()
        {
            close();
        }

        void open(const std::string& filename, Mode mode = Mode::ReadWrite)
        { 
            std::ios::openmode iosMode = std::ios::binary;
            iosMode |= ((mode == Mode::Read) || (mode == Mode::ReadWrite)) ? std::ios::in : 0;
            iosMode |= ((mode == Mode::Write) || (mode == Mode::ReadWrite))? std::ios::out : 0;
            mStream.open(filename.c_str(), iosMode);
            mFilename = filename;
        }

        void close()
        {
            mStream.close();
        }

        void remove()
        {
            if(mStream.is_open())
            {
                close();
            }
            std::remove(mFilename.c_str());
        }

		uint32_t getRemainingStreamSize()
		{	
			std::streamoff currentPos = mStream.tellg();
			mStream.seekg(0, mStream.end);
			std::streamoff length = mStream.tellg();
			mStream.seekg(currentPos);
			return (uint32_t)(length - currentPos); 
		}

        bool isGood() { return mStream.good(); }
        bool isBad()  { return mStream.bad(); }
        bool isFail() { return mStream.fail(); }
        bool isEof() { return mStream.eof(); }

        BinaryFileStream& read(void* pData, size_t Count) { mStream.read((char*)pData, Count); return *this; }

        BinaryFileStream& write(const void* pData, size_t Count) { mStream.write((char*)pData, Count); return *this; }

        // Operator overloads
        template<typename T>
        BinaryFileStream& operator>>(T& val) { return read(&val, sizeof(T)); }

        template<typename T>
        BinaryFileStream& operator<<(const T& val) { return write(&val, sizeof(T)); }
    private:
        std::fstream mStream;
        std::string mFilename;
    };
}