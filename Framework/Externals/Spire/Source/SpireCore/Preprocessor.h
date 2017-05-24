// Preprocessor.h
#ifndef SPIRE_PREPROCESSOR_H_INCLUDED
#define SPIRE_PREPROCESSOR_H_INCLUDED

#include "../CoreLib/Basic.h"
#include "../CoreLib/Tokenizer.h"
#include "../SpireCore/Lexer.h"

namespace Spire{ namespace Compiler {

class DiagnosticSink;

// Callback interface for the preprocessor to use when looking
// for files in `#include` directives.
struct IncludeHandler
{
    virtual bool TryToFindIncludeFile(
        CoreLib::String const& pathToInclude,
        CoreLib::String const& pathIncludedFrom,
        CoreLib::String* outFoundPath,
        CoreLib::String* outFoundSource) = 0;
};

// Take a string of source code and preprocess it into a list of tokens.
TokenList PreprocessSource(
    CoreLib::String const& source,
    CoreLib::String const& fileName,
    DiagnosticSink* sink,
    IncludeHandler* includeHandler);

TokenList PreprocessSource(
    CoreLib::String const& source,
    CoreLib::String const& fileName,
    DiagnosticSink* sink,
    IncludeHandler* includeHandler,
    CoreLib::Dictionary<CoreLib::String, CoreLib::String>  defines);

}}

#endif
