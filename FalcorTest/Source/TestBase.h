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
#define REGISTER_NAME mDerivedName = std::string(typeid(*this).name()).substr(6, std::string::npos);
#define FUNCTION_INFO std::string(__FUNCTION__) + "(Line " + std::to_string(__LINE__) + ")"

class TestBase
{
public:
    struct TestResult
    {
        TestResult() : passed(false) {}
        TestResult(bool pass, std::string fxInfo) : passed(pass), functionInfo(fxInfo) {}
        TestResult(bool pass, std::string fxInfo, std::string err) : 
            passed(pass), functionInfo(fxInfo), error(err) {}

        bool passed;
        std::string functionInfo;
        std::string error;
    };

    void init();
    void run();
    std::string GetDerivedName();
    //needed? 
    //virtual ~TestBase();

protected:
    TestBase();
    virtual std::vector<TestResult> runTests();

    //I dont need this right now, but this is something might be needed in future
    //Maybe not anymore with different exes, there will be no container of testbase
    //virtual void InitDerived();

    //Is this even necessary anymore? used to generate filename i guess
    std::string mDerivedName; 
    std::vector<TestResult> mpTestResults;

private: 
    class ResultSummary
    {
    public:
        ResultSummary() : total(0), pass(0), fail(0) {}
        void addTest(TestResult r) { ++total; r.passed ? ++pass : ++fail; }
        uint32_t total;
        uint32_t pass;
        uint32_t fail;
    };

    void GenerateXML(const std::vector<std::string>& xmlStrings);

    std::string XMLFromTestResult(const TestResult& r);
    ResultSummary mResultSummary;
};
