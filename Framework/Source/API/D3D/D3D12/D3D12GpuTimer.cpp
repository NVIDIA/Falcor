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
#include "API/GpuTimer.h"
#include "API/Buffer.h"
#include "API/Device.h"

namespace Falcor
{
    struct QueryData
    {
        Buffer::SharedPtr pResolveBuffer;
        QueryHeap::ApiHandle pHeap;
        double frequency;
        uint32_t start;
        uint32_t end;
    };

    GpuTimer::SharedPtr GpuTimer::create()
    {
        return SharedPtr(new GpuTimer());
    }

    GpuTimer::GpuTimer()
    {
        QueryData* pData = new QueryData;
        mpApiData = pData;

        pData->pResolveBuffer = Buffer::create(sizeof(uint64_t) * 2, Buffer::BindFlags::None, Buffer::CpuAccess::Read, nullptr);

        uint64_t freq;
        d3d_call(gpDevice->getRenderContext()->getLowLevelData()->getCommandQueue()->GetTimestampFrequency(&freq));
        pData->frequency = 1000.0/(double)freq;

        auto& pHeap = gpDevice->getTimestampQueryHeap();
        pData->start = pHeap->allocate();
        pData->end = pHeap->allocate();
        pData->pHeap = pHeap->getApiHandle();
    }

    GpuTimer::~GpuTimer()
    {
        QueryData* pData = (QueryData*)mpApiData;
        auto& pHeap = gpDevice->getTimestampQueryHeap();
        pHeap->release(pData->start);
        pHeap->release(pData->end);
        delete pData;
    }

    void GpuTimer::begin()
    {
        if (mStatus == Status::Begin)
        {
            logWarning("GpuTimer::begin() was followed by another call to GpuTimer::begin() without a GpuTimer::end() in-between. Ignoring call.");
            return;
        }

        if (mStatus == Status::End)
        {
            logWarning("GpuTimer::begin() was followed by a call to GpuTimer::end() without querying the data first. The previous results will be discarded.");
        }
        QueryData* pData = (QueryData*)mpApiData;
        gpDevice->getRenderContext()->getLowLevelData()->getCommandList()->EndQuery(pData->pHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
        mStatus = Status::Begin;
    }

    void GpuTimer::end()
    {
        if (mStatus != Status::Begin)
        {
            logWarning("GpuTimer::end() was called without a preciding GpuTimer::begin(). Ignoring call.");
            return;
        }
        QueryData* pData = (QueryData*)mpApiData;
        gpDevice->getRenderContext()->getLowLevelData()->getCommandList()->EndQuery(pData->pHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
        mStatus = Status::End;
    }

    bool GpuTimer::getElapsedTime(bool waitForResult, double& elapsedTime)
    {
        if (mStatus != Status::End)
        {
            logWarning("GpuTimer::getElapsedTime() was called but the GpuTimer::end() wasn't called. No data to fetch.");
            return false;
        }
        QueryData* pData = (QueryData*)mpApiData;
        gpDevice->getRenderContext()->getLowLevelData()->getCommandList()->ResolveQueryData(pData->pHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, pData->pResolveBuffer->getApiHandle(), 0);

        uint64_t* pRes = (uint64*)pData->pResolveBuffer->map(Buffer::MapType::Read);
        
        double start = (double)pRes[0];
        double end = (double)pRes[1];
        double range = end - start;
        elapsedTime = range * pData->frequency;
        mStatus = Status::Idle;
        pData->pResolveBuffer->unmap();

        return true;
    }
}
