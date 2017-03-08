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
#include "API/LowLevel/GpuFence.h"
#include "API/Device.h"

namespace Falcor
{
    GpuFence::~GpuFence()
    {
        CloseHandle(mEvent);
    }

    GpuFence::SharedPtr GpuFence::create()
    {
        SharedPtr pFence = SharedPtr(new GpuFence());
        pFence->mEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ID3D12Device* pDevice = gpDevice->getApiHandle().GetInterfacePtr();

        HRESULT hr = pDevice->CreateFence(pFence->mCpuValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence->mApiHandle));
        if(FAILED(hr))
        {
            d3dTraceHR("Failed to create a fence object", hr);
            return nullptr;
        }

        return pFence;
    }

    uint64_t GpuFence::gpuSignal(CommandQueueHandle pQueue)
    {
        mCpuValue++;
        d3d_call(pQueue->Signal(mApiHandle, mCpuValue));
        return mCpuValue;
    }

    uint64_t GpuFence::cpuSignal()
    {
        mCpuValue++;
        d3d_call(mApiHandle->Signal(mCpuValue));
        return mCpuValue;
    }

    void GpuFence::syncGpu(CommandQueueHandle pQueue)
    {
        assert(mCpuValue);
        d3d_call(pQueue->Wait(mApiHandle, mCpuValue));
    }

    void GpuFence::syncCpu()
    {
        assert(mCpuValue);
        uint64_t gpuVal = getGpuValue();
        if (gpuVal < mCpuValue)
        {
            d3d_call(mApiHandle->SetEventOnCompletion(mCpuValue, mEvent));
            WaitForSingleObject(mEvent, INFINITE);
        }
    }

    uint64_t GpuFence::getGpuValue() const
    {
        return mApiHandle->GetCompletedValue();
    }
}
