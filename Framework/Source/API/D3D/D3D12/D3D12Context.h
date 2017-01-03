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
#include "API/LowLevel/FencedPool.h"
#include "API/Device.h"

namespace Falcor
{
    class D3D12ContextData
    {
    public:
        static D3D12ContextData* create(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
        {
            D3D12ContextData* pThis = new D3D12ContextData;
            pThis->mpFence = GpuFence::create();

            // Create the command queue
            D3D12_COMMAND_QUEUE_DESC cqDesc = {};
            cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            cqDesc.Type = type;

            if (FAILED(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pThis->mpQueue))))
            {
                logError("Failed to create command queue");
                delete pThis;
                return nullptr;
            }

            // Create a command allocator
            switch (type)
            {
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
                pThis->mpAllocatorPool = CommandAllocatorPool::create(pThis->mpFence, newCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>);
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                pThis->mpAllocatorPool = CommandAllocatorPool::create(pThis->mpFence, newCommandAllocator<D3D12_COMMAND_LIST_TYPE_COMPUTE>);
                break;
            case D3D12_COMMAND_LIST_TYPE_COPY:
                pThis->mpAllocatorPool = CommandAllocatorPool::create(pThis->mpFence, newCommandAllocator<D3D12_COMMAND_LIST_TYPE_COPY>);
                break;
            default:
                should_not_get_here();
            }
            pThis->mpAllocator = pThis->mpAllocatorPool->newObject();

            // Create a command list
            if (FAILED(pDevice->CreateCommandList(0, type, pThis->mpAllocator, nullptr, IID_PPV_ARGS(&pThis->mpList))))
            {
                logError("Failed to create command list for RenderContext");
                delete pThis;
                return nullptr;
            }
            return pThis;
        }

        ID3D12GraphicsCommandListPtr getCommandList() const { return mpList; }
        ID3D12CommandQueuePtr getCommandQueue() const {return mpQueue;}
        ID3D12CommandAllocatorPtr getCommandAllocator() const { return mpAllocator; }
        GpuFence::SharedPtr getFence() const { return mpFence; }

        void reset()
        {
            mpFence->gpuSignal(mpQueue);
            mpAllocator = mpAllocatorPool->newObject();
            d3d_call(mpList->Close());
            d3d_call(mpAllocator->Reset());
            d3d_call(mpList->Reset(mpAllocator, nullptr));
        }

        void flush()
        {
            d3d_call(mpList->Close());
            ID3D12CommandList* pList = mpList.GetInterfacePtr();
            mpQueue->ExecuteCommandLists(1, &pList);
            mpFence->gpuSignal(mpQueue);
            mpList->Reset(mpAllocator, nullptr);
        }
    private:
        template<D3D12_COMMAND_LIST_TYPE type>
        static ID3D12CommandAllocatorPtr newCommandAllocator()
        {
            ID3D12CommandAllocatorPtr pAllocator;
            if (FAILED(gpDevice->getApiHandle()->CreateCommandAllocator(type, IID_PPV_ARGS(&pAllocator))))
            {
                logError("Failed to create command allocator");
                return nullptr;
            }
            return pAllocator;
        }

        using CommandAllocatorPool = FencedPool<ID3D12CommandAllocatorPtr>;

        CommandAllocatorPool::SharedPtr mpAllocatorPool;
        ID3D12GraphicsCommandListPtr mpList;
        ID3D12CommandQueuePtr mpQueue;                                                                    
        ID3D12CommandAllocatorPtr mpAllocator;
        GpuFence::SharedPtr mpFence;
    };
}