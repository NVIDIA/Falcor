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
#include <vector> 
#include <memory> //unique ptr
#include <string>
#include "Falcor.h"

using namespace Falcor;

//.name() gives Class X, substr is to remove the class prefix and just get classname
#define REGISTER_NAME mpDerivedName = std::string(typeid(*this).name()).substr(6, std::string::npos);
#define ERROR_PREFIX "[" + std::string(__FUNCTION__) + "(Line " + std::to_string(__LINE__) + ")]: "

class TestBase
{
public:
    using UniquePtr = std::unique_ptr<TestBase>;

    struct TestResult
    {
        TestResult() : passed(false), errorDesc() {}
        TestResult(bool pass) : passed(pass), errorDesc() {}
        TestResult(bool pass, std::string err) : passed(pass), errorDesc(err) {}

        bool passed;
        std::string errorDesc;
    };

    virtual std::vector<TestResult> runTests();

    std::string GetDerivedName();
    //virtual ~TestBase();
protected:
    TestBase();
    //I don't love this, it isn't great. But I'm not sure how else to allow 
    //running only tests on particular classes
    std::string mpDerivedName; 
private: 
};
