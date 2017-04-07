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
#include "ParticleData.h"

static const uint numThreads = 256;

cbuffer PerFrame
{
    SimulatePerFrame perFrame;
};

RWStructuredBuffer<uint> IndexList;
RWStructuredBuffer<Particle> ParticlePool;
RWByteAddressBuffer numAlive;
RWStructuredBuffer<uint> drawArgs;

[numthreads(numThreads, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = getParticleIndex(groupID.x, numThreads, groupIndex);
    uint numAliveParticles = (uint)(numAlive.Load(0));

    //make sure this corresponds to an actual alive particle, not a redundant thread
    if (index < numAliveParticles)
    {
        uint poolIndex = IndexList[index];
        ParticlePool[poolIndex].life -= perFrame.dt;
        if (ParticlePool[poolIndex].life <= 0)
        {
            uint counterIndex = IndexList.DecrementCounter();
            uint prevIndex;
            InterlockedExchange(IndexList[counterIndex], poolIndex, prevIndex);
            InterlockedExchange(IndexList[index], prevIndex, poolIndex);
            numAliveParticles -= 1;
        }
        else
        {
            ParticlePool[poolIndex].pos += ParticlePool[poolIndex].vel * perFrame.dt;
            ParticlePool[poolIndex].vel += ParticlePool[poolIndex].accel * perFrame.dt;
            ParticlePool[poolIndex].scale = max(ParticlePool[poolIndex].scale + ParticlePool[poolIndex].growth * perFrame.dt, 0);
        }

        //0, 1, 2, dispatch xyz. 3 vert count per instance, 4 numInstances, 5 start vert loc, 6 start instance loc
        drawArgs[3] = numAliveParticles * 6;
    }
}
