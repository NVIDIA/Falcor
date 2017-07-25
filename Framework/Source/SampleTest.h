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

namespace Falcor
{
    /** Test framework layer for Falcor.
        Runs tests based on config files passed in by command line arguments
    */
    class SampleTest : public Sample
    {
    public:
        /** Checks whether testing is enabled, returns true if either Test Task vector isn't empty
        */
        bool hasTests() const;

        /** Initializes Test Task vectors based on command line arguments
        */
        void initializeTesting();

        /** Callback for anything the testing sample wants to initialize
        */
        virtual void onInitializeTesting() {};

        /** Testing actions that need to happen before the frame renders
        */
        void beginTestFrame();

        /** Callback for anything the testing sample wants to do before the frame renders
        */
        virtual void onBeginTestFrame() {};

        /** Testing actions that need to happen after the frame renders
        */
        void endTestFrame();

        /** Callback for anything the testing sample wants to do after the frame renders
        */
        virtual void onEndTestFrame() {};

        /** Callback for anything the testing sample wants to do right before shutdown
        */
        virtual void onTestShutdown() {};

    protected:

        /** Different ways test tasks can be triggered
        */
        enum class TriggerType
        {
            Frame,  ///< Triggered by frame count
            Time,   ///< Triggered by time since application launch
            None,   ///< No tests will be run
        };
        TriggerType mCurrentTrigger = TriggerType::None;

        /** Types of operations used in testing
        */
        enum class TaskType
        {
            MemoryCheck,    ///< Records the application's current memory usage
            LoadTime,       ///< 
            MeasureFps,     ///< Records the current FPS
            ScreenCapture,  ///< Captures a screenshot
            Shutdown,       ///< Close the application
            Uninitialized   ///< Default value. All tasks must be initialized on startup
        };

        struct Task
        {
            Task() : mStartFrame(0u), mEndFrame(0u), mTask(TaskType::Uninitialized), mResult(0.f) {}
            Task(uint32_t startFrame, uint32_t endFrame, TaskType t) :
                mStartFrame(startFrame), mEndFrame(endFrame), mTask(t), mResult(0.f) {}
            bool operator<(const Task& rhs) { return mStartFrame < rhs.mStartFrame; }

            uint32_t mStartFrame;
            uint32_t mEndFrame;
            float mResult = 0;
            TaskType mTask;
        };

        std::vector<Task> mTestTasks;
        std::vector<Task>::iterator mCurrentFrameTest;

        struct TimedTask
        {
            TimedTask() : mStartTime(0.f), mTask(TaskType::Uninitialized), mStartFrame(0) {};
            TimedTask(float startTime, float endTime, TaskType t) : mStartTime(startTime), mEndTime(endTime), mTask(t), mStartFrame(0) {};
            bool operator<(const TimedTask& rhs) { return mStartTime < rhs.mStartTime; }

            float mStartTime;
            float mEndTime;
            float mResult = 0;
            //used to calc avg fps in a perf range
            uint mStartFrame = 0;
            TaskType mTask;
        };

        std::vector<TimedTask> mTimedTestTasks;
        std::vector<TimedTask>::iterator mCurrentTimeTest;

        /** Outputs xml test results file
        */
        void outputXML();

        /** Initializes tests that start based on frame number
        */
        void initFrameTests();

        /** Initializes tests that start based on time
        */
        void initTimeTests();

        /** Run tests that start based on frame number
        */
        void runFrameTests();

        /** Run tests that start based on time
        */
        void runTimeTests();

        // The Memory Check for one point.
        struct MemoryCheck
        {
            float time;
            float effectiveTime;
            uint64_t frame;
            uint64_t totalVirtualMemory;
            uint64_t totalUsedVirtualMemory;
            uint64_t currentlyUsedVirtualMemory;
        };

        // The Memory Check Between Frames.
        struct MemoryCheckRange
        {
            bool active = false;
            MemoryCheck startCheck;
            MemoryCheck endCheck;
        };

        // The List of Memory Check Ranges.
        MemoryCheckRange mMemoryFrameCheckRange;
        MemoryCheckRange mMemoryTimeCheckRange;

        /** Capture the Current Memory and write it to the provided memory check.
        */
        void getMemoryStatistics(MemoryCheck& memoryCheck);

        /** Write the Memory Check Range in terms of Time to a file. Outputs Difference, Start and End Frames.
        */
        void writeMemoryRange(const MemoryCheckRange& memoryCheckRange, bool frameTest = true);

        /** Capture the Memory Snapshot.
        */
        void captureMemory(uint64_t frameCount, float currentTime, bool frameTest = true, bool endRange = false);

    };
}