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
#include "RenderContext.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "BlendState.h"
#include "FBO.h"

namespace Falcor
{
    RenderContext::RenderContext()
    {
    }

    void RenderContext::prepareForDraw()
    {
        prepareForDrawApi();
    }

    void RenderContext::pushPipelineState(const PipelineState::SharedPtr& pState)
    {
        mPipelineStateStack.push(mpPipelineState);
        setPipelineState(pState);
    }

    void RenderContext::popPipelineState()
    {
        if (mPipelineStateStack.empty())
        {
            logWarning("Can't pop from the PipelineState stack. The stack is empty");
            return;
        }

        setPipelineState(mPipelineStateStack.top());
        mPipelineStateStack.pop();
    }

    void RenderContext::pushProgramVars(const ProgramVars::SharedPtr& pVars)
    {
        mProgramVarsStack.push(mpProgramVars);
        setProgramVariables(pVars);
    }

    void RenderContext::popProgramVars()
    {
        if (mProgramVarsStack.empty())
        {
            logWarning("Can't pop from the ProgramVars stack. The stack is empty");
            return;
        }

        setProgramVariables(mProgramVarsStack.top());
        mProgramVarsStack.pop();
    }
}
