/***************************************************************************
Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
***************************************************************************/
#include "Framework.h"
#include "Process.h"

namespace Falcor
{
    void createPipeHandles(HANDLE& rd, HANDLE& wr)
    {
        SECURITY_ATTRIBUTES secAttr;
        secAttr.bInheritHandle = TRUE;
        secAttr.lpSecurityDescriptor = NULL;

        if(!CreatePipe(&rd, &wr, &secAttr, 0))
        {
            assert(0);
        }

        if(!SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0))
        {
            assert(0);
        }
    }

    void readPipeData(HANDLE pipe, std::string& str)
    {
        for(;;)
        {
            char data[1024] = {0};
            DWORD bytesRead;
            auto success = ReadFile(pipe, data, arraysize(data), &bytesRead, NULL);
            if(!success || bytesRead == 0) 
            {
                break;
            }
            str += data;
        }
    }

    Process::~Process()
    {
        CloseHandle(mStdErrRd);
        CloseHandle(mStdInWr);
        CloseHandle(mStdOutRd);
    }

    Process::Process(const std::string& cmdLine)
    {
        PROCESS_INFORMATION procInfo;
        STARTUPINFOA startInfo;
        BOOL success = FALSE;
        ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

        ZeroMemory(&startInfo, sizeof(STARTUPINFO));
        startInfo.cb = sizeof(STARTUPINFO);

        // Create the pipes
        HANDLE stdOutWr, stdInRd, stdErrWr;
        createPipeHandles(mStdOutRd, stdOutWr);
        createPipeHandles(stdInRd, mStdInWr);
        createPipeHandles(mStdErrRd, stdErrWr);

        startInfo.hStdOutput = stdOutWr;
        startInfo.hStdInput = stdInRd;
        startInfo.hStdError = stdErrWr;

        startInfo.dwFlags |= STARTF_USESTDHANDLES;

        success = CreateProcessA(NULL,
            const_cast<char*>(cmdLine.c_str()),     // command line 
            NULL,          // process security attributes 
            NULL,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            CREATE_NO_WINDOW, // creation flags 
            NULL,          // use parent's environment 
            NULL,          // use parent's current directory 
            &startInfo,  // STARTUPINFO pointer 
            &procInfo);  // receives PROCESS_INFORMATION 

        assert(success);

        mProcess = procInfo.hProcess;
        CloseHandle(procInfo.hThread);
        CloseHandle(stdOutWr);
        CloseHandle(stdInRd);
        CloseHandle(stdErrWr);
    }

    void Process::flush()
    {
        if(mProcess)
        {
            WaitForSingleObject(mProcess, INFINITE);
            DWORD retVal;
            BOOL success = GetExitCodeProcess(mProcess, &retVal);
            assert(success);
            mRetVal = (uint32_t)retVal;
            CloseHandle(mProcess);
            mProcess = nullptr;
        }
    }

    void Process::readStdErr(std::string& str)
    {
        flush();
        readPipeData(mStdErrRd, str);
    }

    void Process::readStdOut(std::string& str)
    {
        flush();
        readPipeData(mStdOutRd, str);
    }

    void Process::writeStdIn(std::string& str)
    {
        if(str.size())
        {
            DWORD bytesWritten;
            auto success = WriteFile(mStdInWr, str.c_str(), (uint32_t)str.size(), &bytesWritten, NULL);
            assert(success && (bytesWritten == str.size()));
        }
    }
}