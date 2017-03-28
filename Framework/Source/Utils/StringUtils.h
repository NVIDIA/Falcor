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
#include <string>
#include <algorithm>
#include <codecvt>

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    /** Check is a string starts with another string
    */
    inline bool hasPrefix(const std::string& str, const std::string& prefix, bool caseSensitive = true)
    {
        if(str.size() >= prefix.size())
        {
            if(caseSensitive == false)
            {
                std::string s = str;
                std::string pfx = prefix;
                std::transform(str.begin(), str.end(), s.begin(), ::tolower);
                std::transform(prefix.begin(), prefix.end(), pfx.begin(), ::tolower);
                return s.compare(0, pfx.length(), pfx) == 0;
            }
            else
            {
                return str.compare(0, prefix.length(), prefix) == 0;
            }
        }
        return false;
    }

    /** Check is a string ends with another string
    */
    inline bool hasSuffix(const std::string& str, const std::string& suffix, bool caseSensitive = true)
    {
        if(str.size() >= suffix.size())
        {
            std::string s = str.substr(str.length() - suffix.length());
            if(caseSensitive == false)
            {
                std::string sfx = suffix;
                std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                std::transform(sfx.begin(), sfx.end(), sfx.begin(), ::tolower);
                return (sfx == s);
            }
            else
            {
                return (s == suffix);
            }
        }
        return false;
    }

    /** Split a string into a vector of strings based on d delimiter        
    */
    inline std::vector<std::string> splitString(const std::string& str, const std::string& delim)
    {
        std::string s;
        std::vector<std::string> vec;
        for(char c : str)
        {
            if(delim.find(c) != std::string::npos)
            {
                if(s.length())
                {
                    vec.push_back(s);
                    s.clear();
                }
            }
            else
            {
                s += c;
            }
        }
        if(s.length())
        {
            vec.push_back(s);
        }
        return vec;
    }

    /** Remove leading whitespaces (space, tab, newline, carriage-return)
    */
    inline std::string removeLeadingWhitespaces(const std::string& str)
    {
        size_t offset = str.find_first_not_of(" \n\r\t");
        std::string ret;
        if(offset != std::string::npos)
        {
            ret = str.substr(offset);
        }
        return ret;
    }

    /** Remove trailing whitespaces (space, tab, newline, carriage-return)
    */
    inline std::string removeTrailingWhitespaces(const std::string& str)
    {
        size_t offset = str.find_last_not_of(" \n\r\t");
        std::string ret;
        if(offset != std::string::npos)
        {
            ret = str.substr(0, offset + 1);
        }
        return ret;
    }

    /** Remote trailing and leading whitespaces
    */
    inline std::string removeLeadingTrailingWhitespaces(const std::string& str)
    {
        return removeTrailingWhitespaces(removeLeadingWhitespaces(str));
    }

    /** Replace all occurrences of a substring in a string. The function doesn't change the original string
        \param Input The input string
        \param Src The substring to replace
        \param Dst The substring to replace Src with
    */
    inline std::string replaceSubstring(const std::string& input, const std::string& src, const std::string& dst)
    {
        std::string res = input;
        size_t offset = res.find(src);
        while(offset != std::string::npos)
        {
            res.replace(offset, src.length(), dst);
            offset += dst.length();
            offset = res.find(src, offset);
        }
        return res;
    }

    /** Remove the last array index from a string, if it exists
    */
    inline const std::string removeLastArrayIndex(const std::string& name)
    {
        size_t dot = name.find_last_of(".");
        size_t bracket = name.find_last_of("[");

        if(bracket != std::string::npos)
        {
            // Ignore cases where the last index is an array of struct index (SomeStruct[1].v should be ignored)
            if((dot == std::string::npos) || (bracket > dot))
            {
                return name.substr(0, bracket);
            }
        }

        return name;
    }

    /** convert string to wstring
    */
    inline std::wstring string_2_wstring(const std::string& s)
    {
        std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
        std::wstring ws = cvt.from_bytes(s);
        return ws;
    }

    inline std::string wstring_2_string(const std::wstring& ws)
    {
        std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
        std::string s = cvt.to_bytes(ws);
        return s;
    }
    /*! @} */
};