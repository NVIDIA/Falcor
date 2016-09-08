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
#include "Core/UniformBuffer.h"
#include "Core/Sampler.h"
#include <unordered_map>
#include <Core/ProgramReflection.h>

namespace Falcor
{
    class ProgramVersion;

    /** This class manages a program's reflection and variable assignment.
        It's a high-level abstraction of variables-related concepts such as UBOs, texture and sampler assignments, root-signature, descriptor tables, etc.
    */
    class ProgramVars : public std::enable_shared_from_this<ProgramVars>
    {
    public:
        class SharedPtr : public std::shared_ptr<ProgramVars>
        {
        public:
            SharedPtr() : std::shared_ptr<ProgramVars>() {}
            SharedPtr(ProgramVars* pProgVars) : std::shared_ptr<ProgramVars>(pProgVars) {}
            UniformBuffer::SharedPtr operator[](const std::string& uboName) { return get()->getUniformBuffer(uboName); }
            UniformBuffer::SharedPtr operator[](uint32_t index) { return get()->getUniformBuffer(index); }
        };

        using SharedConstPtr = std::shared_ptr<const ProgramVars>;

        /** Create a new object
            \param[in] pProgramVer The program containing the variable definitions
            \param[in] createBuffers If true, will create the UniformBuffer objects. Otherwise, the user will have to bind the UBOs himself
        */
        static SharedPtr create(const ProgramVersion* pProgramVer, bool createBuffers = true);

        /** Bind a uniform buffer object by name.
            If the name doesn't exists or the UBOs size doesn't match the required size, the call will fail with the appropriate error code.
            If a buffer was previously bound it will be released.
            \param[in] name The name of the uniform buffer in the program
            \param[in] pUbo The uniform buffer object
            \return error code.
        */
        ErrorCode bindUniformBuffer(const std::string& name, const UniformBuffer::SharedPtr& pUbo);

        /** Bind a uniform buffer object by index.
            If the no UBO exists in the specified index or the UBO size doesn't match the required size, the call will fail with the appropriate error code.
            If a buffer was previously bound it will be released.
            \param[in] name The name of the uniform buffer in the program
            \param[in] pUbo The uniform buffer object
            \return error code.
        */
        ErrorCode bindUniformBuffer(uint32_t index, const UniformBuffer::SharedPtr& pUbo);

        /** Get a uniform buffer object.
            \param[in] name The name of the buffer
            \return If the name is valid, a shared pointer to the UBO. Otherwise returns nullptr
        */
        UniformBuffer::SharedPtr getUniformBuffer(const std::string& name) const;

        /** Get a uniform buffer object.
        \param[in] index The index of the buffer
        \return If the index is valid, a shared pointer to the UBO. Otherwise returns nullptr
        */
        UniformBuffer::SharedPtr getUniformBuffer(uint32_t index) const;

        /** Bind a texture to the program in the global namespace.
            If you are using bindless textures, than this is not the right call for you. You should use the UniformBuffer::setTexture() method instead.
            \param[in] name The name of the texture object in the shader
            \param[in] pTexture The texture object to bind
            \return Error code. This actually depends on the build type. In release build we do minimal validation. In debug build there's more extensive verification the texture object matches the shader declaration.
        */
        ErrorCode setTexture(const std::string& name, const Texture::SharedPtr& pTexture);

        /** Bind a sampler to the program in the global namespace.
        If you are using bindless textures, than this is not the right call for you. You should use the UniformBuffer::setTexture() method instead.
        \param[in] name The name of the sampler object in the shader
        \param[in] pSampler The sampler object to bind
        \return Error code. The function fails if the name doesn't exists
        */
        ErrorCode setSampler(const std::string& name, const Sampler::SharedPtr& pSampler);

        /** Bind a texture to the program in the global namespace.
        If you are using bindless textures, than this is not the right call for you. You should use the UniformBuffer::setTexture() method instead.
        \param[in] index The index of the texture object in the shader
        \param[in] pTexture The texture object to bind
        \return Error code. This actually depends on the build type. In release build we do minimal validation. In debug build there's more extensive verification the texture object matches the shader declaration.
        */
        ErrorCode setTexture(uint32_t index, const Texture::SharedPtr& pTexture);

        /** Bind a sampler to the program in the global namespace.
            If you are using bindless textures, than this is not the right call for you. You should use the UniformBuffer::setTexture() method instead.
            \param[in] name The name of the sampler object in the shader
            \param[in] pSampler The sampler object to bind
            \return Error code. The function fails if the specified slot is not used
        */
        ErrorCode setSampler(uint32_t index, const Sampler::SharedPtr& pSampler);

        /** Set all uniform-buffers into the render-context
        */
        void setIntoContext(RenderContext* pContext) const;

        /** Get the program reflection interface
        */
        ProgramReflection::SharedPtr getProgramReflection() const { return mpReflection; }

    private:
        ProgramVars(const ProgramVersion* pProgramVer, bool createBuffers);
        ProgramReflection::SharedPtr mpReflection;
        std::unordered_map<uint32_t, UniformBuffer::SharedPtr> mUniformBuffers;
    };
}