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
    bool initializeBuffersMap(ProgramVars::ResourceDataMap<typename BufferType::SharedPtr>& bufferMap, bool createBuffers, const ProgramReflection::BufferMap& reflectionMap, const RootSignature* pRootSig)
    {
        for (auto& buf : reflectionMap)
        {
            const ProgramReflection::BufferReflection* pReflector = buf.second.get();
            uint32_t regIndex = pReflector->getRegisterIndex();
            uint32_t regSpace = pReflector->getRegisterSpace();

            // Only create the buffer if needed
            bufferMap[regIndex].pResource = createBuffers ? BufferType::create(buf.second) : nullptr;
            bufferMap[regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::CBV>(pRootSig, regIndex, regSpace);
            if (bufferMap[regIndex].rootSigOffset == -1)
            {
                logError("Can't find a root-signature information matching buffer '" + pReflector->getName() + " when creating ProgramVars");
                return false;
            }

        }
        return true;
    }

    ProgramVars::ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig) : mpReflector(pReflector)
    {
        // Initialize the CB and SSBO maps. We always do it, to mark which slots are used in the shader.
        mpRootSignature = pRootSig ? pRootSig : RootSignature::create(pReflector.get());
        initializeBuffersMap<ConstantBuffer, RootSignature::DescType::CBV>(mConstantBuffers, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Constant), mpRootSignature.get());
        initializeBuffersMap<ShaderStorageBuffer, RootSignature::DescType::UAV>(mStructuredBuffers, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Structured), mpRootSignature.get());

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
            case ProgramReflection::Resource::ResourceType::TextureSrv:
                mAssignedSrvs[desc.regIndex].pResource = nullptr;
                mAssignedSrvs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::SRV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::TextureUav:
                mAssignedUavs[desc.regIndex].pResource = nullptr;
                mAssignedUavs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::UAV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::RawBufferSrv:
                mAssignedBufferSrvs[desc.regIndex].pBuffer = nullptr;
                mAssignedBufferSrvs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::SRV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::RawBufferUav:
                mAssignedBufferUavs[desc.regIndex].pBuffer = nullptr;
                mAssignedBufferUavs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::UAV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            default:
                should_not_get_here();
            }
        }
    }

    ProgramVars::SharedPtr ProgramVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedConstPtr& pRootSig)
    {
        return SharedPtr(new ProgramVars(pReflector, createBuffers, pRootSig));
    }

    template<typename BufferClass>
    typename BufferClass::SharedPtr getBufferCommon(uint32_t index, const ProgramVars::ResourceDataMap<typename BufferClass::SharedPtr>& bufferMap)
    {
        auto& it = bufferMap.find(index);
        if (it == bufferMap.end())
        {
            return nullptr;
        }
        return it->second.pResource;
    }

    template<typename BufferClass, ProgramReflection::BufferReflection::Type bufferType>
    typename BufferClass::SharedPtr getBufferCommon(const std::string& name, const ProgramReflection* pReflector, const ProgramVars::ResourceDataMap<typename BufferClass::SharedPtr>& bufferMap)
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
        return getBufferCommon<ConstantBuffer, ProgramReflection::BufferReflection::Type::Constant>(name, mpReflector.get(), mConstantBuffers);
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(uint32_t index) const
    {
        return getBufferCommon<ConstantBuffer>(index, mConstantBuffers);
    }

    ShaderStorageBuffer::SharedPtr ProgramVars::getStructuredBuffer(const std::string& name) const
    {
        return getBufferCommon<ShaderStorageBuffer, ProgramReflection::BufferReflection::Type::Structured>(name, mpReflector.get(), mStructuredBuffers);
    }

    ShaderStorageBuffer::SharedPtr ProgramVars::getStructuredBuffer(uint32_t index) const
    {
        return getBufferCommon<ShaderStorageBuffer>(index, mStructuredBuffers);
    }

    bool ProgramVars::attachConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB)
    {
        // Check that the index is valid
        if (mConstantBuffers.find(index) == mConstantBuffers.end())
        {
            Logger::log(Logger::Level::Warning, "No constant buffer was found at index " + std::to_string(index) + ". Ignoring attachConstantBuffer() call.");
            return false;
        }

        // Just need to make sure the buffer is large enough
        const auto& desc = mpReflector->getBufferDesc(index, ProgramReflection::BufferReflection::Type::Constant);
        if (desc->getRequiredSize() > pCB->getBuffer()->getSize())
        {
            Logger::log(Logger::Level::Error, "Can't attach the constant-buffer. Size mismatch.");
            return false;
        }

        mConstantBuffers[index].pResource = pCB;
        return true;
    }

    bool ProgramVars::attachConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB)
    {
        // Find the buffer
        uint32_t loc = mpReflector->getBufferBinding(name);
        if (loc == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Warning, "Constant buffer \"" + name + "\" was not found. Ignoring attachConstantBuffer() call.");
            return false;
        }

        return attachConstantBuffer(loc, pCB);
    }

    bool ProgramVars::attachBuffer(const std::string& name, Buffer::SharedPtr pBuf)
    {
        // Find the buffer
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (pDesc == nullptr)
        {
            Logger::log(Logger::Level::Warning, "Constant buffer \"" + name + "\" was not found. Ignoring attachConstantBuffer() call.");
            return false;
        }
        switch (pDesc->type)
        {
        case ProgramReflection::Resource::ResourceType::RawBufferUav:
            mAssignedBufferUavs[pDesc->regIndex].pBuffer = pBuf;
            break;
        case ProgramReflection::Resource::ResourceType::RawBufferSrv:
            mAssignedBufferSrvs[pDesc->regIndex].pBuffer = pBuf;
            break;
        default:
            should_not_get_here();
        }

        return true;
    }

    bool verifyResourceDesc(const ProgramReflection::Resource* pDesc, ProgramReflection::Resource::ResourceType type, const std::string& varName, const std::string& typeName, const std::string& funcName)
    {
        if (pDesc == nullptr)
        {
            logWarning(typeName + " \"" + varName + "\" was not found. Ignoring " + funcName + " call.");
            return false;
        }

        if (pDesc->type != type)
        {
            logWarning("ProgramVars::" + funcName + " was called, but variable \"" + varName + "\" is not a " + typeName + ". Ignoring call.");
            return false;
        }

        return true;
    }

    bool ProgramVars::setSampler(uint32_t index, const Sampler::SharedConstPtr& pSampler)
    {
        mAssignedSamplers[index].pResource = pSampler;
        return true;
    }

    bool ProgramVars::setSampler(const std::string& name, const Sampler::SharedConstPtr& pSampler)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::Sampler, name, "Sampler", "setSampler") == false)
        {
            return false;
        }

        return setSampler(pDesc->regIndex, pSampler);
    }

    bool setUavSrvCommon(const Texture::SharedConstPtr& pTexture,
        std::map<uint32_t, ProgramVars::ResourceData<Texture::SharedConstPtr>>& resMap,
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

    bool ProgramVars::setTexture(uint32_t index, const Texture::SharedConstPtr& pTexture, uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount)
    {
        return setUavSrvCommon(pTexture, mAssignedSrvs, index, firstArraySlice, arraySize, mostDetailedMip, mipCount, "Texture");
    }

    bool ProgramVars::setTexture(const std::string& name, const Texture::SharedConstPtr& pTexture, uint32_t firstArraySlice, uint32_t arraySize, uint32_t mostDetailedMip, uint32_t mipCount)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::TextureSrv, name, "Texture", "setTexture") == false)
        {
            return false;
        }

        return setTexture(pDesc->regIndex, pTexture, firstArraySlice, arraySize, mostDetailedMip, mipCount);
    }

    bool ProgramVars::setUav(uint32_t index, const Texture::SharedConstPtr& pTexture, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        return setUavSrvCommon(pTexture, mAssignedUavs, index, firstArraySlice, arraySize, mipLevel, 1, "UAV");
    }

    bool ProgramVars::setUav(const std::string& name, const Texture::SharedConstPtr& pTexture, uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);

        if (verifyResourceDesc(pDesc, ProgramReflection::Resource::ResourceType::TextureUav, name, "UAV", "setUav") == false)
        {
            return false;
        }
        return setUav(pDesc->regIndex, pTexture, mipLevel, firstArraySlice, arraySize);
    }

    template<typename HandleType, bool isUav>
    void bindTextureUavSrvCommon(RenderContext* pContext, const std::map<uint32_t, ProgramVars::ResourceData<Texture::SharedConstPtr>>& resMap)
    {
        ID3D12GraphicsCommandList* pList = pContext->getCommandListApiHandle();
        for (auto& resIt : resMap)
        {
            const auto& resDesc = resIt.second;
            uint32_t rootOffset = resDesc.rootSigOffset;
            const Texture* pTex = resDesc.pResource.get();
            if (pTex)
            {
                // FIXME D3D12: Handle null textures (should bind a small black texture)
                HandleType handle;
                if (isUav)
                {
                    handle = pTex->getUAV(resDesc.mostDetailedMip, resDesc.firstArraySlice, resDesc.arraySize);
                }
                else
                {
                    handle = pTex->getSRV(resDesc.firstArraySlice, resDesc.arraySize, resDesc.mostDetailedMip, resDesc.mipCount);
                }
                pContext->resourceBarrier(resDesc.pResource.get(), isUav ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                pList->SetGraphicsRootDescriptorTable(rootOffset, handle);
            }
        }
    }

    template<typename HandleType, bool isUav>
    void bindBufferUavSrvCommon(RenderContext* pContext, const std::map<uint32_t, ProgramVars::BufferData>& bufMap)
    {
        ID3D12GraphicsCommandList* pList = pContext->getCommandListApiHandle();
        for (auto& bufIt : bufMap)
        {
            uint32_t rootOffset = bufIt.second.rootSigOffset;
            Buffer* pBuffer = bufIt.second.pBuffer.get();
            // FIXME D3D12: Handle null textures (should bind a small black texture)
            HandleType handle;
            if (isUav)
            {
                handle = pBuffer->getUAV();
            }
            else
            {
                handle = pBuffer->getSRV();
            }
            pList->SetGraphicsRootDescriptorTable(rootOffset, handle);
        }
    }

    void ProgramVars::setIntoRenderContext(RenderContext* pContext) const
    {
        // Get the command list
        ID3D12GraphicsCommandList* pList = pContext->getCommandListApiHandle();
        pList->SetGraphicsRootSignature(mpRootSignature->getApiHandle());

        // Bind the constant-buffers
        for (auto& bufIt : mConstantBuffers)
        {
            uint32_t rootOffset = bufIt.second.rootSigOffset;
            const ConstantBuffer* pCB = bufIt.second.pResource.get();
            pCB->uploadToGPU();
            pList->SetGraphicsRootConstantBufferView(rootOffset, pCB->getBuffer()->getGpuAddress());
        }

        // Bind the SRVs and UAVs
        bindBufferUavSrvCommon<SrvHandle, false>(pContext, mAssignedBufferSrvs);
        bindBufferUavSrvCommon<UavHandle, true>(pContext, mAssignedBufferUavs);
        bindTextureUavSrvCommon<SrvHandle, false>(pContext, mAssignedSrvs);
        bindTextureUavSrvCommon<UavHandle, true>(pContext, mAssignedUavs);

        // Bind the samplers
        for (auto& samplerIt : mAssignedSamplers)
        {
            uint32_t rootOffset = samplerIt.second.rootSigOffset;
            const Sampler* pSampler = samplerIt.second.pResource.get();
            if (pSampler)
            {
                pList->SetGraphicsRootDescriptorTable(rootOffset, pSampler->getApiHandle());
            }
        }
    }

    bool ProgramVars::setTextureRange(uint32_t startIndex, uint32_t count, const Texture::SharedConstPtr pTextures[])
    {
        for (uint32_t i = startIndex; i < startIndex + count; i++)
        {
            setTexture(i, pTextures[i]);
        }
        return true;
    }

    bool ProgramVars::setTextureRange(const std::string& name, uint32_t count, const Texture::SharedConstPtr pTextures[])
    {
        return true;
    }
}