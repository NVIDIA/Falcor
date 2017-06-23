// Preprocessor.h
#ifndef SLANG_PREPROCESSOR_H_INCLUDED
#define SLANG_PREPROCESSOR_H_INCLUDED

#include "../core/basic.h"
#include "../slang/lexer.h"

namespace Slang {

struct CompileRequest;
class DiagnosticSink;
class ProgramSyntaxNode;

enum class IncludeResult
{
    Error,
    NotFound,
    FoundIncludeFile,
    FoundAutoImportFile,
};

// Callback interface for the preprocessor to use when looking
// for files in `#include` directives.
struct IncludeHandler
{
    virtual IncludeResult TryToFindIncludeFile(
        String const& pathToInclude,
        String const& pathIncludedFrom,
        String* outFoundPath,
        String* outFoundSource) = 0;
};

// Take a string of source code and preprocess it into a list of tokens.
TokenList preprocessSource(
    String const&               source,
    String const&               fileName,
    DiagnosticSink*             sink,
    IncludeHandler*             includeHandler,
     Dictionary<String, String> defines,
    ProgramSyntaxNode*          syntax,
    CompileRequest*             compileRequest);

} // namespace Slang

#endif
