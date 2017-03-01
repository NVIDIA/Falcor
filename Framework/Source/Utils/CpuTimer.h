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
#define NOMINMAX
#include <windows.h>

namespace Falcor
{
    /** Provides timer utilities to the application
    */
    class CpuTimer
    {
    public:
        CpuTimer()
        {
            if(!sFrequency)
            {
                LARGE_INTEGER li;
                QueryPerformanceFrequency(&li);
                sFrequency = double(li.QuadPart);
            }
        }

        using TimePoint = uint64_t;//std::chrono::time_point < std::chrono::high_resolution_clock >;

        /** Returns the current time
        */
        static TimePoint getCurrentTimePoint()
        {
            LARGE_INTEGER li;
            QueryPerformanceCounter(&li);
            return li.QuadPart;
        }

        /** Update the timer.
            \return  The TimePoint of the last update() call.
                This value is meaningless on it's own. Call CCpuTimer#CalcDuration() to get the duration that passed between 2 TimePoints
        */
        TimePoint update()
        {
            auto now = getCurrentTimePoint();
            uint64_t delta = (now - mCurrentTime);
            mElpasedTime = float(double(delta) / sFrequency);
            mCurrentTime = now;
            return mCurrentTime;
        }

        /** Get the time that passed from the last update() call to the one before that.
        */
        float getElapsedTime() const
        {
            return mElpasedTime;
        }

        /** Calculate the duration in milliseconds between 2 time points
        */
        static float calcDuration(TimePoint start, TimePoint end)
        {
            uint64_t delta = end - start;
            double ms = (double)delta / sFrequency;
            return float(ms) * 1000;
        }

    private:
        TimePoint mCurrentTime;
        static double sFrequency;
        float mElpasedTime;
    };
}