/***************************************************************************
# Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
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
#include "ShaderCrossCompiler.h"

#include "Framework.h"

#include "Externals/glslang/glslang/Include/ShHandle.h"
#include "Externals/glslang/glslang/Include/revision.h"
#include "Externals/glslang/glslang/Public/ShaderLang.h"
#include "Externals/glslang/glslang/MachineIndependent/localintermediate.h"
#include "Externals/glslang/glslang/MachineIndependent/SymbolTable.h"
#include "Externals/glslang/glslang/Include/Common.h"
#include "Externals/glslang/glslang/Include/revision.h"

#if 0
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

struct StringSpan
{
    char const* begin;
    char const* end;
};

struct SourceFile
{
    StringSpan text;
    char const* path;
};

#define NO_LIMIT 9999
static const TBuiltInResource kResourceLimits =
{
    // 83 integer limits
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT, NO_LIMIT,
    NO_LIMIT, NO_LIMIT, NO_LIMIT,

    // 9 boolean capabilities
    true, true, true, true, true, true, true, true, true,
};

struct Options
{
    char const* inputPath;
    char const* outputPath;
    char const* entryPointName;

    std::vector<char const*> includePaths;
};

StringSpan tryReadTextFile(char const* path)
{
    FILE* file = fopen(path, "rb");
    if(!file)
    {
        StringSpan result = { 0 ,0 };
        return result;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*) malloc(size+1);
    if(!buffer)
    {
        fclose(file);

        StringSpan result = { 0 ,0 };
        return result;
    }
    fread(buffer, size, 1, file);
    buffer[size] = 0;

#if 0 // hack to append a newline to end of buffer if needed
    if(size > 0 && buffer[size-1] != '\r' && buffer[size-1] != '\n')
    {
        buffer[size] = '\n';
        size = size+1;
    }
#endif

    fclose(file);

    StringSpan text = { buffer, buffer + size };
    return text;
}

StringSpan readTextFile(char const* path)
{
    StringSpan result = tryReadTextFile(path);
    if(!result.begin)
    {
        fprintf(stderr, "failed to read file '%s'\n", path);
    }
    return result;
}

class IncluderImpl : public glslang::TShader::Includer
{
public:
    Options* options;

    IncluderImpl(Options* options)
        : options(options)
    {}

    virtual IncludeResult* include(const char*  requestedPath,
                                    IncludeType   includeType,
                                    const char*   requesterPath,
                                    size_t        includeDepth)
    {
        for(auto includeDir : options->includePaths)
        {
            std::string resolvedPath = std::string(includeDir) + requestedPath;
            StringSpan text = tryReadTextFile(resolvedPath.c_str());
            if(!text.begin)
            {
                continue;
            }

            IncludeResult* result = new IncludeResult(resolvedPath, text.begin, text.end - text.begin, NULL);
            return result;
        }

        return NULL;
    }

    virtual void releaseInclude(IncludeResult* result)
    {
        free((void*) result->file_data);
        delete result;
    }
};

class HackShader : public glslang::TShader
{
public:
    glslang::TIntermediate* getIntermediate() { return intermediate; }
};

struct EmitSpan;

struct EmitSpanList
{
    EmitSpan*   first;
    EmitSpan*   last;
};

struct EmitSpan
{
    char*           buffer;
    char*           end;
    char*           cursor;
    EmitSpan*       next;
    EmitSpanList    children;
};

void initEmitSpan(EmitSpan* span)
{
    memset(span, 0, sizeof(*span));
}


struct EmitSymbolInfo
{
    std::string name; // might be different than original symbol name
    std::string prefix;
};

struct EmitStructInfo
{
    // TODO: anything worth tracking?
};

struct EmitNameScope
{
    EmitNameScope*  parent = NULL;
    std::set<std::string> namesUsed;
};

bool isNameUsed(EmitNameScope* scope, std::string const& name)
{
    return scope->namesUsed.find(name) != scope->namesUsed.end();
}

void markNameUsed(EmitNameScope* scope, std::string const& name)
{
    scope->namesUsed.insert(name);
}

struct SharedEmitContext
{
    glslang::TShader* glslangShader;
    char const* entryPointName;
    EmitSpan* mainSpan;
    EmitSpan* structDeclSpan;
    EmitSpan* cbufferDeclSpan;
    EmitSpan* globalVarDeclSpan;
    EmitSpan* inputDeclSpan;
    EmitSpan* systemInputDeclSpan;
    EmitSpan* outputDeclSpan;
    EmitSpan* systemOutputDeclSpan;

    std::map<int, EmitSymbolInfo> mapSymbol;
    std::map<glslang::TTypeList const*, EmitStructInfo> mapStruct;
    EmitNameScope reservedNames;
    EmitNameScope globalNames;
    glslang::TIntermAggregate* entryPointFunc = NULL;
    int inputCounter = 0;
    int outputCounter = 0;
    glslang::TType const* systemInputsUsed[glslang::EbvLast];
    glslang::TType const* systemOutputsUsed[glslang::EbvLast];

    EmitSpanList    spans;

    SharedEmitContext()
    {
        memset(&spans, 0, sizeof(spans));
        memset(&systemInputsUsed, 0, sizeof(systemInputsUsed));
        memset(&systemOutputsUsed, 0, sizeof(systemOutputsUsed));
    }

};

EmitSpan* allocateSpan()
{
    EmitSpan* span = new EmitSpan();
    initEmitSpan(span);
    return span;
}

void appendSpan(EmitSpanList* list, EmitSpan* span)
{
    if(auto last = list->last)
    {
        last->next = span;
    }
    else
    {
        list->first = span;
    }
    list->last = span;
}

EmitSpan* allocateSpan(EmitSpanList* list)
{
    EmitSpan* span = allocateSpan();
    appendSpan(list, span);
    return span;
}

EmitSpan* allocateSpan(SharedEmitContext* shared)
{
    // TODO: another allocator here?
    return allocateSpan(&shared->spans);
}
struct EmitContext
{
    SharedEmitContext*  shared;

    // span for emitting main content
    EmitSpan*           span;

    // span for declaring local variables
    EmitSpan*           localSpan;

    // separator to put between decls when we need to split them...
    char const* declSeparator;

    // scope for checking that names are unique
    EmitNameScope*  nameScope;
};

void emit(EmitContext* context, char const* text);

EmitSpan* allocateSpan(EmitContext* context)
{
    EmitSpan* span = allocateSpan(context->shared);
    context->span = span;
    return span;
}

void initSharedEmitContext(SharedEmitContext* shared)
{
    // set up global name scopes
    shared->globalNames.parent = &shared->reservedNames;

    // reserve names that are keywords in HLSL, but not GLSL
    markNameUsed(&shared->reservedNames, "texture");
    markNameUsed(&shared->reservedNames, "linear");

    // create the sequence of spans we need

    EmitContext contextImpl = {shared};
    EmitContext* context = &contextImpl;

    allocateSpan(context);
    emit(context, "// automatically generated code, do not edit\n");
    emit(context, "\n// type declarations\n");
    shared->structDeclSpan = allocateSpan(context);

    allocateSpan(context);
    emit(context, "\n// uniform buffers\n");
    shared->cbufferDeclSpan = allocateSpan(context);

    allocateSpan(context);
    emit(context, "\n// stage input\n");
    emit(context, "struct STAGE_INPUT\n{\n");
    shared->inputDeclSpan           = allocateSpan(context);
    shared->systemInputDeclSpan     = allocateSpan(context);

    allocateSpan(context);
    emit(context, "};\n");
    emit(context, "\n// stage output\n");
    emit(context, "struct STAGE_OUTPUT\n{\n");
    shared->outputDeclSpan          = allocateSpan(context);
    shared->systemOutputDeclSpan    = allocateSpan(context);

    allocateSpan(context);
    emit(context, "};\n// global variables\n");
    shared->globalVarDeclSpan = allocateSpan(context);
    emit(context, "static STAGE_INPUT stage_input;\n");
    emit(context, "static STAGE_OUTPUT stage_output;\n");

    allocateSpan(context);
    emit(context, "\n// functions\n");
    shared->mainSpan = allocateSpan(context);
}



void emitExp(EmitContext* context, TIntermNode* node);
void emitTypedDecl(EmitContext* context, glslang::TType const& type, glslang::TString const& name);
void emitTypedDecl(EmitContext* context, glslang::TType const& type, std::string const& name);

void internalError(EmitContext* context, char const* message)
{
    Falcor::Logger::log(Falcor::Logger::Level::Error, std::string("shader cross-compiler internal error: ") + message);
}

void emitData(EmitSpan* span, char const* data, size_t size)
{
    char* cursor = span->cursor;
    char* end = span->end;
    while(cursor + size > end)
    {
        char* buffer = span->buffer;
        size_t oldSize = end - buffer;

        size_t newSize = oldSize ? 2*oldSize : 1024;
        char* newBuffer = (char*)realloc(buffer, newSize);
        if(!newBuffer)
        {
            internalError(NULL, "out of memory\n");
            exit(1);
        }

        span->buffer = newBuffer;
        end = newBuffer + newSize;
        cursor = newBuffer + (cursor - buffer);

        span->end = end;
    }

    memcpy(cursor, data, size);
    span->cursor = cursor + size;
}

void emit(EmitContext* context, char const* begin, char const* end)
{
    emitData(context->span, begin, end-begin);
}

void emit(EmitContext* context, char const* text)
{
    emit(context, text, text + strlen(text));
}

void emitInt(EmitContext* context, int val)
{
    char buffer[1024];
    sprintf(buffer, "%d", val);
    emit(context, buffer);
}

void emitUInt(EmitContext* context, unsigned val)
{
    char buffer[1024];
    sprintf(buffer, "%u", val);
    emit(context, buffer);
}

void emitDouble(EmitContext* context, double val)
{
    char buffer[1024];
    sprintf(buffer, "%f", val);
    emit(context, buffer);
}

void emitFloat(EmitContext* context, double val)
{
    char buffer[1024];
    sprintf(buffer, "%ff", val);
    emit(context, buffer);
}

static char const* const kReservedWords[] =
{
    "linear",
    "texture",
    NULL,
};

void emit(EmitContext* context, std::string const& name)
{
    char const* text = name.c_str();
    emit(context, text, text + name.length());
}

void emit(EmitContext* context, glslang::TString const& name)
{
    char const* text = name.c_str();
    emit(context, text, text + name.length());
}

void emitFuncName(EmitContext* context, glslang::TString const& name)
{
    char const* text = name.c_str();
    char const* end = strchr(text, '(');
    if(!end)
    {
        end = text + name.length();
    }
    emit(context, text, end);
}

enum DeclaratorFlavor
{
    kDeclaratorFlavor_Name,
    kDeclaratorFlavor_FuncName,
    kDeclaratorFlavor_Array,
    kDeclaratorFlavor_Suffix,
    kDeclaratorFlavor_Semantic,
};

struct Declarator
{
    DeclaratorFlavor    flavor;
    Declarator*         next;
    union
    {
        char const*             name;
        glslang::TType const*   type;
        char const*             suffix;
        struct
        {
            char const* name;
            int         index;
        } semantic;
    };
};

void emitDeclarator(EmitContext* context, Declarator* declarator)
{
    if(!declarator)
        return;

    emitDeclarator(context, declarator->next);
    switch(declarator->flavor)
    {
    case kDeclaratorFlavor_Name:
        emit(context, " ");
        emit(context, declarator->name);
        break;

    case kDeclaratorFlavor_FuncName:
        emit(context, " ");
        emitFuncName(context, declarator->name);
        break;

    case kDeclaratorFlavor_Semantic:
        emit(context, " : ");
        emit(context, declarator->semantic.name);
        if(declarator->semantic.index)
        {
            emitInt(context, declarator->semantic.index);
        }
        break;

    case kDeclaratorFlavor_Suffix:
        emit(context, declarator->suffix);
        break;

    case kDeclaratorFlavor_Array:
        {
            glslang::TArraySizes const* arraySizes = declarator->type->getArraySizes();
            int rank = arraySizes->getNumDims();
            for(int rr = 0; rr < rank; ++rr)
            {
                int extent = arraySizes->getDimSize(rr);
                emit(context, "[");
                if(extent > 0)
                    emitInt(context, extent);
                emit(context, "]");
            }
        }
        break;

    default:
        internalError(context, "uhandled case in 'emitDeclarator'");
        break;
    }
}

void emitAggTypeMemberDecls(EmitContext* context, glslang::TTypeList const* fields)
{
    for(auto field : *fields)
    {
        // Here we check if the field name is a reserved word, and rename it if needed.
        // TODO: need a more robust handling here, since all the use sites need
        // to deal with this as well...
        std::string fieldName = field.type->getFieldName().c_str();
        if(isNameUsed(&context->shared->reservedNames, fieldName))
        {
            fieldName += "_1";
        }

        emitTypedDecl(context, *field.type, fieldName);
        emit(context, ";\n");
    }
}

void emitStructDecl(EmitContext* inContext, glslang::TType const& type, glslang::TTypeList const* fields)
{
    EmitSpan* span = allocateSpan();

    EmitContext subContext = *inContext;
    subContext.span = span;
    subContext.declSeparator = NULL;
    EmitContext* context = &subContext;

    emit(context, "struct ");
    emit(context, type.getTypeName());
    emit(context, "\n{\n");
    emitAggTypeMemberDecls(context, fields);
    emit(context, "};\n");

    appendSpan(&subContext.shared->structDeclSpan->children, span);
}

void ensureStructDecl(EmitContext* context, glslang::TType const& type)
{
    glslang::TTypeList const* fields = type.getStruct();
    assert(fields);

    auto ii = context->shared->mapStruct.find(fields);
    if(ii == context->shared->mapStruct.end())
    {
        emitStructDecl(context, type, fields);
        context->shared->mapStruct.insert(std::make_pair(fields, EmitStructInfo()));
    }
}

void emitTextureTypedDecl(EmitContext* context, glslang::TType const& type, glslang::TSampler const& sampler, Declarator* declarator)
{
    switch(sampler.dim)
    {
    case glslang::Esd1D:      emit(context, "Texture1D");   break;
    case glslang::Esd2D:      emit(context, "Texture2D");   break;
    case glslang::Esd3D:      emit(context, "Texture3D");   break;
    case glslang::EsdCube:    emit(context, "TextureCube"); break;
    case glslang::EsdRect:    emit(context, "Texture2D");   break; // TODO: is this correct?
    case glslang::EsdBuffer:  emit(context, "Buffer"); break;
    case glslang::EsdSubpass:
    default:
        internalError(context, "unhandled case in 'emitTextureTypedDecl'");
        break;
    }

    if(sampler.ms)
    {
        emit(context, "MS");
    }
    if(sampler.arrayed)
    {
        emit(context, "Array");
    }

    switch (sampler.type) {
    case glslang::EbtFloat:
        break;
    case glslang::EbtInt:   emit(context, "<int4>");    break;
    case glslang::EbtUint:  emit(context, "<uint4>");   break;
    default:
        internalError(context, "unhandled case in 'emitTextureTypedDecl'");
        break;
    }

}

void emitSamplerTypedDecl(EmitContext* context, glslang::TType const& type, glslang::TSampler const& sampler, Declarator* declarator)
{
    if(sampler.shadow)
    {
        emit(context, "SamplerComparisonState");
    }
    else
    {
        emit(context, "SamplerState");
    }
}

void emitSimpleTypedDecl(EmitContext* context, glslang::TType const& type, Declarator* declarator)
{
    // only dealing with HLSL for now
    switch(type.getBasicType())
    {
#define CASE(N,T) case glslang::Ebt##N: emit(context, #T); break
    CASE(Void, void);
    CASE(Float, float);
    CASE(Double, double);
    CASE(Int, int);
    CASE(Uint, uint);
    CASE(Int64, int64_t);
    CASE(Uint64, uint64_t);
    CASE(Bool, bool);
    CASE(AtomicUint, atomic_uint); // TODO: not actually in HLSL
#undef CASE

    case glslang::EbtSampler:
        {
            // TODO: the tricky part here is that things that are a single type
            // in GLSL, like `sampler2d` are actually a pair of types in HLSL:
            // `Texture2D` and `SamplerState`.
            //
            // The obvious solution is to create an HLSL struct with two fields
            // and use *that*, but there are rules against using opaque types
            // in various ways that will make that break down.

            glslang::TSampler const& sampler = type.getSampler();
            if(sampler.sampler || sampler.combined)
            {
                // TODO: sort out correct type...
                
                Declarator suffix = { kDeclaratorFlavor_Suffix, declarator };

                suffix.suffix = "_tex";
                emitTextureTypedDecl(context, type, sampler, &suffix);
                emitDeclarator(context, &suffix);

                // TODO: need to emit an appropriate separator here!
                if(context->declSeparator)
                {
                    emit(context, context->declSeparator);
                }
                else
                {
                    emit(context, ";\n");
                }

                suffix.suffix = "_samp";
                emitSamplerTypedDecl(context, type, sampler, &suffix);
                emitDeclarator(context, &suffix);
                return;
            }
            else
            {
                // actually an image
                internalError(context, "uhandled case in 'emitTypeSpecifier'");
            }
        }
        break;

    case glslang::EbtStruct:
    case glslang::EbtBlock:
        {
            ensureStructDecl(context, type);

            // TODO: this assumes `struct` declarations will
            // always be encountered before references, and
            // that we have no scoping issues...
            emit(context, type.getTypeName());
        }
        break;

    default:
        internalError(context, "uhandled case in 'emitTypeSpecifier'");
        break;
    }

    if(type.isMatrix())
    {
        emitInt(context, type.getMatrixRows());
        emit(context, "x");
        emitInt(context, type.getMatrixCols());
    }
    else if(type.getVectorSize() > 1)
    {
        emitInt(context, type.getVectorSize());
    }

    emitDeclarator(context, declarator);
}

void emitTypedDecl(EmitContext* context, glslang::TType const& type, Declarator* declarator)
{
    Declarator arrayDeclarator = { kDeclaratorFlavor_Array, declarator };
    if( type.isArray() )
    {
        arrayDeclarator.type = &type;
        declarator = &arrayDeclarator;
    }

    emitSimpleTypedDecl(context, type, declarator);
}

void emitTypedDecl(EmitContext* context, glslang::TType const& type, std::string const& name)
{
    Declarator nameDeclarator = { kDeclaratorFlavor_Name, NULL };
    nameDeclarator.name = name.c_str();
    emitTypedDecl(context, type, &nameDeclarator);
}

void emitTypedDecl(EmitContext* context, glslang::TType const& type, glslang::TString const& name)
{
    Declarator nameDeclarator = { kDeclaratorFlavor_Name, NULL };
    nameDeclarator.name = name.c_str();
    emitTypedDecl(context, type, &nameDeclarator);
}

void emitTypeName(EmitContext* context, glslang::TType const& type)
{
    emitTypedDecl(context, type, NULL);
}

void emitArg(EmitContext* context, TIntermNode* node)
{
    if(auto typed = node->getAsTyped())
    {
        glslang::TType const& type = typed->getType();
        if(type.getBasicType() == glslang::EbtSampler)
        {
            glslang::TSampler const& sampler = type.getSampler();
            if(sampler.sampler || sampler.combined)
            {
                // HACK: emit the exp twice, with a different suffix each time:
                emitExp(context, node);
                emit(context, "_tex, ");
                emitExp(context, node);
                emit(context, "_samp");
                return;
            }
        }
    }

    emitExp(context, node);
}

void emitArgList(EmitContext* context, glslang::TIntermAggregate* node)
{
    bool first = true;
    for(auto arg : node->getSequence())
    {
        if(!first) emit(context, ", ");
        first = false;

        emitArg(context, arg);
    }
}

void emitBuiltinCall(EmitContext* context, char const* name, glslang::TIntermAggregate* node)
{
    emit(context, name);
    emit(context, "(");
    emitArgList(context, node);
    emit(context, ")");
}

void emitTextureCall(EmitContext* context, char const* format, glslang::TIntermAggregate* node)
{
    char const* cursor = format;

    char const* beginSpan = cursor;
    for(;;)
    {
        char const* endSpan = cursor;
        int c = *cursor++;
        if(!c)
        {
            emit(context, beginSpan, endSpan);
            return;
        }

        if(c != '$')
        {
            continue;
        }

        emit(context, beginSpan, endSpan);

        c = *cursor++;
        switch(c)
        {
        default:
            internalError(context, "invalid format string for texture call\n");
            return;

        // "$0" through "$9" just print the corresponding argument directly
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            {
                int idx = c - '0';
                if(idx < node->getSequence().size())
                {
                    emitExp(context, node->getSequence()[idx]);
                }
            }
            break;

        // "$,0" through "$,9" print the argument if present, with a leading comma
        // if the argument isn't there, they print nothing
        case ',':
            {
                int idx = *cursor++ - '0';
                if(idx < node->getSequence().size())
                {
                    emit(context, ", ");
                    emitExp(context, node->getSequence()[idx]);
                }
            }
            break;

        // "s" and "t" print the sampler and texture sides of the first argument
        case 's':
            emitExp(context, node->getSequence()[0]);
            emit(context, "_samp");
            break;
        case 't':
            emitExp(context, node->getSequence()[0]);
            emit(context, "_tex");
            break;

        // "$p0" through "$p9" are intended to take a GLSL argument for a projective
        // texture fetch, and turn it into HLSL, by calling a function (that must
        // be provided by the application's shader library)
        case 'p':
            {
                int idx = *cursor++ - '0';
                if(idx < node->getSequence().size())
                {
                    emit(context, "projectTexCoord(");
                    emitExp(context, node->getSequence()[idx]);
                    emit(context, ")");
                }
            }
            break;

        // "$b0" through "$b9" adds the string "Bias" if the corresponding argument is present
        case 'b':
            {
                int idx = *cursor++ - '0';
                if(idx < node->getSequence().size())
                {
                    emit(context, "Bias");
                }
            }
            break;

        // "$L0" through "$L9" is used to take two consecutive arguments,
        // and turn them into a single one for a "Load" operation
        case 'L':
            {
                int idx = *cursor++ - '0';
                if(idx+1 < node->getSequence().size())
                {
                    emit(context, "formTexCoordForLoad(");
                    emitExp(context, node->getSequence()[idx]);
                    emit(context, ", ");
                    emitExp(context, node->getSequence()[idx+1]);
                    emit(context, ")");
                }
            }
            break;

        case 'o':
            {
                int idx = *cursor++ - '0';
                if(idx < node->getSequence().size())
                {
                    for(int ii = 0; ii < 4; ++ii)
                    {
                        if(ii != 0) emit(context, ", ");
                        emitExp(context, node->getSequence()[idx]);
                        emit(context, "[");
                        emitInt(context, ii);
                        emit(context, "]");
                    }
                }
            }
            break;
        }

        beginSpan = cursor;
    }

#if 0
    // TODO: is texture-sampler arg ever in another spot?
    emitExp(context, node->getSequence()[0]);
    emit(context, "_tex.");
    emit(context, name);
    emit(context, "(");

    bool first = true;
    for(auto arg : node->getSequence())
    {
        if(!first) emit(context, ", ");

        emitExp(context, arg);

        if(first) emit(context, "_samp");
        first = false;
    }
    emit(context, ")");
#endif
}

void emitBuiltinCall(EmitContext* context, char const* name, glslang::TIntermUnary* node)
{
    emit(context, name);
    emit(context, "(");
    emitExp(context, node->getOperand());
    emit(context, ")");
}

void emitBuiltinPrefixOp(EmitContext* context, char const* name, glslang::TIntermUnary* node)
{
    emit(context, name);
    emit(context, "(");
    emitExp(context, node->getOperand());
    emit(context, ")");
}

void emitBuiltinPostfixOp(EmitContext* context, char const* name, glslang::TIntermUnary* node)
{
    emit(context, "(");
    emitExp(context, node->getOperand());
    emit(context, ")");
    emit(context, name);
}

void emitBuiltinCall(EmitContext* context, char const* name, glslang::TIntermBinary* node)
{
    emit(context, name);
    emit(context, "(");
    emitExp(context, node->getLeft());
    emit(context, ", ");
    emitExp(context, node->getRight());
    emit(context, ")");
}


void emitBuiltinOp(EmitContext* context, char const* name, glslang::TIntermBinary* node)
{
    emit(context, "((");
    emitExp(context, node->getLeft());
    emit(context, ") ");
    emit(context, name);
    emit(context, " (");
    emitExp(context, node->getRight());
    emit(context, "))");
}

void emitScalarConstantExp(EmitContext* context, glslang::TType const& type, glslang::TConstUnionArray const& data, int* ioIndex)
{
    int index = (*ioIndex)++;
    bool forceZero = index >= data.size();

    switch(type.getBasicType())
    {
    case glslang::EbtInt:
        emitInt(context, forceZero ? int(0) : data[index].getIConst());
        break;

    case glslang::EbtUint:
        emitUInt(context, forceZero ? unsigned(0) : data[index].getUConst());
        break;

    case glslang::EbtFloat:
        emitFloat(context, forceZero ? 0.0 : data[index].getDConst());
        break;

    case glslang::EbtDouble:
        emitDouble(context, forceZero ? 0.0 : data[index].getDConst());
        break;

    case glslang::EbtBool:
        emit(context, forceZero ? "false" : data[index].getBConst() ? "true" : "false");
        break;

    default:
        internalError(context, "uhandled case in 'emitConstantExp'");
        break;
    }
}

void emitConstantExp(EmitContext* context, glslang::TType const& type, glslang::TConstUnionArray const& data, int* ioIndex)
{
    // TODO: this won't work for arrays, since they can't use constructor syntax...
    emitTypeName(context, type);
    emit(context, "(");

    if(type.isArray())
    {
        glslang::TType elementType(type, 0);
        int elementCount = type.getOuterArraySize();
        for(int ii = 0; ii < elementCount; ++ii)
        {
            if(ii != 0) emit(context, ", ");
            emitConstantExp(context, elementType, data, ioIndex);
        }
    }
    else if(type.isMatrix())
    {
        int elementCount = type.getMatrixRows() * type.getMatrixCols();
        for(int ii = 0; ii < elementCount; ++ii)
        {
            if(ii != 0) emit(context, ", ");
            emitScalarConstantExp(context, type, data, ioIndex);
        }
    }
    else if(type.getVectorSize() > 1)
    {
        int elementCount = type.getVectorSize();
        for(int ii = 0; ii < elementCount; ++ii)
        {
            if(ii != 0) emit(context, ", ");
            emitScalarConstantExp(context, type, data, ioIndex);
        }
    }
    else if(auto structType = type.getStruct())
    {
        //
        for(auto field : *structType)
        {
            emitConstantExp(context, *field.type, data, ioIndex);
        }
    }
    else
    {
        emitScalarConstantExp(context, type, data, ioIndex);
    }

    emit(context, ")");
}

void emitConstructorCall(EmitContext* context, glslang::TIntermAggregate* node)
{
    // HLSL uses cast syntax for single-argument case
    if(node->getSequence().size() == 1)
    {
        emit(context, "((");
        emitTypeName(context, node->getType());
        emit(context, ") ");
        emitArgList(context, node);
        emit(context, ")");
        return;
    }

    emitTypeName(context, node->getType());
    emit(context, "(");
    emitArgList(context, node);
    emit(context, ")");
}

void emitConversion(EmitContext* context, glslang::TIntermUnary* node)
{
    emitTypeName(context, node->getType());
    emit(context, "(");
    emitExp(context, node->getOperand());
    emit(context, ")");
}

bool isNameUsed(EmitContext* context, std::string const& name)
{
    // Note: we don't walk up the chain of scopes, so a local name
    // is allowed to conflict with a global name
    if(context->nameScope && isNameUsed(context->nameScope, name))
        return true;

    if(isNameUsed(&context->shared->reservedNames, name))
        return true;

    return false;
}

EmitSymbolInfo createLocalName(EmitContext* context, glslang::TString const& text)
{
    std::string name = text.c_str();
    if(!isNameUsed(context, name))
    {
        EmitSymbolInfo info;
        info.name = name;
        markNameUsed(context->nameScope, name);
        return info;
    }
    else
    {
        for(int ii = 0;; ii++)
        {
            std::string newName = name + "_" + std::to_string(ii);
            if(!isNameUsed(context, newName))
            {
                EmitSymbolInfo info;
                info.name = newName;
                markNameUsed(context->nameScope, newName);
                return info;
            }
        }
    }
}

EmitSymbolInfo createGlobalName(EmitContext* context, glslang::TString const& text)
{
    EmitSymbolInfo info;
    info.name = text.c_str();
    return info;
}

EmitSymbolInfo emitLocalVarDecl(EmitContext* inContext, glslang::TIntermSymbol* node)
{
    EmitSymbolInfo info = createLocalName(inContext, node->getName());

    // redirect output to the right place for locals
    // TODO: can't do that for opaque types...
    EmitContext context = *inContext;
    context.span = context.localSpan;

    emitTypedDecl(&context, node->getType(), info.name);
    emit(&context, ";\n");

    return info;
}

EmitSymbolInfo emitUniformDecl(EmitContext* context, glslang::TIntermSymbol* node)
{
    EmitSymbolInfo info = createGlobalName(context, node->getName());

    // TODO: need to direct this to the right place!!!
    emit(context, "uniform ");
    emitTypedDecl(context, node->getType(), node->getName());
    emit(context, ";\n");

    return info;
}

EmitSymbolInfo emitUniformBlockDecl(EmitContext* inContext, glslang::TIntermSymbol* node)
{
    EmitSpan* span = allocateSpan();

    EmitContext subContext = *inContext;
    subContext.span = span;
    subContext.declSeparator = NULL;
    EmitContext* context = &subContext;


    EmitSymbolInfo info = createGlobalName(context, node->getType().getTypeName());

    // TODO: maybe don't discover uniform blocks on the fly like this,
    // and instead use the reflection interface to walk them more directly

    emit(context, "cbuffer ");
    emit(context, info.name);
    emit(context, "\n{\n");

    // Note: we declare the cbuffer as containing a single struct that
    // uses the same name as the cbuffer itself, so that use sites can
    // refer to it as `block.member`, whereas HLSL defaults to just using `member`
    //
    // TODO: evaluate whether this is a good idea or not.
    emit(context, "struct\n{\n");

    emitAggTypeMemberDecls(context, node->getType().getStruct());

    emit(context, "} ");
    emit(context, info.name);
    emit(context, ";\n");

    // TODO: enumerate the members here!!!
    emit(context, "};\n");

    appendSpan(&subContext.shared->cbufferDeclSpan->children, span);

    return info;
}

EmitSymbolInfo emitInputDecl(EmitContext* inContext, glslang::TIntermSymbol* node)
{
    EmitContext contextImpl = *inContext;
    EmitContext* context = &contextImpl;

    EmitSymbolInfo info = createGlobalName(context, node->getName());
    info.prefix = "stage_input.";

    context->span = context->shared->inputDeclSpan;

    // TODO: how to handle semantics if input/output has a `struct` type?

    Declarator nameDeclarator = { kDeclaratorFlavor_Name };
    nameDeclarator.name = info.name.c_str();

    Declarator semanticDeclarator = { kDeclaratorFlavor_Semantic, &nameDeclarator };
    semanticDeclarator.semantic.name = "USER";
    semanticDeclarator.semantic.index = (context->shared->outputCounter)++;

    emitTypedDecl(context, node->getType(), &semanticDeclarator);
    emit(context, ";\n");

    return info;
}

EmitSymbolInfo emitOutputDecl(EmitContext* inContext, glslang::TIntermSymbol* node)
{
    EmitContext contextImpl = *inContext;
    EmitContext* context = &contextImpl;

    EmitSymbolInfo info = createGlobalName(context, node->getName());
    info.prefix = "stage_output.";

    context->span = context->shared->outputDeclSpan;


    Declarator nameDeclarator = { kDeclaratorFlavor_Name };
    nameDeclarator.name = info.name.c_str();

    Declarator semanticDeclarator = { kDeclaratorFlavor_Semantic, &nameDeclarator };

    // TODO: recognize special input/output semantics
    EShLanguage stage = context->shared->glslangShader->getStage();

    if(stage == EShLangFragment)
    {
        semanticDeclarator.semantic.name = "SV_Target";
        semanticDeclarator.semantic.index = (context->shared->outputCounter)++;
    }
    else
    {
        semanticDeclarator.semantic.name = "USER";
        semanticDeclarator.semantic.index = (context->shared->outputCounter)++;
    }


    emitTypedDecl(context, node->getType(), &semanticDeclarator);
    emit(context, ";\n");

    return info;
}


void ensureSystemInputOutputDecl(EmitContext* context, glslang::TType const* type)
{
    if(type->getQualifier().isParamInput())
    {
        context->shared->systemInputsUsed[type->getQualifier().builtIn] = type;
    }
    else if(type->getQualifier().isPipeOutput())
    {
        context->shared->systemOutputsUsed[type->getQualifier().builtIn] = type;
    }
    else
    {
        // error!
    }
}

char const* mapSystemInputOutputSemantic(EmitContext* context, glslang::TType const& type)
{
    char const* dxName = "SV_Unknown";

    switch(type.getQualifier().builtIn)
    {
    default:
        break; // unhandled

#define CASE(GL, DX) case glslang::GL: dxName = #DX; break

    // The following aren't actually in D3D
    CASE(EbvBaseVertex,         BaseVertex);        // GL_ARB_shader_draw_parameters
    CASE(EbvBaseInstance,       BaseInstance);      // GL_ARB_shader_draw_parameters
    CASE(EbvDrawId,             DrawID);            // GL_ARB_shader_draw_parameters

    CASE(EbvSubGroupSize,       SubGroupSize);          // GL_ARB_shader_ballot
    CASE(EbvSubGroupInvocation, SubGroupInvocation);    // GL_ARB_shader_ballot
    CASE(EbvSubGroupEqMask,     SubGroupEqMask);        // GL_ARB_shader_ballot
    CASE(EbvSubGroupGeMask,     SubGroupGeMask);        // GL_ARB_shader_ballot
    CASE(EbvSubGroupGtMask,     SubGroupGtMask);        // GL_ARB_shader_ballot
    CASE(EbvSubGroupLeMask,     SubGroupLeMask);        // GL_ARB_shader_ballot
    CASE(EbvSubGroupLtMask,     SubGroupLtMask);        // GL_ARB_shader_ballot

    // compatibility/legacy
    CASE(EbvColor,                  Color);
    CASE(EbvSecondaryColor,         SecondaryColor);
    CASE(EbvNormal,                 Normal);
    CASE(EbvVertex,                 Vertex);
    CASE(EbvMultiTexCoord0,         MultiTexCoord0);
    CASE(EbvMultiTexCoord1,         MultiTexCoord1);
    CASE(EbvMultiTexCoord2,         MultiTexCoord2);
    CASE(EbvMultiTexCoord3,         MultiTexCoord3);
    CASE(EbvMultiTexCoord4,         MultiTexCoord4);
    CASE(EbvMultiTexCoord5,         MultiTexCoord5);
    CASE(EbvMultiTexCoord6,         MultiTexCoord6);
    CASE(EbvMultiTexCoord7,         MultiTexCoord7);
    CASE(EbvFogFragCoord,           FogFragCoord);
    CASE(EbvClipVertex,             ClipVertex);
    CASE(EbvFrontColor,             FrontColor);
    CASE(EbvBackColor,              BackColor);
    CASE(EbvFrontSecondaryColor,    FrontSecondaryColor);
    CASE(EbvBackSecondaryColor,     BackSecondaryColor);
    CASE(EbvTexCoord,               TexCoord);

    // TODO: need to check on semantic difference between `ID` and `Index` here
    CASE(EbvVertexId,       SV_VertexID);
    CASE(EbvInstanceId,     SV_InstanceID);
    CASE(EbvVertexIndex,    SV_VertexID);
    CASE(EbvInstanceIndex,  SV_InstanceID);

    CASE(EbvBoundingBox,    BoundingBox); // AEP_primitive_bounding_box

    CASE(EbvPosition,   SV_Position);
    CASE(EbvPointSize,  PointSize);

    CASE(EbvClipDistance,           SV_ClipDistance);
    CASE(EbvCullDistance,           SV_CullDistance);
    CASE(EbvPrimitiveId,            SV_PrimitiveID);

    CASE(EbvLayer,                  SV_RenderTargetArrayIndex);
    CASE(EbvViewportIndex,          SV_ViewportArrayIndex);

    CASE(EbvPatchVertices, PatchVertexCound); // not in D3D

    CASE(EbvTessLevelOuter,     SV_TessFactor);
    CASE(EbvTessLevelInner,     SV_InsideTessFactor);
    CASE(EbvTessCoord,          SV_DomainLocation);


    CASE(EbvFace,               SV_IsFrontFacing); // TODO: match?
    CASE(EbvFragCoord,          SV_Position);
    CASE(EbvPointCoord,       PointCoord);
    CASE(EbvFragColor,        SV_Target);
    CASE(EbvFragDepth,        SV_Depth);
    CASE(EbvHelperInvocation, IsHelperInvocation);

    CASE(EbvSampleId,           SV_SampleIndex);
    CASE(EbvSamplePosition,     SamplePosition);
    CASE(EbvSampleMask,         SampleMask);



    CASE(EbvNumWorkGroups,          NumWorkGroups);
    CASE(EbvWorkGroupSize,          WorkGroupSize);
    CASE(EbvWorkGroupId,            SV_GroupID);
    CASE(EbvLocalInvocationId,      SV_ThreadID);
    CASE(EbvGlobalInvocationId,     SV_DispatchThreadID);
    CASE(EbvLocalInvocationIndex,   SV_ThreadIndex);
#undef CASE

    // cases that need special handling:
    case glslang::EbvInvocationId:
        {
            switch(context->shared->glslangShader->getStage())
            {
            case EShLangGeometry:       dxName = "SV_InstanceID";       break;
            case EShLangTessControl:    dxName = "SV_ControlPointID";   break;
            default:
                break;
            }
        }
        break;
    }

    return dxName;
}

void emitSystemInputOutputDecl(EmitContext* context, glslang::TType const& type)
{
    char const* semantic = mapSystemInputOutputSemantic(context, type);

    Declarator nameDeclarator = { kDeclaratorFlavor_Name };
    nameDeclarator.name = type.getFieldName().c_str();

    Declarator semanticDeclarator = { kDeclaratorFlavor_Semantic, &nameDeclarator };
    semanticDeclarator.semantic.name = semantic;
    semanticDeclarator.semantic.index = 0;

    emitTypedDecl(context, type, &semanticDeclarator);
    emit(context, ";\n");
}

void emitSystemInputOutputDecls(EmitContext* inContext)
{
    EmitContext contextImpl = *inContext;
    EmitContext* context = &contextImpl;


    context->span = context->shared->systemInputDeclSpan;
    for(auto tt : context->shared->systemInputsUsed)
    {
        if(!tt) continue;

        emitSystemInputOutputDecl(context, *tt);
    }

    context->span = context->shared->systemOutputDeclSpan;
    for(auto tt : context->shared->systemOutputsUsed)
    {
        if(!tt) continue;

        emitSystemInputOutputDecl(context, *tt);
    }
}


EmitSymbolInfo emitSymbolDecl(EmitContext* context, glslang::TIntermSymbol* node)
{
    EmitSymbolInfo info;
    glslang::TType const& type = node->getType();

    if(type.getQualifier().isPipeInput())
    {
        return emitInputDecl(context, node);
    }
    else if(type.getQualifier().isPipeOutput())
    {
        return emitOutputDecl(context, node);
    }
    else if(type.getQualifier().isUniformOrBuffer())
    {
        if(type.getQualifier().layoutPushConstant)
        {
            // TODO: map Vulkan->D3D12
            internalError(context, "uhandled case in 'emitSymbolDecl'");
        }
        else if(type.getBasicType() == glslang::EbtBlock)
        {
            return emitUniformBlockDecl(context, node);
        }
        else
        {
            return emitUniformDecl(context, node);
        }
        // 
    }
    else
    {
        switch(type.getQualifier().storage)
        {
        case glslang::EvqIn:
        case glslang::EvqInOut:
            // function parameters already handled
            break;

        case glslang::EvqConstReadOnly:
        case glslang::EvqTemporary:
            return emitLocalVarDecl(context, node);

        default:
            internalError(context, "uhandled case in 'emitSymbolDecl'");
            break;
        }
    }
    return info;
}

EmitSymbolInfo getSymbolInfo(EmitContext* context, glslang::TIntermSymbol* node)
{
    auto ii = context->shared->mapSymbol.find(node->getId());
    if(ii != context->shared->mapSymbol.end())
        return ii->second;

    EmitSymbolInfo info = emitSymbolDecl(context, node);
    context->shared->mapSymbol.insert(std::make_pair(node->getId(), info));
    return info;
}

void emitSymbolExp(EmitContext* context, glslang::TIntermSymbol* node)
{
    // TODO: some special-casing for nodes of composite texture-sampler type?

    EmitSymbolInfo info = getSymbolInfo(context, node);
    emit(context, info.prefix);
    emit(context, info.name);
}

bool isBuiltinInputOutputBlock(EmitContext* context, glslang::TType const& type)
{
    if(type.getBasicType() == glslang::EbtBlock)
    {
        glslang::TString const& typeName = type.getTypeName();
        if(strncmp(typeName.c_str(), "gl_", 3) == 0)
        {
            return true;
        }
    }
    return false;
}

bool isBuiltinInputOutputBlock(EmitContext* context, glslang::TIntermTyped* node)
{
    return isBuiltinInputOutputBlock(context, node->getType());
}

void emitExp(EmitContext* context, TIntermNode* node)
{
    if(auto nn = node->getAsAggregate())
    {
        switch(nn->getOp())
        {
#define CASE(Op, Func) case glslang::EOp##Op: emitBuiltinCall(context, #Func, nn); break
        CASE(Min, min);
        CASE(Max, max);
        CASE(Modf, modf);
        CASE(Pow, pow);
        CASE(Dot, dot);
        CASE(Atan, atan);
        CASE(Clamp, clamp);
        CASE(Mix, lerp);
        CASE(Step, step);
        CASE(SmoothStep, smoothstep);
        CASE(Distance, distance);
        CASE(Cross, cross);
        CASE(FaceForward, faceforward);
        CASE(Reflect, reflect);
        CASE(Refract, refract);
        CASE(InterpolateAtSample, interpolateAtSample);
        CASE(InterpolateAtOffset, interpolateAtOffset);
        CASE(AddCarry, addcarry);
        CASE(SubBorrow, subborrow);
        CASE(UMulExtended, umulextended);
        CASE(IMulExtended, imulextended);
        CASE(BitfieldExtract, bitfieldextract);
        CASE(BitfieldInsert, bitfieldinsert);
        CASE(Fma, fma);
        CASE(Frexp, frexp);
        CASE(Ldexp, ldexp);
        CASE(ReadInvocation, readInvocation);
        // Note: constructors may need special-case work if language rules differ
#undef CASE


        case glslang::EOpConstructMat2x2:
        case glslang::EOpConstructMat2x3:
        case glslang::EOpConstructMat2x4:
        case glslang::EOpConstructMat3x2:
        case glslang::EOpConstructMat3x3:
        case glslang::EOpConstructMat3x4:
        case glslang::EOpConstructMat4x2:
        case glslang::EOpConstructMat4x3:
        case glslang::EOpConstructMat4x4:
        case glslang::EOpConstructDMat2x2:
        case glslang::EOpConstructDMat2x3:
        case glslang::EOpConstructDMat2x4:
        case glslang::EOpConstructDMat3x2:
        case glslang::EOpConstructDMat3x3:
        case glslang::EOpConstructDMat3x4:
        case glslang::EOpConstructDMat4x2:
        case glslang::EOpConstructDMat4x3:
        case glslang::EOpConstructDMat4x4:
        case glslang::EOpConstructFloat:
        case glslang::EOpConstructVec2:
        case glslang::EOpConstructVec3:
        case glslang::EOpConstructVec4:
        case glslang::EOpConstructDouble:
        case glslang::EOpConstructDVec2:
        case glslang::EOpConstructDVec3:
        case glslang::EOpConstructDVec4:
        case glslang::EOpConstructBool:
        case glslang::EOpConstructBVec2:
        case glslang::EOpConstructBVec3:
        case glslang::EOpConstructBVec4:
        case glslang::EOpConstructInt:
        case glslang::EOpConstructIVec2:
        case glslang::EOpConstructIVec3:
        case glslang::EOpConstructIVec4:
        case glslang::EOpConstructUint:
        case glslang::EOpConstructUVec2:
        case glslang::EOpConstructUVec3:
        case glslang::EOpConstructUVec4:
        case glslang::EOpConstructInt64:
        case glslang::EOpConstructI64Vec2:
        case glslang::EOpConstructI64Vec3:
        case glslang::EOpConstructI64Vec4:
        case glslang::EOpConstructUint64:
        case glslang::EOpConstructU64Vec2:
        case glslang::EOpConstructU64Vec3:
        case glslang::EOpConstructU64Vec4:
        case glslang::EOpConstructStruct:
        case glslang::EOpConstructTextureSampler:
            emitConstructorCall(context, nn);
            break;

        case glslang::EOpFunctionCall:
            {
                emitFuncName(context, nn->getName());
                emit(context, "(");
                emitArgList(context, nn);
                emit(context, ")");
            }
            break;

#define CASE(Op, format) case glslang::EOp##Op: emitTextureCall(context, format, nn); break
    CASE(Texture,               "$t.Sample$b2($s, $1 $,2)");            // texture(s, uv [, bias]) ->
    CASE(TextureProj,           "$t.Sample$b2($s, $p1 $,2)");           // textureProj(s, uv, [, bias]) ->
    CASE(TextureLod,            "$t.SampleLevel($s, $1, $2)");          // textureLod(s, uv, lod) ->
    CASE(TextureOffset,         "$t.Sample$b3($s, $1 $,3, $2)");        // textureOffset(s, uv, offset [, bias]) ->
    CASE(TextureFetch,          "$t.Load($L1)");                        // texelFetch(s, uv_int, lod/sample) -> 
    CASE(TextureFetchOffset,    "$t.Load($L1, $3)");                    // texelFetchOffset(s, uv, lod, offset)
    CASE(TextureProjOffset,     "$t.Sample$b3($s, $p1 $,3, $2)");       // textureProjOffset(s, uvw, offset [,bias])
    CASE(TextureLodOffset,      "$t.SampleLevel($s, $1, $2, $3)");      // textureLodOffset(s, uv, lod, offset);
    CASE(TextureProjLod,        "$t.SampleLevel($s, $p1, $2)");         // textureProjLod(s, uvw, lod);
    CASE(TextureProjLodOffset,  "$t.Sample$b3($s, $p1 $,3, $2)");       // textureProjLodOffset(s, uvw, lod, offset);
    CASE(TextureGrad,           "$t.SampleGrad($s, $1, $2, $3)");       // textureGrad(s, uv, dx, dy)
    CASE(TextureGradOffset,     "$t.SampleGrad($s, $1, $2, $3, $4)");   // textureGradOffset(s, uv, dx, dy, offset)
    CASE(TextureProjGrad,       "$t.SampleGrad($s, $p1, $2, $3)");      // textureProjGrad(s, uvw, dx, dy)
    CASE(TextureProjGradOffset, "$t.SampleGrad($s, $p1, $2, $3, $4)");  // textureProjGradOfset(s, uv, dx, dy, offset)
    CASE(TextureGather,         "$t.Gather$g2($s, $1, 0)");             // textureGather(s, uv [, comp])
    CASE(TextureGatherOffset,   "$t.Gather$g3($s, $1, $2)");            // textureGatherOffset(s, uv, offset [, comp])
    CASE(TextureGatherOffsets,  "$t.Gather$g3($s, $1, $o2)");           // textureGatherOffsets(s, uv, offsets[4] [, comp])

    // TODO: it never ends!!!
#if 0
    CASE(TextureClamp, SampleClamp);
    CASE(TextureOffsetClamp, SampleOffsetClamp);
    CASE(TextureGradClamp, SampleGradClamp);
    CASE(TextureGradOffsetClamp, SampleGradOffsetClamp);
    CASE(SparseTexture, Sample);
    CASE(SparseTextureLod, SampleLod);
    CASE(SparseTextureOffset, SampleOffset);
    CASE(SparseTextureFetch, Fetch);
    CASE(SparseTextureFetchOffset, FetchOffset);
    CASE(SparseTextureLodOffset, SampleLodOffset);
    CASE(SparseTextureGrad, SampleGrad);
    CASE(SparseTextureGradOffset, SampleGradOffset);
    CASE(SparseTextureGather, Gather);
    CASE(SparseTextureGatherOffset, GatherOffset);
    CASE(SparseTextureGatherOffsets, GatherOffsets);
    CASE(SparseTexelsResident, IsResident);
    CASE(SparseTextureClamp, SampleClamp);
    CASE(SparseTextureOffsetClamp, SampleOffsetClamp);
    CASE(SparseTextureGradClamp, SampleGradClamp);
    CASE(SparseTextureGradOffsetClamp, SampleGraOffsetClamp);
#endif
#undef CASE

        default:
            internalError(context, "uhandled case in 'emitExp'");
            break;
        }
    }
    else if(auto nn = node->getAsUnaryNode())
    {
        switch(nn->getOp())
        {
#define CASE(Op, Func) case glslang::EOp##Op: emitBuiltinCall(context, #Func, nn); break
        CASE(Radians, radians);
        CASE(Degrees, degrees);
        CASE(Sin, sin);
        CASE(Cos, cos);
        CASE(Tan, tan);
        CASE(Asin, asin);
        CASE(Acos, acos);
        CASE(Atan, atan);
        CASE(Sinh, sinh);
        CASE(Cosh, cosh);
        CASE(Tanh, tanh);
        CASE(Asinh, asinh);
        CASE(Acosh, acosh);
        CASE(Atanh, atanh);
        CASE(Exp, exp);
        CASE(Log, log);
        CASE(Exp2, exp2);
        CASE(Log2, log2);
        CASE(Sqrt, sqrt);
        CASE(InverseSqrt, rsqrt);
        CASE(Determinant, determinant);
        CASE(MatrixInverse, inverse);
        CASE(Transpose, transpose);
        CASE(Length, length);
        CASE(Normalize, normalize);
        CASE(Floor, floor);
        CASE(Trunc, trunc);
        CASE(Round, round);
        CASE(RoundEven, roundEvent);
        CASE(Ceil, ceil);
        CASE(Fract, fract);
        CASE(IsNan, isnan);
        CASE(IsInf, isinf);
        CASE(IsFinite, isfinite);
        CASE(FloatBitsToInt, asint);
        CASE(FloatBitsToUint, asuint);
        CASE(IntBitsToFloat, asfloat);
        CASE(UintBitsToFloat, asfloat);
        CASE(DoubleBitsToInt64, asint);
        CASE(DoubleBitsToUint64, asuint);
        CASE(Int64BitsToDouble, asdouble);
        CASE(Uint64BitsToDouble, asdouble);
        CASE(DPdx, ddx);
        CASE(DPdy, ddy);
        CASE(DPdxFine, ddx_fine);
        CASE(DPdyFine, ddy_fine);
        CASE(DPdxCoarse, ddx_coarse);
        CASE(DPdyCoarse, ddy_coarse);
        CASE(Fwidth, fwidth);
        CASE(FwidthFine, fwidth_fine);
        CASE(FwidthCoarse, fwidth_coarse);
        CASE(InterpolateAtCentroid, InterpolateAtCentroid);
        CASE(Any, any);
        CASE(All, all);
        CASE(Abs, abs);
        CASE(Sign, sgn);
        CASE(BitFieldReverse, bitreverse);
        CASE(BitCount, popcount);
        CASE(FindLSB, findLSB);
        CASE(FindMSB, findMSB);
        CASE(Ballot, ballot);
        CASE(ReadFirstInvocation, readFirstInvocation);
        CASE(AnyInvocation, anyInvocation);
        CASE(AllInvocations, allInvocatiosn);
        CASE(AllInvocationsEqual, allInvocationsEqual);

        // TODO: figure out mapping for these
        CASE(PackSnorm2x16, PackSnorm2x16);
        CASE(UnpackSnorm2x16, UnpackSnorm2x16);
        CASE(PackUnorm2x16, PackUnorm2x16);
        CASE(UnpackUnorm2x16, UnpackUnorm2x16);
        CASE(PackHalf2x16, PackHalf2x16);
        CASE(UnpackHalf2x16, UnpackHalf2x16);
        CASE(PackSnorm4x8, PackSnorm4x8);
        CASE(UnpackSnorm4x8, UnpackSnorm4x8);
        CASE(PackUnorm4x8, PackUnorm4x8);
        CASE(UnpackUnorm4x8, UnpackUnorm4x8);
        CASE(PackDouble2x32, PackDouble2x32);
        CASE(UnpackDouble2x32, UnpackDouble2x32);
        CASE(PackInt2x32, PackInt2x32);
        CASE(UnpackInt2x32, UnpackInt2x32);
        CASE(PackUint2x32, PackUint2x32);
        CASE(UnpackUint2x32, UnpackUint2x32);
        //
        CASE(AtomicCounterIncrement, AtomicCounterIncrement);
        CASE(AtomicCounterDecrement, AtomicCounterDecrement);
        CASE(AtomicCounter, AtomicCounter);
#undef CASE

#define CASE(Name, op) case glslang::EOp##Name: emitBuiltinPrefixOp(context, op, nn); break
        CASE(Negative, "-");
        CASE(LogicalNot, "!");
        CASE(BitwiseNot, "~");
        CASE(PreIncrement, "++");
        CASE(PreDecrement, "--");
#undef CASE

#define CASE(Name, op) case glslang::EOp##Name: emitBuiltinPostfixOp(context, op, nn); break
        CASE(PostIncrement, "++");
        CASE(PostDecrement, "--");
#undef CASE

        case glslang::EOpConvIntToBool:
        case glslang::EOpConvUintToBool:
        case glslang::EOpConvInt64ToBool:
        case glslang::EOpConvUint64ToBool:
        case glslang::EOpConvFloatToBool:
        case glslang::EOpConvDoubleToBool:
        case glslang::EOpConvBoolToFloat:
        case glslang::EOpConvBoolToDouble:
        case glslang::EOpConvBoolToInt:
        case glslang::EOpConvBoolToInt64:
        case glslang::EOpConvBoolToUint:
        case glslang::EOpConvBoolToUint64:
        case glslang::EOpConvIntToFloat:
        case glslang::EOpConvIntToDouble:
        case glslang::EOpConvInt64ToFloat:
        case glslang::EOpConvInt64ToDouble:
        case glslang::EOpConvUintToFloat:
        case glslang::EOpConvUintToDouble:
        case glslang::EOpConvUint64ToFloat:
        case glslang::EOpConvUint64ToDouble:
        case glslang::EOpConvDoubleToFloat:
        case glslang::EOpConvFloatToDouble:
        case glslang::EOpConvFloatToInt:
        case glslang::EOpConvDoubleToInt:
        case glslang::EOpConvFloatToInt64:
        case glslang::EOpConvDoubleToInt64:
        case glslang::EOpConvUintToInt:
        case glslang::EOpConvIntToUint:
        case glslang::EOpConvUint64ToInt64:
        case glslang::EOpConvInt64ToUint64:
        case glslang::EOpConvFloatToUint:
        case glslang::EOpConvDoubleToUint:
        case glslang::EOpConvFloatToUint64:
        case glslang::EOpConvDoubleToUint64:
        case glslang::EOpConvIntToInt64:
        case glslang::EOpConvInt64ToInt:
        case glslang::EOpConvUintToUint64:
        case glslang::EOpConvUint64ToUint:
        case glslang::EOpConvIntToUint64:
        case glslang::EOpConvInt64ToUint:
        case glslang::EOpConvUint64ToInt:
        case glslang::EOpConvUintToInt64:
            emitConversion(context, nn);
            break;

        default:
            internalError(context, "uhandled case in 'emitExp'");
            break;
        }
    }
    else if(auto nn = node->getAsBinaryNode())
    {
        switch(nn->getOp())
        {
#define CASE(Name, op) case glslang::EOp##Name: emitBuiltinOp(context, op, nn); break
        //
        CASE(Add, "+");
        CASE(Sub, "-");
        CASE(Mul, "*");
        CASE(VectorTimesScalar, "*");
        CASE(MatrixTimesScalar, "*");
        CASE(Div, "/");
        CASE(Mod, "%");
        CASE(RightShift, ">>");
        CASE(LeftShift, "<<");
        CASE(And, "&");
        CASE(InclusiveOr, "|");
        CASE(ExclusiveOr, "^");
        CASE(LessThan, "<");
        CASE(GreaterThan, ">");
        CASE(LessThanEqual, "<=");
        CASE(GreaterThanEqual, ">=");
        CASE(Equal, "=="); // TODO: HLSL versions default to component-wise
        CASE(NotEqual, "!=");
        //
        //
        CASE(LogicalOr, "||"); // TODO: need to implement short-circuiting for HLSL...
        CASE(LogicalAnd, "&&");
        //
        CASE(Assign, "="); // TODO: any special-casing needed for assignment?
        CASE(AddAssign, "+=");
        CASE(SubAssign, "-=");
        CASE(MulAssign, "*=");
        CASE(DivAssign, "/=");
        CASE(ModAssign, "%=");
        CASE(AndAssign, "&=");
        CASE(InclusiveOrAssign, "|=");
        CASE(ExclusiveOrAssign, "^=");
        CASE(LeftShiftAssign, "<<=");
        CASE(RightShiftAssign, ">>=");
        //
        CASE(VectorTimesScalarAssign, "*=");
        CASE(MatrixTimesScalarAssign, "*=");
        CASE(VectorTimesMatrixAssign, "*="); // TODO: should map to use of `mul()`
        CASE(MatrixTimesMatrixAssign, "*="); // TODO: should map to use of `mul()`
#undef CASE

#define CASE(Op, Func) case glslang::EOp##Op: emitBuiltinCall(context, #Func, nn); break
        CASE(VectorTimesMatrix, mul);
        CASE(MatrixTimesVector, mul);
        CASE(MatrixTimesMatrix, mul);
#undef CASE

        case glslang::EOpVectorSwizzle:
            {
                emitExp(context, nn->getLeft());
                emit(context, ".");
                if(auto seq = nn->getRight()->getAsAggregate())
                {
                    for(auto idx : seq->getSequence())
                    {
                        auto idxVal = idx->getAsConstantUnion()->getConstArray()[0].getIConst();
                        static const char* kSwizzleStrs[] = { "x", "y", "z", "w" };
                        emit(context, kSwizzleStrs[idxVal]);
                    }
                }
                else
                {
                    internalError(context, "uhandled case in 'emitExp'");
                }
            }
            break;

        case glslang::EOpIndexDirect:
        case glslang::EOpIndexIndirect:
            {
                emit(context, "(");
                emitExp(context, nn->getLeft());
                emit(context, "[");
                emitExp(context, nn->getRight());
                emit(context, "])");
            }
            break;

        case glslang::EOpIndexDirectStruct:
            {
                // in some special cases, we do not want to emit the base expression at all...
                if(isBuiltinInputOutputBlock(context, nn->getLeft()))
                {
                    // TODO: ensure that the field we are accessing has been declared
                    ensureSystemInputOutputDecl(context, &nn->getType());

                    if(nn->getType().getQualifier().isPipeInput())
                    {
                        emit(context, "stage_input.");
                    }
                    else if(nn->getType().getQualifier().isPipeOutput())
                    {
                        emit(context, "stage_output.");
                    }
                    else
                    {
                        // unexpected
                    }
                }
                else
                {
                    emit(context, "(");
                    emitExp(context, nn->getLeft());
                    emit(context, ").");
                }

                // TODO: this logic is duplicated with the declaration site for structs.
                // A better approach would avoid this interaction by saving field names
                // at declaration time, and hten re-using them...
                std::string fieldName = nn->getType().getFieldName().c_str();
                if(isNameUsed(&context->shared->reservedNames, fieldName))
                {
                    fieldName += "_1";
                }
                emit(context, fieldName);
            }
            break;

        default:
            internalError(context, "uhandled case in 'emitExp'");
            break;
        }
    }
    else if(auto nn = node->getAsConstantUnion())
    {
        int index = 0;
        emitConstantExp(context, nn->getType(), nn->getConstArray(), &index);
    }
    else if(auto nn = node->getAsSymbolNode())
    {
        emitSymbolExp(context, nn);
    }
    else if(auto nn = node->getAsSelectionNode())
    {
        // TODO: HLSL ?: doesn't short-circuit!
        emit(context, "(");
        emitExp(context, nn->getCondition());
        emit(context, " ? ");
        emitExp(context, nn->getTrueBlock());
        emit(context, " : ");
        emitExp(context, nn->getTrueBlock());
        emit(context, ")");
    }
    else
    {
        internalError(context, "uhandled case in 'emitExp'");
    }
}

void emitExpStmt(EmitContext* context, TIntermNode* node)
{
    emitExp(context, node);
    emit(context, ";\n");
}

void emitStmt(EmitContext* context, TIntermNode* node)
{
    if(auto nn = node->getAsAggregate())
    {
        switch(nn->getOp())
        {
        case glslang::EOpParameters:
            // already handled in `emitFuncDecl`
            break;

        case glslang::EOpSequence:
            {
                for( auto child : nn->getSequence() )
                {
                    emitStmt(context, child);
                }
            }
            break;

        default:
            // assume any unhandled cases are expressions
            emitExpStmt(context, nn);
//            internalError(context, "uhandled case in 'emitStmt'");
            break;
        }
    }
    else if(auto nn = node->getAsBranchNode())
    {
        switch(nn->getFlowOp())
        {
        case glslang::EOpReturn:
            {
                emit(context, "return");
                if(auto exp = nn->getExpression())
                {
                    emit(context, " ");
                    emitExp(context, exp);
                }
                emit(context, ";\n");
            }
            break;

        case glslang::EOpKill:
            emit(context, "discard;\n");
            break;

        case glslang::EOpBreak:
            emit(context, "break;\n");
            break;

        case glslang::EOpContinue:
            emit(context, "continue;\n");
            break;

        default:
            internalError(context, "uhandled case in 'emitStmt'");
            break;
        }
    }
    else if(auto nn = node->getAsSelectionNode())
    {
        if(nn->getType().getBasicType() != glslang::EbtVoid)
        {
            emitExpStmt(context, nn);
        }
        else
        {
            emit(context, "if(");
            emitExp(context, nn->getCondition());
            emit(context, ")\n{\n");
            emitStmt(context, nn->getTrueBlock());
            emit(context, "}\n");
            if(auto elseBlock = nn->getFalseBlock())
            {
                emit(context, "else\n{\n");
                emitStmt(context, elseBlock);
                emit(context, "}\n");
            }
        }
    }
    else if(auto nn = node->getAsLoopNode())
    {
        auto test = nn->getTest();
        auto body = nn->getBody();
        if(nn->testFirst())
        {
            if( auto iter = nn->getTerminal() )
            {
                emit(context, "for(;");
                if(test)
                {
                    emitExp(context, test);
                }
                emit(context, ";");
                if(iter)
                {
                    emitExp(context, iter);
                }
                emit(context, ")\n{\n");
                emitStmt(context, body);
                emit(context, "}\n");
            }
            else
            {
                emit(context, "while(");
                emitExp(context, test);
                emit(context, ")\n{\n");
                emitStmt(context, body);
                emit(context, "}\n");
            }
        }
        else
        {
                emit(context, "do\n{\n");
                emitStmt(context, body);
                emit(context, "} while(");
                emitExp(context, test);
                emit(context, ");\n");
        }
    }
    else if(auto nn = node->getAsSwitchNode())
    {
        emit(context, "switch(");
        emitExp(context, nn->getCondition());
        emit(context, ")\n{\n");
        for(auto child : nn->getBody()->getSequence())
        {
            if(auto childBranch = child->getAsBranchNode())
            {
                switch(childBranch->getFlowOp())
                {
                case glslang::EOpDefault:
                    emit(context, "default:\n");
                    break;

                case glslang::EOpCase:
                    emit(context, "case ");
                    emitExp(context, childBranch->getExpression());
                    emit(context, ":\n");
                    break;

                default:
                    emitStmt(context, child);
                    break;
                }
            }
            else
            {
                emitStmt(context, child);
            }
        }
        emit(context, "}\n");
    }
    else if(auto nn = node->getAsTyped())
    {
        // by default, assume a typed node is an expression
        emitExpStmt(context, nn);
    }
    else
    {
        internalError(context, "uhandled case in 'emitStmt'");
    }
}

struct WithNameScope
{
    EmitContext*    context;
    EmitNameScope scope;

    WithNameScope(EmitContext* context)
        : context(context)
    {
        scope.parent = context->nameScope;
        context->nameScope = &scope;
    }

    ~WithNameScope()
    {
        context->nameScope = scope.parent;
    }
};

bool isEntryPoint(EmitContext* context, glslang::TIntermAggregate* funcDecl)
{
    char const* entryPointName = context->shared->entryPointName;
    return strncmp(funcDecl->getName().c_str(), entryPointName, strlen(entryPointName)) == 0;
}

void emitFuncDecl(EmitContext* context, glslang::TIntermAggregate* funcDecl)
{
    if(isEntryPoint(context, funcDecl))
    {
        context->shared->entryPointFunc = funcDecl;
        return;
    }
    // TODO: maybe skip the entry-point function during this part...

    // push a scope for locally-declared names
    WithNameScope funcScope(context);

    glslang::TString name = funcDecl->getName();
    Declarator declarator = { kDeclaratorFlavor_FuncName, NULL };
    declarator.name = name.c_str();

    emitTypedDecl(context, funcDecl->getType(), &declarator);
    emit(context, "(");

    glslang::TIntermSequence& params = funcDecl->getSequence()[0]->getAsAggregate()->getSequence();
    context->declSeparator = ", ";
    bool first = true;
    for(auto pp : params)
    {
        if(!first)
        {
            emit(context, ",");
        }
        first = false;
        emit(context, "\n    ");

        glslang::TIntermSymbol* param = pp->getAsSymbolNode();
        glslang::TType const& paramType = param->getType();

        switch(paramType.getQualifier().storage)
        {
        case glslang::EvqInOut:
            emit(context, "in out ");
            break;
        case glslang::EvqOut:
            emit(context, "out ");
            break;
        default:
            break;
        }

        EmitSymbolInfo paramInfo = createLocalName(context, param->getName());
        context->shared->mapSymbol.insert(std::make_pair(param->getId(), paramInfo));

        emitTypedDecl(context, paramType, paramInfo.name);
    }
    context->declSeparator = NULL;
    emit(context, ")\n");
    // TODO: how do we distinguish forward declaration?
    emit(context, "{\n");


    EmitSpan* localSpan = allocateSpan(context);
    EmitSpan* restSpan = allocateSpan(context);

    EmitContext bodyContext = *context;
    bodyContext.localSpan = localSpan;

    for(auto child : funcDecl->getSequence())
    {
        emitStmt(&bodyContext, child);
    }
    emit(context, "}\n");
}

void emitEntryPointDecl(EmitContext* context, glslang::TIntermAggregate* funcDecl)
{

    // push a scope for locally-declared names
    WithNameScope funcScope(context);

    emit(context, "\n// entry point\n");
    emit(context, "STAGE_OUTPUT ");
    emitFuncName(context, funcDecl->getName());
    emit(context, "( STAGE_INPUT stage_input_ )\n");
    emit(context, "{\n");
    emit(context, "stage_input = stage_input_;\n");


    EmitSpan* localSpan = allocateSpan(context);
    EmitSpan* restSpan = allocateSpan(context);

    EmitContext bodyContext = *context;
    bodyContext.localSpan = localSpan;

    // TODO: emit any global initialization logic here

    for(auto child : funcDecl->getSequence())
    {
        emitStmt(&bodyContext, child);
    }

    emit(context, "return stage_output;\n");
    emit(context, "};\n");
}

void emitDecl(EmitContext* context, TIntermNode* node)
{
    if(auto nn = node->getAsAggregate())
    {
        switch(nn->getOp())
        {
        case glslang::EOpFunction:
            emitFuncDecl(context, nn);
            break;

        case glslang::EOpLinkerObjects:
            break;

        default:
            // TODO: other cases at global scope are logically initializers that go
            // at the start of the entry point...
            internalError(context, "uhandled case in 'emitDecls'");
            break;
        }
    }
    else
    {
        internalError(context, "uhandled case in 'emitDecl'");
    }
}

void emitDecls(EmitContext* context, TIntermNode* node)
{
    if(auto nn = node->getAsAggregate())
    {
        switch(nn->getOp())
        {
        case glslang::EOpSequence:
            {
                for( auto child : nn->getSequence() )
                {
                    emitDecl(context, child);
                }
            }
            break;

        default:
            internalError(context, "uhandled case in 'emitDecls'");
            break;
        }
    }
    else
    {
        internalError(context, "uhandled case in 'emitDecls'");
    }
}

size_t computeSize(EmitSpanList const& spans)
{
    size_t size = 0;
    for(auto span = spans.first; span; span = span->next)
    {
        size_t spanSize = (span->cursor - span->buffer);
        size_t childSize = computeSize(span->children);

        size += spanSize + childSize;
    }
    return size;
}

void write(EmitSpanList const& spans, char** ioCursor)
{
    char*& cursor = *ioCursor;
    for(auto span = spans.first; span; span = span->next)
    {
        size_t spanSize = (span->cursor - span->buffer);
        memcpy(cursor, span->buffer, spanSize);
        cursor += spanSize;

        write(span->children, &cursor);
    }
}

StringSpan join(SharedEmitContext* context)
{
    size_t size = computeSize(context->spans);

    char* buffer = (char*)malloc(size + 1);
    if(!buffer)
    {
        internalError(NULL, "out of memory\n");
        exit(1);
    }

    char* cursor = buffer;
    write(context->spans, &cursor);

    buffer[size] = 0;
    StringSpan result = { buffer, buffer + size };
    return result;
}

StringSpan crossCompile(Options* options, char const* inputPath, StringSpan inputText, Falcor::ShaderType shaderType)
{
    glslang::InitializeProcess();

    EShLanguage stage = EShLangCount;
    switch(shaderType)
    {
    case Falcor::ShaderType::Vertex:    stage = EShLangVertex; break;
    case Falcor::ShaderType::Hull:      stage = EShLangTessControl; break;
    case Falcor::ShaderType::Domain:    stage = EShLangTessEvaluation; break;
    case Falcor::ShaderType::Geometry:  stage = EShLangGeometry; break;
    case Falcor::ShaderType::Fragment:  stage = EShLangFragment; break;
    case Falcor::ShaderType::Compute:   stage = EShLangCompute; break;
    default:
        Falcor::Logger::log(Falcor::Logger::Level::Error, "invalid shader type for cross-compiler");
        return StringSpan{};
    }

    glslang::TShader* shader = new glslang::TShader(stage);

    char const* preamble =
        "#extension GL_GOOGLE_include_directive : require\n" // TODO: shouldn't actually be required
        "#extension GL_ARB_bindless_texture : require\n" // TODO: shouldn't actually be required
        "#define FALCOR_GLSL 1\n"
        "#define FALCOR_GLSL_CROSS 1\n"
        "";

    char const* sources[] = { inputText.begin };
    int sourceLengths[] = { (int) (inputText.end - inputText.begin) };
    char const* paths[] = { inputPath };

    shader->setPreamble(preamble);

    shader->setStringsWithLengthsAndNames(&sources[0], &sourceLengths[0], &paths[0], 1);

    if(options->entryPointName)
        shader->setEntryPoint(options->entryPointName);

    int version = 450; // TODO: pick a different default?

    IncluderImpl includer(options);

    if(!shader->parse(&kResourceLimits, version, ECoreProfile, false, false, EShMsgDefault, includer))
    {
        Falcor::Logger::log( Falcor::Logger::Level::Error, shader->getInfoLog());
        StringSpan result = { 0, 0 };
        return result;
    }

    glslang::TIntermediate* intermediate = ((HackShader*) shader)->getIntermediate();

    SharedEmitContext sharedEmitContext;
    sharedEmitContext.glslangShader = shader;
    sharedEmitContext.entryPointName = options->entryPointName ? options->entryPointName : "main";
    initSharedEmitContext(&sharedEmitContext);
    EmitContext emitContext = { &sharedEmitContext, sharedEmitContext.mainSpan };
    emitContext.nameScope = &sharedEmitContext.globalNames;
    emitDecls(&emitContext, intermediate->getTreeRoot());
    if(sharedEmitContext.entryPointFunc)
    {
        emitEntryPointDecl(&emitContext, sharedEmitContext.entryPointFunc);
    }
    emitSystemInputOutputDecls(&emitContext);

    StringSpan outputText = join(&sharedEmitContext);

    // clean up
    delete shader;
    glslang::FinalizeProcess();

    return outputText;
}

namespace Falcor
{
    bool glslToHlslShader(std::string& shader, ShaderType shaderType)
    {
        StringSpan inputText;
        inputText.begin = shader.c_str();
        inputText.end = inputText.begin + shader.size();

        Options options = { 0 };
        StringSpan outputText = crossCompile(&options, "shader_source", inputText, shaderType);

        if(!outputText.begin)
        {
            return false;
        }

        shader = std::string(outputText.begin, outputText.end);
        free((void*) outputText.begin);
        return true;
    }
}
