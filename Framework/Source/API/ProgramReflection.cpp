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
#include "ProgramReflection.h"
#include "Externals/slang/slang.h"

using namespace slang;

namespace Falcor {namespace Reflection
{
    static BaseType::SharedPtr reflectType(TypeLayoutReflection* pSlangType);

    NumericType::Type getNumericType(TypeReflection::ScalarType slangScalarType, uint32_t rows, uint32_t columns)
    {
        switch (slangScalarType)
        {
        case TypeReflection::ScalarType::Bool:
            assert(rows == 1);
            switch (columns)
            {
            case 1: return NumericType::Type::Bool;
            case 2: return NumericType::Type::Bool2;
            case 3: return NumericType::Type::Bool3;
            case 4: return NumericType::Type::Bool4;
            }
        case TypeReflection::ScalarType::UInt32:
            assert(rows == 1);
            switch (columns)
            {
            case 1: return NumericType::Type::Uint;
            case 2: return NumericType::Type::Uint2;
            case 3: return NumericType::Type::Uint3;
            case 4: return NumericType::Type::Uint4;
            }
        case TypeReflection::ScalarType::Int32:
            assert(rows == 1);
            switch (columns)
            {
            case 1: return NumericType::Type::Int;
            case 2: return NumericType::Type::Int2;
            case 3: return NumericType::Type::Int3;
            case 4: return NumericType::Type::Int4;
            }
        case TypeReflection::ScalarType::Float32:
            switch (rows)
            {
            case 1:
                switch (columns)
                {
                case 1: return NumericType::Type::Float;
                case 2: return NumericType::Type::Float2;
                case 3: return NumericType::Type::Float3;
                case 4: return NumericType::Type::Float4;
                }
                break;
            case 2:
                switch (columns)
                {
                case 2: return NumericType::Type::Float2x2;
                case 3: return NumericType::Type::Float2x3;
                case 4: return NumericType::Type::Float2x4;
                }
                break;
            case 3:
                switch (columns)
                {
                case 2: return NumericType::Type::Float3x2;
                case 3: return NumericType::Type::Float3x3;
                case 4: return NumericType::Type::Float3x4;
                }
                break;
            case 4:
                switch (columns)
                {
                case 2: return NumericType::Type::Float4x2;
                case 3: return NumericType::Type::Float4x3;
                case 4: return NumericType::Type::Float4x4;
                }
                break;
            }
        }

        should_not_get_here();
        return NumericType::Type::Unknown;
    }

    static BaseType::SharedPtr reflectArray(TypeLayoutReflection* pSlangType)
    {
        uint32_t elementCount = (uint32_t)pSlangType->getElementCount();
        assert(pSlangType->getCategoryCount() == 1);
        uint32_t stride = (uint32_t)pSlangType->getElementStride(pSlangType->getCategoryByIndex(0));   // ASKTIM what is the category?
        TypeLayoutReflection* pBaseType = pSlangType->getElementTypeLayout();
        ArrayType::SharedPtr pArray = ArrayType::create(elementCount, stride, reflectType(pBaseType));
        return pArray;
    }

    static BaseType::SharedPtr reflectConstantBuffer(TypeLayoutReflection* pSlangType)
    {
        BaseType::SharedPtr pBaseType = reflectType(pSlangType->getElementTypeLayout());
        ResourceType::SharedPtr pResource = ResourceType::create(ResourceType::Type::ConstantBuffer, ResourceType::ShaderAccess::Read, pBaseType);
        return pResource;
    }

    static BaseType::SharedPtr reflectNumericType(TypeLayoutReflection* pSlangType)
    {
        NumericType::SharedPtr pType = NumericType::create(getNumericType(pSlangType->getScalarType(), pSlangType->getRowCount(), pSlangType->getColumnCount()));
        return pType;
    }

    static BaseType::SharedPtr reflectStruct(TypeLayoutReflection* pSlangType)
    {
        StructType::SharedPtr pStruct = StructType::create("", pSlangType->getSize()); // ASKTIM How to get the struct name?
        for (uint32_t f = 0; f < pSlangType->getFieldCount(); f++)
        {
            VariableLayoutReflection* pField = pSlangType->getFieldByIndex(f);
            std::string fieldName(pField->getName());
            BaseType::SharedPtr pFieldType = reflectType(pField->getTypeLayout());
            pStruct->addField(fieldName, pFieldType, pField->getOffset());
        }
        return pStruct;
    }

    static BaseType::SharedPtr reflectType(TypeLayoutReflection* pSlangType)
    {
        size_t uniformSize = pSlangType->getSize();

        switch (pSlangType->getKind())
        {
        case TypeReflection::Kind::Array:          return reflectArray(pSlangType);
        case TypeReflection::Kind::ConstantBuffer: return reflectConstantBuffer(pSlangType);
        case TypeReflection::Kind::Scalar:         return reflectNumericType(pSlangType);
        case TypeReflection::Kind::Vector:         return reflectNumericType(pSlangType);
        case TypeReflection::Kind::Struct:         return reflectStruct(pSlangType);
        default:
            should_not_get_here();
            return nullptr;
        }
    }

    StructType::SharedPtr reflectGlobalResources(ShaderReflection* pSlangReflector, std::string& log)
    {
        // We treat the global space as an unnamed struct
        StructType::SharedPtr pStruct = StructType::create("", 0);

        uint32_t paramCount = pSlangReflector->getParameterCount();
        for (uint32_t pp = 0; pp < paramCount; ++pp)
        {
            VariableLayoutReflection* pParam = pSlangReflector->getParameterByIndex(pp);
            BaseType::SharedPtr pType = reflectType(pParam->getTypeLayout());
            if (!pType) return nullptr;
            pStruct->addField(pParam->getName(), pType, 0);
        }
        return pStruct;
    }

    Reflector::SharedPtr Reflector::create(ShaderReflection* pSlangReflector, std::string& log)
    {
        SharedPtr pReflector = SharedPtr(new Reflector);
        return pReflector->init(pSlangReflector, log) ? pReflector : nullptr;
    }

    bool Reflector::init(ShaderReflection* pSlangReflector, std::string& log)
    {
        mpGlobalResources = reflectGlobalResources(pSlangReflector, log);
        if (mpGlobalResources == nullptr) return false;

        // Also extract entry-point stuff
        SlangUInt entryPointCount = pSlangReflector->getEntryPointCount();
        for (SlangUInt ee = 0; ee < entryPointCount; ++ee)
        {
            EntryPointReflection* entryPoint = pSlangReflector->getEntryPointByIndex(ee);

            switch (entryPoint->getStage())
            {
            case SLANG_STAGE_COMPUTE:
            {
                SlangUInt sizeAlongAxis[3];
                entryPoint->getComputeThreadGroupSize(3, &sizeAlongAxis[0]);
                mThreadGroupSize.x = (uint32_t)sizeAlongAxis[0];
                mThreadGroupSize.y = (uint32_t)sizeAlongAxis[1];
                mThreadGroupSize.z = (uint32_t)sizeAlongAxis[2];
            }
            break;
            case SLANG_STAGE_PIXEL:
#ifdef FALCOR_VK
                mIsSampleFrequency = entryPoint->usesAnySampleRateInput();
#else
                mIsSampleFrequency = true; // #SLANG Slang reports false for DX shaders. There's an open issue, once it's fixed we should remove that
#endif            
            default:
                break;
            }
        }
        return true;
    }
}}