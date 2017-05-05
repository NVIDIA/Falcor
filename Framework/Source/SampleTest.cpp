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

    void SampleTest::initializeTesting()
    {
        if (mArgList.argExists("test"))
        {
            initFrameTests();
            initTimeTests();
            onInitializeTesting();
        }
    }

    void SampleTest::beginTestFrame()
    {
        uint32_t frameId = mFrameRate.getFrameCount();
        //Check if it's time for a time based task
        if (mTimedTestTaskIt != mTimedTestTasks.end() && mCurrentTime >= mTimedTestTaskIt->mStartTime)
        {
            if (mTimedTestTaskIt->mTask == TestTaskType::ScreenCapture)
            {
                //disable text, the fps text will cause image compare failures
                toggleText(false);
                //Set the current time to make the screen capture results deterministic 
                mCurrentTime = mTimedTestTaskIt->mStartTime;
            }
            else if (mTimedTestTaskIt->mTask == TestTaskType::MeasureFps)
            {
                //mark the start frame. Required for time based perf ranges to know the 
                //amount of frames that passed in the time range to calculate the avg frame time
                //across the perf range
                if (mTimedTestTaskIt->mStartFrame == 0)
                {
                    mTimedTestTaskIt->mStartFrame = mFrameRate.getFrameCount();
                }
            }

            mCurrentTest = TestTriggerType::Time;
        }
        //Check if it's time for a frame based task
        else if (mTestTaskIt != mTestTasks.end() && frameId >= mTestTaskIt->mStartFrame)
        {
            if (mTestTaskIt->mTask == TestTaskType::ScreenCapture)
            {                
                //disable text, the fps text will cause image compare failures
                toggleText(false);
            }

            mCurrentTest = TestTriggerType::Frame;
        }
        else
        {
            //No test tasks this frame
            mCurrentTest = TestTriggerType::None;
        }

        onBeginTestFrame();
    }

    void SampleTest::endTestFrame()
    {
        //Begin frame checks against the test tasks and returns a trigger type based
        //on the testing this frame, which is passed into this function
        if (mCurrentTest == TestTriggerType::Frame)
        {
            runFrameTests();
        }
        else if (mCurrentTest == TestTriggerType::Time)
        {
            runTimeTests();
        }

        onEndTestFrame();
    }

    void SampleTest::outputXML()
    {
        //only output a file if there was actually testing
        if (isTestingEnabled())
        {
            float frameTime = 0.f;
            float loadTime = 0.f;
            uint32_t numFpsRanges = 0;
            uint32_t numScreenshots = 0;
            //frame based tests
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

            //time based tests
            for (auto it = mTimedTestTasks.begin(); it != mTimedTestTasks.end(); ++it)
            {
                switch (it->mTask)
                {
                case TestTaskType::ScreenCapture:
                    ++numScreenshots;
                    break;
                case TestTaskType::MeasureFps:
                    frameTime += it->mResult;
                    ++numFpsRanges;
                    break;
                case TestTaskType::LoadTime:
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

    void SampleTest::initFrameTests()
    {
        //Load time
        if (mArgList.argExists("loadtime"))
        {
            Task newTask(2u, 3u, TestTaskType::LoadTime);
            mTestTasks.push_back(newTask);
        }

        //shutdown
        std::vector<ArgList::Arg> shutdownFrame = mArgList.getValues("shutdown");
        if (!shutdownFrame.empty())
        {
            uint32_t startFame = shutdownFrame[0].asUint();
            Task newTask(startFame, startFame + 1, TestTaskType::Shutdown);
            mTestTasks.push_back(newTask);
        }

        //screenshot frames
        std::vector<ArgList::Arg> ssFrames = mArgList.getValues("ssframes");
        for (uint32_t i = 0; i < ssFrames.size(); ++i)
        {
            uint32_t startFrame = ssFrames[i].asUint();
            Task newTask(startFrame, startFrame + 1, TestTaskType::ScreenCapture);
            mTestTasks.push_back(newTask);
        }

        //fps capture frames
        std::vector<ArgList::Arg> fpsRange = mArgList.getValues("perfframes");
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

        //If there are tests, sort them and fix any overalpping ranges
        if (!mTestTasks.empty())
        {
            //Put the tasks in start frame order
            std::sort(mTestTasks.begin(), mTestTasks.end());
            //ensure no task ranges overlap
            auto previousIt = mTestTasks.begin();
            for (auto it = mTestTasks.begin() + 1; it != mTestTasks.end(); ++it)
            {
                //if overlap, log it and remove the overlapping test task
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
    }

    void SampleTest::initTimeTests()
    {
        //screenshots
        std::vector<ArgList::Arg> timedScreenshots = mArgList.getValues("sstimes");
        for (auto it = timedScreenshots.begin(); it != timedScreenshots.end(); ++it)
        {
            float startTime = it->asFloat();
            TimedTask newTask(startTime, startTime + 1, TestTaskType::ScreenCapture);
            mTimedTestTasks.push_back(newTask);
        }

        //fps capture times
        std::vector<ArgList::Arg> fpsTimeRange = mArgList.getValues("perftimes");
        //integer division on purpose, only care about ranges with start and end
        size_t numTimedRanges = fpsTimeRange.size() / 2;
        for (size_t i = 0; i < numTimedRanges; ++i)
        {
            float rangeStart = fpsTimeRange[2 * i].asFloat();
            float rangeEnd = fpsTimeRange[2 * i + 1].asFloat();
            //only add if valid range
            if (rangeEnd > rangeStart)
            {
                TimedTask newTask(rangeStart, rangeEnd, TestTaskType::MeasureFps);
                mTimedTestTasks.push_back(newTask);
            }
            else
            {
                continue;
            }
        }

        //Shutdown
        std::vector<ArgList::Arg> shutdownTimeArg = mArgList.getValues("shutdowntime");
        if (!shutdownTimeArg.empty())
        {
            float shutdownTime = shutdownTimeArg[0].asFloat();
            TimedTask newTask(shutdownTime, shutdownTime + 1, TestTaskType::Shutdown);
            mTimedTestTasks.push_back(newTask);
        }

        //Sort and make sure no times overlap
        if (!mTimedTestTasks.empty())
        {
            //Put the tasks in start time order
            std::sort(mTimedTestTasks.begin(), mTimedTestTasks.end());
            //ensure no task ranges overlap
            auto previousIt = mTimedTestTasks.begin();
            for (auto it = mTimedTestTasks.begin() + 1; it != mTimedTestTasks.end(); ++it)
            {
                //if overlap, log it and remove the overlapping test task
                if (it->mStartTime < previousIt->mEndTime)
                {
                    logInfo("Test Range from time " + std::to_string(it->mStartTime) + " to " + std::to_string(it->mEndTime) +
                        " overlaps existing range from " + std::to_string(previousIt->mStartTime) + " to " + std::to_string(previousIt->mEndTime));
                    it = mTimedTestTasks.erase(it);
                    --it;
                }
                else
                {
                    previousIt = it;
                }
            }
        }
        mTimedTestTaskIt = mTimedTestTasks.begin();
    }

    void SampleTest::runFrameTests()
    {
        if (mFrameRate.getFrameCount() == mTestTaskIt->mEndFrame)
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
                mTestTaskIt->mResult += mFrameRate.getLastFrameTime();
                break;
            case TestTaskType::ScreenCapture:
                captureScreen();
                //re-enable text
                toggleText(true);
                break;
            case TestTaskType::Shutdown:
                outputXML();
                onTestShutdown();
                shutdownApp();
                break;
            default:
                should_not_get_here();
            }
        }
    }

    void SampleTest::runTimeTests()
    {
        switch (mTimedTestTaskIt->mTask)
        {
        case TestTaskType::ScreenCapture:
        {
            captureScreen();
            toggleText(true);
            ++mTimedTestTaskIt;
            break;
        }
        case TestTaskType::MeasureFps:
        {
            if (mCurrentTime >= mTimedTestTaskIt->mEndTime)
            {
                mTimedTestTaskIt->mResult /= (mFrameRate.getFrameCount() - mTimedTestTaskIt->mStartFrame);
                ++mTimedTestTaskIt;
            }
            else
            {
                mTimedTestTaskIt->mResult += mFrameRate.getLastFrameTime();
            }
            break;
        }
        case TestTaskType::Shutdown:
        {
            outputXML();
            onTestShutdown();
            shutdownApp();
            break;
        }
        default:
            should_not_get_here();
        }
    }
}
