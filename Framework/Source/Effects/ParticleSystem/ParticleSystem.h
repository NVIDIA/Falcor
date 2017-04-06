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
#include "API/RenderContext.h"

namespace Falcor
{
    class ParticleSystem
    {
    public:
        struct EmitterData
        {
            f32 duration = 5.f;
            f32 randDuration = 0.f;
            f32 emitFrequency = 0.01f;
            uint32_t emitCount = 4;
            int32_t randEmitCount = 0;
            vec3 spawnPos = vec3(0.f, 0.f, 0.f);
            vec3 randSpawnPos = vec3(0.f, 0.f, 0.f);
            vec3 vel = vec3(0.f, 0.f, 0.f);
            vec3 randVel = vec3(1.f, 1.f, 1.f);
            vec3 accel = vec3(0.f, 0.f, 0.f);
            vec3 randAccel = vec3(2.f, 2.f, 2.f);
            f32 scale = 0.1f;
            f32 randScale = 0.f;
            f32 growth = -0.05f;
            f32 randGrowth = 0.f;
            //rot
        } Emitter;

        void init(RenderContext* pCtx, uint32_t maxParticles);
        void emit(RenderContext* pCtx, uint32_t num);
        void update(RenderContext* pCtx, float dt);
        void render(RenderContext* pCtx, glm::mat4 view, glm::mat4 proj);
        GraphicsVars::SharedPtr getDrawVars() { return mpDrawVars; }
        ComputeVars::SharedPtr getSimulateVars() { return mSimulateVars; }

    private:
        struct SimulatePerFrame
        {
            float dt;
            uint32_t maxParticles;
        };

        struct VSPerFrame
        {
            glm::mat4 view;
            glm::mat4 proj;
        };

        uint32_t mMaxParticles;

        ComputeProgram::SharedPtr mEmitCs;
        ComputeVars::SharedPtr mEmitVars;
        ComputeState::SharedPtr mEmitState;

        ComputeProgram::SharedPtr mSimulateCs;
        ComputeVars::SharedPtr mSimulateVars;
        ComputeState::SharedPtr mSimulateState;

        GraphicsProgram::SharedPtr mDrawParticles;
        GraphicsVars::SharedPtr mpDrawVars;
        GraphicsState::SharedPtr mpDrawState;

        StructuredBuffer::SharedPtr mpParticlePool;
        StructuredBuffer::SharedPtr mpIndexList;
        //bytes 0 - 11 are for simulate dispatch, bytes 12 - 27 are for draw
        StructuredBuffer::SharedPtr mpIndirectArgs;

        float mEmitTimer = 0.f;
    };

}
