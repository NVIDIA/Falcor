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
#include <vector>
#include "API/Shader.h"

namespace Falcor
{
    struct ShaderData
    {
        ID3DBlobPtr pBlob;
    };

    const char* getTargetString(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::Vertex:
            return "vs_5_0";
        case ShaderType::Pixel:
            return "ps_5_0";
        case ShaderType::Hull:
            return "hs_5_0";
        case ShaderType::Domain:
            return "ds_5_0";
        case ShaderType::Geometry:
            return "gs_5_0";
        case ShaderType::Compute:
            return "cs_5_0";
        default:
            should_not_get_here();
            return "";
        }
    }

    struct SlangBlob : ID3DBlob
    {
        void* buffer;
        size_t bufferSize;
        size_t refCount;

        SlangBlob(void* buffer, size_t bufferSize)
            : buffer(buffer)
            , bufferSize(bufferSize)
            , refCount(1)
        {}

        // IUnknown

        virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
        {
            *ppvObject = this;
            return S_OK;
        }

        virtual ULONG STDMETHODCALLTYPE AddRef( void) override
        {
            ++refCount;
            return (ULONG) refCount;
        }

        virtual ULONG STDMETHODCALLTYPE Release( void) override
        {
            --refCount;
            if(refCount == 0)
            {
                delete this;
            }
            return (ULONG) refCount;
        }

        // ID3DBlob

        virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() override
        {
            return buffer;
        }

        virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() override
        {
            return bufferSize;
        }
    };

    ID3DBlobPtr Shader::compile(const Blob& blob, const std::string& entryPointName, std::string& errorLog)
    {
        ID3DBlob* pCode;
        ID3DBlobPtr pErrors;

        UINT flags = D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG;
#endif
        flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        HRESULT hr = D3DCompile(
            blob.data.data(),
            blob.data.size(),
            nullptr,
            nullptr,
            nullptr,
            entryPointName.c_str(),
            getTargetString(mType),
            flags,
            0,
            &pCode,
            &pErrors);
        if(FAILED(hr))
        {
            errorLog = convertBlobToString(pErrors.GetInterfacePtr());
            return nullptr;
        }

        return pCode;
    }

    Shader::Shader(ShaderType type) : mType(type)
    {
        mpPrivateData = new ShaderData;
    }

    Shader::~Shader()
    {
        ShaderData* pData = (ShaderData*)mpPrivateData;
        safe_delete(pData);
    }

    bool Shader::init(const Blob& shaderBlob, const std::string& entryPointName, std::string& log)
    {
        if (shaderBlob.type != Blob::Type::String)
        {
            logError("D3D shader compilation only supports string inputs");
            return false;
        }
        // Compile the shader
        ShaderData* pData = (ShaderData*)mpPrivateData;
        pData->pBlob = compile(shaderBlob, entryPointName, log);

        if (pData->pBlob == nullptr)
        {
            return nullptr;
        }

#ifdef FALCOR_D3D11
        // create the shader object
        switch (type)
        {
        case ShaderType::Vertex:
            pData->pHandle = createVertexShader(pData->pBlob);
            break;
        case ShaderType::Pixel:
            pData->pHandle = createPixelShader(pData->pBlob);
            break;
        case ShaderType::Hull:
            pData->pHandle = createHullShader(pData->pBlob);
            break;
        case ShaderType::Domain:
            pData->pHandle = createDomainShader(pData->pBlob);
            break;
        case ShaderType::Geometry:
            pData->pHandle = createGeometryShader(pData->pBlob);
            break;
        case ShaderType::Compute:
            pData->pHandle = createComputeShader(pData->pBlob);
            break;
        default:
            should_not_get_here();
            return nullptr;
        }

        if (pData->pHandle == nullptr)
        {
            return false;
        }
#elif defined FALCOR_D3D12
        mApiHandle = { pData->pBlob->GetBufferPointer(), pData->pBlob->GetBufferSize() };
#endif

        return true;
    }

    ID3DBlobPtr Shader::getD3DBlob() const
    {
        const ShaderData* pData = (ShaderData*)mpPrivateData;
        return pData->pBlob;
    }
}
