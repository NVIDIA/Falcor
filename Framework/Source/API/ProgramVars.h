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
#include "API/Texture.h"
#include "API/ConstantBuffer.h"
#include "API/Sampler.h"
#include <unordered_map>
#include "API/ProgramReflection.h"
#include "API/ShaderStorageBuffer.h"
#include "API/LowLevel/RootSignature.h"

namespace Falcor
{
    class ProgramVersion;
    class RenderContext;

    /** This class manages a program's reflection and variable assignment.
        It's a high-level abstraction of variables-related concepts such as CBs, texture and sampler assignments, root-signature, descriptor tables, etc.
    */
    class ProgramVars : public std::enable_shared_from_this<ProgramVars>
    {
    public:
        class SharedPtr : public std::shared_ptr<ProgramVars>
        {
        public:
            SharedPtr() : std::shared_ptr<ProgramVars>() {}
            SharedPtr(ProgramVars* pProgVars) : std::shared_ptr<ProgramVars>(pProgVars) {}
            ConstantBuffer::SharedPtr operator[](const std::string& cbName) { return get()->getConstantBuffer(cbName); }
            ConstantBuffer::SharedPtr operator[](uint32_t index) { return get()->getConstantBuffer(index); }
        };

        using SharedConstPtr = std::shared_ptr<const ProgramVars>;

        /** Create a new object
            \param[in] pReflector A program reflection object containing the requested declarations
            \param[in] createBuffers If true, will create the ConstantBuffer objects. Otherwise, the user will have to bind the CBs himself
            \param[in] pRootSignature A root-signature describing how to bind resources into the shader. If this paramter is nullptr, a root-signature object will be created from the program reflection object
        */
        static SharedPtr create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers = true, const RootSignature::SharedConstPtr& pRootSig = nullptr);

        /** Bind a constant buffer object by name.
            If the name doesn't exists or the CBs size doesn't match the required size, the call will fail.
            If a buffer was previously bound it will be released.
            \param[in] name The name of the constant buffer in the program
            \param[in] pCB The constant buffer object
            \return false is the call failed, otherwise true
        */
        bool attachConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB);

        /** Bind a constant buffer object by index.
            If the no CB exists in the specified index or the CB size doesn't match the required size, the call will fail.
            If a buffer was previously bound it will be released.
            \param[in] name The name of the constant buffer in the program
            \param[in] pCB The constant buffer object
            \return false is the call failed, otherwise true
        */
        bool attachConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB);

        /** Get a constant buffer object.
            \param[in] name The name of the buffer
            \return If the name is valid, a shared pointer to the CB. Otherwise returns nullptr
        */
        ConstantBuffer::SharedPtr getConstantBuffer(const std::string& name) const;

        /** Get a constant buffer object.
        \param[in] index The index of the buffer
        \return If the index is valid, a shared pointer to the buffer. Otherwise returns nullptr
        */
        ConstantBuffer::SharedPtr getConstantBuffer(uint32_t index) const;

        /** Get a shader-storage buffer object.
        \param[in] name The name of the buffer
        \return If the name is valid, a shared pointer to the SSBO. Otherwise returns nullptr
        */
        ShaderStorageBuffer::SharedPtr getShaderStorageBuffer(const std::string& name) const;

        /** Get a shader-storage buffer object.
        \param[in] index The index of the buffer
        \return If the index is valid, a shared pointer to the SSBO. Otherwise returns nullptr
        */
        ShaderStorageBuffer::SharedPtr getShaderStorageBuffer(uint32_t index) const;

        /** Bind a texture to the program in the global namespace.
            If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
            \param[in] name The name of the texture object in the shader
            \param[in] pTexture The texture object to bind
            \param[in] firstArraySlice The first array slice to bind
            \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will bind the range [firstArraySlice, pTexture->getArraySize()]
            \param[in] mostDetailedMip The most detailed-mip level
            \param[in] mipCount The number of mip-levels to bind. If this is equal to Texture#kMaxPossible, will bind the range [mostDetailedMip, pTexture->getMipCount()]
            \return false if the texture was not found in the program, otherwise true
        */
        bool setTexture(const std::string& name, const Texture::SharedConstPtr& pTexture, uint32_t firstArraySlice = 0, uint32_t arraySize = Texture::kMaxPossible, uint32_t mostDetailedMip = 0, uint32_t mipCount = Texture::kMaxPossible);

        /** Bind a texture as a UAV
            \param[in] index The index of the UAV object in the shader
            \param[in] pTexture The texture object to bind
            \param[in] mipLevel The requested mip-level
            \param[in] firstArraySlice The first array slice to bind
            \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will bind the range [firstArraySlice, pTexture->getArraySize()]
            \return false if the UAV was not found in the program, otherwise true
        */
        bool setUav(uint32_t index, const Texture::SharedConstPtr& pTexture, uint32_t mipLevel = 0, uint32_t firstArraySlice = 0, uint32_t arraySize = Texture::kMaxPossible);

        /** Bind a texture as a UAV
            \param[in] name The name of the UAV object in the shader
            \param[in] pTexture The texture object to bind
            \param[in] mipLevel The requested mip-level
            \param[in] firstArraySlice The first array slice to bind
            \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will bind the range [firstArraySlice, pTexture->getArraySize()]
            \return false if the UAV was not found in the program, otherwise true
        */
        bool setUav(const std::string& name, const Texture::SharedConstPtr& pTexture, uint32_t mipLevel = 0, uint32_t firstArraySlice = 0, uint32_t arraySize = Texture::kMaxPossible);

        /** Bind an array of texture to the program in the global namespace.
        This can be used to bind a texture declared an array or a number of different variables which are known to be continues in the register space (such as for structure fields)
        If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
        \param[in] name The name of the first texture object in the shader.
        \param[in] pTextures An array of textures to bind
        \return false if any of the textures was not found in the program, otherwise true
        */
        bool setTextureRange(const std::string& name, uint32_t count, const Texture::SharedConstPtr pTextures[]);

        /** Bind a sampler to the program in the global namespace.
        If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
        \param[in] name The name of the sampler object in the shader
        \param[in] pSampler The sampler object to bind
        \return false if the sampler was not found in the program, otherwise true
        */
        bool setSampler(const std::string& name, const Sampler::SharedConstPtr& pSampler);

        /** Bind a texture to the program in the global namespace.
        If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
        \param[in] index The index of the texture object in the shader
        \param[in] firstArraySlice The first array slice to bind
        \param[in] arraySize The array size. If this is equal to Texture#kMaxPossible, will bind the range [firstArraySlice, pTexture->getArraySize()]
        \param[in] mostDetailedMip The most detailed-mip level
        \param[in] mipCount The number of mip-levels to bind. If this is equal to Texture#kMaxPossible, will bind the range [mostDetailedMip, pTexture->getMipCount()]
        \return false if the texture was not found in the program, otherwise true
        */
        bool setTexture(uint32_t index, const Texture::SharedConstPtr& pTexture, uint32_t firstArraySlice = 0, uint32_t arraySize = Texture::kMaxPossible, uint32_t mostDetailedMip = 0, uint32_t mipCount = Texture::kMaxPossible);

        /** Bind an array of texture to the program in the global namespace.
        This can be used to bind a texture declared an array or a number of different variables which are known to be continues in the register space (such as for structure fields)
        If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
        \param[in] startIndex The index of the first texture object in the shader
        \param[in] pTextures An array of textures to bind
        \return false if any of the textures was not found in the program, otherwise true
        */
        bool setTextureRange(uint32_t startIndex, uint32_t count, const Texture::SharedConstPtr pTextures[]);

        /** Bind a sampler to the program in the global namespace.
            If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
            \param[in] name The name of the sampler object in the shader
            \param[in] pSampler The sampler object to bind
            \return false if the sampler was not found in the program, otherwise true
            */
        bool setSampler(uint32_t index, const Sampler::SharedConstPtr& pSampler);
        
        /** Get the program reflection interface
        */
        ProgramReflection::SharedConstPtr getReflection() const { return mpReflector; }

        /** Get the root signature object
        */
        RootSignature::SharedConstPtr getRootSignature() const { return mpRootSignature; }

        template<typename T>
        struct ResourceData
        {
            T pResource;
            uint32_t firstArraySlice = 0;
            uint32_t arraySize = -1;
            uint32_t mostDetailedMip = 0;
            uint32_t mipCount = -1;
            uint32_t rootSigOffset = 0;
        };

        // FIXME: This doesn't work with multiple register spaces
        template<typename T> using ResourceDataMap = std::unordered_map<uint32_t, ResourceData<T>>;

        void setIntoRenderContext(RenderContext* pContext) const;
    private:
        ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig);

        RootSignature::SharedConstPtr mpRootSignature;
        ProgramReflection::SharedConstPtr mpReflector;
        ResourceDataMap<ConstantBuffer::SharedPtr> mConstantBuffers;
        ResourceDataMap<ShaderStorageBuffer::SharedPtr> mSSBO;

        std::map<uint32_t, ResourceData<Texture::SharedConstPtr>> mAssignedSrvs;
        std::map<uint32_t, ResourceData<Texture::SharedConstPtr>> mAssignedUavs;
        std::map<uint32_t, ResourceData<Sampler::SharedConstPtr>> mAssignedSamplers;
    };
}