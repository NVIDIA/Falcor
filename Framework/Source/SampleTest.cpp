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
#include "SampleTest.h"

namespace Falcor
{

    bool SampleTest::isTestingEnabled() const
    {
        return !mTestTasks.empty() || !mTimedTestTasks.empty();
    }

    void SampleTest::initializeTestingArgs(const ArgList& args)
    {
        //Frame based tests
        //Load time
        if (args.argExists("loadtime"))
        {
            Task newTask(2u, 3u, TestTaskType::LoadTime);
            mTestTasks.push_back(newTask);
        }

        //shutdown
        std::vector<ArgList::Arg> shutdownFrame = args.getValues("shutdown");
        if (!shutdownFrame.empty())
        {
            uint32_t startFame = shutdownFrame[0].asUint();
            Task newTask(startFame, startFame + 1, TestTaskType::Shutdown);
            mTestTasks.push_back(newTask);
        }

        //screenshot frames
        std::vector<ArgList::Arg> ssFrames = args.getValues("ssframes");
        for (uint32_t i = 0; i < ssFrames.size(); ++i)
        {
            uint32_t startFrame = ssFrames[i].asUint();
            Task newTask(startFrame, startFrame + 1, TestTaskType::ScreenCapture);
            mTestTasks.push_back(newTask);
        }

        //fps capture frames
        std::vector<ArgList::Arg> fpsRange = args.getValues("perfframes");
        //integer division on purpose, only care about ranges with start and end
        size_t numRanges = fpsRange.size() / 2;
        for (size_t i = 0; i < numRanges; ++i)
        {
            uint32_t rangeStart = fpsRange[2 * i].asUint();
            uint32_t rangeEnd = fpsRange[2 * i + 1].asUint();
            //only add if valid range
            if (rangeEnd > rangeStart)
            {
                Task newTask(rangeStart, rangeEnd, TestTaskType::MeasureFps);
                mTestTasks.push_back(newTask);
            }
            else
            {
                continue;
            }
        }

        if (!mTestTasks.empty())
        {
            //Put the tasks in start frame order
            std::sort(mTestTasks.begin(), mTestTasks.end());
            //ensure no task ranges overlap
            auto previousIt = mTestTasks.begin();
            for (auto it = mTestTasks.begin() + 1; it != mTestTasks.end(); ++it)
            {
                if (it->mStartFrame < previousIt->mEndFrame)
                {
                    logInfo("Test Range from frames " + std::to_string(it->mStartFrame) + " to " + std::to_string(it->mEndFrame) +
                        " overlaps existing range from " + std::to_string(previousIt->mStartFrame) + " to " + std::to_string(previousIt->mEndFrame));
                    it = mTestTasks.erase(it);
                    --it;
                }
                else
                {
                    previousIt = it;
                }
            }
        }

        mTestTaskIt = mTestTasks.begin();

        //Time based tests
        std::vector<ArgList::Arg> timedScreenshots = args.getValues("sstimes");
        for (auto it = timedScreenshots.begin(); it != timedScreenshots.end(); ++it)
        {
            float startTime = it->asFloat();
            TimedTask newTask(startTime, TestTaskType::ScreenCapture);
            mTimedTestTasks.push_back(newTask);
        }

        std::vector<ArgList::Arg> shutdownTimeArg = args.getValues("shutdowntime");
        if (!shutdownTimeArg.empty())
        {
            float shutdownTime = shutdownTimeArg[0].asFloat();
            TimedTask newTask(shutdownTime, TestTaskType::Shutdown);
            mTimedTestTasks.push_back(newTask);
        }

        if (!mTimedTestTasks.empty())
        {
            std::sort(mTimedTestTasks.begin(), mTimedTestTasks.end());
        }

        mTimedTestTaskIt = mTimedTestTasks.begin();

        //callback
        onInitializeTestingArgs(args);
    }

    void SampleTest::runTestTask(const FrameRate& frameRate, Sample* pSample)
    {
        uint32_t frameId = frameRate.getFrameCount();
        if (mTestTaskIt != mTestTasks.end() && frameId >= mTestTaskIt->mStartFrame)
        {
            if (frameId == mTestTaskIt->mEndFrame)
            {
                if (mTestTaskIt->mTask == TestTaskType::MeasureFps)
                {
                    mTestTaskIt->mResult /= (mTestTaskIt->mEndFrame - mTestTaskIt->mStartFrame);
                }

                ++mTestTaskIt;
            }
            else
            {
                switch (mTestTaskIt->mTask)
                {
                case TestTaskType::LoadTime:
                case TestTaskType::MeasureFps:
                    mTestTaskIt->mResult += frameRate.getLastFrameTime();
                    break;
                case TestTaskType::ScreenCapture:
                    captureScreen();
                    break;
                case TestTaskType::Shutdown:
                    outputXML();
                    onTestShutdown();
                    break;
                default:
                    should_not_get_here();
                }
            }
        }

        if (mTimedTestTaskIt != mTimedTestTasks.end() && pSample->mCurrentTime > mTimedTestTaskIt->mStartTime)
        {
            TestTaskType curType = mTimedTestTaskIt->mTask;

            switch (curType)
            {
            case TestTaskType::ScreenCapture:
                //Makes sure the ss is taken at the correct time for image compare
                if (pSample->mFreezeTime)
                {
                    captureScreen();
                    pSample->mFreezeTime = false;
                    break;
                }
                else
                {
                    pSample->mCurrentTime = mTimedTestTaskIt->mStartTime;
                    pSample->mFreezeTime = true;
                    return;
                }
            case TestTaskType::Shutdown:
                outputXML();
                onTestShutdown();
                break;
            default:
                should_not_get_here();
            }

            ++mTimedTestTaskIt;
        }

        onRunTestTask(frameRate);
    }

    void SampleTest::captureScreen()
    {
        std::string filename = getExecutableName();

        // Now we have a folder and a filename, look for an available filename (we don't overwrite existing files)
        std::string prefix = std::string(filename);
        std::string executableDir = getExecutableDirectory();
        std::string pngFile;
        if (findAvailableFilename(prefix, executableDir, "png", pngFile))
        {
            Texture::SharedPtr pTexture = gpDevice->getSwapChainFbo()->getColorTexture(0);
            pTexture->captureToFile(0, 0, pngFile);
        }
        else
        {
            logError("Could not find available filename when capturing screen");
        }
    }

    void SampleTest::outputXML()
    {
        if (!mTestTasks.empty())
        {
            float frameTime = 0.f;
            float loadTime = 0.f;
            uint32_t numFpsRanges = 0;
            uint32_t numScreenshots = 0;
            for (auto it = mTestTasks.begin(); it != mTestTasks.end(); ++it)
            {
                switch (it->mTask)
                {
                case TestTaskType::LoadTime:
                    loadTime = it->mResult;
                    break;
                case TestTaskType::MeasureFps:
                {
                    frameTime += it->mResult;
                    ++numFpsRanges;
                    break;
                }
                case TestTaskType::ScreenCapture:
                    ++numScreenshots;
                    break;
                case TestTaskType::Shutdown:
                    continue;
                default:
                    should_not_get_here();
                }
            }

            //average all performance ranges if there are any
            numFpsRanges ? frameTime /= numFpsRanges : frameTime = 0;

            std::ofstream of;
            std::string exeName = getExecutableName();
            //strip off .exe
            std::string shortName = exeName.substr(0, exeName.size() - 4);
            of.open(shortName + "_TestingLog_0.xml");
            of << "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n";
            of << "<TestLog>\n";
            of << "<Summary\n";
            of << "\tLoadTime=\"" << std::to_string(loadTime) << "\"\n";
            of << "\tFrameTime=\"" << std::to_string(frameTime) << "\"\n";
            of << "\tNumScreenshots=\"" << std::to_string(numScreenshots) << "\"\n";
            of << "/>\n";
            of << "</TestLog>";
            of.close();
        }
    }
}