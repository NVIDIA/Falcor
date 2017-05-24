#ifndef CORELIB_TEXT_PARSER_H
#define CORELIB_TEXT_PARSER_H

#include "Basic.h"

namespace CoreLib
{
	namespace Text
	{
		class TextFormatException : public Exception
		{
		public:
			TextFormatException(String message)
				: Exception(message)
			{}
		};

		inline bool IsLetter(char ch)
		{
			return ((ch >= 'a' && ch <= 'z') ||
				(ch >= 'A' && ch <= 'Z') || ch == '_');
		}

		inline bool IsDigit(char ch)
		{
			return ch >= '0' && ch <= '9';
		}

		inline bool IsPunctuation(char ch)
		{
			return  ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' ||
				ch == '!' || ch == '^' || ch == '&' || ch == '(' || ch == ')' ||
				ch == '=' || ch == '{' || ch == '}' || ch == '[' || ch == ']' ||
				ch == '|' || ch == ';' || ch == ',' ||  ch == '<' ||
                ch == '>' || ch == '~' || ch == '@' || ch == ':' || ch == '?' || ch == '#';
		}

		inline bool IsWhiteSpace(char ch)
		{
			return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\v');
		}

		class CodePosition
		{
		public:
			int Line = -1, Col = -1, Pos = -1;
			String FileName;
			String ToString()
			{
				StringBuilder sb(100);
				sb << FileName;
				if (Line != -1)
					sb << "(" << Line << ")";
				return sb.ProduceString();
			}
			CodePosition() = default;
			CodePosition(int line, int col, int pos, String fileName)
			{
				Line = line;
				Col = col;
				Pos = pos;
				this->FileName = fileName;
			}
			bool operator < (const CodePosition & pos) const
			{
				return FileName < pos.FileName || (FileName == pos.FileName && Line < pos.Line) ||
					(FileName == pos.FileName && Line == pos.Line && Col < pos.Col);
			}
			bool operator == (const CodePosition & pos) const
			{
				return FileName == pos.FileName && Line == pos.Line && Col == pos.Col;
			}
		};

		enum class TokenType
		{
            EndOfFile = -1,
			// illegal
			Unknown,
			// identifier
			Identifier,
			// constant
			IntLiterial, DoubleLiterial, StringLiterial, CharLiterial,
			// operators
			Semicolon, Comma, Dot, LBrace, RBrace, LBracket, RBracket, LParent, RParent,
			OpAssign, OpAdd, OpSub, OpMul, OpDiv, OpMod, OpNot, OpBitNot, OpLsh, OpRsh,
			OpEql, OpNeq, OpGreater, OpLess, OpGeq, OpLeq,
			OpAnd, OpOr, OpBitXor, OpBitAnd, OpBitOr,
			OpInc, OpDec, OpAddAssign, OpSubAssign, OpMulAssign, OpDivAssign, OpModAssign,
			OpShlAssign, OpShrAssign, OpOrAssign, OpAndAssign, OpXorAssign,

			QuestionMark, Colon, RightArrow, At, Pound, PoundPound,
		};

		String TokenTypeToString(TokenType type);

        enum TokenFlag : unsigned int
        {
            AtStartOfLine   = 1 << 0,
            AfterWhitespace = 1 << 1,
        };
        typedef unsigned int TokenFlags;

		class Token
		{
		public:
			TokenType Type = TokenType::Unknown;
			String Content;
			CodePosition Position;
            TokenFlags flags = 0;
			Token() = default;
			Token(TokenType type, const String & content, int line, int col, int pos, String fileName, TokenFlags flags = 0)
                : flags(flags)
			{
				Type = type;
				Content = content;
				Position = CodePosition(line, col, pos, fileName);
			}
		};

		enum class TokenizeErrorType
		{
			InvalidCharacter, InvalidEscapeSequence
		};

		List<Token> TokenizeText(const String & fileName, const String & text, Procedure<TokenizeErrorType, CodePosition> errorHandler);
		List<Token> TokenizeText(const String & fileName, const String & text);
		List<Token> TokenizeText(const String & text);
		
		String EscapeStringLiteral(String str);
		String UnescapeStringLiteral(String str);

		class TokenReader
		{
		private:
			bool legal;
			List<Token> tokens;
			int tokenPtr;
		public:
			TokenReader(Basic::String text);
			int ReadInt()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Content == '-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.Type == TokenType::IntLiterial)
				{
					if (neg)
						return -StringToInt(token.Content);
					else
						return StringToInt(token.Content);
				}
				throw TextFormatException("Text parsing error: int expected.");
			}
			unsigned int ReadUInt()
			{
				auto token = ReadToken();
				if (token.Type == TokenType::IntLiterial)
				{
					return StringToUInt(token.Content);
				}
				throw TextFormatException("Text parsing error: int expected.");
			}
			double ReadDouble()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Content == '-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.Type == TokenType::DoubleLiterial || token.Type == TokenType::IntLiterial)
				{
					if (neg)
						return -StringToDouble(token.Content);
					else
						return StringToDouble(token.Content);
				}
				throw TextFormatException("Text parsing error: floating point value expected.");
			}
			float ReadFloat()
			{
				return (float)ReadDouble();
			}
			String ReadWord()
			{
				auto token = ReadToken();
				if (token.Type == TokenType::Identifier)
				{
					return token.Content;
				}
				throw TextFormatException("Text parsing error: identifier expected.");
			}
			String Read(const char * expectedStr)
			{
				auto token = ReadToken();
				if (token.Content == expectedStr)
				{
					return token.Content;
				}
				throw TextFormatException("Text parsing error: \'" + String(expectedStr) + "\' expected.");
			}
			String Read(String expectedStr)
			{
				auto token = ReadToken();
				if (token.Content == expectedStr)
				{
					return token.Content;
				}
				throw TextFormatException("Text parsing error: \'" + expectedStr + "\' expected.");
			}
			
			String ReadStringLiteral()
			{
				auto token = ReadToken();
				if (token.Type == TokenType::StringLiterial)
				{
					return token.Content;
				}
				throw TextFormatException("Text parsing error: string literal expected.");
			}
			void Back(int count)
			{
				tokenPtr -= count;
			}
			Token ReadToken()
			{
				if (tokenPtr < tokens.Count())
				{
					auto &rs = tokens[tokenPtr];
					tokenPtr++;
					return rs;
				}
				throw TextFormatException("Unexpected ending.");
			}
			Token NextToken(int offset = 0)
			{
				if (tokenPtr + offset < tokens.Count())
					return tokens[tokenPtr + offset];
				else
				{
					Token rs;
					rs.Type = TokenType::Unknown;
					return rs;
				}
			}
			bool LookAhead(String token)
			{
				if (tokenPtr < tokens.Count())
				{
					auto next = NextToken();
					return next.Content == token;
				}
				else
				{
					return false;
				}
			}
			bool IsEnd()
			{
				return tokenPtr == tokens.Count();
			}
		public:
			bool IsLegalText()
			{
				return legal;
			}
		};

		List<String> Split(String str, char c);
	}
}

#endif