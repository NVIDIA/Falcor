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
#include "API/DescriptorSet.h"
#include "API/Device.h"

namespace Falcor
{
    template<RootSignature::DescType descType>
    uint32_t findRootIndex(const RootSignature* pRootSig, uint32_t regIndex, uint32_t regSpace)
    {
        // Find the bind-index in the root descriptor
        bool found = false;

        // Search the descriptor-tables
        for (size_t i = 0; i < pRootSig->getDescriptorSetCount(); i++)
        {
            const RootSignature::DescriptorSet& set = pRootSig->getDescriptorSet(i);
            assert(set.getRangeCount() == 1);
            const RootSignature::DescriptorSet::Range& range = set.getRange(0);
            assert(range.descCount == 1);
            if (range.type == descType && range.firstRegIndex == regIndex && range.regSpace == regSpace)
            {
                return (uint32_t)i;
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

                data.rootIndex = findRootIndex<descType>(pRootSig, regIndex, regSpace);
                if (data.rootIndex == -1)
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
        // Initialize the CB and StructuredBuffer maps. We always do it, to mark which slots are used in the shader.
        mpRootSignature = pRootSig ? pRootSig : RootSignature::create(pReflector.get());

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
            uint32_t count = desc.arraySize ? desc.arraySize : 1;
            for( uint32_t index = 0; index < count; ++index )
            {
                uint32_t regIndex = desc.regIndex + index;
                switch (desc.type)
                {
                case ProgramReflection::Resource::ResourceType::Sampler:
                    mAssignedSamplers[regIndex].pSampler = nullptr;
                    mAssignedSamplers[regIndex].rootIndex = findRootIndex<RootSignature::DescType::Sampler>(mpRootSignature.get(), regIndex, desc.registerSpace);
                    break;
                case ProgramReflection::Resource::ResourceType::Texture:
                case ProgramReflection::Resource::ResourceType::RawBuffer:
                    if (desc.shaderAccess == ProgramReflection::ShaderAccess::Read)
                    {
                        assert(mAssignedSrvs.find(regIndex) == mAssignedSrvs.end());
                        mAssignedSrvs[regIndex].rootIndex = findRootIndex<RootSignature::DescType::SRV>(mpRootSignature.get(), regIndex, desc.registerSpace);
                    }
                    else
                    {
                        assert(mAssignedUavs.find(regIndex) == mAssignedUavs.end());
                        assert(desc.shaderAccess == ProgramReflection::ShaderAccess::ReadWrite);
                        mAssignedUavs[regIndex].rootIndex = findRootIndex<RootSignature::DescType::UAV>(mpRootSignature.get(), regIndex, desc.registerSpace);
                    }
                    break;
                default:
                    should_not_get_here();
                }
            }
        }
    }

    GraphicsVars::SharedPtr GraphicsVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig)
    {
        return SharedPtr(new GraphicsVars(pReflector, createBuffers, pRootSig));
    }

    ComputeVars::SharedPtr ComputeVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig)
    {
        return SharedPtr(new ComputeVars(pReflector, createBuffers, pRootSig));
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(const std::string& name) const
    {
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
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(uint32_t index) const
    {
        auto& it = mAssignedCbs.find(index);
        if (it == mAssignedCbs.end())
        {
            logWarning("Can't find constant buffer at index " + std::to_string(index) + ". Ignoring getConstantBuffer() call.");
            return nullptr;
        }

        return std::static_pointer_cast<ConstantBuffer>(it->second.pResource);
    }

    bool ProgramVars::setConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB)
    {
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
    }

    bool ProgramVars::setConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB)
    {
        // Find the buffer
        uint32_t loc = mpReflector->getBufferBinding(name).regIndex;
        if (loc == ProgramReflection::kInvalidLocation)
        {
            logWarning("Constant buffer \"" + name + "\" was not found. Ignoring setConstantBuffer() call.");
            return false;
        }

        return setConstantBuffer(loc, pCB);
    }

    void setResourceSrvUavCommon(uint32_t regIndex, ProgramReflection::ShaderAccess shaderAccess, const Resource::SharedPtr& resource, ProgramVars::ResourceMap<ShaderResourceView>& assignedSrvs, ProgramVars::ResourceMap<UnorderedAccessView>& assignedUavs)
    {
        switch (shaderAccess)
        {
        case ProgramReflection::ShaderAccess::ReadWrite:
        {
            auto uavIt = assignedUavs.find(regIndex);
            assert(uavIt != assignedUavs.end());
            auto resUav = resource ? resource->getUAV() : nullptr;

            if(uavIt->second.pView != resUav)
            {
                uavIt->second.pDescSet = nullptr;
                uavIt->second.pResource = resource;
                uavIt->second.pView = resUav;
            }
            break;
        }

        case ProgramReflection::ShaderAccess::Read:
        {
            auto srvIt = assignedSrvs.find(regIndex);
            assert(srvIt != assignedSrvs.end());

            auto resSrv = resource ? resource->getSRV() : nullptr;

            if(srvIt->second.pView != resSrv)
            {
                srvIt->second.pDescSet = nullptr;
                srvIt->second.pResource = resource;
                srvIt->second.pView = resSrv;
            }
            break;
        }

        default:
            should_not_get_here();
        }
    }

    void setResourceSrvUavCommon(const ProgramReflection::Resource* pDesc, const Resource::SharedPtr& resource, ProgramVars::ResourceMap<ShaderResourceView>& assignedSrvs, ProgramVars::ResourceMap<UnorderedAccessView>& assignedUavs)
    {
        setResourceSrvUavCommon(pDesc->regIndex, pDesc->shaderAccess, resource, assignedSrvs, assignedUavs);
    }

    void setResourceSrvUavCommon(const ProgramReflection::BufferReflection *pDesc, const Resource::SharedPtr& resource, ProgramVars::ResourceMap<ShaderResourceView>& assignedSrvs, ProgramVars::ResourceMap<UnorderedAccessView>& assignedUavs)
    {
        setResourceSrvUavCommon(pDesc->getRegisterIndex(), pDesc->getShaderAccess(), resource, assignedSrvs, assignedUavs);
    }

    bool verifyBufferResourceDesc(const ProgramReflection::Resource *pDesc, const std::string& name, ProgramReflection::Resource::ResourceType expectedType, ProgramReflection::Resource::Dimensions expectedDims, const std::string& funcName)
    {
        if (pDesc == nullptr)
        {
            logWarning("ProgramVars::" + funcName + " - resource \"" + name + "\" was not found. Ignoring " + funcName + " call.");
            return false;
        }

        if (pDesc->type != expectedType || pDesc->dims != expectedDims)
        {
            logWarning("ProgramVars::" + funcName + " - variable '" + name + "' is the incorrect type. VarType = " + to_string(pDesc->type) + ", VarDims = " + to_string(pDesc->dims) + ". Ignoring call");
            return false;
        }

        return true;
    }

    bool ProgramVars::setRawBuffer(const std::string& name, Buffer::SharedPtr pBuf)
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyBufferResourceDesc(pDesc, name, ProgramReflection::Resource::ResourceType::RawBuffer, ProgramReflection::Resource::Dimensions::Unknown, "setRawBuffer()") == false)
        {
            return false;
        }

        setResourceSrvUavCommon(pDesc, pBuf, mAssignedSrvs, mAssignedUavs);

        return true;
    }

    bool ProgramVars::setTypedBuffer(const std::string& name, TypedBufferBase::SharedPtr pBuf)
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyBufferResourceDesc(pDesc, name, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::Resource::Dimensions::Buffer, "setTypedBuffer()") == false)
        {
            return false;
        }

        setResourceSrvUavCommon(pDesc, pBuf, mAssignedSrvs, mAssignedUavs);

        return true;
    }

    bool ProgramVars::setStructuredBuffer(const std::string& name, StructuredBuffer::SharedPtr pBuf)
    {
        // Find the buffer
        const ProgramReflection::BufferReflection* pBufDesc = mpReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Structured).get();

        if (pBufDesc == nullptr)
        {
            logWarning("Structured buffer \"" + name + "\" was not found. Ignoring setStructuredBuffer() call.");
            return false;
        }

        setResourceSrvUavCommon(pBufDesc, pBuf, mAssignedSrvs, mAssignedUavs);

        return true;
    }

    template<typename ResourceType>
    typename ResourceType::SharedPtr getResourceFromSrvUavCommon(uint32_t regIndex, ProgramReflection::ShaderAccess shaderAccess, const ProgramVars::ResourceMap<ShaderResourceView>& assignedSrvs, const ProgramVars::ResourceMap<UnorderedAccessView>& assignedUavs, const std::string& varName, const std::string& funcName)
    {
        switch (shaderAccess)
        {
        case ProgramReflection::ShaderAccess::ReadWrite:
            if (assignedUavs.find(regIndex) == assignedUavs.end())
            {
                logWarning("ProgramVars::" + funcName + " - variable \"" + varName + "\' was not found in UAVs. Shader Access = " + to_string(shaderAccess));
                return nullptr;
            }
            return std::dynamic_pointer_cast<ResourceType>(assignedUavs.at(regIndex).pResource);

        case ProgramReflection::ShaderAccess::Read:
            if (assignedSrvs.find(regIndex) == assignedSrvs.end())
            {
                logWarning("ProgramVars::" + funcName + " - variable \"" + varName + "\' was not found in SRVs. Shader Access = " + to_string(shaderAccess));
                return nullptr;
            }
            return std::dynamic_pointer_cast<ResourceType>(assignedSrvs.at(regIndex).pResource);

        default:
            should_not_get_here();
        }

        return nullptr;
    }

    template<typename ResourceType>
    typename ResourceType::SharedPtr getResourceFromSrvUavCommon(const ProgramReflection::Resource *pDesc, const ProgramVars::ResourceMap<ShaderResourceView>& assignedSrvs, const ProgramVars::ResourceMap<UnorderedAccessView>& assignedUavs, const std::string& varName, const std::string& funcName)
    {
        return getResourceFromSrvUavCommon<ResourceType>(pDesc->regIndex, pDesc->shaderAccess, assignedSrvs, assignedUavs, varName, funcName);
    }

    template<typename ResourceType>
    typename ResourceType::SharedPtr getResourceFromSrvUavCommon(const ProgramReflection::BufferReflection *pBufDesc, const ProgramVars::ResourceMap<ShaderResourceView>& assignedSrvs, const ProgramVars::ResourceMap<UnorderedAccessView>& assignedUavs, const std::string& varName, const std::string& funcName)
    {
        return getResourceFromSrvUavCommon<ResourceType>(pBufDesc->getRegisterIndex(), pBufDesc->getShaderAccess(), assignedSrvs, assignedUavs, varName, funcName);
    }

    Buffer::SharedPtr ProgramVars::getRawBuffer(const std::string& name) const
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyBufferResourceDesc(pDesc, name, ProgramReflection::Resource::ResourceType::RawBuffer, ProgramReflection::Resource::Dimensions::Unknown, "getRawBuffer()") == false)
        {
            return false;
        }

        return getResourceFromSrvUavCommon<Buffer>(pDesc, mAssignedSrvs, mAssignedUavs, name, "getRawBuffer()");
    }

    TypedBufferBase::SharedPtr ProgramVars::getTypedBuffer(const std::string& name) const
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyBufferResourceDesc(pDesc, name, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::Resource::Dimensions::Buffer, "getTypedBuffer()") == false)
        {
            return false;
        }

        return getResourceFromSrvUavCommon<TypedBufferBase>(pDesc, mAssignedSrvs, mAssignedUavs, name, "getTypedBuffer()");
    }

    StructuredBuffer::SharedPtr ProgramVars::getStructuredBuffer(const std::string& name) const
    {
        const ProgramReflection::BufferReflection* pBufDesc = mpReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Structured).get();

        if (pBufDesc == nullptr)
        {
            logWarning("Structured buffer \"" + name + "\" was not found. Ignoring getStructuredBuffer() call.");
            return false;
        }
        
        return getResourceFromSrvUavCommon<StructuredBuffer>(pBufDesc, mAssignedSrvs, mAssignedUavs, name, "getStructuredBuffer()");
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
        auto& it = mAssignedSamplers.at(index);
        if(it.pSampler != pSampler)
        {
            it.pSampler = pSampler;
            it.pDescSet = nullptr;
        }
        return true;
    }

    bool ProgramVars::setSampler(const std::string& name, const Sampler::SharedPtr& pSampler)
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
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Sampler, ProgramReflection::ShaderAccess::Read, name, "getSampler()") == false)
        {
            return nullptr;
        }

        return getSampler(pDesc->regIndex);
    }

    Sampler::SharedPtr ProgramVars::getSampler(uint32_t index) const
    {
        auto it = mAssignedSamplers.find(index);
        if (it == mAssignedSamplers.end())
        {
            logWarning("ProgramVars::getSampler() - Cannot find sampler at index " + index);
            return nullptr;
        }

        return it->second.pSampler;
    }

    ShaderResourceView::SharedPtr ProgramVars::getSrv(uint32_t index) const
    {
        auto it = mAssignedSrvs.find(index);
        if (it == mAssignedSrvs.end())
        {
            logWarning("ProgramVars::getSrv() - Cannot find SRV at index " + index);
            return nullptr;
        }

        return it->second.pView;
    }

    UnorderedAccessView::SharedPtr ProgramVars::getUav(uint32_t index) const
    {
        auto it = mAssignedUavs.find(index);
        if (it == mAssignedUavs.end())
        {
            logWarning("ProgramVars::getUav() - Cannot find UAV at index " + index);
            return nullptr;
        }

        return it->second.pView;
    }

    bool ProgramVars::setTexture(const std::string& name, const Texture::SharedPtr& pTexture)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::ShaderAccess::Undefined, name, "setTexture()") == false)
        {
            return false;
        }

        setResourceSrvUavCommon(pDesc, pTexture, mAssignedSrvs, mAssignedUavs);

        return true;
    }

    Texture::SharedPtr ProgramVars::getTexture(const std::string& name) const
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::ShaderAccess::Undefined, name, "getTexture()") == false)
        {
            return nullptr;
        }

        return getResourceFromSrvUavCommon<Texture>(pDesc, mAssignedSrvs, mAssignedUavs, name, "getTexture()");
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
        auto it = mAssignedSrvs.find(index);
        if (it != mAssignedSrvs.end())
        {
            if(it->second.pView != pSrv)
            {
                it->second.pDescSet = nullptr;
                it->second.pView = pSrv;
                it->second.pResource = getResourceFromView(pSrv.get()); // TODO: Fix resource/view const-ness so we don't need to do this
            }
        }
        else
        {
            logWarning("Can't find SRV with index " + std::to_string(index) + ". Ignoring call to ProgramVars::setSrv()");
            return false;
        }

        return true;
    }

    bool ProgramVars::setUav(uint32_t index, const UnorderedAccessView::SharedPtr& pUav)
    {
        auto it = mAssignedUavs.find(index);
        if (it != mAssignedUavs.end())
        {
            if(it->second.pView != pUav)
            {
                it->second.pDescSet = nullptr;
                it->second.pView = pUav;                
                it->second.pResource = getResourceFromView(pUav.get()); // TODO: Fix resource/view const-ness so we don't need to do this
            }
        }
        else
        {
            logWarning("Can't find UAV with index " + std::to_string(index) + ". Ignoring call to ProgramVars::setUav()");
            return false;
        }

        return true;
    }

    template<typename ViewType, bool isUav, bool forGraphics>
    void bindUavSrvCommon(CopyContext* pContext, const ProgramVars::ResourceMap<ViewType>& resMap)
    {
        ID3D12GraphicsCommandList* pList = pContext->getLowLevelData()->getCommandList();
        for (auto& resIt : resMap)
        {
            auto& resDesc = resIt.second;
            uint32_t rootIndex = resDesc.rootIndex;
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

                    if (isUav && pStructured->hasUAVCounter())
                    {
                        pContext->resourceBarrier(pStructured->getUAVCounter().get(), Resource::State::UnorderedAccess);
                    }
                }

                pContext->resourceBarrier(resDesc.pResource.get(), isUav ? Resource::State::UnorderedAccess : Resource::State::ShaderResource);
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

            // Allocate a GPU descriptor
            if (resDesc.pDescSet == nullptr)
            {
                DescriptorSet::Layout layout;
                layout.addRange(isUav ? DescriptorSet::Type::Uav : DescriptorSet::Type::Srv, 1);
                resDesc.pDescSet = DescriptorSet::create(gpDevice->getGpuDescriptorPool(), layout);
                auto srcHandle = handle->getCpuHandle(0);
                auto dstHandle = resDesc.pDescSet->getCpuHandle(0);
                gpDevice->getApiHandle()->CopyDescriptorsSimple(1, dstHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

            auto viewHandle = resDesc.pDescSet->getGpuHandle(0);
            if(forGraphics)
            {
                pList->SetGraphicsRootDescriptorTable(rootIndex, viewHandle);
            }
            else
            {
                pList->SetComputeRootDescriptorTable(rootIndex, viewHandle);
            }
        }
    }

    template<bool forGraphics>
    void applyProgramVarsCommon(const ProgramVars* pVars, CopyContext* pContext)
    {
        ID3D12GraphicsCommandList* pList = pContext->getLowLevelData()->getCommandList();

        if(forGraphics)
        {
            pList->SetGraphicsRootSignature(pVars->getRootSignature()->getApiHandle());
        }
        else
        {
            pList->SetComputeRootSignature(pVars->getRootSignature()->getApiHandle());
        }

        // Bind the constant-buffers
        for (auto& bufIt : pVars->getAssignedCbs())
        {
            uint32_t rootIndex = bufIt.second.rootIndex;
            const ConstantBuffer* pCB = dynamic_cast<const ConstantBuffer*>(bufIt.second.pResource.get());
            pCB->uploadToGPU();
            if(forGraphics)
            {
                pList->SetGraphicsRootConstantBufferView(rootIndex, pCB->getGpuAddress());
            }
            else
            {
                pList->SetComputeRootConstantBufferView(rootIndex, pCB->getGpuAddress());
            }
        }

        // Bind the SRVs and UAVs
        bindUavSrvCommon<ShaderResourceView, false, forGraphics>(pContext, pVars->getAssignedSrvs());
        bindUavSrvCommon<UnorderedAccessView, true, forGraphics>(pContext, pVars->getAssignedUavs());

        // Bind the samplers
        for (auto& samplerIt : pVars->getAssignedSamplers())
        {
            uint32_t rootIndex = samplerIt.second.rootIndex;
            const Sampler* pSampler = samplerIt.second.pSampler.get();
            if (pSampler == nullptr)
            {
                pSampler = Sampler::getDefault().get();
            }

            // Allocate a GPU descriptor
            if (samplerIt.second.pDescSet == nullptr)
            {
                DescriptorSet::Layout layout;
                layout.addRange(DescriptorSet::Type::Sampler, 1);
                samplerIt.second.pDescSet = DescriptorSet::create(gpDevice->getGpuDescriptorPool(), layout);
                auto srcHandle = pSampler->getApiHandle()->getCpuHandle(0);
                auto dstHandle = samplerIt.second.pDescSet->getCpuHandle(0);
                gpDevice->getApiHandle()->CopyDescriptorsSimple(1, dstHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }

            auto samplerHandler = samplerIt.second.pDescSet->getGpuHandle(0);

            if (forGraphics)
            {
                pList->SetGraphicsRootDescriptorTable(rootIndex, samplerHandler);
            }
            else
            {
                pList->SetComputeRootDescriptorTable(rootIndex, samplerHandler);
            }
        }
    }

    void ComputeVars::apply(ComputeContext* pContext) const
    {
        applyProgramVarsCommon<false>(this, pContext);
    }

    void GraphicsVars::apply(RenderContext* pContext) const
    {
        applyProgramVarsCommon<true>(this, pContext);
    }
}