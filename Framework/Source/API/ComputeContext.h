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
#include "CopyContext.h"
#include "API/ProgramVars.h"
#include "Graphics/ComputeState.h"

namespace Falcor
{
    class ComputeContext : public CopyContext, public inherit_shared_from_this<CopyContext, ComputeContext>
    {
    public:
        using SharedPtr = std::shared_ptr<ComputeContext>;
        using SharedConstPtr = std::shared_ptr<const ComputeContext>;
        ~ComputeContext();

        static SharedPtr create();

        /** Set the compute variables
        */
        void setComputeVars(const ComputeVars::SharedPtr& pVars) { mpComputeVars = pVars; applyComputeVars(); }

        /** Get the bound program variables object
        */
        ComputeVars::SharedPtr getComputeVars() const { return mpComputeVars; }

        /** Push the current compute vars and sets a new one
        */
        void pushComputeVars(const ComputeVars::SharedPtr& pVars);

        /** Pops the last ProgramVars from the stack and sets it
        */
        void popComputeVars();

        /** Set a compute state
        */
        void setComputeState(const ComputeState::SharedPtr& pState) { mpComputeState = pState; applyComputeState(); }

        /** Get the currently bound compute state
        */
        ComputeState::SharedPtr getComputeState() const { return mpComputeState; }

        /** Push the current compute state and sets a new one
        */
        void pushComputeState(const ComputeState::SharedPtr& pState);

        /** Pops the last PipelineState from the stack and sets it
        */
        void popComputeState();

        /** Dispatch a compute task
        */
        void dispatch(uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ);

    protected:
        ComputeContext() = default;
        void prepareForDispatch();
        void applyComputeState();
        void applyComputeVars();

        std::stack<ComputeState::SharedPtr> mpComputeStateStack;
        std::stack<ComputeVars::SharedPtr> mpComputeVarsStack;

        ComputeVars::SharedPtr mpComputeVars;
        ComputeState::SharedPtr mpComputeState;
    };
}
