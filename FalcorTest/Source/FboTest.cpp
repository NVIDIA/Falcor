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
#include "FboTest.h"
#include <stdio.h>

void FboTest::addTests()
{
    addTestToList<TestDefault>();
    addTestToList<TestCreate>();
    addTestToList<TestCaptureToFile>();
    //addTestToList<TestDepthStencilAttach>();
    //addTestToList<TestColorAttach>();
    addTestToList<TestZeroAttachment>();
    //addTestToList<TestGetWidthHeight>();
}

TESTING_FUNC(FboTest, TestDefault)
{
    Fbo::SharedPtr fbo = Fbo::getDefault();
    if (fbo->getApiHandle() == -1)
    {
        return TestBase::TestData(TestBase::TestResult::Pass, mName);
    }
    else
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName, 
            "Error creating default fbo, api handle is initialized and should not be.");
    }
}

TESTING_FUNC(FboTest, TestCreate)
{
    Fbo::SharedPtr fbo = Fbo::create();
    //This is exactly the same as getDefault. It passes true for initapihandle but that param is unused in Fbo::Fbo(bool)
    if (fbo->getApiHandle() == -1)
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName,
            "Error creating default fbo, api handle isn't initialized and should be.");
    }
    else
    {
        return TestBase::TestData(TestBase::TestResult::Pass, mName);
    }
}

TESTING_FUNC(FboTest, TestCaptureToFile)
{
    //this will fail, capture to file currently does nothing. This might need changing depending on the implementation
    Fbo::SharedPtr fbo = Fbo::create();
    std::string executableDir = getExecutableDirectory();
    std::ifstream inFile;
    for (uint32_t i = 0; i < fbo->getMaxColorTargetCount(); ++i)
    {
        std::string pngFile;
        findAvailableFilename("FboTest", executableDir, "png", pngFile);
        fbo->captureToFile(i, pngFile, Bitmap::FileFormat::PngFile);
        inFile.open(pngFile, std::ios::in);
        if (inFile.is_open() && !inFile.fail())
        {
            inFile.close();
            remove(pngFile.c_str());
        }
        else
        {
            return TestBase::TestData(TestBase::TestResult::Fail, mName, 
                "(The tested function is not implemented) Error opening created png: " + pngFile);
        }
    }

    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestDepthStencilAttach)
{
    //hmm what do?
    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestColorAttach)
{
    //what do
    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestZeroAttachment)
{
    //gonna depend on what im doing for attach
    return TestBase::TestData(TestBase::TestResult::Fail, mName, "This function is only implemented in GL");
}

TESTING_FUNC(FboTest, TestGetWidthHeight)
{
    Fbo::SharedPtr fbo = Fbo::create();
    const uint32_t numResolutions = 6;
    uint32_t widths[numResolutions] = { 1920u, 1440u, 1280u, 1600u, 1280u, 800u };
    uint32_t heights[numResolutions] = { 1080u, 900u, 800u, 1200u, 960u, 700u };
    for (size_t i = 0; i < numResolutions; i++)
    {
        //The code below gives access violation, i probably need to do some device init or something that sample is doing,
        //look at this later
        Texture::SharedPtr tex = Texture::create2D(widths[i], heights[i], ResourceFormat::RGBA32Float);
        for (uint32_t j = 0; j < fbo->getMaxColorTargetCount(); ++j)
        {
            fbo->attachColorTarget(tex, j);
            if (fbo->getWidth() != widths[i] || fbo->getHeight() != heights[i])
            {
                return TestBase::TestData(TestBase::TestResult::Fail, mName,
                    "Fbo dimensions do not match dimensions used to set");
            }
        }
    }

    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

int main()
{
    FboTest fbot;
    fbot.init();
    fbot.run();
    return 0;
}