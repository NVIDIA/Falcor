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
#include "ProgramVars.h"
#include "API/Buffer.h"
#include "API/CopyContext.h"
#include "API/RenderContext.h"

namespace Falcor
{
    template<RootSignature::DescType descType>
    uint32_t findRootSignatureOffset(const RootSignature* pRootSig, uint32_t regIndex, uint32_t regSpace)
    {
        // Find the bind-index in the root descriptor
        bool found = false;

        // First search the root-descriptors
        for (size_t i = 0; i < pRootSig->getRootDescriptorCount(); i++)
        {
            const RootSignature::DescriptorDesc& desc = pRootSig->getRootDescriptor(i);
            found = (desc.type == descType) && (desc.regIndex == regIndex) && (desc.regSpace == regSpace);
            if (found)
            {
                return pRootSig->getDescriptorRootOffset(i);
            }
        }

        // Search the descriptor-tables
        for (size_t i = 0; i < pRootSig->getDescriptorTableCount(); i++)
        {
            const RootSignature::DescriptorTable& table = pRootSig->getDescriptorTable(i);
            assert(table.getRangeCount() == 1);
            const RootSignature::DescriptorTable::Range& range = table.getRange(0);
            assert(range.descCount == 1);
            if (range.type == descType && range.firstRegIndex == regIndex && range.regSpace == regSpace)
            {
                return pRootSig->getDescriptorTableRootOffset(i);
            }
        }
        should_not_get_here();
        return -1;
    }

    template<typename BufferType, typename ViewType, RootSignature::DescType descType, typename ViewInitFunc>
    bool initializeBuffersMap(ProgramVars::ResourceMap<ViewType>& bufferMap, bool createBuffers, const ViewInitFunc& viewInitFunc, const ProgramReflection::BufferMap& reflectionMap, ProgramReflection::ShaderAccess shaderAccess, const RootSignature* pRootSig)
    {
        for (auto& buf : reflectionMap)
        {
            const ProgramReflection::BufferReflection* pReflector = buf.second.get();
            if(pReflector->getShaderAccess() == shaderAccess)
            {
                uint32_t regIndex = pReflector->getRegisterIndex();
                uint32_t regSpace = pReflector->getRegisterSpace();

                ProgramVars::ResourceData<ViewType> data;
                
                // Only create the buffer if needed
                if (createBuffers)
                {
                    data.pResource = BufferType::create(buf.second);
                    data.pView = viewInitFunc(data.pResource);
                }

                data.rootSigOffset = findRootSignatureOffset<descType>(pRootSig, regIndex, regSpace);
                if (data.rootSigOffset == -1)
                {
                    logError("Can't find a root-signature information matching buffer '" + pReflector->getName() + " when creating ProgramVars");
                    return false;
                }

                bufferMap.emplace(regIndex, data);
            }
        }
        return true;
    }

    ProgramVars::ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig) : mpReflector(pReflector)
    {
        // SPIRE:
#if 0
        // Initialize the CB and StructuredBuffer maps. We always do it, to mark which slots are used in the shader.

        mpRootSignature = pRootSig ? pRootSig : RootSignature::create(pReflector.get());

        //SPIRE: in the Spire case, we really can't create the root signature up front,
        //so trying to do it here is bogus.
//        mpRootSignature = nullptr;
        if( pReflector->getComponentCount() )
        {
            // Spire-based shaders will re-build their root signature as needed
            mRootSignatureDirty = true;
        }

        auto getNullptrFunc = [](const Resource::SharedPtr& pResource) { return nullptr; };
        auto getSrvFunc = [](const Resource::SharedPtr& pResource) { return pResource->getSRV(0, 1, 0, 1); };
        auto getUavFunc = [](const Resource::SharedPtr& pResource) { return pResource->getUAV(0, 0, 1); };

        initializeBuffersMap<ConstantBuffer, ConstantBuffer, RootSignature::DescType::CBV>(mAssignedCbs, createBuffers, getNullptrFunc, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Constant), ProgramReflection::ShaderAccess::Read, mpRootSignature.get());
        initializeBuffersMap<StructuredBuffer, ShaderResourceView, RootSignature::DescType::SRV>(mAssignedSrvs, createBuffers, getSrvFunc, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Structured), ProgramReflection::ShaderAccess::Read, mpRootSignature.get());
        initializeBuffersMap<StructuredBuffer, UnorderedAccessView, RootSignature::DescType::UAV>(mAssignedUavs, createBuffers, getUavFunc, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Structured), ProgramReflection::ShaderAccess::ReadWrite, mpRootSignature.get());

        // Initialize the textures and samplers map
        for (const auto& res : pReflector->getResourceMap())
        {
            const auto& desc = res.second;
            switch (desc.type)
            {
            case ProgramReflection::Resource::ResourceType::Sampler:
                mAssignedSamplers[desc.regIndex].pSampler = nullptr;
                mAssignedSamplers[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::Sampler>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::Texture:
            case ProgramReflection::Resource::ResourceType::RawBuffer:
                if (desc.shaderAccess == ProgramReflection::ShaderAccess::Read)
                {
                    assert(mAssignedSrvs.find(desc.regIndex) == mAssignedSrvs.end());
                    mAssignedSrvs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::SRV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                }
                else
                {
                    assert(mAssignedUavs.find(desc.regIndex) == mAssignedUavs.end());
                    assert(desc.shaderAccess == ProgramReflection::ShaderAccess::ReadWrite);
                    mAssignedUavs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::UAV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                }
                break;
            default:
                should_not_get_here();
            }
        }
#endif
        // Initialization of a program using Spire is easy: we just need to hold the components
        // that are currently bound, and use the 0th element of that array to hold a
        // pre-generated component instance for the parmaters of this program (that last
        // bit is Falcor policy, rather than a Spire requirement).

        mRootSignatureDirty = true;
        mAssignedComponents.resize(pReflector->getComponentCount());
        if( pReflector->getComponentCount() > 0 )
        {
            // There is at least one component, and we assume the first one represents
            // the "direct" parameters of the program...
            auto componentInstance = ComponentInstance::create(pReflector->getComponent(0));
            mAssignedComponents[0] = componentInstance;
        }
    }

    GraphicsVars::SharedPtr GraphicsVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig)
    {
        return SharedPtr(new GraphicsVars(pReflector, createBuffers, pRootSig));
    }

    GraphicsVars::SharedPtr GraphicsVars::create(const Program::SharedConstPtr& pProgram)\
    {
        if( pProgram->isSpire() )
        {
            return create(pProgram->getSpireReflector());
        }

        return create(pProgram->getActiveVersion()->getReflector());
    }


    ComputeVars::SharedPtr ComputeVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig)
    {
        return SharedPtr(new ComputeVars(pReflector, createBuffers, pRootSig));
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(const std::string& name) const
    {
#if 0
        uint32_t index = mpReflector->getBufferBinding(name).regIndex;
        if (index == ProgramReflection::kInvalidLocation)
        {
            logWarning("Constant buffer \"" + name + "\" was not found. Ignoring getConstantBuffer() call.");
            return nullptr;
        }

        auto& pDesc = mpReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Constant);

        if (pDesc->getType() != ProgramReflection::BufferReflection::Type::Constant)
        {
            logWarning("Buffer \"" + name + "\" is not a constant buffer. Type = " + to_string(pDesc->getType()));
            return nullptr;
        }

        return getConstantBuffer(index);
#else
        logWarning("Spire-based shaders don't expose direct access to constant buffers");
        return nullptr;
#endif
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(uint32_t index) const
    {
#if 0
        auto& it = mAssignedCbs.find(index);
        if (it == mAssignedCbs.end())
        {
            logWarning("Can't find constant buffer at index " + std::to_string(index) + ". Ignoring getConstantBuffer() call.");
            return nullptr;
        }

        return std::static_pointer_cast<ConstantBuffer>(it->second.pResource);
#else
        logWarning("Spire-based shaders don't expose direct access to constant buffers");
        return nullptr;
#endif
    }

    bool ProgramVars::setConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB)
    {
#if 0
        // Check that the index is valid
        if (mAssignedCbs.find(index) == mAssignedCbs.end())
        {
            logWarning("No constant buffer was found at index " + std::to_string(index) + ". Ignoring setConstantBuffer() call.");
            return false;
        }

        // Just need to make sure the buffer is large enough
        const auto& desc = mpReflector->getBufferDesc(index, ProgramReflection::ShaderAccess::Read, ProgramReflection::BufferReflection::Type::Constant);
        if (desc->getRequiredSize() > pCB->getSize())
        {
            logError("Can't attach the constant buffer. Size mismatch.");
            return false;
        }

        assert(mAssignedCbs.find(index) != mAssignedCbs.end());
        mAssignedCbs[index].pResource = pCB;
        return true;
#else
        logWarning("Spire-based shaders don't expose direct access to constant buffers");
        return nullptr;
#endif
    }

    bool ProgramVars::setConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB)
    {
#if 0
        // Find the buffer
        uint32_t loc = mpReflector->getBufferBinding(name).regIndex;
        if (loc == ProgramReflection::kInvalidLocation)
        {
            logWarning("Constant buffer \"" + name + "\" was not found. Ignoring setConstantBuffer() call.");
            return false;
        }

        return setConstantBuffer(loc, pCB);
#else
        logWarning("Spire-based shaders don't expose direct access to constant buffers");
        return nullptr;
#endif
    }

    void setResourceSrvUavCommon(uint32_t regIndex, ProgramReflection::ShaderAccess shaderAccess, const Resource::SharedPtr& resource, ComponentInstance* pComponent)
    {
        switch (shaderAccess)
        {
        case ProgramReflection::ShaderAccess::ReadWrite:
        {
            pComponent->setUav(regIndex, resource->getUAV(), resource);
            break;
        }

        case ProgramReflection::ShaderAccess::Read:
        {
            pComponent->setSrv(regIndex, resource->getSRV(), resource);
            break;
        }

        default:
            should_not_get_here();
        }
    }

    void setResourceSrvUavCommon(const ProgramReflection::Resource* pDesc, const Resource::SharedPtr& resource, ComponentInstance* pComponent)
    {
        setResourceSrvUavCommon(pDesc->regIndex, pDesc->shaderAccess, resource, pComponent);
    }

    void setResourceSrvUavCommon(const ProgramReflection::BufferReflection *pDesc, const Resource::SharedPtr& resource, ComponentInstance* pComponent)
    {
        setResourceSrvUavCommon(pDesc->getRegisterIndex(), pDesc->getShaderAccess(), resource, pComponent);
    }

    bool verifyBufferResourceDesc(const ProgramReflection::Resource *pDesc, const std::string& name, ProgramReflection::Resource::ResourceType expectedType, ProgramReflection::Resource::Dimensions expectedDims, char const* funcName)
    {
        if (pDesc == nullptr)
        {
            logWarning(std::string("ProgramVars::") + funcName + " - resource \"" + name + "\" was not found. Ignoring " + funcName + " call.");
            return false;
        }

        if (pDesc->type != expectedType || pDesc->dims != expectedDims)
        {
            logWarning(std::string("ProgramVars::") + funcName + " - variable '" + name + "' is the incorrect type. VarType = " + to_string(pDesc->type) + ", VarDims = " + to_string(pDesc->dims) + ". Ignoring call");
            return false;
        }

        return true;
    }

    static bool setResourceCommon(
        ComponentInstance*                          pComponent,
        const std::string&                          name,
        const Resource::SharedPtr&                  pResource,
        ProgramReflection::Resource::ResourceType   type,
        ProgramReflection::Resource::Dimensions     dimensions,
        char const*                                 funcName)
    {
        // Find the resource
        const ProgramReflection::Resource* pDesc = pComponent->mpReflector->getResourceDesc(name);
        if (verifyBufferResourceDesc(pDesc, name, type, ProgramReflection::Resource::Dimensions::Unknown, funcName) == false)
        {
            return false;
        }

        setResourceSrvUavCommon(pDesc, pResource, pComponent);
        return true;
    }

    bool ProgramVars::setRawBuffer(const std::string& name, Buffer::SharedPtr pBuf)
    {
        return getDefaultComponent()->setRawBuffer(name, pBuf);
    }
    bool ComponentInstance::setRawBuffer(const std::string& name, Buffer::SharedPtr const& pBuf)
    {
        return setResourceCommon(this, name, pBuf, ProgramReflection::Resource::ResourceType::RawBuffer, ProgramReflection::Resource::Dimensions::Unknown, "setRawBuffer()");
    }

    bool ProgramVars::setTypedBuffer(const std::string& name, TypedBufferBase::SharedPtr pBuf)
    {
        return getDefaultComponent()->setTypedBuffer(name, pBuf);
    }
    bool ComponentInstance::setTypedBuffer(const std::string& name, TypedBufferBase::SharedPtr const& pBuf)
    {
        return setResourceCommon(this, name, pBuf, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::Resource::Dimensions::Buffer, "setTypedBuffer()");
    }

    bool ProgramVars::setStructuredBuffer(const std::string& name, StructuredBuffer::SharedPtr pBuf)
    {
        return getDefaultComponent()->setStructuredBuffer(name, pBuf);
    }
    bool ComponentInstance::setStructuredBuffer(const std::string& name, StructuredBuffer::SharedPtr const& pBuf)
    {
        // Find the buffer
        const ProgramReflection::BufferReflection* pBufDesc = mpReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Structured).get();

        if (pBufDesc == nullptr)
        {
            logWarning("Structured buffer \"" + name + "\" was not found. Ignoring setStructuredBuffer() call.");
            return false;
        }

        setResourceSrvUavCommon(pBufDesc, pBuf, this);
        return true;
    }

    template<typename ResourceType>
    typename ResourceType::SharedPtr getResourceFromSrvUavCommon(uint32_t regIndex, ProgramReflection::ShaderAccess shaderAccess, ComponentInstance const* pComponent, const std::string& varName, char const* funcName)
    {
        auto& assignedUavs = pComponent->mAssignedUAVs;
        auto& assignedSrvs = pComponent->mAssignedSRVs;
        switch (shaderAccess)
        {
        case ProgramReflection::ShaderAccess::ReadWrite:
            if (regIndex >= assignedUavs.size())
            {
                logWarning(std::string("ProgramVars::") + funcName + " - variable \"" + varName + "\' was not found in UAVs. Shader Access = " + to_string(shaderAccess));
                return nullptr;
            }
            return std::dynamic_pointer_cast<ResourceType>(assignedUavs.at(regIndex).pResource);

        case ProgramReflection::ShaderAccess::Read:
            if (regIndex >= assignedSrvs.size())
            {
                logWarning(std::string("ProgramVars::") + funcName + " - variable \"" + varName + "\' was not found in SRVs. Shader Access = " + to_string(shaderAccess));
                return nullptr;
            }
            return std::dynamic_pointer_cast<ResourceType>(assignedSrvs.at(regIndex).pResource);

        default:
            should_not_get_here();
        }

        return nullptr;
    }

    template<typename ResourceType>
    typename ResourceType::SharedPtr getResourceFromSrvUavCommon(const ProgramReflection::Resource *pDesc, ComponentInstance const* pComponent, const std::string& varName, char const* funcName)
    {
        return getResourceFromSrvUavCommon<ResourceType>(pDesc->regIndex, pDesc->shaderAccess, pComponent, varName, funcName);
    }

    template<typename ResourceType>
    typename ResourceType::SharedPtr getResourceFromSrvUavCommon(const ProgramReflection::BufferReflection *pBufDesc, ComponentInstance const* pComponent, const std::string& varName, char const* funcName)
    {
        return getResourceFromSrvUavCommon<ResourceType>(pBufDesc->getRegisterIndex(), pBufDesc->getShaderAccess(), pComponent, varName, funcName);
    }

    Buffer::SharedPtr ProgramVars::getRawBuffer(const std::string& name) const
    {
        return getDefaultComponent()->getRawBuffer(name);
    }
    Buffer::SharedPtr ComponentInstance::getRawBuffer(const std::string& name) const
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyBufferResourceDesc(pDesc, name, ProgramReflection::Resource::ResourceType::RawBuffer, ProgramReflection::Resource::Dimensions::Unknown, "getRawBuffer()") == false)
        {
            return false;
        }

        return getResourceFromSrvUavCommon<Buffer>(pDesc, this, name, "getRawBuffer()");
    }

    TypedBufferBase::SharedPtr ProgramVars::getTypedBuffer(const std::string& name) const
    {
        return getDefaultComponent()->getTypedBuffer(name);
    }
    TypedBufferBase::SharedPtr ComponentInstance::getTypedBuffer(const std::string& name) const
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyBufferResourceDesc(pDesc, name, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::Resource::Dimensions::Buffer, "getTypedBuffer()") == false)
        {
            return false;
        }

        return getResourceFromSrvUavCommon<TypedBufferBase>(pDesc, this, name, "getTypedBuffer()");
    }

    StructuredBuffer::SharedPtr ProgramVars::getStructuredBuffer(const std::string& name) const
    {
        return getDefaultComponent()->getStructuredBuffer(name);
    }
    StructuredBuffer::SharedPtr ComponentInstance::getStructuredBuffer(const std::string& name) const
    {
        const ProgramReflection::BufferReflection* pBufDesc = mpReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Structured).get();

        if (pBufDesc == nullptr)
        {
            logWarning("Structured buffer \"" + name + "\" was not found. Ignoring getStructuredBuffer() call.");
            return false;
        }
        
        return getResourceFromSrvUavCommon<StructuredBuffer>(pBufDesc, this, name, "getStructuredBuffer()");
    }

    bool verifyResourceDesc(const ProgramReflection::Resource* pDesc, ProgramReflection::Resource::ResourceType type, ProgramReflection::ShaderAccess access, const std::string& varName, const std::string& funcName)
    {
        if (pDesc == nullptr)
        {
            logWarning(to_string(type) + " \"" + varName + "\" was not found. Ignoring " + funcName + " call.");
            return false;
        }

        if (pDesc->type != type)
        {
            logWarning("ProgramVars::" + funcName + " was called, but variable \"" + varName + "\" has different resource type. Expecting + " + to_string(pDesc->type) + " but provided resource is " + to_string(type) + ". Ignoring call");
            return false;
        }

        if (access != ProgramReflection::ShaderAccess::Undefined && pDesc->shaderAccess != access)
        {
            logWarning("ProgramVars::" + funcName + " was called, but variable \"" + varName + "\" has different shader access type. Expecting + " + to_string(pDesc->shaderAccess) + " but provided resource is " + to_string(access) + ". Ignoring call");
            return false;
        }

        return true;
    }

    bool ProgramVars::setSampler(uint32_t index, const Sampler::SharedPtr& pSampler)
    {
        return getDefaultComponent()->setSampler(index, pSampler);
    }
    bool ComponentInstance::setSampler(uint32_t index, const Sampler::SharedPtr& pSampler)
    {
        if(index >= mAssignedSamplers.size())
        {
            logError("sampler index out of range");
            return false;
        }

        if( mAssignedSamplers[index].get() == pSampler.get())
            return true;

        mAssignedSamplers[index] = pSampler ? pSampler->shared_from_this() : nullptr;

        mSamplerTableDirty = true;
        return true;
    }

    bool ProgramVars::setSampler(const std::string& name, const Sampler::SharedPtr& pSampler)
    {
        return getDefaultComponent()->setSampler(name, pSampler);
    }
    bool ComponentInstance::setSampler(const std::string& name, const Sampler::SharedPtr& pSampler)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Sampler, ProgramReflection::ShaderAccess::Read, name, "setSampler()") == false)
        {
            return false;
        }

        return setSampler(pDesc->regIndex, pSampler);
    }

    Sampler::SharedPtr ProgramVars::getSampler(const std::string& name) const
    {
        return getDefaultComponent()->getSampler(name);
    }
    Sampler::SharedPtr ComponentInstance::getSampler(const std::string& name) const
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Sampler, ProgramReflection::ShaderAccess::Read, name, "getSampler()") == false)
        {
            return nullptr;
        }

        return getSampler(pDesc->regIndex);
    }

    Sampler::SharedPtr ProgramVars::getSampler(uint32_t index) const
    {
        return getDefaultComponent()->getSampler(index);
    }
    Sampler::SharedPtr ComponentInstance::getSampler(uint32_t index) const
    {
        if(index >= mAssignedSamplers.size())
        {
            logWarning("ProgramVars::getSampler() - Cannot find sampler at index " + index);
            return nullptr;
        }

        return mAssignedSamplers[index];
    }

    ShaderResourceView::SharedPtr ProgramVars::getSrv(uint32_t index) const
    {
        return getDefaultComponent()->getSrv(index);
    }
    ShaderResourceView::SharedPtr ComponentInstance::getSrv(uint32_t index) const
    {
        if( index >= mAssignedSRVs.size() )
        {
            logWarning("ProgramVars::getSrv() - Cannot find SRV at index " + index);
            return nullptr;
        }

        return mAssignedSRVs[index].pView;
    }

    UnorderedAccessView::SharedPtr ProgramVars::getUav(uint32_t index) const
    {
        return getDefaultComponent()->getUav(index);
    }
    UnorderedAccessView::SharedPtr ComponentInstance::getUav(uint32_t index) const
    {
        if( index >= mAssignedUAVs.size() )
        {
            logWarning("ProgramVars::getUav() - Cannot find UAV at index " + index);
            return nullptr;
        }

        return mAssignedUAVs[index].pView;
    }

    bool ProgramVars::setTexture(const std::string& name, const Texture::SharedPtr& pTexture)
    {
        return getDefaultComponent()->setTexture(name, pTexture);
    }
    bool ComponentInstance::setTexture(const std::string& name, const Texture::SharedPtr& pTexture)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::ShaderAccess::Undefined, name, "setTexture()") == false)
        {
            return false;
        }

        setResourceSrvUavCommon(pDesc, pTexture, this);
        return true;
    }

    Texture::SharedPtr ProgramVars::getTexture(const std::string& name) const
    {
        return getDefaultComponent()->getTexture(name);
    }
    Texture::SharedPtr ComponentInstance::getTexture(const std::string& name) const
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::ShaderAccess::Undefined, name, "getTexture()") == false)
        {
            return nullptr;
        }

        return getResourceFromSrvUavCommon<Texture>(pDesc, this, name, "getTexture()");
    }


    template<typename ViewType>
    Resource::SharedPtr getResourceFromView(const ViewType* pView)
    {
        if (pView)
        {
            return const_cast<Resource*>(pView->getResource())->shared_from_this();
        }
        else
        {
            return nullptr;
        }
    }

    bool ProgramVars::setSrv(uint32_t index, const ShaderResourceView::SharedPtr& pSrv)
    {
        return getDefaultComponent()->setSrv(index, pSrv, getResourceFromView(pSrv.get()));
    }
    bool ComponentInstance::setSrv(
        uint32_t index,
        const ShaderResourceView::SharedPtr& pSrv)
    {
        return setSrv(index, pSrv, getResourceFromView(pSrv.get()));
    }
    bool ComponentInstance::setSrv(
        uint32_t index,
        const ShaderResourceView::SharedPtr& pSrv,
        const Resource::SharedPtr& pResource)
    {
        if( index >= mAssignedSRVs.size() )
        {
            logWarning("Can't find SRV with index " + std::to_string(index) + ". Ignoring call to ProgramVars::setSrv()");
            return false;
        }

        if( mAssignedSRVs[index].pView.get() == pSrv.get() )
        {
            return true;
        }

        mAssignedSRVs[index].pView = pSrv ? pSrv : ShaderResourceView::getNullView();
        mAssignedSRVs[index].pResource = pResource;

        mResourceTableDirty = true;
        return true;
    }

    bool ProgramVars::setUav(uint32_t index, const UnorderedAccessView::SharedPtr& pUav)
    {
        return getDefaultComponent()->setUav(index, pUav);
    }
    bool ComponentInstance::setUav(uint32_t index, UnorderedAccessView::SharedPtr const& pUav)
    {
        return setUav(index, pUav, getResourceFromView(pUav.get()));
    }
    bool ComponentInstance::setUav(
        uint32_t index,
        UnorderedAccessView::SharedPtr const& pUav,
        const Resource::SharedPtr& pResource)
    {
        if( index >= mAssignedUAVs.size() )
        {
            logWarning("Can't find UAV with index " + std::to_string(index) + ". Ignoring call to ProgramVars::setUav()");
            return false;
        }

        if( mAssignedUAVs[index].pView.get() == pUav.get() )
        {
            return true;
        }

        mAssignedUAVs[index].pView = pUav ? pUav : UnorderedAccessView::getNullView();
        mAssignedUAVs[index].pResource = pResource;

        mResourceTableDirty = true;
        return true;
    }

    template<typename ViewType, bool isUav, bool forGraphics, typename ContextType>
    void bindUavSrvCommon(ContextType* pContext, const ProgramVars::ResourceMap<ViewType>& resMap)
    {
        ID3D12GraphicsCommandList* pList = pContext->getLowLevelData()->getCommandList();
        for (auto& resIt : resMap)
        {
            const auto& resDesc = resIt.second;
            uint32_t rootOffset = resDesc.rootSigOffset;
            const Resource* pResource = resDesc.pResource.get();

            ViewType::ApiHandle handle;
            if (pResource)
            {
                // If it's a typed buffer, upload it to the GPU
                const TypedBufferBase* pTypedBuffer = dynamic_cast<const TypedBufferBase*>(pResource);
                if (pTypedBuffer)
                {
                    pTypedBuffer->uploadToGPU();
                }

                const StructuredBuffer* pStructured = dynamic_cast<const StructuredBuffer*>(pResource);
                if (pStructured)
                {
                    pStructured->uploadToGPU();
                }

                pContext->resourceBarrier(pResource, isUav ? Resource::State::UnorderedAccess : Resource::State::ShaderResource);

                if (isUav)
                {
                    if (pTypedBuffer)
                    {
                        pTypedBuffer->setGpuCopyDirty();
                    }
                    if (pStructured)
                    {
                        pStructured->setGpuCopyDirty();
                    }
                }

                handle = resDesc.pView->getApiHandle();
            }
            else
            {
                handle = isUav ? UnorderedAccessView::getNullView()->getApiHandle() : ShaderResourceView::getNullView()->getApiHandle();
            }

            if(forGraphics)
            {
                pList->SetGraphicsRootDescriptorTable(rootOffset, handle->getGpuHandle());
            }
            else
            {
                pList->SetComputeRootDescriptorTable(rootOffset, handle->getGpuHandle());
            }
        }
    }

    static void setRootConstantBuffer(
        CopyContext*                pContext,
        ID3D12GraphicsCommandList*  pList,
        uint32_t                    rootOffset,
        uint64_t                    gpuAddress)
    {
        pList->SetComputeRootConstantBufferView(rootOffset, gpuAddress);
    }

    static void setRootConstantBuffer(
        RenderContext*              pContext,
        ID3D12GraphicsCommandList*  pList,
        uint32_t                    rootOffset,
        uint64_t                    gpuAddress)
    {
        pList->SetGraphicsRootConstantBufferView(rootOffset, gpuAddress);
    }

    
    static void setRootDescriptorTable(
        CopyContext*                pContext,
        ID3D12GraphicsCommandList*  pList,
        uint32_t                    rootOffset,
        DescriptorHeap::GpuHandle   gpuHandle)
    {
        pList->SetComputeRootDescriptorTable(rootOffset, gpuHandle);
    }

    static void setRootDescriptorTable(
        RenderContext*              pContext,
        ID3D12GraphicsCommandList*  pList,
        uint32_t                    rootOffset,
        DescriptorHeap::GpuHandle   gpuHandle)
    {
        pList->SetGraphicsRootDescriptorTable(rootOffset, gpuHandle);
    }

    template<bool forGraphics, typename ContextType>
    void ProgramVars::applyCommon(ContextType* pContext) const
    {
        // Get the command list
        ID3D12GraphicsCommandList* pList = pContext->getLowLevelData()->getCommandList();

        // Spire
        getRootSignature();

        if(forGraphics)
        {
            pList->SetGraphicsRootSignature(mpRootSignature->getApiHandle());
        }
        else
        {
            pList->SetComputeRootSignature(mpRootSignature->getApiHandle());
        }

// SPIRE: no more parameters stored directly in `ProgramVars`: they are all in components...
#if 0
        // Bind the constant-buffers
        for (auto& bufIt : mAssignedCbs)
        {
            uint32_t rootOffset = bufIt.second.rootSigOffset;
            const ConstantBuffer* pCB = dynamic_cast<const ConstantBuffer*>(bufIt.second.pResource.get());
            pCB->uploadToGPU();
            if(forGraphics)
            {
                pList->SetGraphicsRootConstantBufferView(rootOffset, pCB->getGpuAddress());
            }
            else
            {
                pList->SetComputeRootConstantBufferView(rootOffset, pCB->getGpuAddress());
            }
        }

        // Bind the SRVs and UAVs
        bindUavSrvCommon<ShaderResourceView, false, forGraphics>(pContext, mAssignedSrvs);
        bindUavSrvCommon<UnorderedAccessView, true, forGraphics>(pContext, mAssignedUavs);

        // Bind the samplers
        for (auto& samplerIt : mAssignedSamplers)
        {
            uint32_t rootOffset = samplerIt.second.rootSigOffset;
            const Sampler* pSampler = samplerIt.second.pSampler.get();
            if (pSampler)
            {
                if(forGraphics)
                {
                    pList->SetGraphicsRootDescriptorTable(rootOffset, pSampler->getApiHandle()->getGpuHandle());
                }
                else
                {
                    pList->SetComputeRootDescriptorTable(rootOffset, pSampler->getApiHandle()->getGpuHandle());
                }
            }
        }
#endif

        // Spire: bind the components that have been specified...
        uint32_t rootIndex = 0;
        for( auto& component : mAssignedComponents )
        {
#if 0
            if( component->mConstantBuffer )
            {
                component->mConstantBuffer->uploadToGPU();
                setRootConstantBuffer(pContext, pList, rootIndex, component->mConstantBuffer->getGpuAddress());
                rootIndex++;
            }

            for( auto& entry : component->mBoundSRVs )
            {
                auto pResource = entry.resource.get();
                if(pResource)
                {
                    // SPIRE: NOTE: The existing Falcor code does a bunch of work here,
                    // and I'm not clear on whether this is the best place for it.

                    // If it's a typed buffer, upload it to the GPU
                    const TypedBufferBase* pTypedBuffer = dynamic_cast<const TypedBufferBase*>(pResource);
                    if (pTypedBuffer)
                    {
                        pTypedBuffer->uploadToGPU();
                    }

                    const StructuredBuffer* pStructured = dynamic_cast<const StructuredBuffer*>(pResource);
                    if (pStructured)
                    {
                        pStructured->uploadToGPU();
                    }

                    // SPIRE: TODO: follow this through and make sure we aren't issuing barriers we don't need to...
                    pContext->resourceBarrier(pResource, Resource::State::ShaderResource);

                    if (pTypedBuffer)
                    {
                        pTypedBuffer->setGpuCopyDirty();
                    }
                    if (pStructured)
                    {
                        pStructured->setGpuCopyDirty();
                    }
                }

                auto& srv = entry.srv;
                assert(srv);

                // TODO: get the SRV earlier than this... don't re-create it per draw...
                setRootDescriptorTable(pContext, pList, rootIndex,
                    srv->getApiHandle()->getGpuHandle());
                rootIndex++;
            }

            for( auto& sampler : component->mBoundSamplers )
            {
                assert(sampler);

                // TODO: get the SRV earlier than this... don't re-create it per draw...
                setRootDescriptorTable(pContext, pList, rootIndex, sampler->getApiHandle()->getGpuHandle());
                rootIndex++;
            }
#else
            // SPIRE: NOTE: Still have to do everything related to automatic/implicit upload of
            // modified data, as well as implicit barriers for read-after-write of textures, etc.
            if( component->mConstantBuffer )
            {
                component->mConstantBuffer->uploadToGPU();
            }
            for( auto& entry : component->mAssignedSRVs )
            {
                auto pResource = entry.pResource.get();
                if(pResource)
                {
                    // SPIRE: NOTE: The existing Falcor code does a bunch of work here,
                    // and I'm not clear on whether this is the best place for it.

                    // If it's a typed buffer, upload it to the GPU
                    const TypedBufferBase* pTypedBuffer = dynamic_cast<const TypedBufferBase*>(pResource);
                    if (pTypedBuffer)
                    {
                        pTypedBuffer->uploadToGPU();
                    }

                    const StructuredBuffer* pStructured = dynamic_cast<const StructuredBuffer*>(pResource);
                    if (pStructured)
                    {
                        pStructured->uploadToGPU();
                    }

                    // SPIRE: TODO: follow this through and make sure we aren't issuing barriers we don't need to...
                    pContext->resourceBarrier(pResource, Resource::State::ShaderResource);

                    if (pTypedBuffer)
                    {
                        pTypedBuffer->setGpuCopyDirty();
                    }
                    if (pStructured)
                    {
                        pStructured->setGpuCopyDirty();
                    }
                }
            }

            // SPIRE: once we've dealt with data hazard issues, the logic is really simple:
            // we bind zero, one, or two descriptor tables per component, based on what
            // kind of parameters it uses.
            //
            // NOTE: the `getApiHandle()` operation there hides more implicit update
            // stuff, where we build the table on first use...

            auto& apiHandle = component->getApiHandle();
            if( apiHandle.resourceDescriptorTable )
            {
                setRootDescriptorTable(
                    pContext,
                    pList,
                    rootIndex,
                    apiHandle.resourceDescriptorTable->getGpuHandle());
                rootIndex++;
            }
            if( apiHandle.samplerDescriptorTable )
            {
                setRootDescriptorTable(
                    pContext,
                    pList,
                    rootIndex,
                    apiHandle.samplerDescriptorTable->getGpuHandle());
                rootIndex++;
            }
#endif
        }
    }

    void ComputeVars::apply(CopyContext* pContext) const
    {
        applyCommon<false>(pContext);
    }

    void GraphicsVars::apply(RenderContext* pContext) const
    {
        applyCommon<true>(pContext);
    }

    // Spire stuff

    void ProgramVars::setComponent(uint32_t index, ComponentInstance::SharedPtr const& pComponent)
    {
        auto newType = pComponent ? pComponent->mpReflector : nullptr;
        auto oldType = mAssignedComponents[index] ? mAssignedComponents[index]->mpReflector : nullptr;

        if( newType != oldType )
        {
            mRootSignatureDirty = true;
        }

        mAssignedComponents[index] = pComponent;
    }

    RootSignature::SharedPtr ProgramVars::getRootSignature() const
    {
        if( mRootSignatureDirty )
        {
            mpRootSignature = createRootSignature();
            mRootSignatureDirty = false;
        }
        return mpRootSignature;
    }


    RootSignature::SharedPtr ProgramVars::createRootSignature() const
    {
        // Need to create a root signature that reflects the components we have bound...

        // TODO: this can (and should) be cached...

        RootSignature::Desc desc;

        uint32_t regSpace = 0;

        uint32_t bReg = 0;
        uint32_t sReg = 0;
        uint32_t tReg = 0;

        for( auto& component : mAssignedComponents )
        {
            auto& reflector = component->mpReflector;

            // We will allocate up to two descriptor tables per component,
            // one to hold resources (constant buffers, textures, UAVs, etc.)
            // and another one to hold samplers.
            // Any uniform parameters of the component are grouped into a
            // constant buffer that comes first in the resource table.
            //
            // We won't allocate a table to a component unless it needs
            // at least *some* parameters of the corresponding category.
            // This should mean that we automatically skip allocating
            // root-signature space for any component that only has
            // logic, and no parameters.
            //
            // Eventually it would be nice to handle more special cases.
            // The most important of these would be the case where a
            // component has only a single constant buffer, and no
            // other resource parameters. In this case we might as
            // well use a root constant buffer slot.
            uint32_t uniformCount = (uint32_t) reflector->getVariableCount();
            uint32_t resourceCount = reflector->getResourceCount();
            uint32_t samplerCount = reflector->getSamplerCount();

            //
            //
            // TODO(tfoley): if there are no resources, then just use a root CBV slot
            if( resourceCount || uniformCount )
            {
                RootSignature::DescriptorTable tableDesc;
                if( uniformCount )
                {
                    tableDesc.addRange(RootSignature::DescType::CBV, bReg, 1, regSpace);
                    bReg++;
                }
                if( resourceCount )
                {
                    tableDesc.addRange(RootSignature::DescType::SRV, tReg, resourceCount, regSpace);
                    tReg += resourceCount;
                }
                desc.addDescriptorTable(tableDesc);
            }

            if( samplerCount )
            {
                RootSignature::DescriptorTable tableDesc;

                tableDesc.addRange(RootSignature::DescType::Sampler, sReg, samplerCount, regSpace);
                sReg += samplerCount;

                desc.addDescriptorTable(tableDesc);
            }
        }

        return RootSignature::create(desc);
    }

}