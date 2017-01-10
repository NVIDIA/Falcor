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
#include "TestBase.h"
#include <iostream>

TestBase::TestBase()
{}

void TestBase::init()
{
    //Turns off crash window 
    SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
    //Turn off debug assertion window
    _CrtSetReportMode(_CRT_ASSERT, 0);

    //Not needed right now, maybe later.
    //Whatever specific things the derived might want to init
    //InitDerived();
}

void TestBase::run()
{
    std::vector<TestResult> testResults = runTests();
    std::vector<std::string> xmlStrings;
    xmlStrings.resize(testResults.size());
    for (int32_t i = 0; i < testResults.size(); ++i)
    {
        mResultSummary.addTest(testResults[i]);
        xmlStrings[i] = XMLFromTestResult(testResults[i]);
    }

    GenerateXML(xmlStrings);
}

std::string TestBase::GetDerivedName()
{
    return mDerivedName;
}

std::vector<TestBase::TestResult> TestBase::runTests()
{
    return std::vector<TestResult>();
}

void TestBase::GenerateXML(const std::vector<std::string>& xmlStrings)
{
    std::ofstream of;
    
    of.open(mDerivedName + "_" + CONFIG_NAME + "_TestingLog.xml");
    of << "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n";
    of << "<TestLog>\n";
    of << "<Summary\n";
    of << "\tTotalTests=\"" + std::to_string(mResultSummary.total) + "\"\n";
    of << "\tPassedTests=\"" + std::to_string(mResultSummary.pass) + "\"\n";
    of << "\tFailedTests=\"" + std::to_string(mResultSummary.fail) + "\"\n";
    of << "/>\n";

    for (auto it = xmlStrings.begin(); it != xmlStrings.end(); ++it)
        of << *it;

    of << "</TestLog>";
    of.close();
}

std::string TestBase::XMLFromTestResult(const TestResult& r)
{
    std::string xml;
    xml += "<TestResult\n";
    xml += "\tTestName=\"" + r.functionInfo + "\"\n";
    xml += "\tPassed=\"" + std::to_string(r.passed) + "\"\n";
    xml += "\tErrorMessage=\"" + r.error + "\"\n";
    xml += "/>\n";
    return xml;
}