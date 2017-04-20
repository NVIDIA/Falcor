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
#include "Falcor.h"

#define init_tests() if (mArgList.argExists("test")) { toggleText(false); initializeTestingArgs(mArgList); }
#define run_test() if(isTestingEnabled()) { runTestTask(frameRate(), this); }

namespace Falcor
{
    class SampleTest
    {
    public:
        /** Checks whether testing is enabled, returns true if Test Tasks vector isn't empty
        */
        bool isTestingEnabled() const;

        /** Initializes Test Tasks vector based on command line
        \param args ArgList object
        */
        void initializeTestingArgs(const ArgList& args);

        /** Callback for anything the testing sample wants to initialize
        \param args the arg list object
        */
        virtual void onInitializeTestingArgs(const ArgList& args) {};

        /** Checks each frame against testing ranges to see if a testing task should be run and runs it
        \param frameRate the frame rate object to get frame times and frame count
        */
        void runTestTask(const FrameRate& frameRate, Sample* pSample);

        /** Callback for anything the testing sample wants to do each frame
        \param frameRate the frame rate object to get frame count
        */
        virtual void onRunTestTask(const FrameRate& frameRate) {};

        /**
        */
        virtual void onTestShutdown() { exit(1); }

    private:
        /** Captures screen to a png, very similar to Sample::captureScreen
        */
        void captureScreen();

        /** Outputs xml test results file
        */
        void outputXML();

        enum class TestTaskType
        {
            LoadTime,
            MeasureFps,
            ScreenCapture,
            Shutdown,
            Uninitialized
        };

        struct Task
        {
            Task() : mStartFrame(0u), mEndFrame(0u), mTask(TestTaskType::Uninitialized), mResult(0.f) {}
            Task(uint32_t startFrame, uint32_t endFrame, TestTaskType t) :
                mStartFrame(startFrame), mEndFrame(endFrame), mTask(t), mResult(0.f) {}
            bool operator<(const Task& rhs) { return mStartFrame < rhs.mStartFrame; }

            uint32_t mStartFrame;
            uint32_t mEndFrame;
            TestTaskType mTask;
            float mResult = 0;
        };

        std::vector<Task> mTestTasks;
        std::vector<Task>::iterator mTestTaskIt;

        struct TimedTask
        {
            TimedTask() : mStartTime(0.f), mTask(TestTaskType::Uninitialized) {};
            TimedTask(float startTime, TestTaskType t) : mStartTime(startTime), mTask(t) {};
            bool operator<(const TimedTask& rhs) { return mStartTime < rhs.mStartTime; }

            float mStartTime;
            TestTaskType mTask;
        };

        std::vector<TimedTask> mTimedTestTasks;
        std::vector<TimedTask>::iterator mTimedTestTaskIt;
    };
}