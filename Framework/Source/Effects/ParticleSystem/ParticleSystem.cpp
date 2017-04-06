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
#include "Data/Effects/ParticleData.h"

namespace Falcor
{
    void ParticleSystem::init(RenderContext* pCtx, uint32_t maxParticles, std::string ps, std::string cs)
    {
        mMaxParticles = maxParticles;

        //Shaders
        mSimulateCs = ComputeProgram::createFromFile(cs);
        uint32_t simThreadsX = 1, simThreadsY = 1, simThreadsZ= 1;
        mSimulateCs->getActiveVersion()->getShader(ShaderType::Compute)->
            getReflectionInterface()->GetThreadGroupSize(&simThreadsX, &simThreadsY, &simThreadsZ);
        Program::DefineList emitDefines;
        emitDefines.add("_SIMULATE_THREADS", std::to_string(simThreadsX * simThreadsY * simThreadsZ));
        mEmitCs = ComputeProgram::createFromFile("Effects/ParticleEmit.cs.hlsl", emitDefines);
        Program::DefineList defines;
        mDrawParticles = GraphicsProgram::createFromFile("Effects/ParticleVertex.vs.hlsl", ps);

        //Buffers
        //IndexList
        std::vector<uint32_t> indices;
        indices.resize(maxParticles);
        uint32_t counter = 0;
        std::generate(indices.begin(), indices.end(), [&counter] {return counter++; });
        ProgramReflection::SharedConstPtr pEmitReflect = mEmitCs->getActiveVersion()->getReflector();
        ProgramReflection::BufferReflection::SharedConstPtr indexListReflect = 
            pEmitReflect->getBufferDesc("IndexList", ProgramReflection::BufferReflection::Type::Structured);
        mpIndexList = StructuredBuffer::create(indexListReflect, maxParticles);
        mpIndexList->setBlob(indices.data(), 0, indices.size() * sizeof(uint32_t));

        //ParticlePool
        ProgramReflection::BufferReflection::SharedConstPtr particlePoolReflect =
            pEmitReflect->getBufferDesc("ParticlePool", ProgramReflection::BufferReflection::Type::Structured);
        mpParticlePool = StructuredBuffer::create(particlePoolReflect, maxParticles);

        //Indirect Arg buffer (for simulate dispatch and draw)
        uint32_t indirectArgsSize = sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(D3D12_DRAW_ARGUMENTS);
        mpIndirectArgs = StructuredBuffer::create(pEmitReflect->
            getBufferDesc("dispatchArgs", ProgramReflection::BufferReflection::Type::Structured), 
            indirectArgsSize / sizeof(uint32_t), Resource::BindFlags::UnorderedAccess | Resource::BindFlags::IndirectArg);
        uint32_t initialValues[7] = { 1, 1, 1, 1, 1, 0, 0 };
        mpIndirectArgs->setBlob(initialValues, 0, indirectArgsSize);

        //Vars
        mEmitVars = ComputeVars::create(pEmitReflect);
        mEmitVars->setStructuredBuffer("IndexList", mpIndexList);
        mEmitVars->setStructuredBuffer("ParticlePool", mpParticlePool);
        mEmitVars->setStructuredBuffer("dispatchArgs", mpIndirectArgs);
        mEmitVars->setRawBuffer("numAlive", mpIndexList->getUAVCounter());
        mSimulateVars = ComputeVars::create(mSimulateCs->getActiveVersion()->getReflector());
        mSimulateVars->setStructuredBuffer("IndexList", mpIndexList);
        mSimulateVars->setStructuredBuffer("ParticlePool", mpParticlePool);
        mSimulateVars->setStructuredBuffer("drawArgs", mpIndirectArgs);
        mSimulateVars->setRawBuffer("numAlive", mpIndexList->getUAVCounter());
        mpDrawVars = GraphicsVars::create(mDrawParticles->getActiveVersion()->getReflector());
        mpDrawVars->setStructuredBuffer("IndexList", mpIndexList);
        mpDrawVars->setStructuredBuffer("ParticlePool", mpParticlePool);

        //State
        mEmitState = ComputeState::create();
        mEmitState->setProgram(mEmitCs);
        mSimulateState = ComputeState::create();
        mSimulateState->setProgram(mSimulateCs);
        mpDrawState = GraphicsState::create();
        mpDrawState->setProgram(mDrawParticles);
        mpDrawState->setProgram(mDrawParticles);

        //Create empty vbo for draw 
        Vao::BufferVec bufferVec;
        VertexLayout::SharedPtr pLayout = VertexLayout::create();
        Vao::Topology topology = Vao::Topology::TriangleList;
        Vao::SharedPtr pVao = Vao::create(bufferVec, pLayout, nullptr, ResourceFormat::R32Uint, topology);
        mpDrawState->setVao(pVao);
    }

    void ParticleSystem::emit(RenderContext* pCtx, uint32_t num)
    {
        //Fill emit data
        EmitData emitData;
        for (uint32_t i = 0; i < num; ++i)
        {
            Particle p;  
            p.pos = Emitter.spawnPos + glm::linearRand(-Emitter.randSpawnPos, Emitter.randSpawnPos);
            p.vel = Emitter.vel + glm::linearRand(-Emitter.randVel, Emitter.randVel);
            p.accel = Emitter.accel + glm::linearRand(-Emitter.randAccel, Emitter.randAccel);
            //total scale of the billboard, so the amount to actually move to billboard corners is half scale. 
            p.scale = Emitter.scale + glm::linearRand(-Emitter.randScale, Emitter.randScale);
            p.growth = 0.5f * Emitter.growth + glm::linearRand(-Emitter.randGrowth, Emitter.randGrowth);
            p.life = Emitter.duration + glm::linearRand(-Emitter.randDuration, Emitter.randDuration);
            emitData.particles[i] = p;
        }
        emitData.numEmit = num;
        emitData.maxParticles = mMaxParticles;

        //Send vars and call
        pCtx->pushComputeState(mEmitState);
        mEmitVars->getConstantBuffer(0)->setBlob(&emitData, 0u, sizeof(EmitData));
        pCtx->pushComputeVars(mEmitVars);
        u32 numGroups = num % 64 == 0 ? num / 64 : (num / 64) + 1;
        pCtx->dispatch(1, numGroups, 1);
        pCtx->popComputeVars();
        pCtx->popComputeState();
    }

    void ParticleSystem::update(RenderContext* pCtx, float dt)
    {
        SimulatePerFrame perFrame;
        perFrame.dt = dt;
        perFrame.maxParticles = mMaxParticles;

        mEmitTimer += dt;
        if (mEmitTimer >= Emitter.emitFrequency)
        {
            mEmitTimer -= Emitter.emitFrequency;
            emit(pCtx, Emitter.emitCount + glm::linearRand(-Emitter.randEmitCount, Emitter.randEmitCount));
        }

        pCtx->pushComputeState(mSimulateState);
        mSimulateVars->getConstantBuffer(0)->setBlob(&perFrame, 0u, sizeof(SimulatePerFrame));
        pCtx->pushComputeVars(mSimulateVars);
        pCtx->dispatchIndirect(mpIndirectArgs.get(), 0);
        pCtx->popComputeVars();
        pCtx->popComputeState();
    }

    void ParticleSystem::render(RenderContext* pCtx, glm::mat4 view, glm::mat4 proj)
    {
        VSPerFrame cbuf;
        cbuf.view = view;
        cbuf.proj = proj;
        mpDrawVars->getConstantBuffer(0)->setBlob(&cbuf, 0, sizeof(cbuf));

        //TODO
        //I think I should be able to do this in init, pass in fbo once. But For some reason, that gives the error 
        //"A single command list cannot write to multiple buffers within a particular swapchain."
        //[STATE_SETTING ERROR #904: COMMAND_LIST_MULTIPLE_SWAPCHAIN_BUFFER_REFERENCES]
        mpDrawState->setFbo(pCtx->getGraphicsState()->getFbo());

        pCtx->pushGraphicsState(mpDrawState);
        pCtx->pushGraphicsVars(mpDrawVars);
        pCtx->drawIndirect(mpIndirectArgs.get(), sizeof(D3D12_DISPATCH_ARGUMENTS));
        pCtx->popGraphicsVars();
        pCtx->popGraphicsState();
    }

}
