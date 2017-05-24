#ifndef RASTER_RENDERER_PARSER_H
#define RASTER_RENDERER_PARSER_H

#include "Lexer.h"
#include "ShaderCompiler.h"
#include "Syntax.h"

namespace Spire
{
    namespace Compiler
    {
        RefPtr<ProgramSyntaxNode> ParseProgram(
            CompileOptions&     options,
            TokenSpan const&    tokens,
            DiagnosticSink*     sink,
            String const&       fileName,
            ProgramSyntaxNode*	predefUnit);

        // Parse a source file into an existing translation unit
        void parseSourceFile(
            ProgramSyntaxNode*  translationUnitSyntax,
            CompileOptions&     options,
            TokenSpan const&    tokens,
            DiagnosticSink*     sink,
            String const&       fileName,
            ProgramSyntaxNode*	predefUnit);
    }
}

#endif