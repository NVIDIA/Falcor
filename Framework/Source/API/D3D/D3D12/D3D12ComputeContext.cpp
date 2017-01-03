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
#include "API/ComputeContext.h"

namespace Falcor
{
    ComputeContext::~ComputeContext() = default;

    ComputeContext::SharedPtr ComputeContext::create()
    {
        SharedPtr pCtx = SharedPtr(new ComputeContext());
        pCtx->mpLowLevelData = LowLevelContextData::create(LowLevelContextData::CommandListType::Compute);
        if (pCtx->mpLowLevelData == nullptr)
        {
            return nullptr;
        }
        pCtx->bindDescriptorHeaps();

        return pCtx;
    }

    void ComputeContext::prepareForDispatch()
    {
        assert(mpComputeState);

        // Bind the root signature and the root signature data
        if (mpComputeVars)
        {
            mpComputeVars->apply(const_cast<ComputeContext*>(this));
        }
        else
        {
            mpLowLevelData->getCommandList()->SetComputeRootSignature(RootSignature::getEmpty()->getApiHandle());
        }

        mpLowLevelData->getCommandList()->SetPipelineState(mpComputeState->getCSO()->getApiHandle());
        mCommandsPending = true;
    }

    void ComputeContext::dispatch(uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ)
    {
        prepareForDispatch();
        mpLowLevelData->getCommandList()->Dispatch(groupSizeX, groupSizeY, groupSizeZ);
    }

    void ComputeContext::applyComputeVars() {}
    void ComputeContext::applyComputeState() {}
}
