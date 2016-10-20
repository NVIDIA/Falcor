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
    // D3D12_FIX. Need to correctly abstract the class so that it doesn't depend on the low-level objects
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
            if(bufferMap[regIndex].rootSigOffset == -1)
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
        initializeBuffersMap<ShaderStorageBuffer, RootSignature::DescType::UAV>(mSSBO, createBuffers, mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::UnorderedAccess), mpRootSignature.get());

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
                mAssignedSrvs[desc.regIndex].pResource = nullptr;
                mAssignedSrvs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::SRV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            case ProgramReflection::Resource::ResourceType::UAV:
                mAssignedUavs[desc.regIndex].pResource = nullptr;
                mAssignedUavs[desc.regIndex].rootSigOffset = findRootSignatureOffset<RootSignature::DescType::UAV>(mpRootSignature.get(), desc.regIndex, desc.registerSpace);
                break;
            default:
                break;
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
        if(it == bufferMap.end())
        {
            return nullptr;
        }
        return it->second.pResource;
    }

    template<typename BufferClass, ProgramReflection::BufferReflection::Type bufferType>
    typename BufferClass::SharedPtr getBufferCommon(const std::string& name, const ProgramReflection* pReflector, const ProgramVars::ResourceDataMap<typename BufferClass::SharedPtr>& bufferMap)
    {
        uint32_t bindLocation = pReflector->getBufferBinding(name);

        if(bindLocation == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Error, "Can't find buffer named \"" + name + "\"");
            return nullptr;
        }

        auto& pDesc = pReflector->getBufferDesc(name, bufferType);

        if(pDesc->getType() != bufferType)
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

    ShaderStorageBuffer::SharedPtr ProgramVars::getShaderStorageBuffer(const std::string& name) const
    {
        return getBufferCommon<ShaderStorageBuffer, ProgramReflection::BufferReflection::Type::UnorderedAccess>(name, mpReflector.get(), mSSBO);
    }

    ShaderStorageBuffer::SharedPtr ProgramVars::getShaderStorageBuffer(uint32_t index) const
    {
        return getBufferCommon<ShaderStorageBuffer>(index, mSSBO);
    }

    ErrorCode ProgramVars::attachConstantBuffer(uint32_t index, const ConstantBuffer::SharedPtr& pCB)
    {
        // Check that the index is valid
        if(mConstantBuffers.find(index) == mConstantBuffers.end())
        {
            Logger::log(Logger::Level::Warning, "No constant buffer was found at index " + std::to_string(index) + ". Ignoring attachConstantBuffer() call.");
            return ErrorCode::NotFound;
        }

        // Just need to make sure the buffer is large enough
        const auto& desc = mpReflector->getBufferDesc(index, ProgramReflection::BufferReflection::Type::Constant);
        if(desc->getRequiredSize() > pCB->getBuffer()->getSize())
        {
            Logger::log(Logger::Level::Error, "Can't attach the constant-buffer. Size mismatch.");
            return ErrorCode::SizeMismatch;
        }

        mConstantBuffers[index].pResource = pCB;
        return ErrorCode::None;
    }

    ErrorCode ProgramVars::attachConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB)
    {
        // Find the buffer
        uint32_t loc = mpReflector->getBufferBinding(name);
        if(loc == ProgramReflection::kInvalidLocation)
        {
            Logger::log(Logger::Level::Warning, "Constant buffer \"" + name + "\" was not found. Ignoring attachConstantBuffer() call.");
            return ErrorCode::NotFound;
        }

        return attachConstantBuffer(loc, pCB);
    }

    ErrorCode ProgramVars::setTexture(uint32_t index, const Texture::SharedConstPtr& pTexture)
    {
        mAssignedSrvs[index].pResource = pTexture;
        return ErrorCode::None;
    }

    ErrorCode ProgramVars::setTexture(const std::string& name, const Texture::SharedConstPtr& pTexture)
    {
        const ProgramReflection::Resource* pDesc = mpReflector->getResourceDesc(name);
        if (pDesc == nullptr)
        {
            Logger::log(Logger::Level::Warning, "Texture \"" + name + "\" was not found. Ignoring setTexture() call.");
            return ErrorCode::NotFound;
        }

        if (pDesc->type != ProgramReflection::Resource::ResourceType::Texture)
        {
            Logger::log(Logger::Level::Warning, "ProgramVars::setTexture() was called, but variable \"" + name + "\" is not a texture. Ignoring call.");
            return ErrorCode::TypeMismatch;
        }

        return setTexture(pDesc->regIndex, pTexture);
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

        // Bind the SRVs
        for (auto& resIt : mAssignedSrvs)
        {
            uint32_t rootOffset = resIt.second.rootSigOffset;
            const Texture* pTex = resIt.second.pResource.get();
            pList->SetGraphicsRootDescriptorTable(rootOffset, pTex->getShaderResourceView());
        }
    }
}