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
            vec3 spawnPos = vec3(0.f, 0.f, -5.f);
            vec3 randSpawnPos = vec3(0.f, 0.f, 0.f);
            vec3 vel = vec3(0.f, 2.f, 0.f);
            vec3 randVel = vec3(1.f, 1.f, 1.f);
            vec3 accel = vec3(0.f, -1.f, 0.f);
            vec3 randAccel = vec3(0.f, 0.f, 0.f);
            f32 duration = 7.f;
            f32 randDuration = 0.f;
            //scale
            //rot
        } Emitter;

        void init(RenderContext* pCtx, uint32_t maxParticles);
        void emit(RenderContext* pCtx, uint32_t num);
        void update(RenderContext* pCtx, float dt);
        void render(RenderContext* pCtx, glm::mat4 view, glm::mat4 proj);


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
        //todo 
        //This shouldnt be in the class, it should just get added to a draw gfx state and then that state can be set 
        //However right now setting the draw state fbo is giving me multi buffer command list error. Just gonna do this 
        //and move on for now and figure it out before i merge
        Vao::SharedPtr pVao;
    };

}
