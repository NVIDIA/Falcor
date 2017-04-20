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
    // numPasses = (lg2(n) * (lg2(n) + 1)) / 2
    SortParams sortParams[_NUM_PASSES];
};

RWStructuredBuffer<SortData> sortList;
// [0], num particles touched each frame, [1] counter to -=1 for each sort pass and turn off shader
RWStructuredBuffer<uint> iterationCounter;
RWStructuredBuffer<uint> sortArgs;

void Swap(uint index, uint compareIndex)
{
    SortData temp = sortList[index];
    sortList[index] = sortList[compareIndex];
    sortList[compareIndex] = temp;
}

[numthreads(numThreads, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    InterlockedAdd(iterationCounter[2], 1);
    uint count = max(iterationCounter[2], 1) - 1;
    uint iterationIndex = count / iterationCounter[0];

    int threadIndex = (int)getParticleIndex(groupID.x, numThreads, groupIndex);
    if (threadIndex == 0)
    {
        iterationCounter[1] -= 1;
        if (iterationCounter[1] <= 0)
        {
            sortArgs[4] = 0;
            sortArgs[5] = 0;
            sortArgs[6] = 0;
        }
    }

    SortParams params = sortParams[iterationIndex];
    uint index = params.twoCompareDist * (threadIndex / params.compareDist) + threadIndex % params.compareDist;
    uint compareIndex = index + params.compareDist;

    uint descending = (index / params.setSize) % 2;
    if (descending)
    {
        //if this is less than other, not descending
        if (sortList[index].zDistance < sortList[compareIndex].zDistance)
        {
            Swap(index, compareIndex);
        }
    }
    else
    {
        //if this is greater than other, not ascending
        if (sortList[index].zDistance > sortList[compareIndex].zDistance)
        {
            Swap(index, compareIndex);
        }
    }
}
