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
#include "LowLevel/DescriptorTable.h"

namespace Falcor
{
    class DescriptorSet
    {
    public:
        using SharedPtr = std::shared_ptr<DescriptorSet>;

        struct Desc
        {
        public:
            Desc& setCbvCount(uint32_t cbCount) { mCbCount = cbCount; return *this; }
            Desc& setSrvCount(uint32_t srvCount) { mSrvCount = srvCount; return *this; }
            Desc& setUavCount(uint32_t uavCount) { mUavCount = uavCount; return *this; }
            Desc& setSamplerCount(uint32_t samplerCount) { mSamplerCount = samplerCount; return *this; }
        private:
            friend class DescriptorSet;
            uint32_t mCbCount = 0;
            uint32_t mSrvCount = 0;
            uint32_t mUavCount = 0;
            uint32_t mSamplerCount = 0;
        };

        static SharedPtr create(const Desc& desc);

        uint32_t getCbCount() const { return mDesc.mCbCount; }
        uint32_t getSrvCount() const { return mDesc.mSrvCount; }
        uint32_t getUavCount() const { return mDesc.mUavCount; }
        uint32_t getSamplerCount() const { return mDesc.mSamplerCount; }

        uint32_t getDescTableCount() const { return (uint32_t)mDescTableVec.size(); }
        DescriptorTable::SharedPtr getDescriptorTable(uint32_t index) { return mDescTableVec[index]; }

    private:
        DescriptorSet(const Desc& desc);
        void apiInit();
        Desc mDesc;
        using DescTableVec = std::vector<DescriptorTable::SharedPtr>;

        DescTableVec mDescTableVec;
    };
}