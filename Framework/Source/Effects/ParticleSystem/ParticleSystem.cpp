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
#include "ParticleSystem.h"
#include "API/Resource.h"
#include "glm/gtc/random.hpp"
#include <algorithm>
#include "Utils/Gui.h"

namespace Falcor
{
    ParticleSystem::SharedPtr ParticleSystem::create(RenderContext* pCtx, uint32_t maxParticles, std::string drawPixelShader, std::string simulateComputeShader)
    {
        return ParticleSystem::SharedPtr(new ParticleSystem(pCtx, maxParticles, drawPixelShader, simulateComputeShader));
    }

    ParticleSystem::ParticleSystem(RenderContext* pCtx, uint32_t maxParticles, std::string drawPixelShader, std::string simulateComputeShader)
    {
        mMaxParticles = maxParticles;

        //Shaders
        ComputeProgram::SharedPtr simulateCs = ComputeProgram::createFromFile(simulateComputeShader);
        //Add num sim threads to emit defines (for calculating needed thread groups in indirect arg buffer)
        uint32_t simThreadsX = 1, simThreadsY = 1, simThreadsZ= 1;
        simulateCs->getActiveVersion()->getShader(ShaderType::Compute)->
            getReflectionInterface()->GetThreadGroupSize(&simThreadsX, &simThreadsY, &simThreadsZ);
        Program::DefineList emitDefines;
        emitDefines.add("_SIMULATE_THREADS", std::to_string(simThreadsX * simThreadsY * simThreadsZ));
        ComputeProgram::SharedPtr emitCs = ComputeProgram::createFromFile("Effects/ParticleEmit.cs.hlsl", emitDefines);
        mDrawResources.shader = GraphicsProgram::createFromFile("Effects/ParticleVertex.vs.hlsl", drawPixelShader);

        //Buffers
        //IndexList
        std::vector<uint32_t> indices;
        indices.resize(maxParticles);
        uint32_t counter = 0;
        std::generate(indices.begin(), indices.end(), [&counter] {return counter++; });
        ProgramReflection::SharedConstPtr pEmitReflect = emitCs->getActiveVersion()->getReflector();
        auto indexListReflect = pEmitReflect->getBufferDesc("IndexList", ProgramReflection::BufferReflection::Type::Structured);
        mpIndexList = StructuredBuffer::create(indexListReflect, maxParticles);
        mpIndexList->setBlob(indices.data(), 0, indices.size() * sizeof(uint32_t));

        //ParticlePool
        auto particlePoolReflect = pEmitReflect->getBufferDesc("ParticlePool", ProgramReflection::BufferReflection::Type::Structured);
        mpParticlePool = StructuredBuffer::create(particlePoolReflect, maxParticles);

        //Indirect Arg buffer (for simulate dispatch and draw)
        uint32_t indirectArgsSize = sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(D3D12_DRAW_ARGUMENTS);
        mpIndirectArgs = StructuredBuffer::create(pEmitReflect->
            getBufferDesc("dispatchArgs", ProgramReflection::BufferReflection::Type::Structured), 
            indirectArgsSize / sizeof(uint32_t), Resource::BindFlags::UnorderedAccess | Resource::BindFlags::IndirectArg);
        uint32_t initialValues[7] = { 1, 1, 1, 4, 1, 0, 0 };
        mpIndirectArgs->setBlob(initialValues, 0, indirectArgsSize);

        //Vars
        mEmitResources.vars = ComputeVars::create(pEmitReflect);
        mEmitResources.vars->setStructuredBuffer("IndexList", mpIndexList);
        mEmitResources.vars->setStructuredBuffer("ParticlePool", mpParticlePool);
        mEmitResources.vars->setStructuredBuffer("dispatchArgs", mpIndirectArgs);
        mEmitResources.vars->setRawBuffer("numAlive", mpIndexList->getUAVCounter());
        mSimulateResources.vars = ComputeVars::create(simulateCs->getActiveVersion()->getReflector());
        mSimulateResources.vars->setStructuredBuffer("IndexList", mpIndexList);
        mSimulateResources.vars->setStructuredBuffer("ParticlePool", mpParticlePool);
        mSimulateResources.vars->setStructuredBuffer("drawArgs", mpIndirectArgs);
        mSimulateResources.vars->setRawBuffer("numAlive", mpIndexList->getUAVCounter());
        mDrawResources.vars = GraphicsVars::create(mDrawResources.shader->getActiveVersion()->getReflector());
        mDrawResources.vars->setStructuredBuffer("IndexList", mpIndexList);
        mDrawResources.vars->setStructuredBuffer("ParticlePool", mpParticlePool);

        //State
        mEmitResources.state = ComputeState::create();
        mEmitResources.state->setProgram(emitCs);
        mSimulateResources.state = ComputeState::create();
        mSimulateResources.state->setProgram(simulateCs);

        //Create empty vbo for draw 
        Vao::BufferVec bufferVec;
        VertexLayout::SharedPtr pLayout = VertexLayout::create();
        Vao::Topology topology = Vao::Topology::TriangleStrip;
        mDrawResources.vao = Vao::create(bufferVec, pLayout, nullptr, ResourceFormat::R32Uint, topology);
    }

    void ParticleSystem::emit(RenderContext* pCtx, uint32_t num)
    {
        //Fill emit data
        EmitData emitData;
        for (uint32_t i = 0; i < num; ++i)
        {
            Particle p;  
            p.pos = mEmitter.spawnPos + glm::linearRand(-mEmitter.spawnPosOffset, mEmitter.spawnPosOffset);
            p.vel = mEmitter.vel + glm::linearRand(-mEmitter.velOffset, mEmitter.velOffset);
            p.accel = mEmitter.accel + glm::linearRand(-mEmitter.accelOffset, mEmitter.accelOffset);
            //total scale of the billboard, so the amount to actually move to billboard corners is half scale. 
            p.scale = 0.5f * mEmitter.scale + glm::linearRand(-mEmitter.scaleOffset, mEmitter.scaleOffset);
            p.growth = 0.5f * mEmitter.growth + glm::linearRand(-mEmitter.growthOffset, mEmitter.growthOffset);
            p.life = mEmitter.duration + glm::linearRand(-mEmitter.growthOffset, mEmitter.growthOffset);
            p.rot = mEmitter.billboardRotation + glm::linearRand(-mEmitter.billboardRotationOffset, mEmitter.billboardRotationOffset);
            p.rotVel = mEmitter.billboardRotationVel + glm::linearRand(-mEmitter.billboardRotationVelOffset, mEmitter.billboardRotationVelOffset);
            emitData.particles[i] = p;
        }
        emitData.numEmit = num;
        emitData.maxParticles = mMaxParticles;

        //Send vars and call
        pCtx->pushComputeState(mEmitResources.state);
        mEmitResources.vars->getConstantBuffer(0)->setBlob(&emitData, 0u, sizeof(EmitData));
        pCtx->pushComputeVars(mEmitResources.vars);
        uint32_t numGroups = (uint32_t)std::ceil((float)num / EMIT_THREADS);
        pCtx->dispatch(1, numGroups, 1);
        pCtx->popComputeVars();
        pCtx->popComputeState();
    }

    void ParticleSystem::update(RenderContext* pCtx, float dt)
    {
        SimulatePerFrame perFrame;
        perFrame.dt = dt;

        mEmitTimer += dt;
        if (mEmitTimer >= mEmitter.emitFrequency)
        {
            mEmitTimer -= mEmitter.emitFrequency;
            emit(pCtx, max(mEmitter.emitCount + glm::linearRand(-mEmitter.emitCountOffset, mEmitter.emitCountOffset), 0));
        }

        pCtx->pushComputeState(mSimulateResources.state);
        mSimulateResources.vars->getConstantBuffer(0)->setBlob(&perFrame, 0u, sizeof(SimulatePerFrame));
        pCtx->pushComputeVars(mSimulateResources.vars);
        pCtx->dispatchIndirect(mpIndirectArgs.get(), 0);
        pCtx->popComputeVars();
        pCtx->popComputeState();
    }

    void ParticleSystem::render(RenderContext* pCtx, glm::mat4 view, glm::mat4 proj)
    {
        VSPerFrame cbuf;
        cbuf.view = view;
        cbuf.proj = proj;
        mDrawResources.vars->getConstantBuffer(0)->setBlob(&cbuf, 0, sizeof(cbuf));

        //save prev vao and shader program
        GraphicsState::SharedPtr state = pCtx->getGraphicsState();
        GraphicsProgram::SharedPtr prevShader = state->getProgram();
        Vao::SharedConstPtr prevVao = state->getVao();
        //update to ps's vao/draw shader
        state->setProgram(mDrawResources.shader);
        state->setVao(mDrawResources.vao);
        pCtx->pushGraphicsVars(mDrawResources.vars);
        pCtx->drawIndirect(mpIndirectArgs.get(), sizeof(D3D12_DISPATCH_ARGUMENTS));
        pCtx->popGraphicsVars();
        //setting null vao causes crash
        if (prevVao)
        {
            state->setVao(prevVao);
        }
        state->setProgram(prevShader);
    }

    void ParticleSystem::renderUi(Gui* pGui)
    {
        float floatMax = std::numeric_limits<float>::max();
        pGui->addFloatVar("Duration", mEmitter.duration, 0.f);
        pGui->addFloatVar("DurationOffset", mEmitter.durationOffset, 0.f);
        pGui->addFloatVar("Frequency", mEmitter.emitFrequency, 0.01f);
        int32_t emitCount = mEmitter.emitCount;
        pGui->addIntVar("EmitCount", emitCount, 0, MAX_EMIT);
        mEmitter.emitCount = emitCount;
        pGui->addIntVar("EmitCountOffset", mEmitter.emitCountOffset, 0);
        pGui->addFloat3Var("SpawnPos", mEmitter.spawnPos, -floatMax, floatMax);
        pGui->addFloat3Var("SpawnPosOffset", mEmitter.spawnPosOffset, 0.f, floatMax);
        pGui->addFloat3Var("Velocity", mEmitter.vel, -floatMax, floatMax);
        pGui->addFloat3Var("VelOffset", mEmitter.velOffset, 0.f, floatMax);
        pGui->addFloat3Var("Accel", mEmitter.accel, -floatMax, floatMax);
        pGui->addFloat3Var("AccelOffset", mEmitter.accelOffset, 0.f, floatMax);
        pGui->addFloatVar("Scale", mEmitter.scale, 0.001f);
        pGui->addFloatVar("ScaleOffset", mEmitter.scaleOffset, 0.001f);
        pGui->addFloatVar("Growth", mEmitter.growth);
        pGui->addFloatVar("GrowthOffset", mEmitter.growthOffset, 0.001f);
        pGui->addFloatVar("BillboardRotation", mEmitter.billboardRotation);
        pGui->addFloatVar("BillboardRotationOffset", mEmitter.billboardRotationOffset);
        pGui->addFloatVar("BillboardRotationVel", mEmitter.billboardRotationVel);
        pGui->addFloatVar("BillboardRotationVelOffset", mEmitter.billboardRotationVelOffset);
    }

    void ParticleSystem::setParticleDuration(float dur, float offset)
    { 
        mEmitter.duration = dur; 
        mEmitter.durationOffset = offset; 
    }

    void ParticleSystem::setEmitData(uint32_t emitCount, uint32_t emitCountOffset, float emitFrequency)
    {
        mEmitter.emitCount = (int32_t)emitCount;
        mEmitter.emitCountOffset = (int32_t)emitCountOffset;
        mEmitter.emitFrequency = emitFrequency;
    }

    void ParticleSystem::setSpawnPos(vec3 spawnPos, vec3 offset)
    {
        mEmitter.spawnPos = spawnPos;
        mEmitter.spawnPosOffset = offset;
    }

    void ParticleSystem::setVelocity(vec3 velocity, vec3 offset)
    {
        mEmitter.vel = velocity;
        mEmitter.velOffset = offset;
    }

    void ParticleSystem::setAcceleration(vec3 accel, vec3 offset)
    {
        mEmitter.accel = accel;
        mEmitter.accelOffset = offset;
    }

    void ParticleSystem::setScale(float scale, float offset)
    {
        mEmitter.scale = scale;
        mEmitter.scaleOffset = offset;
    }

    void ParticleSystem::setGrowth(float growth, float offset)
    {
        mEmitter.growth = growth;
        mEmitter.growthOffset = offset;
    }

    void ParticleSystem::setBillboardRotation(float rot, float offset)
    {
        mEmitter.billboardRotation = rot;
        mEmitter.billboardRotationOffset = offset;
    }
    
    void ParticleSystem::setBillboardRotationVelocity(float rotVel, float offset)
    {
        mEmitter.billboardRotationVel = rotVel;
        mEmitter.billboardRotationVelOffset = offset;
    }
}
