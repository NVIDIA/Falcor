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
#include "Data/Effects/ParticleData.h"

namespace Falcor
{
    class Gui;

    class ParticleSystem
    {
    public:
        static const char* kVertexShader;
        static const char* kSortShader;
        static const char* kEmitShader;
        static const char* kDefaultPixelShader;
        static const char* kDefaultSimulateShader;

        using SharedPtr = std::shared_ptr<ParticleSystem>;

        static SharedPtr create(RenderContext* pCtx, uint32_t maxParticles,
            std::string drawPixelShader = kDefaultPixelShader,
            std::string simulateComputeShader = kDefaultSimulateShader,
            bool sorted = true);
        void update(RenderContext* pCtx, float dt, glm::mat4 view);
        void render(RenderContext* pCtx, glm::mat4 view, glm::mat4 proj);
        void renderUi(Gui* pGui);
        GraphicsProgram::SharedPtr getDrawProgram() { return mDrawResources.shader; }
        GraphicsVars::SharedPtr getDrawVars() { return mDrawResources.vars; }
        ComputeProgram::SharedPtr getSimulateProgram() { return mSimulateResources.state->getProgram(); }
        ComputeVars::SharedPtr getSimulateVars() { return mSimulateResources.vars; }

        void setParticleDuration(float dur, float offset);
        float getParticleDuration() { return mEmitter.duration; }
        void setEmitData(uint32_t emitCount, uint32_t emitCountOffset, float emitFrequency);
        void setSpawnPos(vec3 spawnPos, vec3 offset);
        void setVelocity(vec3 velocity, vec3 offset);
        void setAcceleration(vec3 accel, vec3 offset);
        void setScale(float scale, float offset);
        void setGrowth(float growth, float offset);
        //in radians
        void setBillboardRotation(float rot, float offset);
        //in radians/sec
        void setBillboardRotationVelocity(float rotVel, float offset);

    private:
        ParticleSystem() = delete;
        ParticleSystem(RenderContext* pCtx, uint32_t maxParticles,
            std::string drawPixelShader, std::string simulateComputeShader, bool sorted);
        void emit(RenderContext* pCtx, uint32_t num);

        struct EmitterData
        {
            EmitterData() : duration(3.f), durationOffset(0.f), emitFrequency(0.1f), emitCount(32),
                emitCountOffset(0), spawnPos(0.f, 0.f, 0.f), spawnPosOffset(0.f, 0.5f, 0.f),
                vel(0, 5, 0), velOffset(2, 1, 2), accel(0, -3, 0), accelOffset(0.f, 0.f, 0.f),
                scale(0.2f), scaleOffset(0.f), growth(-0.05f), growthOffset(0.f), billboardRotation(0.f),
                billboardRotationOffset(0.25f), billboardRotationVel(0.f), billboardRotationVelOffset(0.f) {}
            float duration;
            float durationOffset; 
            float emitFrequency;
            int32_t emitCount;
            int32_t emitCountOffset;
            vec3 spawnPos;
            vec3 spawnPosOffset;
            vec3 vel;
            vec3 velOffset;
            vec3 accel;
            vec3 accelOffset;
            float scale;
            float scaleOffset;
            float growth;
            float growthOffset;
            float billboardRotation;
            float billboardRotationOffset;
            float billboardRotationVel;
            float billboardRotationVelOffset;
        } mEmitter;

        struct EmitResources
        {
            ComputeVars::SharedPtr vars;
            ComputeState::SharedPtr state;
        } mEmitResources;

        struct SimulateResources
        {
            ComputeVars::SharedPtr vars;
            ComputeState::SharedPtr state;
        } mSimulateResources;

        struct DrawResources
        {
            GraphicsProgram::SharedPtr shader;
            GraphicsVars::SharedPtr vars;
            Vao::SharedPtr vao;
        } mDrawResources;

        uint32_t mMaxParticles;
        uint32_t mSimulateThreads;
        float mEmitTimer = 0.f;

        //buffers
        StructuredBuffer::SharedPtr mpParticlePool;
        StructuredBuffer::SharedPtr mpDeadList;
        StructuredBuffer::SharedPtr mpAliveList;
        //for draw and sort (Draw 0, 1, 2, 3) (Dispatch 4, 5, 6)
        StructuredBuffer::SharedPtr mpIndirectArgs;

        //Maybe it'd be better to have a derived "SortedParticleSystem" class?
        void initSortResources();
        bool mShouldSort;
        std::vector<SortData> mSortDataReset;
        struct SortResources
        {
            StructuredBuffer::SharedPtr sortIterationCounter;
            ComputeState::SharedPtr state;
            ComputeVars::SharedPtr vars;
        } mSortResources;
    };
}
