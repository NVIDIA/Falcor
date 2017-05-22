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
#include "API/ProgramReflection.h"
#include "API/ProgramVersion.h"
#include "API/Shader.h"
#include "Utils/StringUtils.h"

namespace Falcor
{
#if 0
    ShaderType getShaderType(uint32_t d3d12Ver)
    {
        switch (D3D12_SHVER_GET_TYPE(d3d12Ver))
        {
        case D3D12_SHVER_PIXEL_SHADER:
            return ShaderType::Pixel;
        case D3D12_SHVER_VERTEX_SHADER:
            return ShaderType::Vertex;
        case D3D12_SHVER_DOMAIN_SHADER:
            return ShaderType::Domain;
        case D3D12_SHVER_HULL_SHADER:
            return ShaderType::Hull;
        case D3D12_SHVER_GEOMETRY_SHADER:
            return ShaderType::Geometry;
        case D3D12_SHVER_COMPUTE_SHADER:
            return ShaderType::Compute;
        default:
            return ShaderType::Extended;
        }
    }
#endif

    ProgramReflection::Variable::Type getVariableType(
        spire::TypeReflection::ScalarType   spireScalarType,
        uint32_t                            rows,
        uint32_t                            columns)
    {
        switch(spireScalarType)
        {
        case spire::TypeReflection::ScalarType::None:
            // This isn't a scalar/matrix/vector, so it can't
            // be encoded in the `enum` that Falcor provides.
            return ProgramReflection::Variable::Type::Unknown;

        case spire::TypeReflection::ScalarType::Bool:
            assert(rows == 1);
            switch (columns)
            {
            case 1:
                return ProgramReflection::Variable::Type::Bool;
            case 2:
                return ProgramReflection::Variable::Type::Bool2;
            case 3:
                return ProgramReflection::Variable::Type::Bool3;
            case 4:
                return ProgramReflection::Variable::Type::Bool4;
            }
        case spire::TypeReflection::ScalarType::UInt32:
            assert(rows == 1);
            switch (columns)
            {
            case 1:
                return ProgramReflection::Variable::Type::Uint;
            case 2:
                return ProgramReflection::Variable::Type::Uint2;
            case 3:
                return ProgramReflection::Variable::Type::Uint3;
            case 4:
                return ProgramReflection::Variable::Type::Uint4;
            }
        case spire::TypeReflection::ScalarType::Int32:
            assert(rows == 1);
            switch (columns)
            {
            case 1:
                return ProgramReflection::Variable::Type::Int;
            case 2:
                return ProgramReflection::Variable::Type::Int2;
            case 3:
                return ProgramReflection::Variable::Type::Int3;
            case 4:
                return ProgramReflection::Variable::Type::Int4;
            }
        case spire::TypeReflection::ScalarType::Float32:
            switch (rows)
            {
            case 1:
                switch (columns)
                {
                case 1:
                    return ProgramReflection::Variable::Type::Float;
                case 2:
                    return ProgramReflection::Variable::Type::Float2;
                case 3:
                    return ProgramReflection::Variable::Type::Float3;
                case 4:
                    return ProgramReflection::Variable::Type::Float4;
                }
                break;
            case 2:
                switch (columns)
                {
                case 2:
                    return ProgramReflection::Variable::Type::Float2x2;
                case 3:
                    return ProgramReflection::Variable::Type::Float2x3;
                case 4:
                    return ProgramReflection::Variable::Type::Float2x4;
                }
                break;
            case 3:
                switch (columns)
                {
                case 2:
                    return ProgramReflection::Variable::Type::Float3x2;
                case 3:
                    return ProgramReflection::Variable::Type::Float3x3;
                case 4:
                    return ProgramReflection::Variable::Type::Float3x4;
                }
                break;
            case 4:
                switch (columns)
                {
                case 2:
                    return ProgramReflection::Variable::Type::Float4x2;
                case 3:
                    return ProgramReflection::Variable::Type::Float4x3;
                case 4:
                    return ProgramReflection::Variable::Type::Float4x4;
                }
                break;
            }
        }

        should_not_get_here();
        return ProgramReflection::Variable::Type::Unknown;
    }

    size_t getRowCountFromType(ProgramReflection::Variable::Type type)
    {
        switch (type)
        {
        case ProgramReflection::Variable::Type::Unknown:
            return 0;
        case ProgramReflection::Variable::Type::Bool:
        case ProgramReflection::Variable::Type::Bool2:
        case ProgramReflection::Variable::Type::Bool3:
        case ProgramReflection::Variable::Type::Bool4:
        case ProgramReflection::Variable::Type::Uint:
        case ProgramReflection::Variable::Type::Uint2:
        case ProgramReflection::Variable::Type::Uint3:
        case ProgramReflection::Variable::Type::Uint4:
        case ProgramReflection::Variable::Type::Int:
        case ProgramReflection::Variable::Type::Int2:
        case ProgramReflection::Variable::Type::Int3:
        case ProgramReflection::Variable::Type::Int4:
        case ProgramReflection::Variable::Type::Float:
        case ProgramReflection::Variable::Type::Float2:
        case ProgramReflection::Variable::Type::Float3:
        case ProgramReflection::Variable::Type::Float4:
            return 1;
        case ProgramReflection::Variable::Type::Float2x2:
        case ProgramReflection::Variable::Type::Float2x3:
        case ProgramReflection::Variable::Type::Float2x4:
            return 2;
        case ProgramReflection::Variable::Type::Float3x2:
        case ProgramReflection::Variable::Type::Float3x3:
        case ProgramReflection::Variable::Type::Float3x4:
            return 3;
        case ProgramReflection::Variable::Type::Float4x2:
        case ProgramReflection::Variable::Type::Float4x3:
        case ProgramReflection::Variable::Type::Float4x4:
            return 4;
        default:
            should_not_get_here();
            return 0;
        }
    }

    size_t getBytesPerVarType(ProgramReflection::Variable::Type type)
    {
        switch (type)
        {
        case ProgramReflection::Variable::Type::Unknown:
            return 0;
        case ProgramReflection::Variable::Type::Bool:
        case ProgramReflection::Variable::Type::Uint:
        case ProgramReflection::Variable::Type::Int:
        case ProgramReflection::Variable::Type::Float:
            return 4;
        case ProgramReflection::Variable::Type::Bool2:
        case ProgramReflection::Variable::Type::Uint2:
        case ProgramReflection::Variable::Type::Int2:
        case ProgramReflection::Variable::Type::Float2:
            return 8;
        case ProgramReflection::Variable::Type::Bool3:
        case ProgramReflection::Variable::Type::Uint3:
        case ProgramReflection::Variable::Type::Int3:
        case ProgramReflection::Variable::Type::Float3:
            return 12;
        case ProgramReflection::Variable::Type::Bool4:
        case ProgramReflection::Variable::Type::Uint4:
        case ProgramReflection::Variable::Type::Int4:
        case ProgramReflection::Variable::Type::Float4:
        case ProgramReflection::Variable::Type::Float2x2:
            return 16;
        case ProgramReflection::Variable::Type::Float2x3:
        case ProgramReflection::Variable::Type::Float3x2:
            return 24;
        case ProgramReflection::Variable::Type::Float2x4:
        case ProgramReflection::Variable::Type::Float4x2:
            return 32;
        case ProgramReflection::Variable::Type::Float3x3:
            return 36;
        case ProgramReflection::Variable::Type::Float3x4:
        case ProgramReflection::Variable::Type::Float4x3:
            return 48;
        case ProgramReflection::Variable::Type::Float4x4:
            return 64;
        default:
            should_not_get_here();
            return 0;
        }

    }

    // Information we need to track when converting Spire reflection
    // information over to the Falcor equivalent
    struct ReflectionGenerationContext
    {
        ProgramReflection*  pReflector = nullptr;
        ProgramReflection::VariableMap* pVariables = nullptr;
        ProgramReflection::ResourceMap* pResourceMap = nullptr;
        std::string* pLog = nullptr;

        ProgramReflection::VariableMap& getVariableMap() { return *pVariables; }
        ProgramReflection::ResourceMap& getResourceMap() { return *pResourceMap; }
        std::string& getLog() { return *pLog; }
    };

    // Represents a "breadcrumb trail" leading from a particular variable
    // back to the path over member-access and array-indexing operations
    // that led to it.
    // E.g., when trying to construct information for `foo.bar[3].baz`
    // we might have a path that consists of:
    //
    // - An entry for the field `baz` in type `Bar` (which knows its offset)
    // - An entry for element 3 in the type `Bar[]`
    // - An entry for the field `bar` in type `Foo`
    // - An entry for the top-level shader parameter `foo`
    //
    // To compute the correct offset for `baz` we can walk up this chain
    // and add up offsets (taking element stride into account for arrays).
    //
    // In simple cases, one can track this info top-down, by simply keeping
    // a "running total" offset, but that doesn't account for the fact that
    // `baz` might be a texture, UAV, sampler, or uniform, and the offset
    // we'd need to track for each case is different.
    struct ReflectionPath
    {
        ReflectionPath*                     parent = nullptr;
        spire::VariableLayoutReflection*    var = nullptr;
        spire::TypeLayoutReflection*        typeLayout = nullptr;
        uint32_t                            childIndex = 0;
    };

    // Once we've found the path from the root down to a particular leaf
    // variable, `getBindingIndex` can be used to find the final summed-up
    // index (or byte offset, in the case of a uniform).
    uint32_t getBindingIndex(ReflectionPath* path, SpireParameterCategory category)
    {
        uint32_t offset = 0;
        for(auto pp = path; pp; pp = pp->parent)
        {
            if(pp->var)
            {
                offset += (uint32_t) pp->var->getOffset(category);
                continue;
            }
            else if(pp->typeLayout)
            {
                switch(pp->typeLayout->getKind())
                {
                case spire::TypeReflection::Kind::Array:
                    offset += (uint32_t) pp->typeLayout->getElementStride(category) * pp->childIndex;
                    continue;

                case spire::TypeReflection::Kind::Struct:
                    offset += (uint32_t) pp->typeLayout->getFieldByIndex(int(pp->childIndex))->getOffset(category);
                    continue;

                default:
                    break;
                }
            }

            logError("internal error: invalid reflection path");
            return 0;
        }
        return offset;
    }

    size_t getUniformOffset(ReflectionPath* path)
    {
        return getBindingIndex(path, SPIRE_PARAMETER_CATEGORY_UNIFORM);
    }

    uint32_t getBindingSpace(ReflectionPath* path, SpireParameterCategory category)
    {
        // TODO: implement
        return 0;
    }

    static ProgramReflection::Resource::ResourceType getResourceType(spire::TypeReflection* pSpireType)
    {
        switch(pSpireType->unwrapArray()->getKind())
        {
        case spire::TypeReflection::Kind::SamplerState:
            return ProgramReflection::Resource::ResourceType::Sampler;

        case spire::TypeReflection::Kind::Resource:
            switch(pSpireType->getResourceShape() & SPIRE_RESOURCE_BASE_SHAPE_MASK)
            {
            case SPIRE_STRUCTURED_BUFFER:
                return ProgramReflection::Resource::ResourceType::StructuredBuffer;

            case SPIRE_BYTE_ADDRESS_BUFFER:
                return ProgramReflection::Resource::ResourceType::RawBuffer;

            default:
                return ProgramReflection::Resource::ResourceType::Texture;
            }
            break;

        default:
            break;
        }
        should_not_get_here();
        return ProgramReflection::Resource::ResourceType::Unknown;
    }

    static ProgramReflection::ShaderAccess getShaderAccess(spire::TypeReflection* pSpireType)
    {
        // Compute access for an array using the underlying type...
        pSpireType = pSpireType->unwrapArray();

        switch(pSpireType->getKind())
        {
        case spire::TypeReflection::Kind::SamplerState:
        case spire::TypeReflection::Kind::ConstantBuffer:
            return ProgramReflection::ShaderAccess::Read;
            break;

        case spire::TypeReflection::Kind::Resource:
            switch(pSpireType->getResourceAccess())
            {
            case SPIRE_RESOURCE_ACCESS_NONE:
                return ProgramReflection::ShaderAccess::Undefined;

            case SPIRE_RESOURCE_ACCESS_READ:
                return ProgramReflection::ShaderAccess::Read;

            default:
                return ProgramReflection::ShaderAccess::ReadWrite;
            }
            break;

        default:
            should_not_get_here();
            return ProgramReflection::ShaderAccess::Undefined;
        }
    }

    static ProgramReflection::Resource::ReturnType getReturnType(spire::TypeReflection* pType)
    {
        // Could be a resource that doesn't have a specific element type (e.g., a raw buffer)
        if(!pType)
            return ProgramReflection::Resource::ReturnType::Unknown;

        switch (pType->getScalarType())
        {
        case spire::TypeReflection::ScalarType::Float32:
            return ProgramReflection::Resource::ReturnType::Float;
        case spire::TypeReflection::ScalarType::Int32:
            return ProgramReflection::Resource::ReturnType::Int;
        case spire::TypeReflection::ScalarType::UInt32:
            return ProgramReflection::Resource::ReturnType::Uint;
        case spire::TypeReflection::ScalarType::Float64:
            return ProgramReflection::Resource::ReturnType::Double;

        // Could be a resource that uses an aggregate element type (e.g., a structured buffer)
        case spire::TypeReflection::ScalarType::None:
            return ProgramReflection::Resource::ReturnType::Unknown;

        default:
            should_not_get_here();
            return ProgramReflection::Resource::ReturnType::Unknown;
        }
    }

    static ProgramReflection::Resource::Dimensions getResourceDimensions(SpireResourceShape shape)
    {
        switch (shape)
        {
        case SPIRE_TEXTURE_1D:
            return ProgramReflection::Resource::Dimensions::Texture1D;
        case SPIRE_TEXTURE_1D_ARRAY:
            return ProgramReflection::Resource::Dimensions::Texture1DArray;
        case SPIRE_TEXTURE_2D:
            return ProgramReflection::Resource::Dimensions::Texture2D;
        case SPIRE_TEXTURE_2D_ARRAY:
            return ProgramReflection::Resource::Dimensions::Texture2DArray;
        case SPIRE_TEXTURE_2D_MULTISAMPLE:
            return ProgramReflection::Resource::Dimensions::Texture2DMS;
        case SPIRE_TEXTURE_2D_MULTISAMPLE_ARRAY:
            return ProgramReflection::Resource::Dimensions::Texture2DMSArray;
        case SPIRE_TEXTURE_3D:
            return ProgramReflection::Resource::Dimensions::Texture3D;
        case SPIRE_TEXTURE_CUBE:
            return ProgramReflection::Resource::Dimensions::TextureCube;
        case SPIRE_TEXTURE_CUBE_ARRAY:
            return ProgramReflection::Resource::Dimensions::TextureCubeArray;

        case SPIRE_TEXTURE_BUFFER:
        case SPIRE_STRUCTURED_BUFFER:
        case SPIRE_BYTE_ADDRESS_BUFFER:
            return ProgramReflection::Resource::Dimensions::Buffer;

        default:
            should_not_get_here();
            return ProgramReflection::Resource::Dimensions::Unknown;
        }
    }

    static bool verifyResourceDefinition(const ProgramReflection::Resource& prev, ProgramReflection::Resource& current, std::string& log)
    {
        bool match = true;
#define error_msg(msg_) std::string(msg_) + " mismatch.\n";
#define test_field(field_)                                           \
            if(prev.field_ != current.field_)                        \
            {                                                        \
                log += error_msg(#field_)                            \
                match = false;                                       \
            }

        test_field(type);
        test_field(dims);
        test_field(retType);
        test_field(regIndex);
        test_field(registerSpace);
        test_field(arraySize);
#undef test_field
#undef error_msg

        return match;
    }

    static bool reflectStructuredBuffer(
        ReflectionGenerationContext*    pContext,
        spire::TypeLayoutReflection*    pSpireType,
        const std::string&              name,
        ReflectionPath*                 path);

    static bool reflectConstantBuffer(
        ReflectionGenerationContext*    pContext,
        spire::TypeLayoutReflection*    pSpireType,
        const std::string&              name,
        ReflectionPath*                 path);


    // Generate reflection data for a single variable
    static bool reflectResource(
        ReflectionGenerationContext*    pContext,
        spire::TypeLayoutReflection*    pSpireType,
        const std::string&              name,
        ReflectionPath*                 path)
    {
        auto resourceType = getResourceType(pSpireType->getType());
        if( resourceType == ProgramReflection::Resource::ResourceType::StructuredBuffer)
        {
            // reflect this parameter as a buffer
            return reflectStructuredBuffer(
                pContext,
                pSpireType,
                name,
                path);
        }

        ProgramReflection::Resource falcorDesc;
        falcorDesc.type = resourceType;
        falcorDesc.shaderAccess = getShaderAccess(pSpireType->getType());
        if (resourceType == ProgramReflection::Resource::ResourceType::Texture)
        {
            falcorDesc.retType = getReturnType(pSpireType->getResourceResultType());
            falcorDesc.dims = getResourceDimensions(pSpireType->getResourceShape());
        }
        bool isArray = pSpireType->isArray();
        falcorDesc.regIndex = (uint32_t) getBindingIndex(path, pSpireType->getParameterCategory());
        falcorDesc.registerSpace = (uint32_t) getBindingSpace(path, pSpireType->getParameterCategory());
        assert(falcorDesc.registerSpace == 0);
        falcorDesc.arraySize = isArray ? (uint32_t)pSpireType->getTotalArrayElementCount() : 0;

        // If this already exists, definitions should match
        auto& resourceMap = * pContext->pResourceMap;
        const auto& prevDef = resourceMap.find(name);
        if (prevDef == resourceMap.end())
        {
            resourceMap[name] = falcorDesc;
        }
        else
        {
            std::string varLog;
            if (verifyResourceDefinition(prevDef->second, falcorDesc, varLog) == false)
            {
                pContext->getLog() += "Shader resource '" + std::string(name) + "' has different definitions between different shader stages. " + varLog;
                return false;
            }
        }

        // For now, we expose all resources as visible to all stages
        resourceMap[name].shaderMask = 0xFFFFFFFF;

        return true;
    }

    static void reflectType(
        ReflectionGenerationContext*    pContext,
        spire::TypeLayoutReflection*    pSpireType,
        const std::string&              name,
        ReflectionPath*                 pPath)
    {
        size_t uniformSize = pSpireType->getSize();

        // For any variable that actually occupies space in
        // uniform data, we want to add an entry to
        // the variables map that can be directly queried:
        if(uniformSize != 0)
        {
            ProgramReflection::Variable desc;

            desc.location = getUniformOffset(pPath);
            desc.type = getVariableType(
                pSpireType->getScalarType(),
                pSpireType->getRowCount(),
                pSpireType->getColumnCount());

            switch( pSpireType->getKind() )
            {
            default:
                break;

            case spire::TypeReflection::Kind::Array:
                desc.arraySize = (uint32_t) pSpireType->getElementCount();
                desc.arrayStride = (uint32_t) pSpireType->getElementStride(SPIRE_PARAMETER_CATEGORY_UNIFORM);
                break;

            case spire::TypeReflection::Kind::Matrix:
                // TODO(tfoley): Spire needs to report this information!
//                desc.isRowMajor = (typeDesc.Class == D3D_SVC_MATRIX_ROWS);
                break;
            }

            if( !pContext->pVariables )
            {
                logError("unimplemented: global-scope uniforms");
            }

            (*pContext->pVariables)[name] = desc;
        }

        // We want to reflect resource parameters as soon as we find a
        // type that is an (array of)* (sampler|texture|...)
        //
        // That is, we will look through any number of levels of array-ness
        // to see the type underneath:
        switch(pSpireType->unwrapArray()->getKind())
        {
        case spire::TypeReflection::Kind::Struct:
            // A `struct` type obviously isn't a resource
            break;

        // TODO: If we ever start using arrays of constant buffers,
        // we'd probably want to handle them here too...

        default:
            // This might be a resource, or an array of resources.
            // To find out, let's ask what category of resource
            // it consumes.
            switch(pSpireType->getParameterCategory())
            {
            case spire::ParameterCategory::ShaderResource:
            case spire::ParameterCategory::UnorderedAccess:
            case spire::ParameterCategory::SamplerState:
                // This is a resource, or an array of them (or an array of arrays ...)
                reflectResource(
                    pContext,
                    pSpireType,
                    name,
                    pPath);

                // We don't want to enumerate each individual field
                // as a separate entry in the resources map, so bail out here
                return;

            default:
                break;
            }
            break;
        }

        // If we didn't early exit in the resource case above, then we
        // will go ahead and recurse into the sub-elements of the type
        // (fields of a struct, elements of an array, etc.)
        switch (pSpireType->getKind())
        {
        default:
            // All the interesting cases for non-aggregate values
            // have been handled above.
            break;

        case spire::TypeReflection::Kind::ConstantBuffer:
            // We've found a constant buffer, so reflect it as a top-level buffer
            reflectConstantBuffer(
                pContext,
                pSpireType,
                name,
                pPath);
            break;

        case spire::TypeReflection::Kind::Array:
            {
                // For a variable with array type, we are going to create
                // entries for each element of the array.
                //
                // TODO: This probably isn't a good idea for very large
                // arrays, and obviously doesn't work for arrays that
                // aren't statically sized.
                //
                // TODO: we should probably also handle arrays-of-textures
                // and arrays-of-samplers specially here.

                auto elementCount = (uint32_t) pSpireType->getElementCount();
                spire::TypeLayoutReflection* elementType = pSpireType->getElementTypeLayout();

                assert(name.size());

                for (uint32_t ee = 0; ee < elementCount; ++ee)
                {
                    ReflectionPath elementPath;
                    elementPath.parent = pPath;
                    elementPath.typeLayout = pSpireType;
                    elementPath.childIndex = ee;

                    reflectType(pContext, elementType, name + '[' + std::to_string(ee) + "]", &elementPath);
                }
            }
            break;

        case spire::TypeReflection::Kind::Struct:
            {
                // For a variable with structure type, we are going
                // to create entries for each field of the structure.
                //
                // TODO: it isn't strictly necessary to do this, but
                // doing something more clever involves additional
                // parsing work during lookup, to deal with `.`
                // operations in the path to a variable.

                uint32_t fieldCount = pSpireType->getFieldCount();
                for(uint32_t ff = 0; ff < fieldCount; ++ff)
                {
                    spire::VariableLayoutReflection* field = pSpireType->getFieldByIndex(ff);
                    std::string memberName(field->getName());
                    std::string fullName = name.size() ? name + '.' + memberName : memberName;

                    ReflectionPath fieldPath;
                    fieldPath.parent = pPath;
                    fieldPath.typeLayout = pSpireType;
                    fieldPath.childIndex = ff;

                    reflectType(pContext, field->getTypeLayout(), fullName, &fieldPath);
                }
            }
            break;
        }
    }

    static void reflectVariable(
        ReflectionGenerationContext*        pContext,
        spire::VariableLayoutReflection*    pSpireVar,
        ReflectionPath*                     pParentPath)
    {
        // Get the variable name
        std::string name(pSpireVar->getName());

        // Create a path element for the variable
        ReflectionPath varPath;
        varPath.parent = pParentPath;
        varPath.var = pSpireVar;

        // Reflect the Type
        reflectType(pContext, pSpireVar->getTypeLayout(), name, &varPath);
    }

    static void initializeBufferVariables(
        ReflectionGenerationContext*    pContext,
        ReflectionPath*                 pBufferPath,
        spire::TypeLayoutReflection*    pSpireElementType)
    {
        // Element type of a structured buffer need not be a structured type,
        // so don't recurse unless needed...
        if(pSpireElementType->getKind() != spire::TypeReflection::Kind::Struct)
            return;

        uint32_t fieldCount = pSpireElementType->getFieldCount();

        for (uint32_t ff = 0; ff < fieldCount; ff++)
        {
            auto var = pSpireElementType->getFieldByIndex(ff);

            reflectVariable(pContext, var, pBufferPath);
        }
    }

    bool validateBufferDeclaration(const ProgramReflection::BufferReflection* pPrevDesc, const ProgramReflection::VariableMap& varMap, std::string& log)
    {
        bool match = true;
#define error_msg(msg_) std::string(msg_) + " mismatch.\n";
        if (pPrevDesc->getVariableCount() != varMap.size())
        {
            log += error_msg("Variable count");
            match = false;
        }

        for (auto& prevVar = pPrevDesc->varBegin(); prevVar != pPrevDesc->varEnd(); prevVar++)
        {
            const std::string& name = prevVar->first;
            const auto& curVar = varMap.find(name);
            if (curVar == varMap.end())
            {
                log += "Can't find variable '" + name + "' in the new definitions";
            }
            else
            {
#define test_field(field_, msg_)                                      \
            if(prevVar->second.field_ != curVar->second.field_)       \
            {                                                         \
                log += error_msg(prevVar->first + " " + msg_)         \
                match = false;                                       \
            }

                test_field(location, "offset");
                test_field(arraySize, "array size");
                test_field(arrayStride, "array stride");
                test_field(isRowMajor, "row major");
                test_field(type, "Type");
#undef test_field
            }
        }

#undef error_msg

        return match;
    }

    static ProgramReflection::BufferReflection::StructuredType getStructuredBufferType(
        spire::TypeReflection* pSpireType)
    {
        auto invalid = ProgramReflection::BufferReflection::StructuredType::Invalid;

        if(pSpireType->getKind() != spire::TypeReflection::Kind::Resource)
            return invalid; // not a structured buffer

        if(pSpireType->getResourceShape() != SPIRE_STRUCTURED_BUFFER)
            return invalid; // not a structured buffer

        switch( pSpireType->getResourceAccess() )
        {
        default:
            should_not_get_here();
            return invalid;

        case SPIRE_RESOURCE_ACCESS_READ:
            return ProgramReflection::BufferReflection::StructuredType::Default;

        case SPIRE_RESOURCE_ACCESS_READ_WRITE:
        case SPIRE_RESOURCE_ACCESS_RASTER_ORDERED:
            return ProgramReflection::BufferReflection::StructuredType::Counter;
        case SPIRE_RESOURCE_ACCESS_APPEND:
            return ProgramReflection::BufferReflection::StructuredType::Append;
        case SPIRE_RESOURCE_ACCESS_CONSUME:
            return ProgramReflection::BufferReflection::StructuredType::Consume;
        }
    }

#if FALCOR_USE_SPIRE_AS_COMPILER
    static bool reflectBuffer(
        ReflectionGenerationContext*                pContext,
        spire::TypeLayoutReflection*                pSpireType,
        const std::string&                          name,
        ReflectionPath*                             pPath,
        ProgramReflection::BufferData&              bufferDesc,
        ProgramReflection::BufferReflection::Type   bufferType,
        ProgramReflection::ShaderAccess             shaderAccess)
    {
        auto pSpireElementType = pSpireType->getElementTypeLayout();

        ProgramReflection::VariableMap varMap;

        ReflectionGenerationContext context = *pContext;
        context.pVariables = &varMap;

        initializeBufferVariables(
            &context,
            pPath,
            pSpireElementType);

        // TODO(tfoley): This is a bit of an ugly workaround, and it would
        // be good for the Spire API to make it unnecessary...
        auto category = pSpireType->getParameterCategory();
        if(category == spire::ParameterCategory::Mixed)
        {
            if(pSpireType->getKind() == spire::TypeReflection::Kind::ConstantBuffer)
            {
                category = spire::ParameterCategory::ConstantBuffer;
            }
        }

        auto bindingIndex = getBindingIndex(pPath, category);
        auto bindingSpace = getBindingSpace(pPath, category);
        ProgramReflection::BindLocation bindLocation(
            bindingIndex,
            shaderAccess);
        // If the buffer already exists in the program, make sure the definitions match
        const auto& prevDef = bufferDesc.nameMap.find(name);

        if (prevDef != bufferDesc.nameMap.end())
        {
            if (bindLocation != prevDef->second)
            {
                pContext->getLog() += to_string(bufferType) + " buffer '" + name + "' has different bind locations between different shader stages. Falcor do not support that. Use explicit bind locations to avoid this error";
                return false;
            }
            ProgramReflection::BufferReflection* pPrevBuffer = bufferDesc.descMap[bindLocation].get();
            std::string bufLog;
            if (validateBufferDeclaration(pPrevBuffer, varMap, bufLog) == false)
            {
                pContext->getLog() += to_string(bufferType) + " buffer '" + name + "' has different definitions between different shader stages. " + bufLog;
                return false;
            }
        }
        else
        {
            // Create the buffer reflection
            bufferDesc.nameMap[name] = bindLocation;
            bufferDesc.descMap[bindLocation] = ProgramReflection::BufferReflection::create(
                name,
                bindingIndex,
                bindingSpace,
                bufferType,
                getStructuredBufferType(pSpireType->getType()),
                (uint32_t) pSpireElementType->getSize(),
                varMap,
                ProgramReflection::ResourceMap(),
                shaderAccess);
        }

        // For now we expose all buffers as visible to every stage
        uint32_t mask = 0xFFFFFFFF;
        bufferDesc.descMap[bindLocation]->setShaderMask(mask);

        return true;
    }
#else
    bool reflectBuffer(ShaderReflectionHandle pReflection, const char* bufName, ProgramReflection::BufferData& bufferDesc, ProgramReflection::BufferReflection::Type bufferType, ProgramReflection::ShaderAccess shaderAccess, uint32_t shaderIndex, std::string& log)
    {
        D3D_SHADER_BUFFER_DESC d3dBufDesc;
        ID3DShaderReflectionConstantBuffer* pBuffer = pReflection->GetConstantBufferByName(bufName);
        d3d_call(pBuffer->GetDesc(&d3dBufDesc));

        D3D_SHADER_INPUT_BIND_DESC bindDesc;
        d3d_call(pReflection->GetResourceBindingDescByName(d3dBufDesc.Name, &bindDesc));
        assert(bindDesc.BindCount == 1);
        ProgramReflection::VariableMap varMap;
        initializeBufferVariables(pBuffer, d3dBufDesc, bindDesc.Type, varMap);

        ProgramReflection::BindLocation bindLocation(bindDesc.BindPoint, shaderAccess);
        // If the buffer already exists in the program, make sure the definitions match
        const auto& prevDef = bufferDesc.nameMap.find(d3dBufDesc.Name);

        if (prevDef != bufferDesc.nameMap.end())
        {
            if (bindLocation != prevDef->second)
            {
                log += to_string(bufferType) + " buffer '" + std::string(d3dBufDesc.Name) + "' has different bind locations between different shader stages. Falcor do not support that. Use explicit bind locations to avoid this error";
                return false;
            }
            ProgramReflection::BufferReflection* pPrevBuffer = bufferDesc.descMap[bindLocation].get();
            std::string bufLog;
            if (validateBufferDeclaration(pPrevBuffer, varMap, bufLog) == false)
            {
                log += to_string(bufferType) + " buffer '" + std::string(d3dBufDesc.Name) + "' has different definitions between different shader stages. " + bufLog;
                return false;
            }
        }
        else
        {
            // Create the buffer reflection
            bufferDesc.nameMap[d3dBufDesc.Name] = bindLocation;
            bufferDesc.descMap[bindLocation] = ProgramReflection::BufferReflection::create(d3dBufDesc.Name, bindDesc.BindPoint, bindDesc.Space, bufferType, d3dBufDesc.Size, varMap, ProgramReflection::ResourceMap(), shaderAccess);
        }

        // Update the shader mask
        uint32_t mask = bufferDesc.descMap[bindLocation]->getShaderMask() | (1 << shaderIndex);
        bufferDesc.descMap[bindLocation]->setShaderMask(mask);

        return true;
    }
#endif

    bool ProgramReflection::reflectVertexAttributes(
        spire::ShaderReflection*    pSpireReflector,
        std::string&                log)
    {
        // TODO(tfoley): Add vertex input reflection capability to Spire
        return true;
    }

    bool ProgramReflection::reflectPixelShaderOutputs(
        spire::ShaderReflection*    pSpireReflector,
        std::string&                log)
    {
        // TODO(tfoley): Add fragment output reflection capability to Spire
        return true;
    }

    // TODO(tfoley): Should try to strictly use type...
    static ProgramReflection::Resource::ResourceType getResourceType(spire::VariableLayoutReflection* pParameter)
    {
        switch (pParameter->getCategory())
        {
        case spire::ParameterCategory::SamplerState:
            return ProgramReflection::Resource::ResourceType::Sampler;
        case spire::ParameterCategory::ShaderResource:
        case spire::ParameterCategory::UnorderedAccess:
            switch(pParameter->getType()->getResourceShape() & SPIRE_RESOURCE_BASE_SHAPE_MASK)
            {
            case SPIRE_BYTE_ADDRESS_BUFFER:
                return ProgramReflection::Resource::ResourceType::RawBuffer;

            case SPIRE_STRUCTURED_BUFFER:
                return ProgramReflection::Resource::ResourceType::StructuredBuffer;

            default:
                return ProgramReflection::Resource::ResourceType::Texture;

            case SPIRE_RESOURCE_NONE:
                break;
            }
            break;
        case spire::ParameterCategory::Mixed:
            // TODO: propagate this information up the Falcor level
            return ProgramReflection::Resource::ResourceType::Unknown;
        default:
            break;
        }
        should_not_get_here();
        return ProgramReflection::Resource::ResourceType::Unknown;
    }

    static ProgramReflection::ShaderAccess getShaderAccess(spire::ParameterCategory category)
    {
        switch (category)
        {
        case spire::ParameterCategory::ShaderResource:
        case spire::ParameterCategory::SamplerState:
            return ProgramReflection::ShaderAccess::Read;
        case spire::ParameterCategory::UnorderedAccess:
            return ProgramReflection::ShaderAccess::ReadWrite;
        case spire::ParameterCategory::Mixed:
            return ProgramReflection::ShaderAccess::Undefined;
        default:
            should_not_get_here();
            return ProgramReflection::ShaderAccess::Undefined;
        }
    }

#if 0
    bool reflectResource(
        ReflectionGenerationContext*    pContext,
        spire::ParameterReflection*     pParameter )
    {
        ProgramReflection::Resource falcorDesc;
        std::string name(pParameter->getName());

        falcorDesc.type = getResourceType(pParameter);
        falcorDesc.shaderAccess = getShaderAccess(pParameter->getCategory());
        if (falcorDesc.type == ProgramReflection::Resource::ResourceType::Texture)
        {
            falcorDesc.retType = getReturnType(pParameter->getType()->getTextureResultType());
            falcorDesc.dims = getResourceDimensions(pParameter->getType()->getTextureShape());
        }
        bool isArray = pParameter->getType()->isArray();
        falcorDesc.regIndex = pParameter->getBindingIndex();
        falcorDesc.registerSpace = pParameter->getBindingSpace();
        assert(falcorDesc.registerSpace == 0);
        falcorDesc.arraySize = isArray ? (uint32_t)pParameter->getType()->getTotalArrayElementCount() : 0;

        // If this already exists, definitions should match
        auto& resourceMap = pContext->getResourceMap();
        const auto& prevDef = resourceMap.find(name);
        if (prevDef == resourceMap.end())
        {
            resourceMap[name] = falcorDesc;
        }
        else
        {
            std::string varLog;
            if (verifyResourceDefinition(prevDef->second, falcorDesc, varLog) == false)
            {
                pContext->getLog() += "Shader resource '" + std::string(name) + "' has different definitions between different shader stages. " + varLog;
                return false;
            }
        }

        // Update the mask
        resourceMap[name].shaderMask |= (1 << pContext->shaderIndex);

        return true;
    }
#endif

    static bool reflectStructuredBuffer(
        ReflectionGenerationContext*    pContext,
        spire::TypeLayoutReflection*    pSpireType,
        const std::string&              name,
        ReflectionPath*                 path)
    {
        auto shaderAccess = getShaderAccess(pSpireType->getType());
        return reflectBuffer(
            pContext,
            pSpireType,
            name,
            path,
            pContext->pReflector->mBuffers[(uint32_t)ProgramReflection::BufferReflection::Type::Structured],
            ProgramReflection::BufferReflection::Type::Structured,
            shaderAccess);
    }

    static bool reflectConstantBuffer(
        ReflectionGenerationContext*    pContext,
        spire::TypeLayoutReflection*    pSpireType,
        const std::string&              name,
        ReflectionPath*                 path)
    {
        return reflectBuffer(
            pContext,
            pSpireType,
            name,
            path,
            pContext->pReflector->mBuffers[(uint32_t)ProgramReflection::BufferReflection::Type::Constant],
            ProgramReflection::BufferReflection::Type::Constant,
            ProgramReflection::ShaderAccess::Read);
    }

#if 0
    static bool reflectStructuredBuffer(
        ReflectionGenerationContext*    pContext,
        spire::BufferReflection*        spireParam)
    {
        auto shaderAccess = getShaderAccess(spireParam->getCategory());
        return reflectBuffer(
            pContext,
            spireParam,
            pContext->pReflector->mBuffers[(uint32_t)ProgramReflection::BufferReflection::Type::Structured],
            ProgramReflection::BufferReflection::Type::Structured,
            ProgramReflection::ShaderAccess::Read);
    }

    static bool reflectConstantBuffer(
        ReflectionGenerationContext*    pContext,
        spire::BufferReflection*        spireParam)
    {
        return reflectBuffer(
            pContext,
            spireParam,
            pContext->pReflector->mBuffers[(uint32_t)ProgramReflection::BufferReflection::Type::Constant],
            ProgramReflection::BufferReflection::Type::Constant,
            ProgramReflection::ShaderAccess::Read);
    }

    static bool reflectVariable(
        ReflectionGenerationContext*        pContext,
        spire::VariableLayoutReflection*    spireVar)
    {
        switch (spireVar->getCategory())
        {
        case spire::ParameterCategory::ConstantBuffer:
            return reflectConstantBuffer(pContext, spireVar->asBuffer());

        case spire::ParameterCategory::ShaderResource:
        case spire::ParameterCategory::UnorderedAccess:
        case spire::ParameterCategory::SamplerState:
            return reflectResource(pContext, spireVar);

        case spire::ParameterCategory::Mixed:
            {
                // The parameter spans multiple binding kinds (e.g., both texture and uniform).
                // We need to recursively split it into sub-parameters, each using a single
                // kind of bindable resource.
                //
                // Also, the parameter may have been declared as a constant buffer, so
                // we need to reflect it directly in that case:
                //
                switch(spireVar->getType()->getKind())
                {
                case spire::TypeReflection::Kind::ConstantBuffer:
                    return reflectConstantBuffer(pContext, spireVar->asBuffer());

                default:
                    //
                    // Okay, let's walk it recursively to bind the sub-pieces...
                    //
                    logError("unimplemented: global-scope uniforms");
                    break;
                }
            }
            break;


        case spire::ParameterCategory::Uniform:
            // Ignore uniform parameters during this pass...
            logError("unimplemented: global-scope uniforms");
            return true;

        default:
            break;
        }
    }
#endif

    static bool reflectParameter(
        ReflectionGenerationContext*        pContext,
        spire::VariableLayoutReflection*    spireParam)
    {
        reflectVariable(pContext, spireParam, nullptr);
        return true;
    }

    bool ProgramReflection::reflectResources(
        spire::ShaderReflection*    pSpireReflector,
        std::string&                log)
    {
        ReflectionGenerationContext context;
        context.pReflector = this;
        context.pResourceMap = &mResources;
        context.pLog = &log;

        bool res = true;

        uint32_t paramCount = pSpireReflector->getParameterCount();
        for (uint32_t pp = 0; pp < paramCount; ++pp)
        {
            spire::VariableLayoutReflection* param = pSpireReflector->getParameterByIndex(pp);
            res = reflectParameter(&context, param);
        }

        // Also extract entry-point stuff

        SpireUInt entryPointCount = pSpireReflector->getEntryPointCount();
        for( SpireUInt ee = 0; ee < entryPointCount; ++ee )
        {
            spire::EntryPointReflection* entryPoint = pSpireReflector->getEntryPointByIndex(ee);

            switch( entryPoint->getStage() )
            {
            case SPIRE_STAGE_COMPUTE:
                {
                    SpireUInt sizeAlongAxis[3];
                    entryPoint->getComputeThreadGroupSize(3, &sizeAlongAxis[0]);
                    mThreadGroupSizeX = (uint32_t) sizeAlongAxis[0];
                    mThreadGroupSizeY = (uint32_t) sizeAlongAxis[1];
                    mThreadGroupSizeZ = (uint32_t) sizeAlongAxis[2];
                }
                break;

            default:
                break;
            }
        }

        return res;
    }
}
