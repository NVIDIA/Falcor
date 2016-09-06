/***************************************************************************
Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
***************************************************************************/
#pragma once
#include <windows.h>
#include <string>

namespace Falcor
{
    class Process
    {
    public:
        Process(const std::string& cmdLine);
        ~Process();
        uint32_t getRetVal() { flush(); return mRetVal; }

        void readStdOut(std::string& str);
        void readStdErr(std::string& str);
        void writeStdIn(std::string& str);

        void flush();
    private:
        HANDLE mStdInWr = nullptr;
        HANDLE mStdOutRd = nullptr;
        HANDLE mStdErrRd = nullptr;

        HANDLE mProcess = nullptr;

        uint32_t mRetVal = 0;
    };
}