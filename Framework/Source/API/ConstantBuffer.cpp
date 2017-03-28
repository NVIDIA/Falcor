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
#include "ConstantBuffer.h"
#include "ProgramVersion.h"
#include "buffer.h"
#include "glm/glm.hpp"
#include "texture.h"
#include "API/ProgramReflection.h"
#include "API/Device.h"

namespace Falcor
{
    ConstantBuffer::ConstantBuffer(const ProgramReflection::BufferTypeReflection::SharedConstPtr& pReflector, size_t size) :
        VariablesBuffer(pReflector, size, 1, Buffer::BindFlags::Constant, Buffer::CpuAccess::Write)
    {
    }

    ConstantBuffer::SharedPtr ConstantBuffer::create(const ProgramReflection::BufferTypeReflection::SharedConstPtr& pReflector, size_t overrideSize)
    {
        size_t size = (overrideSize == 0) ? pReflector->getRequiredSize() : overrideSize;        
        SharedPtr pBuffer = SharedPtr(new ConstantBuffer(pReflector, size));
        return pBuffer;
    }

    ConstantBuffer::SharedPtr ConstantBuffer::create(Program::SharedPtr& pProgram, const std::string& name, size_t overrideSize)
    {
        auto& pProgReflector = pProgram->getActiveVersion()->getReflector();
        auto& pBufferReflector = pProgReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Constant);
        if (pBufferReflector)
        {
            return create(pBufferReflector, overrideSize);
        }
        else
        {
            logError("Can't find a constant buffer named \"" + name + "\" in the program");
        }
        return nullptr;
    }

    void ConstantBuffer::uploadToGPU(size_t offset, size_t size) const
    {
        VariablesBuffer::uploadToGPU(offset, size);
        mCBV = nullptr;
    }

    DescriptorHeap::Entry ConstantBuffer::getCBV() const
    {
        if (mCBV == nullptr)
        {
            DescriptorHeap* pHeap = gpDevice->getCpuSrvDescriptorHeap().get();

            mCBV = pHeap->allocateEntry();
            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
            viewDesc.BufferLocation = getGpuAddress();
            viewDesc.SizeInBytes = (uint32_t)getSize();
            gpDevice->getApiHandle()->CreateConstantBufferView(&viewDesc, mCBV->getCpuHandle());
        }

        return mCBV;
    }

    //

    ComponentInstance::SharedPtr ComponentInstance::create(const ProgramReflection::ComponentClassReflection::SharedPtr& pReflector)
    {
        auto componentInstance = SharedPtr(new ComponentInstance());

        componentInstance->mSamplerTableDirty = true;
        componentInstance->mResourceTableDirty = true;

        componentInstance->mpReflector = pReflector;

        uint32_t resourceCount = pReflector->getResourceCount();
        uint32_t samplerCount = pReflector->getSamplerCount();

        componentInstance->mAssignedSRVs.resize(resourceCount);
        componentInstance->mAssignedSRVs.resize(samplerCount);
        // TODO: also allocate space for UAVs here...

        // need to construct the constant buffer, if needed
        if( pReflector->getVariableCount() )
        {
            componentInstance->mConstantBuffer = ConstantBuffer::create(pReflector);
        }

        return componentInstance;
    }

#if 0
        void ComponentInstance::setTexture(const std::string& name, const Texture* pTexture);

        Texture::SharedPtr ComponentInstance::getTexture(std::string const& name);

        void ComponentInstance::setSampler(uint32_t index, Sampler::SharedPtr const& pSampler);
        void ComponentInstance::setSampler(const std::string& name, const Sampler* pSampler);

        Sampler::SharedPtr ComponentInstance::getSampler(uint32_t index);
        Sampler::SharedPtr ComponentInstance::getSampler(const std::string& name);

        void ComponentInstance::setResource(std::string const& name, Resource::SharedPtr pResource);
#endif
























    ComponentInstance::ApiHandle const& ComponentInstance::getApiHandle() const
    {
        auto device = gpDevice;

        if( mResourceTableDirty )
        {
            uint32_t resourceCount = (uint32_t) mAssignedSRVs.size();
            if(mConstantBuffer)
                resourceCount++;

            if( resourceCount )
            {
                // TODO(tfoley): we know the size of allocation we will make ahead of time,
                // which means that the "reflector" can go ahead and cache a pointer to
                // the proper allocation pool in the heap and avoid the lookup step
                // that we do here...
                mApiHandle.resourceDescriptorTable = gpDevice->getShaderSrvDescriptorHeap()->allocateTable(resourceCount);

                // now copy things in!

                uint32_t srvIndex = 0;

                if( mConstantBuffer )
                {
                    // SPIRE: NOTE: The call to `getCBV()` instead of `getSRV()` on the constant
                    // buffer is important!

                    device->copyDescriptor(
                        mApiHandle.resourceDescriptorTable->getCpuHandle(srvIndex++),
                        mConstantBuffer->getCBV()->getCpuHandle(),
                        DescriptorHeap::Type::SRV);
                }

                for( auto& entry : mAssignedSRVs )
                {
                    auto& srv = entry.pView;

                    device->copyDescriptor(
                        mApiHandle.resourceDescriptorTable->getCpuHandle(srvIndex++),
                        srv->getApiHandle()->getCpuHandle(),
                        DescriptorHeap::Type::SRV);
                }
            }

            mResourceTableDirty = false;
        }

        if( mSamplerTableDirty )
        {
            uint32_t samplerCount = (uint32_t) mAssignedSamplers.size();
            if( samplerCount )
            {
                mApiHandle.samplerDescriptorTable = gpDevice->getShaderSamplerDescriptorHeap()->allocateTable(samplerCount);

                uint32_t samplerIndex = 0;

                for( auto& sampler : mAssignedSamplers )
                {
                    device->copyDescriptor(
                        mApiHandle.samplerDescriptorTable->getCpuHandle(samplerIndex++),
                        sampler->getApiHandle()->getCpuHandle(),
                        DescriptorHeap::Type::Sampler);
                }
            }

            mSamplerTableDirty = false;
        }

        return mApiHandle;
    }

}