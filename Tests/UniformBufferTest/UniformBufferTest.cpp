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
#include "UniformBufferTest.h"

template <bool shouldFail>
void UniformBufferTest::test(const std::string& var)
{
    Logger::showBoxOnError(false);
    auto res = mpBuffer->getVariableOffset(var);
    Logger::showBoxOnError(true);
    bool b = shouldFail ? (res == UniformBuffer::kInvalidUniformOffset) : (res != UniformBuffer::kInvalidUniformOffset);
    if(b == false)
    {
        Logger::log(Logger::Level::Error, "Test failed for variable " + var);
    }
}

#define success(a) test<false>(a);
#define fail(a) test<true>(a);

void UniformBufferTest::onLoad()
{
    // create a the program
    auto pProg = Program::createFromFile("UniformBufferTest.vs", "UniformBufferTest.fs");

    // create the uniform buffer
    mpBuffer = UniformBuffer::create(pProg->getActiveProgramVersion().get(), "CB");

    // No array
    success("V");
    fail("V[1]");
    fail("V[0]");

    // 1D
    success("V1");
    success("V1[5]");
    fail("V1[10]");
    fail("V1[13]");
    fail("V1[1][1]");

    // 2D
    fail("V2");
    success("V2[3]");
    success("V2[3][2]");
    fail("V2[6]");
    fail("V2[5]");
    fail("V2[1][7]");
    fail("V2[1][3]");
    fail("V2[0][0][0]");

    // Struct
    fail("S");
    success("S.F");
    fail("S.F[0]");
    success("S.F1");
    success("S.F1[5]");
    fail("S.F1[11]");
    fail("S.F1[0][0]");
    fail("S.F2");
    success("S.F2[2]");
    success("S.F2[0][0]");
    fail("S.F2[0][0][0]");

    // AoS 1D
    fail("S1");
    fail("S1[0]");
    success("S1[2].F");
    fail("S1[5].F");
    fail("S1[0].F[0]");

    success("S1[2].F1");
    success("S1[2].F1[4]");
    fail("S1[2].F1[10]");
    fail("S1[2].F1[0][0]");
        
    fail("S1[2].F2"); 
    success("S1[2].F2[2]");
    success("S1[2].F2[1][0]");
    fail("S1[5].F2[3]");
    fail("S1[2].F2[0][0][0]");
    fail("S1[2].F2[0][3]");

    // AoS 2D
    fail("S2");
    fail("S2[0]");
    fail("S2[0][0]");
    success("S2[1][2].F");
    fail("S2[1][2].F[9]");
    fail("S2[2][2].F");

    success("S2[1][2].F1");
    success("S2[1][2].F1[3]");
    fail("S2[1][2].F1[11]");
    fail("S2[1][2].F1[0][0]");

    fail("S2[1][2].F2");
    success("S2[1][2].F2[1]");
    success("S2[1][2].F2[1][1]");
    fail("S2[1][2].F2[1][2]");
    fail("S2[4][2].F2[1][0]");

    // ST2
    fail("st2");
    fail("st2[0][0]");
    success("st2[0][0].st[2][1].F2[2]");
    success("st2[0][0].st[2][1].F");
    success("st2[0][0].v3[1][1][1]");

    shutdownApp();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    UniformBufferTest UniformBufferTest;
    SampleConfig Config;
    UniformBufferTest.run(Config);
}
