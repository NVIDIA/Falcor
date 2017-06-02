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

    bool SampleTest::hasTests() const
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
        if (!hasTests()) return;

        uint32_t frameId = frameRate().getFrameCount();
        //Check if it's time for a time based task
        if (mCurrentTimeTest != mTimedTestTasks.end() && mCurrentTime >= mCurrentTimeTest->mStartTime)
        {
            if (mCurrentTimeTest->mTask == TaskType::ScreenCapture)
            {
                //disable text, the fps text will cause image compare failures
                toggleText(false);
                //Set the current time to make the screen capture results deterministic 
                mCurrentTime = mCurrentTimeTest->mStartTime;
            }
            else if (mCurrentTimeTest->mTask == TaskType::MeasureFps)
            {
                //mark the start frame. Required for time based perf ranges to know the 
                //amount of frames that passed in the time range to calculate the avg frame time
                //across the perf range
                if (mCurrentTimeTest->mStartFrame == 0)
                {
                    mCurrentTimeTest->mStartFrame = frameRate().getFrameCount();
                }
            }

            mCurrentTrigger = TriggerType::Time;
        }
        //Check if it's time for a frame based task
        else if (mCurrentFrameTest != mTestTasks.end() && frameId >= mCurrentFrameTest->mStartFrame)
        {
            if (mCurrentFrameTest->mTask == TaskType::ScreenCapture)
            {                
                //disable text, the fps text will cause image compare failures
                toggleText(false);
            }
			else if (frameId == mCurrentFrameTest->mStartFrame && mCurrentFrameTest->mTask == TaskType::MemoryCheck)
			{
				//	
				for (uint32_t i = 0; i < memoryCheckRanges.size(); i++)
				{
					if (mCurrentFrameTest->mStartFrame == memoryCheckRanges[i].startCheck.pFrame)
					{
						captureMemory(memoryCheckRanges[i].startCheck);
					}
					else if(mCurrentFrameTest->mStartFrame == memoryCheckRanges[i].endCheck.pFrame)
					{
						captureMemory(memoryCheckRanges[i].endCheck);
						writeMemoryRange(memoryCheckRanges[i]);
					}
				}

				mCurrentTrigger = TriggerType::Frame;
			}

            mCurrentTrigger = TriggerType::Frame;
        }
        else
        {
            //No test tasks this frame
            mCurrentTrigger = TriggerType::None;
        }

        onBeginTestFrame();
    }

    void SampleTest::endTestFrame()
    {
        if (!hasTests()) return;

        //Begin frame checks against the test tasks and returns a trigger type based
        //on the testing this frame, which is passed into this function
        if (mCurrentTrigger == TriggerType::Frame)
        {
            runFrameTests();
        }
        else if (mCurrentTrigger == TriggerType::Time)
        {
            runTimeTests();
        }

        onEndTestFrame();
    }

    void SampleTest::outputXML()
    {
        //only output a file if there was actually testing
        if (hasTests())
        {
            float frameTime = 0.f;
            float loadTime = 0.f;
            uint32_t numFpsRanges = 0;
            uint32_t numScreenshots = 0;
			uint32_t numMemCheck = 0;

            //frame based tests
            for (auto it = mTestTasks.begin(); it != mTestTasks.end(); ++it)
            {
                switch (it->mTask)
                {
				case TaskType::MemoryCheck:
					numMemCheck++;
					break;
                case TaskType::LoadTime:
                    loadTime = it->mResult;
                    break;
                case TaskType::MeasureFps:
                {
                    frameTime += it->mResult;
                    ++numFpsRanges;
                    break;
                }
                case TaskType::ScreenCapture:
                    ++numScreenshots;
                    break;
                case TaskType::Shutdown:
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
                case TaskType::ScreenCapture:
                    ++numScreenshots;
                    break;
                case TaskType::MeasureFps:
                    frameTime += it->mResult;
                    ++numFpsRanges;
                    break;
                case TaskType::LoadTime:
                case TaskType::Shutdown:
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
			of << "\tNumMemoryChecks=\"" << std::to_string(numMemCheck) << "\"\n";
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
            Task newTask(2u, 3u, TaskType::LoadTime);
            mTestTasks.push_back(newTask);
        }

        //shutdown
        std::vector<ArgList::Arg> shutdownFrame = mArgList.getValues("shutdown");
        if (!shutdownFrame.empty())
        {
            uint32_t startFame = shutdownFrame[0].asUint();
            Task newTask(startFame, startFame + 1, TaskType::Shutdown);
            mTestTasks.push_back(newTask);
        }

		std::vector<ArgList::Arg> mCheckFrames = mArgList.getValues("memCheck");
		for (uint32_t i = 0; i < mCheckFrames.size(); ++i)
		{
			std::vector<std::string> frames = splitString(mCheckFrames[i].asString(), "-");

			if (frames.size() > 2)
			{
				logWarning("Bad Frame Range : " + mCheckFrames[i].asString() + " Memory Check Ignored.");
			}
			if (std::stoul(frames[0]) >= std::stoul(frames[1]))
			{
				logWarning("Bad Frame Range : " + mCheckFrames[i].asString() + " Memory Check Ignored.");
			}

			Task startCheckTask(std::stoul(frames[0]), std::stoul(frames[0]), TaskType::MemoryCheck);
			mTestTasks.push_back(startCheckTask);

			Task endCheckTask(std::stoul(frames[1]), std::stoul(frames[1]), TaskType::MemoryCheck);
			mTestTasks.push_back(endCheckTask);

			MemoryCheckRange newCheckRange;
			newCheckRange.startCheck.pFrame = std::stoul(frames[0]);
			newCheckRange.endCheck.pFrame = std::stoul(frames[1]);

			memoryCheckRanges.push_back(newCheckRange);
		}

        //screenshot frames
        std::vector<ArgList::Arg> ssFrames = mArgList.getValues("ssframes");
        for (uint32_t i = 0; i < ssFrames.size(); ++i)
        {
            uint32_t startFrame = ssFrames[i].asUint();
            Task newTask(startFrame, startFrame + 1, TaskType::ScreenCapture);
            mTestTasks.push_back(newTask);
        }

        //fps capture frames
        std::vector<ArgList::Arg> fpsRange = mArgList.getValues("perfframes");
        //integer division on purpose, only care about ranges with start and end
        size_t numRanges = fpsRange.size() / 2;
        if (fpsRange.size() % 2 != 0)
        {
            logInfo(std::to_string(fpsRange.size()) + " values were provided for perfframes. " +
                "Perfframes expects an even number of values, as each pair of values represents a start and end of a testing range." +
                "The final odd value out will be ignored.");
        }
        for (size_t i = 0; i < numRanges; ++i)
        {
            uint32_t rangeStart = fpsRange[2 * i].asUint();
            uint32_t rangeEnd = fpsRange[2 * i + 1].asUint();
            //only add if valid range
            if (rangeEnd > rangeStart)
            {
                Task newTask(rangeStart, rangeEnd, TaskType::MeasureFps);
                mTestTasks.push_back(newTask);
            }
            else
            {
                logInfo("Test Range from frames " + std::to_string(rangeStart) + " to " + std::to_string(rangeEnd) +
                    " is invalid. End must be greater than start");
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
        mCurrentFrameTest = mTestTasks.begin();
    }

    void SampleTest::initTimeTests()
    {
        //screenshots
        std::vector<ArgList::Arg> timedScreenshots = mArgList.getValues("sstimes");
        for (auto it = timedScreenshots.begin(); it != timedScreenshots.end(); ++it)
        {
            float startTime = it->asFloat();
            TimedTask newTask(startTime, startTime + 1, TaskType::ScreenCapture);
            mTimedTestTasks.push_back(newTask);
        }

        //fps capture times
        std::vector<ArgList::Arg> fpsTimeRange = mArgList.getValues("perftimes");
        //integer division on purpose, only care about ranges with start and end
        size_t numTimedRanges = fpsTimeRange.size() / 2;
        if (fpsTimeRange.size() % 2 != 0)
        {
            logInfo(std::to_string(fpsTimeRange.size()) + " values were provided for perftimes. " +
            "Perftimes expects an even number of values, as each pair of values represents a start and end of a testing range." + 
            "The final odd value out will be ignored.");
        }

        for (size_t i = 0; i < numTimedRanges; ++i)
        {
            float rangeStart = fpsTimeRange[2 * i].asFloat();
            float rangeEnd = fpsTimeRange[2 * i + 1].asFloat();
            //only add if valid range
            if (rangeEnd > rangeStart)
            {
                TimedTask newTask(rangeStart, rangeEnd, TaskType::MeasureFps);
                mTimedTestTasks.push_back(newTask);
            }
            else
            {
                logInfo("Test Range from frames " + std::to_string(rangeStart) + " to " + std::to_string(rangeEnd) +
                    " is invalid. End must be greater than start");
                continue;
            }
        }

        //Shutdown
        std::vector<ArgList::Arg> shutdownTimeArg = mArgList.getValues("shutdowntime");
        if (!shutdownTimeArg.empty())
        {
            float shutdownTime = shutdownTimeArg[0].asFloat();
            TimedTask newTask(shutdownTime, shutdownTime + 1, TaskType::Shutdown);
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
        mCurrentTimeTest = mTimedTestTasks.begin();
    }

    void SampleTest::runFrameTests()
    {
        if (frameRate().getFrameCount() == mCurrentFrameTest->mEndFrame)
        {
            if (mCurrentFrameTest->mTask == TaskType::MeasureFps)
            {
                mCurrentFrameTest->mResult /= (mCurrentFrameTest->mEndFrame - mCurrentFrameTest->mStartFrame);
            }
            ++mCurrentFrameTest;
        }
        else
        {
            switch (mCurrentFrameTest->mTask)
            {
			case TaskType::MemoryCheck:
				break;
            case TaskType::LoadTime:
            case TaskType::MeasureFps:
                mCurrentFrameTest->mResult += frameRate().getLastFrameTime();
                break;
            case TaskType::ScreenCapture:
                captureScreen();
                //re-enable text
                toggleText(true);
                break;
            case TaskType::Shutdown:
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
        switch (mCurrentTimeTest->mTask)
        {
        case TaskType::ScreenCapture:
        {
            captureScreen();
            toggleText(true);
            ++mCurrentTimeTest;
            break;
        }
        case TaskType::MeasureFps:
        {
            if (mCurrentTime >= mCurrentTimeTest->mEndTime)
            {
                mCurrentTimeTest->mResult /= (frameRate().getFrameCount() - mCurrentTimeTest->mStartFrame);
                ++mCurrentTimeTest;
            }
            else
            {
                mCurrentTimeTest->mResult += frameRate().getLastFrameTime();
            }
            break;
        }
        case TaskType::Shutdown:
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
