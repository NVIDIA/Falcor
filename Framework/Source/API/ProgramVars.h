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
#include "API/StructuredBuffer.h"
#include "API/TypedBuffer.h"
#include "API/LowLevel/RootSignature.h"

namespace Falcor
{
    class ProgramVersion;
    class CopyContext;

    /** This class manages a program's reflection and variable assignment.
        It's a high-level abstraction of variables-related concepts such as CBs, texture and sampler assignments, root-signature, descriptor tables, etc.
    */
    class ProgramVars
    {
    public:
        template<typename T>
        class SharedPtrT : public std::shared_ptr<T>
        {
        public:
            SharedPtrT() : std::shared_ptr<T>() {}
            SharedPtrT(T* pProgVars) : std::shared_ptr<T>(pProgVars) {}
            ConstantBuffer::SharedPtr operator[](const std::string& cbName) { return get()->getConstantBuffer(cbName); }
            ConstantBuffer::SharedPtr operator[](uint32_t index) { return get()->getConstantBuffer(index); }
        };

        /** Bind a constant buffer object by name.
            If the name doesn't exists or the CBs size doesn't match the required size, the call will fail.
            If a buffer was previously bound it will be released.
            \param[in] name The name of the constant buffer in the program
            \param[in] pCB The constant buffer object
            \return false is the call failed, otherwise true
        */
        bool setConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB);

        /** Bind a constant buffer object by index.
            If the no CB exists in the specified index or the CB size doesn't match the required size, the call will fail.
            If a buffer was previously bound it will be released.
            \param[in] name The name of the constant buffer in the program
            \param[in] pCB The constant buffer object
            \return false is the call failed, otherwise true
        */
        bool setConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB);

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

        /** Set a raw-buffer. Based on the shader reflection, it will be bound as either an SRV or a UAV
            \param[in] name The name of the buffer
            \param[in] pBuf The buffer object
        */
        bool setRawBuffer(const std::string& name, Buffer::SharedPtr pBuf);
        
        /** Set a typed buffer. Based on the shader reflection, it will be bound as either an SRV or a UAV
            \param[in] name The name of the buffer
            \param[in] pBuf The buffer object
        */
        bool setTypedBuffer(const std::string& name, TypedBufferBase::SharedPtr pBuf);

        /** Set a structured buffer. Based on the shader reflection, it will be bound as either an SRV or a UAV
            \param[in] name The name of the buffer
            \param[in] pBuf The buffer object
        */
        bool setStructuredBuffer(const std::string& name, StructuredBuffer::SharedPtr pBuf);

        /** Get a raw-buffer object.
            \param[in] name The name of the buffer
            \return If the name is valid, a shared pointer to the buffer object. Otherwise returns nullptr
        */
        Buffer::SharedPtr getRawBuffer(const std::string& name) const;

        /** Get a typed buffer object.
            \param[in] name The name of the buffer
            \return If the name is valid, a shared pointer to the buffer object. Otherwise returns nullptr
        */
        TypedBufferBase::SharedPtr getTypedBuffer(const std::string& name) const;
        
        /** Get a structured buffer object.
            \param[in] name The name of the buffer
            \return If the name is valid, a shared pointer to the buffer object. Otherwise returns nullptr
        */
        StructuredBuffer::SharedPtr getStructuredBuffer(const std::string& name) const;

        /** Bind a texture. Based on the shader reflection, it will be bound as either an SRV or a UAV
            \param[in] name The name of the texture object in the shader
            \param[in] pTexture The texture object to bind
        */
        bool setTexture(const std::string& name, const Texture::SharedPtr& pTexture);

        /** Get a texture object.
            \param[in] name The name of the texture
            \return If the name is valid, a shared pointer to the texture object. Otherwise returns nullptr
        */
        Texture::SharedPtr getTexture(const std::string& name) const;

        /** Bind an SRV
            \param[in] index The index of the SRV object in the shader
            \param[in] pSrv The SRV object to bind 
        */
        bool setSrv(uint32_t index, const ShaderResourceView::SharedPtr& pSrv);

        /** Bind a UAV
            \param[in] index The index of the UAV object in the shader
            \param[in] pSrv The UAV object to bind
        */
        bool setUav(uint32_t index, const UnorderedAccessView::SharedPtr& pUav);

        /** Get an SRV object
            \param[in] index The index of the SRV
            \return If the index is valid, a shared pointer to the SRV. Otherwise returns nullptr
        */
        ShaderResourceView::SharedPtr getSrv(uint32_t index) const;

        /** Get a UAV object
            \param[in] index The index of the UAV
            \return If the index is valid, a shared pointer to the UAV. Otherwise returns nullptr
        */
        UnorderedAccessView::SharedPtr getUav(uint32_t index) const;

        /** Bind a sampler to the program in the global namespace.
            If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
            \param[in] name The name of the sampler object in the shader
            \param[in] pSampler The sampler object to bind
            \return false if the sampler was not found in the program, otherwise true
        */
        bool setSampler(const std::string& name, const Sampler::SharedPtr& pSampler);

        /** Bind a sampler to the program in the global namespace.
            If you are using bindless textures, than this is not the right call for you. You should use the ConstantBuffer::setTexture() method instead.
            \param[in] name The name of the sampler object in the shader
            \param[in] pSampler The sampler object to bind
            \return false if the sampler was not found in the program, otherwise true
        */
        bool setSampler(uint32_t index, const Sampler::SharedPtr& pSampler);

        /** Gets a sampler object.
            \return If the index is valid, a shared pointer to the sampler. Otherwise returns nullptr
        */
        Sampler::SharedPtr getSampler(const std::string& name) const;

        /** Gets a sampler object.
            \return If the index is valid, a shared pointer to the sampler. Otherwise returns nullptr
        */
        Sampler::SharedPtr getSampler(uint32_t index) const;


        // Spire-related stuff

        size_t getComponentCount() const { return mAssignedComponents.size(); }

        void setComponent(uint32_t index, ComponentInstance::SharedPtr const& pComponent);

        template<typename T>
        void setVariable(const std::string& name, const T& value)
        {
            mAssignedComponents[0]->setVariable(name, value);
        }

        template<typename T>
        void setVariable(size_t offset, const T& value)
        {
            mAssignedComponents[0]->setVariable(offset, value);
        }

        size_t getVariableOffset(const std::string& name)
        {
            return mAssignedComponents[0]->getVariableOffset(name);
        }

        /** Get the program reflection interface
        */
        ProgramReflection::SharedConstPtr getReflection() const { return mpReflector; }

        /** Get the root signature object
        */
        RootSignature::SharedPtr getRootSignature() const { return mpRootSignature; }

        template<typename ViewType>
        struct ResourceData
        {
            typename ViewType::SharedPtr pView;
            Resource::SharedPtr pResource;
            uint32_t rootSigOffset = 0;
        };

        template<>
        struct ResourceData<Sampler>
        {
            Sampler::SharedPtr pSampler;
            uint32_t rootSigOffset = 0;
        };

        template<typename T>
        using ResourceMap = std::unordered_map<uint32_t, ResourceData<T>>;

    protected:
        template<bool forGraphics, typename ContextType>
        void applyCommon(ContextType* pContext) const;

        ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig);

        mutable RootSignature::SharedPtr mpRootSignature;
        ProgramReflection::SharedConstPtr mpReflector;

        ResourceMap<ConstantBuffer> mAssignedCbs;        // HLSL 'b' registers
        ResourceMap<ShaderResourceView> mAssignedSrvs;   // HLSL 't' registers
        ResourceMap<UnorderedAccessView> mAssignedUavs;  // HLSL 'u' registers
        ResourceMap<Sampler> mAssignedSamplers;          // HLSL 's' registers

        // SPIRE:
    public:
        std::vector<ComponentInstance::SharedPtr> mAssignedComponents;
        mutable bool mRootSignatureDirty = false;

        RootSignature::SharedPtr createRootSignature() const;
    };

    class GraphicsVars : public ProgramVars, public std::enable_shared_from_this<ProgramVars>
    {
    public:
        using SharedPtr = SharedPtrT<GraphicsVars>;
        using SharedConstPtr = std::shared_ptr<const GraphicsVars>;

        /** Create a new object
            \param[in] pReflector A program reflection object containing the requested declarations
            \param[in] createBuffers If true, will create the ConstantBuffer objects. Otherwise, the user will have to bind the CBs himself
            \param[in] pRootSignature A root-signature describing how to bind resources into the shader. If this parameter is nullptr, a root-signature object will be created from the program reflection object
        */
        static SharedPtr create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers = true, const RootSignature::SharedPtr& pRootSig = nullptr);

        static SharedPtr create(const Program::SharedConstPtr& pProgram);

        void apply(RenderContext* pContext) const;
    private:
        GraphicsVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig) :
            ProgramVars(pReflector, createBuffers, pRootSig) {}
    };

    class ComputeVars : public ProgramVars, public std::enable_shared_from_this<ProgramVars>
    {
    public:
        using SharedPtr = SharedPtrT<ComputeVars>;
        using SharedConstPtr = std::shared_ptr<const ComputeVars>;

        /** Create a new object
            \param[in] pReflector A program reflection object containing the requested declarations
            \param[in] createBuffers If true, will create the ConstantBuffer objects. Otherwise, the user will have to bind the CBs himself
            \param[in] pRootSignature A root-signature describing how to bind resources into the shader. If this parameter is nullptr, a root-signature object will be created from the program reflection object
        */
        static SharedPtr create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers = true, const RootSignature::SharedPtr& pRootSig = nullptr);
        void apply(CopyContext* pContext) const;
    private:
        ComputeVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig) :
            ProgramVars(pReflector, createBuffers, pRootSig) {}
    };
}