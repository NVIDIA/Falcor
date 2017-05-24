// Reflection.cpp
#include "Reflection.h"

#include "ShaderCompiler.h"
#include "TypeLayout.h"

#include <assert.h>

// Implementation to back public-facing reflection API

using namespace Spire;
using namespace Spire::Compiler;


// Conversion routines to help with strongly-typed reflection API

static inline ExpressionType* convert(SpireReflectionType* type)
{
    return (ExpressionType*) type;
}

static inline SpireReflectionType* convert(ExpressionType* type)
{
    return (SpireReflectionType*) type;
}

static inline TypeLayout* convert(SpireReflectionTypeLayout* type)
{
    return (TypeLayout*) type;
}

static inline SpireReflectionTypeLayout* convert(TypeLayout* type)
{
    return (SpireReflectionTypeLayout*) type;
}

static inline VarDeclBase* convert(SpireReflectionVariable* var)
{
    return (VarDeclBase*) var;
}

static inline SpireReflectionVariable* convert(VarDeclBase* var)
{
    return (SpireReflectionVariable*) var;
}

static inline VarLayout* convert(SpireReflectionVariableLayout* var)
{
    return (VarLayout*) var;
}

static inline SpireReflectionVariableLayout* convert(VarLayout* var)
{
    return (SpireReflectionVariableLayout*) var;
}

static inline EntryPointLayout* convert(SpireReflectionEntryPoint* entryPoint)
{
    return (EntryPointLayout*) entryPoint;
}

static inline SpireReflectionEntryPoint* convert(EntryPointLayout* entryPoint)
{
    return (SpireReflectionEntryPoint*) entryPoint;
}


static inline ProgramLayout* convert(SpireReflection* program)
{
    return (ProgramLayout*) program;
}

static inline SpireReflection* convert(ProgramLayout* program)
{
    return (SpireReflection*) program;
}

// Type Reflection


SPIRE_API SpireTypeKind spReflectionType_GetKind(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return SPIRE_TYPE_KIND_NONE;

    // TODO(tfoley: Don't emit the same type more than once...

    if (auto basicType = type->As<BasicExpressionType>())
    {
        return SPIRE_TYPE_KIND_SCALAR;
    }
    else if (auto vectorType = type->As<VectorExpressionType>())
    {
        return SPIRE_TYPE_KIND_VECTOR;
    }
    else if (auto matrixType = type->As<MatrixExpressionType>())
    {
        return SPIRE_TYPE_KIND_MATRIX;
    }
    else if (auto constantBufferType = type->As<ConstantBufferType>())
    {
        return SPIRE_TYPE_KIND_CONSTANT_BUFFER;
    }
    else if (auto samplerStateType = type->As<SamplerStateType>())
    {
        return SPIRE_TYPE_KIND_SAMPLER_STATE;
    }
    else if (auto textureType = type->As<TextureType>())
    {
        return SPIRE_TYPE_KIND_RESOURCE;
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE)                          \
    else if(type->As<TYPE>()) do {          \
        return SPIRE_TYPE_KIND_RESOURCE;    \
    } while(0)

    CASE(HLSLBufferType);
    CASE(HLSLRWBufferType);
    CASE(HLSLBufferType);
    CASE(HLSLRWBufferType);
    CASE(HLSLStructuredBufferType);
    CASE(HLSLRWStructuredBufferType);
    CASE(HLSLAppendStructuredBufferType);
    CASE(HLSLConsumeStructuredBufferType);
    CASE(HLSLByteAddressBufferType);
    CASE(HLSLRWByteAddressBufferType);
    CASE(UntypedBufferResourceType);
#undef CASE

    else if (auto arrayType = type->As<ArrayExpressionType>())
    {
        return SPIRE_TYPE_KIND_ARRAY;
    }
    else if( auto declRefType = type->As<DeclRefType>() )
    {
        auto declRef = declRefType->declRef;
        if( auto structDeclRef = declRef.As<StructDeclRef>() )
        {
            return SPIRE_TYPE_KIND_STRUCT;
        }
    }

    assert(!"unexpected");
    return SPIRE_TYPE_KIND_NONE;
}

SPIRE_API unsigned int spReflectionType_GetFieldCount(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    // TODO: maybe filter based on kind

    if(auto declRefType = dynamic_cast<DeclRefType*>(type))
    {
        auto declRef = declRefType->declRef;
        if( auto structDeclRef = declRef.As<StructDeclRef>())
        {
            return structDeclRef.GetFields().Count();
        }
    }

    return 0;
}

SPIRE_API SpireReflectionVariable* spReflectionType_GetFieldByIndex(SpireReflectionType* inType, unsigned index)
{
    auto type = convert(inType);
    if(!type) return nullptr;

    // TODO: maybe filter based on kind

    if(auto declRefType = dynamic_cast<DeclRefType*>(type))
    {
        auto declRef = declRefType->declRef;
        if( auto structDeclRef = declRef.As<StructDeclRef>())
        {
            auto fieldDeclRef = structDeclRef.GetFields().ToArray()[index];
            return (SpireReflectionVariable*) fieldDeclRef.GetDecl();
        }
    }

    return nullptr;
}

SPIRE_API size_t spReflectionType_GetElementCount(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    if(auto arrayType = dynamic_cast<ArrayExpressionType*>(type))
    {
        return GetIntVal(arrayType->ArrayLength);
    }
    else if( auto vectorType = dynamic_cast<VectorExpressionType*>(type))
    {
        return GetIntVal(vectorType->elementCount);
    }

    return 0;
}

SPIRE_API SpireReflectionType* spReflectionType_GetElementType(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return nullptr;

    if(auto arrayType = dynamic_cast<ArrayExpressionType*>(type))
    {
        return (SpireReflectionType*) arrayType->BaseType.Ptr();
    }
    else if( auto constantBufferType = dynamic_cast<ConstantBufferType*>(type))
    {
        return convert(constantBufferType->elementType.Ptr());
    }
    else if( auto vectorType = dynamic_cast<VectorExpressionType*>(type))
    {
        return convert(vectorType->elementType.Ptr());
    }
    else if( auto matrixType = dynamic_cast<MatrixExpressionType*>(type))
    {
        return convert(matrixType->elementType.Ptr());
    }

    return nullptr;
}

SPIRE_API unsigned int spReflectionType_GetRowCount(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    if(auto matrixType = dynamic_cast<MatrixExpressionType*>(type))
    {
        return GetIntVal(matrixType->rowCount);
    }
    else if(auto vectorType = dynamic_cast<VectorExpressionType*>(type))
    {
        return 1;
    }
    else if( auto basicType = dynamic_cast<BasicExpressionType*>(type) )
    {
        return 1;
    }

    return 0;
}

SPIRE_API unsigned int spReflectionType_GetColumnCount(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    if(auto matrixType = dynamic_cast<MatrixExpressionType*>(type))
    {
        return GetIntVal(matrixType->colCount);
    }
    else if(auto vectorType = dynamic_cast<VectorExpressionType*>(type))
    {
        return GetIntVal(vectorType->elementCount);
    }
    else if( auto basicType = dynamic_cast<BasicExpressionType*>(type) )
    {
        return 1;
    }

    return 0;
}

SPIRE_API SpireScalarType spReflectionType_GetScalarType(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    if(auto matrixType = dynamic_cast<MatrixExpressionType*>(type))
    {
        type = matrixType->elementType.Ptr();
    }
    else if(auto vectorType = dynamic_cast<VectorExpressionType*>(type))
    {
        type = vectorType->elementType.Ptr();
    }

    if(auto basicType = dynamic_cast<BasicExpressionType*>(type))
    {
        switch (basicType->BaseType)
        {
#define CASE(BASE, TAG) \
        case BaseType::BASE: return SPIRE_SCALAR_TYPE_##TAG

            CASE(Void, VOID);
            CASE(Int, INT32);
            CASE(Float, FLOAT32);
            CASE(UInt, UINT32);
            CASE(Bool, BOOL);
            CASE(UInt64, UINT64);

#undef CASE

        default:
            assert(!"unexpected");
            return SPIRE_SCALAR_TYPE_NONE;
            break;
        }
    }

    return SPIRE_SCALAR_TYPE_NONE;
}

SPIRE_API SpireResourceShape spReflectionType_GetResourceShape(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    while(auto arrayType = type->As<ArrayExpressionType>())
    {
        type = arrayType->BaseType.Ptr();
    }

    if(auto textureType = type->As<TextureType>())
    {
        return textureType->getShape();
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE, SHAPE, ACCESS)   \
    else if(type->As<TYPE>()) do {  \
        return SHAPE;               \
    } while(0)

    CASE(HLSLBufferType,                    SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWBufferType,                  SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLBufferType,                    SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWBufferType,                  SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLStructuredBufferType,          SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWStructuredBufferType,        SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLAppendStructuredBufferType,    SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_APPEND);
    CASE(HLSLConsumeStructuredBufferType,   SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_CONSUME);
    CASE(HLSLByteAddressBufferType,         SPIRE_BYTE_ADDRESS_BUFFER,  SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWByteAddressBufferType,       SPIRE_BYTE_ADDRESS_BUFFER,  SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(UntypedBufferResourceType,         SPIRE_BYTE_ADDRESS_BUFFER,  SPIRE_RESOURCE_ACCESS_READ);
#undef CASE

    return SPIRE_RESOURCE_NONE;
}

SPIRE_API SpireResourceAccess spReflectionType_GetResourceAccess(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return 0;

    while(auto arrayType = type->As<ArrayExpressionType>())
    {
        type = arrayType->BaseType.Ptr();
    }

    if(auto textureType = type->As<TextureType>())
    {
        return textureType->getAccess();
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE, SHAPE, ACCESS)   \
    else if(type->As<TYPE>()) do {  \
        return ACCESS;              \
    } while(0)

    CASE(HLSLBufferType,                    SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWBufferType,                  SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLBufferType,                    SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWBufferType,                  SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLStructuredBufferType,          SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWStructuredBufferType,        SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLAppendStructuredBufferType,    SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_APPEND);
    CASE(HLSLConsumeStructuredBufferType,   SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_CONSUME);
    CASE(HLSLByteAddressBufferType,         SPIRE_BYTE_ADDRESS_BUFFER,  SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWByteAddressBufferType,       SPIRE_BYTE_ADDRESS_BUFFER,  SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(UntypedBufferResourceType,         SPIRE_BYTE_ADDRESS_BUFFER,  SPIRE_RESOURCE_ACCESS_READ);
#undef CASE

    return SPIRE_RESOURCE_ACCESS_NONE;
}

SPIRE_API SpireReflectionType* spReflectionType_GetResourceResultType(SpireReflectionType* inType)
{
    auto type = convert(inType);
    if(!type) return nullptr;

    while(auto arrayType = type->As<ArrayExpressionType>())
    {
        type = arrayType->BaseType.Ptr();
    }

    if (auto textureType = type->As<TextureType>())
    {
        return convert(textureType->elementType.Ptr());
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE, SHAPE, ACCESS)                                                       \
    else if(type->As<TYPE>()) do {                                                      \
        return convert(type->As<TYPE>()->elementType.Ptr());                            \
    } while(0)

    CASE(HLSLBufferType,                    SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWBufferType,                  SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLBufferType,                    SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWBufferType,                  SPIRE_TEXTURE_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);

    // TODO: structured buffer needs to expose type layout!

    CASE(HLSLStructuredBufferType,          SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_READ);
    CASE(HLSLRWStructuredBufferType,        SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLAppendStructuredBufferType,    SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_APPEND);
    CASE(HLSLConsumeStructuredBufferType,   SPIRE_STRUCTURED_BUFFER, SPIRE_RESOURCE_ACCESS_CONSUME);
#undef CASE

    return nullptr;
}

// Type Layout Reflection

SPIRE_API SpireReflectionType* spReflectionTypeLayout_GetType(SpireReflectionTypeLayout* inTypeLayout)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    return (SpireReflectionType*) typeLayout->type.Ptr();
}

SPIRE_API size_t spReflectionTypeLayout_GetSize(SpireReflectionTypeLayout* inTypeLayout, SpireParameterCategory category)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    auto info = typeLayout->FindResourceInfo(LayoutResourceKind(category));
    if(!info) return 0;

    return info->count;
}

SPIRE_API SpireReflectionVariableLayout* spReflectionTypeLayout_GetFieldByIndex(SpireReflectionTypeLayout* inTypeLayout, unsigned index)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if(auto structTypeLayout = dynamic_cast<StructTypeLayout*>(typeLayout))
    {
        return (SpireReflectionVariableLayout*) structTypeLayout->fields[index].Ptr();
    }

    return nullptr;
}

SPIRE_API size_t spReflectionTypeLayout_GetElementStride(SpireReflectionTypeLayout* inTypeLayout, SpireParameterCategory category)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    if( auto arrayTypeLayout = dynamic_cast<ArrayTypeLayout*>(typeLayout))
    {
        if(category == SPIRE_PARAMETER_CATEGORY_UNIFORM)
        {
            return arrayTypeLayout->uniformStride;
        }
        else
        {
            auto elementTypeLayout = arrayTypeLayout->elementTypeLayout;
            auto info = elementTypeLayout->FindResourceInfo(LayoutResourceKind(category));
            if(!info) return 0;
            return info->count;
        }
    }

    return 0;
}

SPIRE_API SpireReflectionTypeLayout* spReflectionTypeLayout_GetElementTypeLayout(SpireReflectionTypeLayout* inTypeLayout)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if( auto arrayTypeLayout = dynamic_cast<ArrayTypeLayout*>(typeLayout))
    {
        return (SpireReflectionTypeLayout*) arrayTypeLayout->elementTypeLayout.Ptr();
    }
    else if( auto constantBufferTypeLayout = dynamic_cast<ConstantBufferTypeLayout*>(typeLayout))
    {
        return convert(constantBufferTypeLayout->elementTypeLayout.Ptr());
    }
    else if( auto structuredBufferTypeLayout = dynamic_cast<StructuredBufferTypeLayout*>(typeLayout))
    {
        return convert(structuredBufferTypeLayout->elementTypeLayout.Ptr());
    }

    return nullptr;
}

static SpireParameterCategory getParameterCategory(
    LayoutResourceKind kind)
{
    return SpireParameterCategory(kind);
}

static SpireParameterCategory getParameterCategory(
    TypeLayout*  typeLayout)
{
    auto resourceInfoCount = typeLayout->resourceInfos.Count();
    if(resourceInfoCount == 1)
    {
        return getParameterCategory(typeLayout->resourceInfos[0].kind);
    }
    else if(resourceInfoCount == 0)
    {
        // TODO: can this ever happen?
        return SPIRE_PARAMETER_CATEGORY_NONE;
    }
    return SPIRE_PARAMETER_CATEGORY_MIXED;
}

SPIRE_API SpireParameterCategory spReflectionTypeLayout_GetParameterCategory(SpireReflectionTypeLayout* inTypeLayout)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return SPIRE_PARAMETER_CATEGORY_NONE;

    return getParameterCategory(typeLayout);
}

SPIRE_API unsigned spReflectionTypeLayout_GetCategoryCount(SpireReflectionTypeLayout* inTypeLayout)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    return (unsigned) typeLayout->resourceInfos.Count();
}

SPIRE_API SpireParameterCategory spReflectionTypeLayout_GetCategoryByIndex(SpireReflectionTypeLayout* inTypeLayout, unsigned index)
{
    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return SPIRE_PARAMETER_CATEGORY_NONE;

    return typeLayout->resourceInfos[index].kind;
}

// Variable Reflection

SPIRE_API char const* spReflectionVariable_GetName(SpireReflectionVariable* inVar)
{
    auto var = convert(inVar);
    if(!var) return nullptr;

    return var->getName().begin();
}

SPIRE_API SpireReflectionType* spReflectionVariable_GetType(SpireReflectionVariable* inVar)
{
    auto var = convert(inVar);
    if(!var) return nullptr;

    return  convert(var->getType());
}

// Variable Layout Reflection

SPIRE_API SpireReflectionVariable* spReflectionVariableLayout_GetVariable(SpireReflectionVariableLayout* inVarLayout)
{
    auto varLayout = convert(inVarLayout);
    if(!varLayout) return nullptr;

    return convert(varLayout->varDecl.GetDecl());
}

SPIRE_API SpireReflectionTypeLayout* spReflectionVariableLayout_GetTypeLayout(SpireReflectionVariableLayout* inVarLayout)
{
    auto varLayout = convert(inVarLayout);
    if(!varLayout) return nullptr;

    return convert(varLayout->getTypeLayout());
}

SPIRE_API size_t spReflectionVariableLayout_GetOffset(SpireReflectionVariableLayout* inVarLayout, SpireParameterCategory category)
{
    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    auto info = varLayout->FindResourceInfo(LayoutResourceKind(category));
    if(!info) return 0;

    return info->index;
}

SPIRE_API size_t spReflectionVariableLayout_GetSpace(SpireReflectionVariableLayout* inVarLayout, SpireParameterCategory category)
{
    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    auto info = varLayout->FindResourceInfo(LayoutResourceKind(category));
    if(!info) return 0;

    return info->space;
}


// Shader Parameter Reflection

SPIRE_API unsigned spReflectionParameter_GetBindingIndex(SpireReflectionParameter* inVarLayout)
{
    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    if(varLayout->resourceInfos.Count() > 0)
    {
        return (unsigned) varLayout->resourceInfos[0].index;
    }

    return 0;
}

SPIRE_API unsigned spReflectionParameter_GetBindingSpace(SpireReflectionParameter* inVarLayout)
{
    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    if(varLayout->resourceInfos.Count() > 0)
    {
        return (unsigned) varLayout->resourceInfos[0].space;
    }

    return 0;
}

// Entry Point Reflection

SPIRE_API SpireStage spReflectionEntryPoint_getStage(SpireReflectionEntryPoint* inEntryPoint)
{
    auto entryPointLayout = convert(inEntryPoint);

    if(!entryPointLayout) return SPIRE_STAGE_NONE;

    return SpireStage(entryPointLayout->profile.GetStage());
}

SPIRE_API void spReflectionEntryPoint_getComputeThreadGroupSize(
    SpireReflectionEntryPoint*  inEntryPoint,
    SpireUInt                   axisCount,
    SpireUInt*                  outSizeAlongAxis)
{
    auto entryPointLayout = convert(inEntryPoint);

    if(!entryPointLayout)   return;
    if(!axisCount)          return;
    if(!outSizeAlongAxis)   return;

    auto entryPointFunc = entryPointLayout->entryPoint;
    if(!entryPointFunc) return;

    auto numThreadsAttribute = entryPointFunc->FindModifier<HLSLNumThreadsAttribute>();
    if(!numThreadsAttribute) return;

    if(axisCount > 0) outSizeAlongAxis[0] = numThreadsAttribute->x;
    if(axisCount > 1) outSizeAlongAxis[1] = numThreadsAttribute->y;
    if(axisCount > 2) outSizeAlongAxis[2] = numThreadsAttribute->z;
    for( SpireUInt aa = 3; aa < axisCount; ++aa )
    {
        outSizeAlongAxis[aa] = 1;
    }
}


// Shader Reflection

SPIRE_API unsigned spReflection_GetParameterCount(SpireReflection* inProgram)
{
    auto program = convert(inProgram);
    if(!program) return 0;

    auto globalLayout = program->globalScopeLayout;
    if(auto globalConstantBufferLayout = globalLayout.As<ConstantBufferTypeLayout>())
    {
        globalLayout = globalConstantBufferLayout->elementTypeLayout;
    }

    if(auto globalStructLayout = globalLayout.As<StructTypeLayout>())
    {
        return globalStructLayout->fields.Count();
    }

    return 0;
}

SPIRE_API SpireReflectionParameter* spReflection_GetParameterByIndex(SpireReflection* inProgram, unsigned index)
{
    auto program = convert(inProgram);
    if(!program) return nullptr;

    auto globalLayout = program->globalScopeLayout;
    if(auto globalConstantBufferLayout = globalLayout.As<ConstantBufferTypeLayout>())
    {
        globalLayout = globalConstantBufferLayout->elementTypeLayout;
    }

    if(auto globalStructLayout = globalLayout.As<StructTypeLayout>())
    {
        return convert(globalStructLayout->fields[index].Ptr());
    }

    return nullptr;
}

SPIRE_API SpireUInt spReflection_getEntryPointCount(SpireReflection* inProgram)
{
    auto program = convert(inProgram);
    if(!program) return 0;

    return SpireUInt(program->entryPoints.Count());
}

SPIRE_API SpireReflectionEntryPoint* spReflection_getEntryPointByIndex(SpireReflection* inProgram, SpireUInt index)
{
    auto program = convert(inProgram);
    if(!program) return 0;

    return convert(program->entryPoints[(int) index].Ptr());
}




















namespace Spire {
namespace Compiler {







// Debug helper code: dump reflection data after generation

struct PrettyWriter
{
    StringBuilder sb;
    bool startOfLine = true;
    int indent = 0;
};

static void adjust(PrettyWriter& writer)
{
    if (!writer.startOfLine)
        return;

    int indent = writer.indent;
    for (int ii = 0; ii < indent; ++ii)
        writer.sb << "    ";

    writer.startOfLine = false;
}

static void indent(PrettyWriter& writer)
{
    writer.indent++;
}

static void dedent(PrettyWriter& writer)
{
    writer.indent--;
}

static void write(PrettyWriter& writer, char const* text)
{
    // TODO: can do this more efficiently...
    char const* cursor = text;
    for(;;)
    {
        char c = *cursor++;
        if (!c) break;

        if (c == '\n')
        {
            writer.startOfLine = true;
        }
        else
        {
            adjust(writer);
        }

        writer.sb << c;
    }
}

static void write(PrettyWriter& writer, UInt val)
{
    adjust(writer);
    writer.sb << ((unsigned int) val);
}

static void emitReflectionVarInfoJSON(PrettyWriter& writer, spire::VariableReflection* var);
static void emitReflectionTypeLayoutJSON(PrettyWriter& writer, spire::TypeLayoutReflection* type);
static void emitReflectionTypeJSON(PrettyWriter& writer, spire::TypeReflection* type);

static void emitReflectionVarBindingInfoJSON(
    PrettyWriter&           writer,
    SpireParameterCategory  category,
    UInt                    index,
    UInt                    count,
    UInt                    space = 0)
{
    if( category == SPIRE_PARAMETER_CATEGORY_UNIFORM )
    {
        write(writer,"\"kind\": \"uniform\"");
        write(writer, ", ");
        write(writer,"\"offset\": ");
        write(writer, index);
        write(writer, ", ");
        write(writer, "\"size\": ");
        write(writer, count);
    }
    else
    {
        write(writer, "\"kind\": \"");
        switch( category )
        {
    #define CASE(NAME, KIND) case SPIRE_PARAMETER_CATEGORY_##NAME: write(writer, #KIND); break
    CASE(CONSTANT_BUFFER, constantBuffer);
    CASE(SHADER_RESOURCE, shaderResource);
    CASE(UNORDERED_ACCESS, unorderedAccess);
    CASE(VERTEX_INPUT, vertexInput);
    CASE(FRAGMENT_OUTPUT, fragmentOutput);
    CASE(SAMPLER_STATE, samplerState);
    #undef CASE

        default:
            write(writer, "unknown");
            assert(!"unexpected");
            break;
        }
        write(writer, "\"");
        if( space )
        {
            write(writer, ", ");
            write(writer, "\"space\": ");
            write(writer, space);
        }
        write(writer, ", ");
        write(writer, "\"index\": ");
        write(writer, index);
        if( count != 1)
        {
            write(writer, ", ");
            write(writer, "\"count\": ");
            write(writer, count);
        }
    }
}

static void emitReflectionVarBindingInfoJSON(
    PrettyWriter&                       writer,
    spire::VariableLayoutReflection*    var)
{
    auto typeLayout = var->getTypeLayout();
    auto categoryCount = var->getCategoryCount();

    if( categoryCount != 1 )
    {
        write(writer,"\"bindings\": [\n");
    }
    else
    {
        write(writer,"\"binding\": ");
    }
    indent(writer);

    for(uint32_t cc = 0; cc < categoryCount; ++cc )
    {
        auto category = var->getCategoryByIndex(cc);
        auto index = var->getOffset(category);
        auto space = var->getBindingSpace(category);
        auto count = typeLayout->getSize(category);

        if (cc != 0) write(writer, ",\n");

        write(writer,"{");
        emitReflectionVarBindingInfoJSON(
            writer,
            category,
            index,
            count,
            space);
        write(writer,"}");
    }

    dedent(writer);
    if( categoryCount != 1 )
    {
        write(writer,"\n]");
    }
}

static void emitReflectionNameInfoJSON(
    PrettyWriter&   writer,
    char const*     name)
{
    // TODO: deal with escaping special characters if/when needed
    write(writer, "\"name\": \"");
    write(writer, name);
    write(writer, "\"");
}

static void emitReflectionVarLayoutJSON(
    PrettyWriter&                       writer,
    spire::VariableLayoutReflection*    var)
{
    write(writer, "{\n");
    indent(writer);

    emitReflectionNameInfoJSON(writer, var->getName());
    write(writer, ",\n");

    write(writer, "\"type\": ");
    emitReflectionTypeLayoutJSON(writer, var->getTypeLayout());
    write(writer, ",\n");

    emitReflectionVarBindingInfoJSON(writer, var);

    dedent(writer);
    write(writer, "\n}");
}

static void emitReflectionScalarTypeInfoJSON(
    PrettyWriter&   writer,
    SpireScalarType scalarType)
{
    write(writer, "\"scalarType\": \"");
    switch (scalarType)
    {
    default:
        write(writer, "unknown");
        assert(!"unexpected");
        break;
#define CASE(TAG, ID) case spire::TypeReflection::ScalarType::TAG: write(writer, #ID); break
        CASE(Void, void);
        CASE(Bool, bool);
        CASE(Int32, int32);
        CASE(UInt32, uint32);
        CASE(Int64, int64);
        CASE(UInt64, uint64);
        CASE(Float16, float16);
        CASE(Float32, float32);
        CASE(Float64, float64);
#undef CASE
    }
    write(writer, "\"");
}

static void emitReflectionTypeInfoJSON(
    PrettyWriter&           writer,
    spire::TypeReflection*  type)
{
    switch( type->getKind() )
    {
    case SPIRE_TYPE_KIND_SAMPLER_STATE:
        write(writer, "\"kind\": \"samplerState\"");
        break;

    case SPIRE_TYPE_KIND_RESOURCE:
        {
            auto shape  = type->getResourceShape();
            auto access = type->getResourceAccess();
            write(writer, "\"kind\": \"resource\"");
            write(writer, ",\n");
            write(writer, "\"baseShape\": \"");
            switch (shape & SPIRE_RESOURCE_BASE_SHAPE_MASK)
            {
            default:
                write(writer, "unknown");
                assert(!"unexpected");
                break;

#define CASE(SHAPE, NAME) case SPIRE_##SHAPE: write(writer, #NAME); break
                CASE(TEXTURE_1D, texture1D);
                CASE(TEXTURE_2D, texture2D);
                CASE(TEXTURE_3D, texture3D);
                CASE(TEXTURE_CUBE, textureCube);
                CASE(TEXTURE_BUFFER, textureBuffer);
                CASE(STRUCTURED_BUFFER, structuredBuffer);
                CASE(BYTE_ADDRESS_BUFFER, byteAddressBuffer);
#undef CASE
            }
            write(writer, "\"");
            if (shape & SPIRE_TEXTURE_ARRAY_FLAG)
            {
                write(writer, ",\n");
                write(writer, "\"array\": true");
            }
            if (shape & SPIRE_TEXTURE_MULTISAMPLE_FLAG)
            {
                write(writer, ",\n");
                write(writer, "\"multisample\": true");
            }

            if( access != SPIRE_RESOURCE_ACCESS_READ )
            {
                write(writer, ",\n\"access\": \"");
                switch(access)
                {
                default:
                    write(writer, "unknown");
                    assert(!"unexpected");
                    break;

                case SPIRE_RESOURCE_ACCESS_READ:
                    break;

                case SPIRE_RESOURCE_ACCESS_READ_WRITE:      write(writer, "readWrite"); break;
                case SPIRE_RESOURCE_ACCESS_RASTER_ORDERED:  write(writer, "rasterOrdered"); break;
                case SPIRE_RESOURCE_ACCESS_APPEND:          write(writer, "append"); break;
                case SPIRE_RESOURCE_ACCESS_CONSUME:         write(writer, "consume"); break;
                }
                write(writer, "\"");
            }
        }
        break;

    case SPIRE_TYPE_KIND_CONSTANT_BUFFER:
        write(writer, "\"kind\": \"constantBuffer\"");
        write(writer, ",\n");
        write(writer, "\"elementType\": ");
        emitReflectionTypeJSON(
            writer,
            type->getElementType());
        break;

    case SPIRE_TYPE_KIND_SCALAR:
        write(writer, "\"kind\": \"scalar\"");
        write(writer, ",\n");
        emitReflectionScalarTypeInfoJSON(
            writer,
            type->getScalarType());
        break;

    case SPIRE_TYPE_KIND_VECTOR:
        write(writer, "\"kind\": \"vector\"");
        write(writer, ",\n");
        write(writer, "\"elementCount\": ");
        write(writer, type->getElementCount());
        write(writer, ",\n");
        write(writer, "\"elementType\": ");
        emitReflectionTypeJSON(
            writer,
            type->getElementType());
        break;

    case SPIRE_TYPE_KIND_MATRIX:
        write(writer, "\"kind\": \"matrix\"");
        write(writer, ",\n");
        write(writer, "\"rowCount\": ");
        write(writer, type->getRowCount());
        write(writer, ",\n");
        write(writer, "\"columnCount\": ");
        write(writer, type->getColumnCount());
        write(writer, ",\n");
        write(writer, "\"elementType\": ");
        emitReflectionTypeJSON(
            writer,
            type->getElementType());
        break;

    case SPIRE_TYPE_KIND_ARRAY:
        {
            auto arrayType = type;
            write(writer, "\"kind\": \"array\"");
            write(writer, ",\n");
            write(writer, "\"elementCount\": ");
            write(writer, arrayType->getElementCount());
            write(writer, ",\n");
            write(writer, "\"elementType\": ");
            emitReflectionTypeJSON(writer, arrayType->getElementType());
        }
        break;

    case SPIRE_TYPE_KIND_STRUCT:
        {
            write(writer, "\"kind\": \"struct\",\n");
            write(writer, "\"fields\": [\n");
            indent(writer);

            auto structType = type;
            auto fieldCount = structType->getFieldCount();
            for( uint32_t ff = 0; ff < fieldCount; ++ff )
            {
                if (ff != 0) write(writer, ",\n");
                emitReflectionVarInfoJSON(
                    writer,
                    structType->getFieldByIndex(ff));
            }
            dedent(writer);
            write(writer, "\n]");
        }
        break;

    default:
        assert(!"unimplemented");
        break;
    }
}

static void emitReflectionTypeLayoutInfoJSON(
    PrettyWriter&                   writer,
    spire::TypeLayoutReflection*    typeLayout)
{
    switch( typeLayout->getKind() )
    {
    default:
        emitReflectionTypeInfoJSON(writer, typeLayout->getType());
        break;

    case SPIRE_TYPE_KIND_ARRAY:
        {
            auto arrayTypeLayout = typeLayout;
            auto elementTypeLayout = arrayTypeLayout->getElementTypeLayout();
            write(writer, "\"kind\": \"array\"");
            write(writer, ",\n");
            write(writer, "\"elementCount\": ");
            write(writer, arrayTypeLayout->getElementCount());
            write(writer, ",\n");
            write(writer, "\"elementType\": ");
            emitReflectionTypeLayoutJSON(
                writer,
                elementTypeLayout);
            if (arrayTypeLayout->getSize(SPIRE_PARAMETER_CATEGORY_UNIFORM) != 0)
            {
                write(writer, ",\n");
                write(writer, "\"uniformStride\": ");
                write(writer, arrayTypeLayout->getElementStride(SPIRE_PARAMETER_CATEGORY_UNIFORM));
            }
        }
        break;

    case SPIRE_TYPE_KIND_STRUCT:
        {
            write(writer, "\"kind\": \"struct\",\n");
            write(writer, "\"fields\": [\n");
            indent(writer);

            auto structTypeLayout = typeLayout;
            auto fieldCount = structTypeLayout->getFieldCount();
            for( uint32_t ff = 0; ff < fieldCount; ++ff )
            {
                if (ff != 0) write(writer, ",\n");
                emitReflectionVarLayoutJSON(
                    writer,
                    structTypeLayout->getFieldByIndex(ff));
            }
            dedent(writer);
            write(writer, "\n]");
        }
        break;

    case SPIRE_TYPE_KIND_CONSTANT_BUFFER:
        write(writer, "\"kind\": \"constantBuffer\"");
        write(writer, ",\n");
        write(writer, "\"elementType\": ");
        emitReflectionTypeLayoutJSON(
            writer,
            typeLayout->getElementTypeLayout());
        break;

    }

    // TODO: emit size info for types
}

static void emitReflectionTypeLayoutJSON(
    PrettyWriter&                   writer,
    spire::TypeLayoutReflection*    typeLayout)
{
    write(writer, "{\n");
    indent(writer);
    emitReflectionTypeLayoutInfoJSON(writer, typeLayout);
    dedent(writer);
    write(writer, "\n}");
}

static void emitReflectionTypeJSON(
    PrettyWriter&           writer,
    spire::TypeReflection*  type)
{
    write(writer, "{\n");
    indent(writer);
    emitReflectionTypeInfoJSON(writer, type);
    dedent(writer);
    write(writer, "\n}");
}

static void emitReflectionVarInfoJSON(
    PrettyWriter&               writer,
    spire::VariableReflection*  var)
{
    emitReflectionNameInfoJSON(writer, var->getName());
    write(writer, ",\n");

    write(writer, "\"type\": ");
    emitReflectionTypeJSON(writer, var->getType());
}

#if 0
static void emitReflectionBindingInfoJSON(
    PrettyWriter& writer,
    
    ReflectionParameterNode* param)
{
    auto info = &param->binding;

    if( info->category == SPIRE_PARAMETER_CATEGORY_MIXED )
    {
        write(writer,"\"bindings\": [\n");
        indent(writer);

        ReflectionSize bindingCount = info->bindingCount;
        assert(bindingCount);
        ReflectionParameterBindingInfo* bindings = info->bindings;
        for( ReflectionSize bb = 0; bb < bindingCount; ++bb )
        {
            if (bb != 0) write(writer, ",\n");

            write(writer,"{");
            auto& binding = bindings[bb];
            emitReflectionVarBindingInfoJSON(
                writer,
                binding.category,
                binding.index,
                (ReflectionSize) param->GetTypeLayout()->GetSize(binding.category),
                binding.space);

            write(writer,"}");
        }
        dedent(writer);
        write(writer,"\n]");
    }
    else
    {
        write(writer,"\"binding\": {");
        indent(writer);

        emitReflectionVarBindingInfoJSON(
            writer,
            info->category,
            info->index,
            (ReflectionSize) param->GetTypeLayout()->GetSize(info->category),
            info->space);

        dedent(writer);
        write(writer,"}");
    }
}
#endif

static void emitReflectionParamJSON(
    PrettyWriter&                       writer,
    spire::VariableLayoutReflection*    param)
{
    write(writer, "{\n");
    indent(writer);

    emitReflectionNameInfoJSON(writer, param->getName());
    write(writer, ",\n");

    emitReflectionVarBindingInfoJSON(writer, param);
    write(writer, ",\n");

    write(writer, "\"type\": ");
    emitReflectionTypeLayoutJSON(writer, param->getTypeLayout());

    dedent(writer);
    write(writer, "\n}");
}

template<typename T>
struct Range
{
public:
    Range(
        T begin,
        T end)
        : mBegin(begin)
        , mEnd(end)
    {}

    struct Iterator
    {
    public:
        explicit Iterator(T value)
            : mValue(value)
        {}

        T operator*() const { return mValue; }
        void operator++() { mValue++; }

        bool operator!=(Iterator const& other)
        {
            return mValue != other.mValue;
        }

    private:
        T mValue;
    };

    Iterator begin() const { return Iterator(mBegin); }
    Iterator end()   const { return Iterator(mEnd); }

private:
    T mBegin;
    T mEnd;
};

template<typename T>
Range<T> range(T begin, T end)
{
    return Range<T>(begin, end);
}

template<typename T>
Range<T> range(T end)
{
    return Range<T>(T(0), end);
}

static void emitReflectionJSON(
    PrettyWriter&               writer,
    spire::ShaderReflection*    programReflection)
{
    write(writer, "{\n");
    indent(writer);
    write(writer, "\"parameters\": [\n");
    indent(writer);

    auto parameterCount = programReflection->getParameterCount();
    for( auto pp : range(parameterCount) )
    {
        if(pp != 0) write(writer, ",\n");

        auto parameter = programReflection->getParameterByIndex(pp);
        emitReflectionParamJSON(writer, parameter);
    }

    dedent(writer);
    write(writer, "\n]");
    dedent(writer);
    write(writer, "\n}\n");
}

#if 0
ReflectionBlob* ReflectionBlob::Create(
    CollectionOfTranslationUnits*   program)
{
    ReflectionGenerationContext context;
    ReflectionBlob* blob = GenerateReflectionBlob(&context, program);
#if 0
    String debugDump = blob->emitAsJSON();
    OutputDebugStringA("REFLECTION BLOB\n");
    OutputDebugStringA(debugDump.begin());
#endif
    return blob;
}
#endif

// JSON emit logic



String emitReflectionJSON(
    ProgramLayout* programLayout)
{
    auto programReflection = (spire::ShaderReflection*) programLayout;

    PrettyWriter writer;
    emitReflectionJSON(writer, programReflection);
    return writer.sb.ProduceString();
}

}}
