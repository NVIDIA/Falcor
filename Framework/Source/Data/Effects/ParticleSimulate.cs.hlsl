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
#ifdef _SORT
    SimulateWithSortPerFrame perFrame;
#else
    SimulatePerFrame perFrame;
#endif
};

AppendStructuredBuffer<uint> deadList;
#ifdef _SORT
AppendStructuredBuffer<SortData> aliveList;
RWStructuredBuffer<uint> sortIterationCounter;
#else
AppendStructuredBuffer<uint> aliveList;
#endif
RWStructuredBuffer<Particle> ParticlePool;
RWByteAddressBuffer numDead;
RWStructuredBuffer<uint> drawArgs;

uint getNextPow2(uint n)
{
    if (n == 0)
        return 0;

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

[numthreads(numThreads, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = getParticleIndex(groupID.x, numThreads, groupIndex);
    uint numDeadParticles = (uint)(numDead.Load(0));
    uint numAliveParticles = perFrame.maxParticles - numDeadParticles;

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
        #ifdef _SORT
            SortData data;
            data.index = index;
            data.zDistance = 0.f;
            data.zDistance = mul(perFrame.view, float4(ParticlePool[index].pos, 1.f)).z;
            aliveList.Append(data);
        #else
            aliveList.Append(index);
        #endif
        }

        //0 vert count per instance, 1 numInstances, 2 start vert loc, 3 start instance loc
    #ifdef _SORT
        uint nextPow2 = getNextPow2(numAliveParticles);
        uint twoExp = log2(nextPow2);
        sortIterationCounter[0] = nextPow2 / 2; //sort only needs to touch 
        sortIterationCounter[1] = (twoExp * (twoExp + 1)) / 2;
        sortIterationCounter[2] = 0;
        //4 = dispatchX, 5 = dispatchY, 6 = dispatchZ
        //todo unhardcode this
        drawArgs[4] = nextPow2 / (256 * 2);
        drawArgs[5] = 1;
        drawArgs[6] = 1;
    #endif

        drawArgs[1] = numAliveParticles;
    }
}
