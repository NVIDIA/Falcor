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
#include "CommandListD3D12.h"

namespace Falcor
{
    CommandList::~CommandList() = default;

    CommandList::SharedPtr CommandList::create(uint32_t allocatorCount)
    {
        SharedPtr pList = SharedPtr(new CommandList);
        // Create the command queue
        D3D12_COMMAND_QUEUE_DESC cqDesc = {};
        cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ID3D12DevicePtr pDevice = getD3D12Device();

        if(FAILED(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pList->mpQueue))))
        {
            Logger::log(Logger::Level::Error, "Failed to create command queue");
            return nullptr;
        }

        // Create a command allocator
        pList->mpAllocators.resize(allocatorCount);
        for(uint32_t i = 0; i < allocatorCount; i++)
        {
            if(FAILED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pList->mpAllocators[i]))))
            {
                Logger::log(Logger::Level::Error, "Failed to create command allocator");
                return nullptr;
            }
        }

        // Create a command queue
        if(FAILED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pList->mpAllocators[0], nullptr, IID_PPV_ARGS(&pList->mpList))))
        {
            Logger::log(Logger::Level::Error, "Failed to create command list");
            return nullptr;
        }

        // We expect the list to be closed before we start using it
        d3d_call(pList->mpList->Close());

        return pList;
    }

    void CommandList::reset()
    {
        // Skip to the next allocator
        mActiveAllocator = (mActiveAllocator + 1) % mpAllocators.size();
        d3d_call(mpAllocators[mActiveAllocator]->Reset());
        d3d_call(mpList->Reset(mpAllocators[mActiveAllocator], nullptr));
    }

    void CommandList::submit()
    {
        d3d_call(mpList->Close());
        ID3D12CommandList* pList = mpList.GetInterfacePtr();
        mpQueue->ExecuteCommandLists(1, &pList);
    }
}

#endif //#ifdef FALCOR_D3D12
