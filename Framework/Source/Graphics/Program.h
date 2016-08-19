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
#include "Framework.h"
#include <string>
#include <map>
#include <vector>
#include "Core/ProgramVersion.h"

namespace Falcor
{
    class Texture;
    class Sampler;
    class UniformBuffer;
    class Shader;

    /** High-level abstraction of a program class.
        This class manages different versions of the same program. Different versions means same shader files, different macro definitions. This allows simple usage in case different macros are required - for example static vs. animated models.
    */
    class Program : public std::enable_shared_from_this<Program>
    {
    public:
        using SharedPtr = std::shared_ptr<Program>;
        using SharedConstPtr = std::shared_ptr<const Program>;

        /** create a new program object.
            \param[in] vertexFile Vertex shader filename.
            \param[in] fragmentFile Fragment shader filename.
            \param[in] shaderDefines A string of macro definitions to set into the shaders. Definitions are separated by a newline characters. The macro definition will be assigned to all the shaders.
            \return A new object, or nullptr if creation failed.
        */
        static SharedPtr createFromFile(const std::string& vertexFile, const std::string& fragmentFile, const std::string& shaderDefines = "");

        /** create a new program object.
        \param[in] vertexFile Vertex shader string.
        \param[in] fragmentFile Fragment shader string.
        \param[in] shaderDefines A string of macro definitions to set into the shaders. Definitions are separated by a newline characters. The macro definition will be assigned to all the shaders.
        \return A new object, or nullptr if creation failed.
        */
        static SharedPtr createFromString(const std::string& vertexShader, const std::string& fragmentShader, const std::string& shaderDefines = "");

        /** create a new program object.
            \param[in] vertexFile Vertex shader filename.
            \param[in] fragmentFile Fragment shader filename.
            \param[in] geometryFile Geometry shader filename.
            \param[in] hullFile Hull shader filename.
            \param[in] domainFile Domain shader filename.
            \param[in] shaderDefines A string of macro definitions to set into the shaders. Definitions are separated by a newline characters. The macro definition will be assigned to all the shaders.
            \return A new object, or nullptr if creation failed.
        */
        static SharedPtr createFromFile(const std::string& vertexFile, const std::string& fragmentFile, const std::string& geometryFile, const std::string& hullFile, const std::string& domainFile, const std::string& shaderDefines = "");

        /** create a new program object.
        \param[in] vertexShader Vertex shader string.
        \param[in] fragmentShader Fragment shader string.
        \param[in] geometryShader Geometry shader string.
        \param[in] hullShader Hull shader string.
        \param[in] domainShader Domain shader string.
        \param[in] shaderDefines A string of macro definitions to set into the shaders. Definitions are separated by a newline characters. The macro definition will be assigned to all the shaders.
        \return A new object, or nullptr if creation failed.
        */
        static SharedPtr createFromString(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::string& hullShader, const std::string& domainShader, const std::string& shaderDefines = "");

        ~Program();
        /** Get a shader object associated with this program
            \param[in] Type The Type of the shader object to fetch.
            \return The requested shader object, or nullptr if the shader doesn't exist.
        */
        const Shader* getShader(ShaderType type) const;

        /** Get the API handle of the active program
        */
        ProgramVersion::SharedConstPtr getActiveProgramVersion() const { return mpActiveProgram; }

        /** Sets the active program defines. This might cause the creation of a new API program object.
            \param[in] Defines A string of macro definitions to set into the shaders. Definitions are separated by a newline characters. The macro definition will be assigned to all the shaders.
        */
        bool setActiveProgramDefines(const std::string defines);

        /** Get the location of an input attribute for the active program version. Note that different versions might return different locations.
            \param[in] Attribute The attribute name in the program
            \return The index of the attribute if it is found, otherwise CApiProgram#InvalidLocation
        */
        int32_t getAttributeLocation(const std::string& attribute) const;

        /** Get the location of a uniform buffer for the active program version. Note that different versions might return different locations.
            \param[in] Attribute The attribute name in the program
            \return The index of the attribute if it is found, otherwise CApiProgram#InvalidLocation
        */
        uint32_t getUniformBufferBinding(const std::string& name) const;

        /** Get the macro definition string of the active program version
        */
        const std::string& getActiveDefines() const { return mActiveDefineString; }

        /** Reload and relink all programs.
        */
        static void reloadAllPrograms();
    private:
        static const uint32_t kShaderCount = (uint32_t)ShaderType::Count;

        Program();
        static SharedPtr createInternal(const std::string& vs, const std::string& fs, const std::string& gs, const std::string& hs, const std::string& ds, const std::string& shaderDefines, bool isFromFile);
        ProgramVersion::SharedConstPtr link(const std::string& defines);
        std::string mShaderStrings[kShaderCount]; // Either a filename or a string, depending on the value of mCreatedFromFile

        std::map<const std::string, ProgramVersion::SharedConstPtr> mProgramVersions;
        ProgramVersion::SharedConstPtr mpActiveProgram = nullptr;
        std::string mActiveDefineString;

        std::string getProgramDescString() const;
        static std::vector<Program*> sPrograms;

        bool mCreatedFromFile = false;
    };
}