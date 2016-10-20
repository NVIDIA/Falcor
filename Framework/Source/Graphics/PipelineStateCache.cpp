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
#include "Framework.h"
#include "PipelineStateCache.h"

namespace Falcor
{
    std::vector<PipelineState::SharedPtr> gStates;
    PipelineState::SharedPtr PipelineStateCache::getPSO()
    {
        // Check if we need to create a root-signature
        // FIXME Is this the correct place for this?
        const ProgramVersion* pProgVersion = mpProgram ? mpProgram->getActiveVersion().get() : nullptr;
        if (pProgVersion != mCachedData.pProgramVersion)
        {
            mCachedData.pProgramVersion = pProgVersion;
            if (mCachedData.isUserRootSignature == false)
            {
                mpRootSignature = RootSignature::create(pProgVersion->getReflector().get());
            }
        }

        mDesc.setProgramVersion(mpProgram ? mpProgram->getActiveVersion() : nullptr);
        mDesc.setFboFormats(mpFbo ? mpFbo->getDesc() : Fbo::Desc());
        mDesc.setVertexLayout(mpVao ? mpVao->getVertexLayout() : nullptr);
        mDesc.setRootSignature(mpRootSignature);

        // D3D12_FIX Need real cache
        auto p = PipelineState::create(mDesc);
        gStates.push_back(p);
        return p;
    }
}