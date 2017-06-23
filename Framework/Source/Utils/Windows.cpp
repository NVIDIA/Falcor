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
#include "Framework.h"
#include "Utils/OS.h"
#include <fstream>
#include <vector>
#include <stdint.h>
#include "Utils/StringUtils.h"
#include <Shlwapi.h>
#include <shlobj.h>
#include <sys/types.h>
#include "API/Window.h"
#include "psapi.h"

// Always run in Optimus mode on laptops
extern "C"
{
    _declspec(dllexport) DWORD NvOptimusEnablement = 1;
}

namespace Falcor
{
    MsgBoxButton msgBox(const std::string& msg, MsgBoxType mbType)
    {
        UINT Type = MB_OK;
        switch (mbType)
        {
        case MsgBoxType::Ok:
            Type = MB_OK;
            break;
        case MsgBoxType::OkCancel:
            Type = MB_OKCANCEL;
            break;
        case  MsgBoxType::RetryCancel:
            Type = MB_RETRYCANCEL;
            break;
        default:
            should_not_get_here();
            break;
        }

        int value = MessageBoxA(nullptr, msg.c_str(), "Falcor", Type | MB_TOPMOST);
        switch (value)
        {
        case IDOK:
            return MsgBoxButton::Ok;
        case IDCANCEL:
            return MsgBoxButton::Cancel;
        case IDRETRY:
            return MsgBoxButton::Retry;
        default:
            should_not_get_here();
            return MsgBoxButton::Cancel;
        }
    }

    bool doesFileExist(const std::string& filename)
    {
        DWORD attr = GetFileAttributesA(filename.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES);
    }

    bool isDirectoryExists(const std::string& filename)
    {
        DWORD attr = GetFileAttributesA(filename.c_str());
        return ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));
    }

    bool createDirectory(const std::string& path)
    {
        DWORD res = CreateDirectoryA(path.c_str(), NULL);

        return res == TRUE;
    }

    const std::string& getExecutableDirectory()
    {
        static std::string folder;
        if (folder.size() == 0)
        {
            CHAR exeName[MAX_PATH];
            GetModuleFileNameA(nullptr, exeName, ARRAYSIZE(exeName));
            const std::string tmp(exeName);

            auto last = tmp.find_last_of("/\\");
            folder = tmp.substr(0, last);
        }
        return folder;
    }

    const std::string getWorkingDirectory()
    {
        CHAR curDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, curDir);
        return std::string(curDir);
    }

    const std::string& getExecutableName()
    {
        static std::string filename;
        if (filename.size() == 0)
        {
            CHAR exeName[MAX_PATH];
            GetModuleFileNameA(nullptr, exeName, ARRAYSIZE(exeName));
            const std::string tmp(exeName);

            auto last = tmp.find_last_of("/\\");
            filename = tmp.substr(last + 1, std::string::npos);
        }
        return filename;
    }

    bool getEnvironemntVariable(const std::string& varName, std::string& value)
    {
        static char buff[4096];
        int numChar = GetEnvironmentVariableA(varName.c_str(), buff, arraysize(buff)); //what is the best way to deal with wchar ?
        assert(numChar < arraysize(buff));
        if (numChar == 0)
        {
            return false;
        }
        value = std::string(buff);
        return true;
    }

    std::vector<std::string> gDataDirectories =
    {
        // Ordering matters here, we want that while developing, resources will be loaded from the development media directory
        std::string(getWorkingDirectory()),
        std::string(getWorkingDirectory() + "\\data"),
        std::string(getExecutableDirectory()),
        std::string(getExecutableDirectory() + "\\data"),

        // The local solution media folder
        std::string(getExecutableDirectory() + "\\..\\..\\..\\Media"),
    };

    const std::vector<std::string>& getDataDirectoriesList()
    {
        return gDataDirectories;
    }

    void addDataDirectory(const std::string& dataDir)
    {
        //Insert unique elements
        if (std::find(gDataDirectories.begin(), gDataDirectories.end(), dataDir) == gDataDirectories.end())
        {
            gDataDirectories.push_back(dataDir);
        }
    }

    std::string canonicalizeFilename(const std::string& filename)
    {
        //  It might be tempting to try to figure out a nicer bound ourselves, but the documentation says "You must set the
        //  size of this buffer to MAX_PATH to ensure that it is large enough to hold the returned string.".
        char buffer[MAX_PATH];
        PathCanonicalizeA(buffer, filename.c_str());
        return replaceSubstring(buffer, "/", "\\");
    }

    bool findFileInDataDirectories(const std::string& filename, std::string& fullpath)
    {
        static bool bInit = false;
        if (bInit == false)
        {
            std::string dataDirs;
            if (getEnvironemntVariable("FALCOR_MEDIA_FOLDERS", dataDirs))
            {
                auto folders = splitString(dataDirs, ";");
                gDataDirectories.insert(gDataDirectories.end(), folders.begin(), folders.end());
            }
            bInit = true;
        }

        // Check if this is an absolute path
        if (doesFileExist(filename))
        {
            fullpath = canonicalizeFilename(filename);
            return true;
        }

        for (const auto& Dir : gDataDirectories)
        {
            fullpath = canonicalizeFilename(Dir + '\\' + filename);
            if (doesFileExist(fullpath))
            {
                return true;
            }
        }

        return false;
    }

    template<bool bOpen>
    static bool fileDialogCommon(const char* pFilters, std::string& filename)
    {
        OPENFILENAMEA ofn;
        CHAR chars[512] = "";
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = GetForegroundWindow();
        ofn.lpstrFilter = pFilters;
        ofn.lpstrFile = chars;
        ofn.nMaxFile = arraysize(chars);
        ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        if (bOpen == true)
        {
            ofn.Flags |= OFN_FILEMUSTEXIST;
        }
        ofn.lpstrDefExt = "";

        BOOL b = bOpen ? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn);
        if (b)
        {
            filename = std::string(chars);
            return true;
        }

        return false;
    }


    bool openFileDialog(const char* pFilters, std::string& filename)
    {
        return fileDialogCommon<true>(pFilters, filename);
    }

    bool saveFileDialog(const char* pFilters, std::string& filename)
    {
        return fileDialogCommon<false>(pFilters, filename);
    }

    bool readFileToString(const std::string& fullpath, std::string& str)
    {
        std::ifstream t(fullpath.c_str());
        if ((t.rdstate() & std::ifstream::failbit) == 0)
        {
            str = std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            return true;
        }
        return false;
    }

    bool findAvailableFilename(const std::string& prefix, const std::string& directory, const std::string& extension, std::string& filename)
    {
        for (UINT32 i = 0; i < UINT32_MAX; i++)
        {
            std::string newPrefix = prefix + '.' + std::to_string(i);
            filename = directory + '\\' + newPrefix + "." + extension;

            if (doesFileExist(filename) == false)
            {
                return true;
            }
        }
        should_not_get_here();
        filename = "";
        return false;
    }

    void setWindowIcon(const std::string& iconFile, Window::ApiHandle windowHandle)
    {
        std::string fullpath;
        if (findFileInDataDirectories(iconFile, fullpath))
        {
            HANDLE hIcon = LoadImageA(GetModuleHandle(NULL), fullpath.c_str(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
            HWND hWnd = windowHandle ? windowHandle : GetActiveWindow();
            SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
        else
        {
            logError("Error when loading icon. Can't find the file " + iconFile + ".");
        }
    }

    int getDisplayDpi()
    {
        ::SetProcessDPIAware();
        HDC screen = GetDC(NULL);
        double hPixelsPerInch = GetDeviceCaps(screen, LOGPIXELSX);
        double vPixelsPerInch = GetDeviceCaps(screen, LOGPIXELSY);
        ::ReleaseDC(NULL, screen);
        return int((hPixelsPerInch + vPixelsPerInch) * 0.5);
    }

    bool isDebuggerPresent()
    {
#ifdef _DEBUG
        return ::IsDebuggerPresent() == TRUE;
#else
        return false;
#endif
    }

    void printToDebugWindow(const std::string& s)
    {
        OutputDebugStringA(s.c_str());
    }

    void debugBreak()
    {
        __debugbreak();
    }

    std::string stripDataDirectories(const std::string& filename)
    {
        std::string stripped = filename;
        std::string canonFile = canonicalizeFilename(filename);
        for (const auto& dir : gDataDirectories)
        {
            std::string canonDir = canonicalizeFilename(dir);
            if (hasPrefix(canonFile, canonDir, false))
            {
                // canonicalizeFilename adds trailing \\ to drive letters and removes them from paths containing folders
                size_t len = canonDir.back() == '\\' ? canonDir.length() : canonDir.length() + 1;
                std::string tmp = canonFile.erase(0, len);
                if (tmp.length() < stripped.length())
                {
                    stripped = tmp;
                }
            }
        }

        return stripped;
    }

    std::string getDirectoryFromFile(const std::string& filename)
    {
        char *cstr = new char[filename.length() + 1];
        strcpy_s(cstr, filename.length() + 1, filename.c_str());

        PathRemoveFileSpecA(cstr);

        std::string ret = std::string(cstr);
        delete[] cstr;
        return ret;
    }

    std::string getFilenameFromPath(const std::string& filename)
    {
        char *cstr = new char[filename.length() + 1];
        strcpy_s(cstr, filename.length() + 1, filename.c_str());

        PathStripPathA(cstr);

        std::string ret = std::string(cstr);
        delete[] cstr;
        return ret;
    }

    std::string swapFileExtension(const std::string& str, const std::string& currentExtension, const std::string& newExtension)
    {
        if (hasSuffix(str, currentExtension))
        {
            std::string ret = str;
            return (ret.erase(ret.rfind(currentExtension)) + newExtension);
        }
        else
        {
            return str;
        }
    }

    void enumerateFiles(std::string searchString, std::vector<std::string>& filenames)
    {
        WIN32_FIND_DATAA ffd;
        HANDLE hFind = INVALID_HANDLE_VALUE;

        char szFile[512];
        strcpy_s(szFile, searchString.length() + 1, searchString.c_str());

        hFind = FindFirstFileA(szFile, &ffd);

        if (INVALID_HANDLE_VALUE == hFind)
        {
            return;
        }
        else
        {
            do
            {
                filenames.push_back(std::string(ffd.cFileName));
            } while (FindNextFileA(hFind, &ffd) != 0);
        }
    }

    std::thread::native_handle_type getCurrentThread()
    {
        return ::GetCurrentThread();
    }

    void setThreadAffinity(std::thread::native_handle_type thread, uint32_t affinityMask)
    {
        ::SetThreadAffinityMask(thread, affinityMask);
        if (DWORD dwError = GetLastError() != 0)
        {
            LPVOID lpMsgBuf;
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
            std::wstring err((LPTSTR)lpMsgBuf);
            logWarning("setThreadAffinity failed with error: " + std::string(err.begin(), err.end()));
            LocalFree(lpMsgBuf);
        }
    }

    void setThreadPriority(std::thread::native_handle_type thread, ThreadPriorityType priority)
    {
        if (priority >= ThreadPriorityType::Lowest)
            ::SetThreadPriority(thread, THREAD_BASE_PRIORITY_MIN + (int32_t)priority);
        else if (priority == ThreadPriorityType::BackgroundBegin)
            ::SetThreadPriority(thread, THREAD_MODE_BACKGROUND_BEGIN);
        else if (priority == ThreadPriorityType::BackgroundEnd)
            ::SetThreadPriority(thread, THREAD_MODE_BACKGROUND_END);
        else
            should_not_get_here();

        if (DWORD dwError = GetLastError() != 0)
        {
            LPVOID lpMsgBuf;
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
            std::wstring err((LPTSTR)lpMsgBuf);
            logWarning("setThreadPriority failed with error: " + std::string(err.begin(), err.end()));
            LocalFree(lpMsgBuf);
        }
    }

    time_t getFileModifiedTime(const std::string& filename)
    {
        struct stat s;
        if (stat(filename.c_str(), &s) != 0)
        {
            logError("Can't get file time for '" + filename + "'");
            return 0;
        }

        return s.st_mtime;
    }

    uint64_t getTotalVirtualMemory()
    {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;

        return totalVirtualMem;
    }

    uint64_t getUsedVirtualMemory()
    {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
        DWORDLONG virtualMemUsed = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;

        return virtualMemUsed;
    }

    uint64_t getProcessUsedVirtualMemory()
    {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;

        return virtualMemUsedByMe;
    }


}