#include "Lexer.h"
#include "../CoreLib/Tokenizer.h"

#include <assert.h>

using namespace CoreLib::Text;

namespace Spire
{
    namespace Compiler
    {
        static Token GetEndOfFileToken()
        {
            return Token(TokenType::EndOfFile, "", 0, 0, 0, "");
        }

        Token* TokenList::begin() const
        {
            assert(mTokens.Count());
            return &mTokens[0];
        }

        Token* TokenList::end() const
        {
            assert(mTokens.Count());
            assert(mTokens[mTokens.Count()-1].Type == TokenType::EndOfFile);
            return &mTokens[mTokens.Count() - 1];
        }

        TokenSpan::TokenSpan()
            : mBegin(NULL)
            , mEnd  (NULL)
        {}

        TokenReader::TokenReader()
            : mCursor(NULL)
            , mEnd   (NULL)
        {}


        Token TokenReader::PeekToken() const
        {
            if (!mCursor)
                return GetEndOfFileToken();

            Token token = *mCursor;
            if (mCursor == mEnd)
                token.Type = TokenType::EndOfFile;
            return token;
        }

        CoreLib::Text::TokenType TokenReader::PeekTokenType() const
        {
            if (mCursor == mEnd)
                return TokenType::EndOfFile;
            assert(mCursor);
            return mCursor->Type;
        }

        CodePosition TokenReader::PeekLoc() const
        {
            if (!mCursor)
                return CodePosition();
            assert(mCursor);
            return mCursor->Position;
        }

        Token TokenReader::AdvanceToken()
        {
            if (!mCursor)
                return GetEndOfFileToken();

            Token token = *mCursor;
            if (mCursor == mEnd)
                token.Type = TokenType::EndOfFile;
            else
                mCursor++;
            return token;
        }


        TokenList Lexer::Parse(const String & fileName, const String & str, DiagnosticSink * sink)
        {
            TokenList tokenList;
            tokenList.mTokens = CoreLib::Text::TokenizeText(fileName, str, [&](CoreLib::Text::TokenizeErrorType errType, CoreLib::Text::CodePosition pos)
            {
                auto curChar = str[pos.Pos];
                switch (errType)
                {
                case CoreLib::Text::TokenizeErrorType::InvalidCharacter:
                    // Check if inside the ASCII "printable" range
                    if(curChar >= 0x20 && curChar <=  0x7E)
                    {
                        char buffer[] = { curChar, 0 };
                        sink->diagnose(pos, Diagnostics::illegalCharacterPrint, buffer);
                    }
                    else
                    {
                        // Fallback: print as hexadecimal
                        sink->diagnose(pos, Diagnostics::illegalCharacterHex, String((unsigned char)curChar, 16));
                    }
                    break;
                case CoreLib::Text::TokenizeErrorType::InvalidEscapeSequence:
                    sink->diagnose(pos, Diagnostics::illegalCharacterLiteral);
                    break;
                default:
                    break;
                }
            });

            // Add an end-of-file token so that we can reference it in diagnostic messages
            tokenList.mTokens.Add(Token(TokenType::EndOfFile, "", 0, 0, 0, fileName, TokenFlag::AtStartOfLine | TokenFlag::AfterWhitespace));

            return tokenList;
        }
    }
}