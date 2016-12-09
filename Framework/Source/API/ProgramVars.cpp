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
#include "API/RenderContext.h"

namespace Falcor
{
    // FIXME D3D12. Need to correctly abstract the class so that it doesn't depend on the low-level objects
    template<RootSignature::DescType descType>
    uint32_t findRootSignatureOffset(const RootSignature* pRootSig, uint32_t regIndex, uint32_t regSpace)
    {
        // Find the bind-index in the root descriptor. Views
        // FIXME: Add support for descriptor tables
        // OPTME: This can be more efficient

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

        // Search the desciptor-tables
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

    template<typename BufferType, RootSignature::DescType descType>
    bool initializeBuffersMap(ProgramVars::ResourceDataMap& bufferMap, bool createBuffers, const ProgramReflection::BufferMap& reflectionMap, ProgramReflection::BufferReflection::ShaderAccess shaderAccess, const RootSignature* pRootSig)
    {
        for (auto& buf : reflectionMap)
        {
            const ProgramReflection::BufferReflection* pReflector = buf.second.get();
            if(pReflector->getShaderAccess() == shaderAccess)
            {
                uint32_t regIndex = pReflector->getRegisterIndex();
                uint32_t regSpace = pReflector->getRegisterSpace();

                // Only create the buffer if needed
                bufferMap[regIndex].pResource = createBuffers ? BufferType::create(buf.second) : nullptr;
                bufferMap[regIndex].rootSigOffset = findRootSignatureOffset<descType>(pRootSig, regIndex, regSpace);
                if (bufferMap[regIndex].rootSigOffset == -1)
                {
                    logError("Can't find a root-signature information matching buffer '" + pReflector->getName() + " when creating ProgramVars");
                    return false;
                }
            }
        }
        return true;
    }

    ProgramVars::ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig) : mpReflector(pReflector)
    {
        // Initialize the CB and SSBO maps. We always do it, to mark which slots are used in the shader.
        mpRootSignature = pRootSig ? pRootSig : RootSignature::create(pReflector.get());
        initializeBuffersMap<ConstantBuffer, RootSignature::DescType::CBV>(mAssignedCbs, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Constant), ProgramReflection::BufferReflection::ShaderAccess::Read, mpRootSignature.get());
        initializeBuffersMap<StructuredBuffer, RootSignature::DescType::SRV>(mAssignedSrvs, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Structured), ProgramReflection::BufferReflection::ShaderAccess::Read, mpRootSignature.get());
        initializeBuffersMap<StructuredBuffer, RootSignature::DescType::SRV>(mAssignedSrvs, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Structured), ProgramReflection::BufferReflection::ShaderAccess::ReadWrite, mpRootSignature.get());

        // Initialize the textures and samplers map
        for (const auto& res : pReflector->getResourceMap())
        {
            const auto& desc = res.second;
            switch (desc.type)
            {
            case ProgramReflection::Resource::ResourceType::Sampler:
                mAssignedSamplers[desc.regIndex].pResource = nullptr;
                mAssignedSamplers[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::Sampler>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::Texture:
            case ProgramReflection::Resource::ResourceType::RawBuffer:
                if (desc.shaderAccess == ProgramReflection::Resource::ShaderAccess::Read)
                {
                    assert(mAssignedSrvs.find(desc.regIndex) == mAssignedSrvs.end());
                    mAssignedSrvs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::SRV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                }
                else
                {
                    assert(mAssignedUavs.find(desc.regIndex) == mAssignedUavs.end());
                    assert(desc.shaderAccess == ProgramReflection::Resource::ShaderAccess::ReadWrite);
                    mAssignedUavs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::UAV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                }
                break;
            default:
                should_not_get_here();
            }
        }
    }

    GraphicsVars::SharedPtr GraphicsVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig)
    {
        return SharedPtr(new GraphicsVars(pReflector, createBuffers, pRootSig));
    }

    ComputeVars::SharedPtr ComputeVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig)
    {
        return SharedPtr(new ComputeVars(pReflector, createBuffers, pRootSig));
    }

    template<typename BufferClass>
    typename BufferClass::SharedPtr getBufferCommon(uint32_t index, const ProgramVars::ResourceDataMap& bufferMap)
    {
        auto& it = bufferMap.find(index);
        if (it == bufferMap.end())
        {
            return nullptr;
        }

        return std::dynamic_pointer_cast<BufferClass>(it->second.pResource);
    }

    template<typename BufferClass, ProgramReflection::BufferReflection::Type bufferType>
    typename BufferClass::SharedPtr getBufferCommon(const std::string& name, const ProgramReflection* pReflector, const ProgramVars::ResourceDataMap& bufferMap)
    {
        uint32_t bindLocation = pReflector->getBufferBinding(name);

        if (bindLocation == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Error, "Can't find buffer named \"" + name + "\"");
            return nullptr;
        }

        auto& pDesc = pReflector->getBufferDesc(name, bufferType);

        if (pDesc->getType() != bufferType)
        {
            Logger::log(Logger::Level::Error, "Buffer \"" + name + "\" is a " + to_string(pDesc->getType()) + " buffer, while requesting for " + to_string(bufferType) + " buffer");
            return nullptr;
        }

        return getBufferCommon<BufferClass>(bindLocation, bufferMap);
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(const std::string& name) const
    {
        return getBufferCommon<ConstantBuffer, ProgramReflection::BufferReflection::Type::Constant>(name, mpReflector.get(), mAssignedCbs);
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(uint32_t index) const
    {
        return getBufferCommon<ConstantBuffer>(index, mAssignedCbs);
    }

    StructuredBuffer::SharedPtr ProgramVars::getStructuredBuffer(const std::string& name) const
    {
        // Look in the SRVs
        StructuredBuffer::SharedPtr pBuffer = getBufferCommon<StructuredBuffer, ProgramReflection::BufferReflection::Type::Structured>(name, mpReflector.get(), mAssignedSrvs);
        if (pBuffer == nullptr)
        {
            pBuffer = getBufferCommon<StructuredBuffer, ProgramReflection::BufferReflection::Type::Structured>(name, mpReflector.get(), mAssignedUavs);
        }

        return pBuffer;
    }

    StructuredBuffer::SharedPtr ProgramVars::getStructuredBuffer(uint32_t index) const
    {
        StructuredBuffer::SharedPtr pBuffer = getBufferCommon<StructuredBuffer>(index, mAssignedSrvs);
        if (pBuffer == nullptr)
        {
            pBuffer = getBufferCommon<StructuredBuffer>(index, mAssignedUavs);
        }
        return pBuffer;
    }

    bool ProgramVars::setConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB)
    {
        // Check that the index is valid
        if (mAssignedCbs.find(index) == mAssignedCbs.end())
        {
            Logger::log(Logger::Level::Warning, "No constant buffer was found at index " + std::to_string(index) + ". Ignoring attachConstantBuffer() call.");
            return false;
        }

        // Just need to make sure the buffer is large enough
        const auto& desc = mpReflector->getBufferDesc(index, ProgramReflection::BufferReflection::Type::Constant);
        if (desc->getRequiredSize() > pCB->getSize())
        {
            Logger::log(Logger::Level::Error, "Can't attach the constant-buffer. Size mismatch.");
            return false;
        }

        assert(mAssignedCbs.find(index) != mAssignedCbs.end());
        mAssignedCbs[index].pResource = pCB;
        return true;
    }

    bool ProgramVars::setConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB)
    {
        // Find the buffer
        uint32_t loc = mpReflector->getBufferBinding(name);
        if (loc == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Warning, "Constant buffer \"" + name + "\" was not found. Ignoring attachConstantBuffer() call.");
            return false;
        }

        return setConstantBuffer(loc, pCB);
    }

    bool ProgramVars::setRawBuffer(const std::string& name, Buffer::SharedPtr pBuf)
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (pDesc == nullptr)
        {
            Logger::log(Logger::Level::Warning, "Raw buffer \"" + name + "\" was not found. Ignoring setRawBuffer() call.");
            return false;
        }

        if (pDesc->type != ProgramReflection::Resource::ResourceType::RawBuffer)
        {
            logWarning("ProgramVars::setTypedBuffer() - variable '" + name + "' is not a raw buffer. VarType = " + to_string(pDesc->type) + ". Ignoring call");
            return false;
        }

        switch (pDesc->shaderAccess)
        {
        case ProgramReflection::Resource::ShaderAccess::ReadWrite:
            mAssignedUavs.at(pDesc->regIndex).pResource = pBuf;
            break;
        case ProgramReflection::Resource::ShaderAccess::Read:
            mAssignedSrvs.at(pDesc->regIndex).pResource = pBuf;
            break;
        default:
            should_not_get_here();
        }

        return true;
    }

    bool ProgramVars::setTypedBuffer(const std::string& name, TypedBufferBase::SharedPtr pBuf)
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (pDesc == nullptr)
        {
            logWarning("Typed buffer \"" + name + "\" was not found. Ignoring setTypedBuffer() call.");
            return false;
        }

        if (pDesc->type != ProgramReflection::Resource::ResourceType::Texture || pDesc->dims != ProgramReflection::Resource::Dimensions::Buffer)
        {
            logWarning("ProgramVars::setTypedBuffer() - variable '" + name + "' is not a typed buffer. VarType = " + to_string(pDesc->type) + ", VarDims = " + to_string(pDesc->dims) + ". Ignoring call");
            return false;
        }

        // A note about what happens here. We don't need any additional information about the TypedBuffer. We just store is as a resource, since in effect it's just a wrapper around the buffer (the views are the same)
        switch (pDesc->shaderAccess)
        {
        case ProgramReflection::Resource::ShaderAccess::ReadWrite:
            mAssignedUavs.at(pDesc->regIndex).pResource = pBuf;
            break;
        case ProgramReflection::Resource::ShaderAccess::Read:
            assert(mAssignedSrvs.find(pDesc->regIndex) != mAssignedSrvs.end());
            mAssignedSrvs.at(pDesc->regIndex).pResource = pBuf;
            break;
        default:
            should_not_get_here();
        }

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

        // A note about what happens here. We don't need any additional information about the TypedBuffer. We just store is as a resource, since in effect it's just a wrapper around the buffer (the views are the same)
        uint32_t regIndex = pBufDesc->getRegisterIndex();
        switch (pBufDesc->getShaderAccess())
        {
        case ProgramReflection::BufferReflection::ShaderAccess::ReadWrite:
            mAssignedUavs.at(regIndex).pResource = pBuf;
            break;
        case ProgramReflection::BufferReflection::ShaderAccess::Read:
            mAssignedSrvs.at(regIndex).pResource = pBuf;
            break;
        default:
            should_not_get_here();
        }

        return true;
    }

    bool verifyResourceDesc(const ProgramReflection::Resource* pDesc, ProgramReflection::Resource::ResourceType type, ProgramReflection::Resource::ShaderAccess access, const std::string& varName, const std::string& funcName)
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

        if (pDesc->shaderAccess != access)
        {
            logWarning("ProgramVars::" + funcName + " was called, but variable \"" + varName + "\" has different shader access type. Expecting + " + to_string(pDesc->shaderAccess) + " but provided resource is " + to_string(access) + ". Ignoring call");
            return false;
        }

        return true;
    }

    bool ProgramVars::setSampler(uint32_t index, const Sampler::SharedPtr& pSampler)
    {
        mAssignedSamplers.at(index).pResource = pSampler;
        return true;
    }

    bool ProgramVars::setSampler(const std::string& name, const Sampler::SharedPtr& pSampler)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Sampler, ProgramReflection::Resource::ShaderAccess::Read, name, "setSampler") == false)
        {
            return false;
        }

        return setSampler(pDesc->regIndex, pSampler);
    }

    bool setUavSrvCommon(const Texture::SharedPtr& pTexture,
        ProgramVars::ResourceDataMap& resMap,
        uint32_t index,
        uint32_t firstArraySlice,
        uint32_t arraySize,
        uint32_t mostDetailedMip,
        uint32_t mipCount,
        const std::string& typeName)
    {
        if (resMap.find(index) != resMap.end())
        {
            auto& resData = resMap[index];
            resData.pResource = pTexture;

            if (pTexture)
            {
                assert(firstArraySlice < pTexture->getArraySize());
                if (arraySize == Texture::kMaxPossible)
                {
                    arraySize = pTexture->getArraySize() - firstArraySlice;
                }
                assert(mostDetailedMip < pTexture->getMipCount());
                if (mipCount == Texture::kMaxPossible)
                {
                    mipCount = pTexture->getMipCount() - mostDetailedMip;
                }
                assert(mostDetailedMip + mipCount <= pTexture->getMipCount());
                assert(firstArraySlice + arraySize <= pTexture->getArraySize());

                resData.arraySize = arraySize;
                resData.firstArraySlice = firstArraySlice;
                resData.mipCount = mipCount;
                resData.mostDetailedMip = mostDetailedMip;
            }
            return true;
        }
        else
        {
            logWarning("Can't find " + typeName + " with index " + std::to_string(index) + ". Ignoring call to ProgramVars::set" + typeName + "()");
            return false;
        }
    }

    bool ProgramVars::setTexture(uint32_t index, const Texture::SharedPtr& pTexture, uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount)
    {
        return setUavSrvCommon(pTexture, mAssignedSrvs, index, firstArraySlice, arraySize, mostDetailedMip, mipCount, "Texture");
    }

    bool ProgramVars::setTexture(const std::string& name, const Texture::SharedPtr& pTexture, uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::Resource::ShaderAccess::Read, name, "setTexture") == false)
        {
            return false;
        }

        return setTexture(pDesc->regIndex, pTexture, firstArraySlice, arraySize, mostDetailedMip, mipCount);
    }

    bool ProgramVars::setUav(uint32_t index, const Texture::SharedPtr& pTexture, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        return setUavSrvCommon(pTexture, mAssignedUavs, index, firstArraySlice, arraySize, mipLevel, 1, "UAV");
    }

    bool ProgramVars::setUav(const std::string& name, const Texture::SharedPtr& pTexture, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Texture, ProgramReflection::Resource::ShaderAccess::ReadWrite, name, "setUav") == false)
        {
            return false;
        }
        return setUav(pDesc->regIndex, pTexture, mipLevel, firstArraySlice, arraySize);
    }

    template<typename HandleType, bool isUav, bool forGraphics>
    void bindUavSrvCommon(RenderContext* pContext, const ProgramVars::ResourceDataMap& resMap)
    {
        ID3D12GraphicsCommandList* pList = pContext->getCommandListApiHandle();
        for (auto& resIt : resMap)
        {
            const auto& resDesc = resIt.second;
            uint32_t rootOffset = resDesc.rootSigOffset;
            const Resource* pResource = resDesc.pResource.get();

            HandleType handle;
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
                    handle = pResource->getUAV(resDesc.mostDetailedMip, resDesc.firstArraySlice, resDesc.arraySize)->getApiHandle();
                }
                else
                {
                    handle = pResource->getSRV(resDesc.mostDetailedMip, resDesc.mipCount, resDesc.firstArraySlice, resDesc.arraySize)->getApiHandle();
                }
            }
            else
            {
                // FIXME D3D12: I was under the assumption I'll need to bind a black texture to get the default values. Need to verify it
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

    template<bool forGraphics>
    void ProgramVars::applyCommon(RenderContext* pContext) const
    {
        // Get the command list
        ID3D12GraphicsCommandList* pList = pContext->getCommandListApiHandle();
        if(forGraphics)
        {
            pList->SetGraphicsRootSignature(mpRootSignature->getApiHandle());
        }
        else
        {
            pList->SetComputeRootSignature(mpRootSignature->getApiHandle());
        }

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
        bindUavSrvCommon<SrvHandle, false, forGraphics>(pContext, mAssignedSrvs);
        bindUavSrvCommon<UavHandle, true, forGraphics>(pContext, mAssignedUavs);

        // Bind the samplers
        for (auto& samplerIt : mAssignedSamplers)
        {
            uint32_t rootOffset = samplerIt.second.rootSigOffset;
            const Sampler* pSampler = samplerIt.second.pResource.get();
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
    }

    void ComputeVars::apply(RenderContext* pContext) const
    {
        applyCommon<false>(pContext);
    }

    void GraphicsVars::apply(RenderContext* pContext) const
    {
        applyCommon<true>(pContext);
    }

    bool ProgramVars::setTextureRange(uint32_t startIndex, uint32_t count, const Texture::SharedPtr pTextures[])
    {
        for (uint32_t i = startIndex; i < startIndex + count; i++)
        {
            setTexture(i, pTextures[i]);
        }
        return true;
    }

    bool ProgramVars::setTextureRange(const std::string& name, uint32_t count, const Texture::SharedPtr pTextures[])
    {
        return true;
    }
}