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

AppendStructuredBuffer<uint> deadList;
AppendStructuredBuffer<uint> aliveList;
RWStructuredBuffer<Particle> ParticlePool;
RWByteAddressBuffer numDead;
RWStructuredBuffer<uint> drawArgs;

[numthreads(numThreads, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    int index = (int)getParticleIndex(groupID.x, numThreads, groupIndex);
    uint numDeadParticles = (uint)(numDead.Load(0));
    int numAliveParticles = perFrame.maxParticles - numDeadParticles;

    //check if the particle is alive
    if (ParticlePool[index].life > 0)
    {
        ParticlePool[index].life -= perFrame.dt;
        //check if the particle died this frame
        if (ParticlePool[index].life <= 0)
        {
            deadList.Append(index);
        }
        else
        {
            ParticlePool[index].pos += ParticlePool[index].vel * perFrame.dt;
            ParticlePool[index].vel += ParticlePool[index].accel * perFrame.dt;
            ParticlePool[index].scale = max(ParticlePool[index].scale + ParticlePool[index].growth * perFrame.dt, 0);            
            ParticlePool[index].rot += ParticlePool[index].rotVel * perFrame.dt;
            aliveList.Append(index);
        }

        //0 vert count per instance, 1 numInstances, 2 start vert loc, 3 start instance loc
        drawArgs[1] = numAliveParticles;
    }
}
