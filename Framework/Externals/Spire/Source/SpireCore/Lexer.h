#ifndef RASTER_RENDERER_LEXER_H
#define RASTER_RENDERER_LEXER_H

#include "../CoreLib/Basic.h"
#include "Diagnostics.h"

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib::Basic;

        struct TokenList
        {
            Token* begin() const;
            Token* end() const;

            List<Token> mTokens;
        };

        struct TokenSpan
        {
            TokenSpan();
            TokenSpan(
                TokenList const& tokenList)
                : mBegin(tokenList.begin())
                , mEnd  (tokenList.end  ())
            {}

            Token* begin() const { return mBegin; }
            Token* end  () const { return mEnd  ; }

            int GetCount() { return (int)(mEnd - mBegin); }

            Token* mBegin;
            Token* mEnd;
        };

        struct TokenReader
        {
            TokenReader();
            explicit TokenReader(TokenSpan const& tokens)
                : mCursor(tokens.begin())
                , mEnd   (tokens.end  ())
            {}
            explicit TokenReader(TokenList const& tokens)
                : mCursor(tokens.begin())
                , mEnd   (tokens.end  ())
            {}

            bool IsAtEnd() const { return mCursor == mEnd; }
            Token PeekToken() const;
            CoreLib::Text::TokenType PeekTokenType() const;
            CodePosition PeekLoc() const;

            Token AdvanceToken();

            int GetCount() { return (int)(mEnd - mCursor); }

            Token* mCursor;
            Token* mEnd;
        };

        
        class Lexer
        {
        public:
            TokenList Parse(const String & fileName, const String & str, DiagnosticSink * sink);
        };
    }
}

#endif