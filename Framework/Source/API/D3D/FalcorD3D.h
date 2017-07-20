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
#define NOMINMAX
#include <d3dcompiler.h>
#include "API/Formats.h"
#include <comdef.h>
#include <dxgi1_4.h>
#include <dxgiformat.h>

#ifndef FALCOR_D3D
#define FALCOR_D3D
#endif

#define MAKE_SMART_COM_PTR(_a) _COM_SMARTPTR_TYPEDEF(_a, __uuidof(_a))

__forceinline BOOL dxBool(bool b) { return b ? TRUE : FALSE; }

#ifdef _LOG_ENABLED
#define d3d_call(a) {HRESULT hr_ = a; if(FAILED(hr_)) { d3dTraceHR( #a, hr_); }}
#else
#define d3d_call(a) a
#endif

#ifdef FALCOR_D3D11
#include "D3D11/FalcorD3D11.h"
#endif

#ifdef FALCOR_D3D12
#include "D3D12/FalcorD3D12.h"
#endif

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

#define UNSUPPORTED_IN_D3D(msg_) {logWarning(msg_ + std::string(" is not supported in D3D. Ignoring call."));}

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    struct DxgiFormatDesc
    {
        ResourceFormat falcorFormat;
        DXGI_FORMAT dxgiFormat;
    };

    extern const DxgiFormatDesc kDxgiFormatDesc[];

    /** Get the DXGI format
    */
    inline DXGI_FORMAT getDxgiFormat(ResourceFormat format)
    {
        assert(kDxgiFormatDesc[(uint32_t)format].falcorFormat == format);
        return kDxgiFormatDesc[(uint32_t)format].dxgiFormat;
    }

    inline DXGI_FORMAT getTypelessFormatFromDepthFormat(ResourceFormat format)
    {
        switch (format)
        {
        case ResourceFormat::D16Unorm:
            return DXGI_FORMAT_R16_TYPELESS;
        case ResourceFormat::D32FloatS8X24:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case ResourceFormat::D24UnormS8:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case ResourceFormat::D32Float:
            return DXGI_FORMAT_R32_TYPELESS;
        default:
            assert(isDepthFormat(format) == false);
            return kDxgiFormatDesc[(uint32_t)format].dxgiFormat;
        }
    }

    /** Get D3D_FEATURE_LEVEL
    */
    D3D_FEATURE_LEVEL getD3DFeatureLevel(uint32_t majorVersion, uint32_t minorVersion);

    /** Log a message if hr indicates an error
    */
    void d3dTraceHR(const std::string& Msg, HRESULT hr);

    // DXGI
    MAKE_SMART_COM_PTR(IDXGISwapChain3);
    MAKE_SMART_COM_PTR(IDXGIDevice);
    MAKE_SMART_COM_PTR(IDXGIAdapter1);
    MAKE_SMART_COM_PTR(IDXGIFactory4);
    MAKE_SMART_COM_PTR(ID3DBlob);
    /*! @} */

    template<typename BlobType>
    inline std::string convertBlobToString(BlobType* pBlob)
    {
        std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
        memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
        infoLog[pBlob->GetBufferSize()] = 0;
        return std::string(infoLog.data());
    }

#define appendShaderExtension(_a)  _a ".hlsl"
}