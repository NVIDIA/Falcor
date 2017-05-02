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
#include <map>
#include <vector>
#include "API/ProgramVersion.h"

namespace Falcor
{
    class Shader;
    class RenderContext;

    /** High-level abstraction of a program class.
        This class manages different versions of the same program. Different versions means same shader files, different macro definitions. This allows simple usage in case different macros are required - for example static vs. animated models.
    */
    class Program : public std::enable_shared_from_this<Program>
    {
    public:
        using SharedPtr = std::shared_ptr<Program>;
        using SharedConstPtr = std::shared_ptr<const Program>;

        using DefineList = Shader::DefineList;

        virtual ~Program() = 0;

        /** Get the API handle of the active program
        */
        ProgramVersion::SharedConstPtr getActiveVersion() const;

        /** Adds a macro definition to the program. If the macro already exists, its will be replaced.

            \param[in] name The name of define. Must be valid
            \param[in] value Optional. The value of the define string
        */
        void addDefine(const std::string& name, const std::string& value = "");

        /** Remove a macro definition from the program. If the definition doesn't exist, the function call will be silently ignored.
            \param[in] name The name of define. Must be valid
        */
        void removeDefine(const std::string& name);

        /** Clear the macro definition list
        */
        void clearDefines() { mDefineList.clear(); }
    
        /** Get the macro definition string of the active program version
        */
        const DefineList& getActiveDefinesList() const { return mDefineList; }

        /** Reload and relink all programs.
        */
        static void reloadAllPrograms();

        /** update define list
        */
        void replaceAllDefines(const DefineList& dl) { mDefineList = dl; }
    protected:
        static const uint32_t kShaderCount = (uint32_t)ShaderType::Count;

        Program();
        void init(const std::string& vs, const std::string& fs, const std::string& gs, const std::string& hs, const std::string& ds, const DefineList& programDefines, bool createdFromFile);
        void init(const std::string& cs, const DefineList& programDefines, bool createdFromFile);

        bool link() const;
        virtual ProgramVersion::SharedPtr createProgramVersion(std::string& log) const;

        std::string mShaderStrings[kShaderCount]; // Either a filename or a string, depending on the value of mCreatedFromFile

        DefineList mDefineList;

        // We are doing lazy compilation, so these are mutable
        mutable bool mLinkRequired = true;
        mutable std::map<const DefineList, ProgramVersion::SharedConstPtr> mProgramVersions;
        mutable ProgramVersion::SharedConstPtr mpActiveProgram = nullptr;

        std::string getProgramDescString() const;
        static std::vector<Program*> sPrograms;

        bool mCreatedFromFile = false;
        using string_time_map = std::unordered_map<std::string, time_t>;
        mutable string_time_map mFileTimeMap;

        bool checkIfFilesChanged();
        void reset();
    };
}