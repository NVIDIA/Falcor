#include "Tokenizer.h"

using namespace CoreLib::Basic;

namespace CoreLib
{
	namespace Text
	{
		TokenReader::TokenReader(String text)
		{
			this->tokens = TokenizeText("", text, [&](TokenizeErrorType, CodePosition) {legal = false; });
			tokenPtr = 0;
		}

		enum class State
		{
			Start, Identifier, Operator, Int, Hex, Fixed, Double, Char, String, MultiComment, SingleComment
		};

		enum class LexDerivative
		{
			None, Line, File
		};

		void ParseOperators(const String & str, List<Token> & tokens, TokenFlags& tokenFlags, int line, int col, int startPos, String fileName)
		{
			int pos = 0;
			while (pos < str.Length())
			{
				wchar_t curChar = str[pos];
				wchar_t nextChar = (pos < str.Length() - 1) ? str[pos + 1] : '\0';
				wchar_t nextNextChar = (pos < str.Length() - 2) ? str[pos + 2] : '\0';
				auto InsertToken = [&](TokenType type, const String & ct)
				{
					tokens.Add(Token(type, ct, line, col + pos, pos + startPos, fileName, tokenFlags));
                    tokenFlags = 0;
				};
				switch (curChar)
				{
				case '+':
					if (nextChar == '+')
					{
						InsertToken(TokenType::OpInc, "++");
						pos += 2;
					}
					else if (nextChar == '=')
					{
						InsertToken(TokenType::OpAddAssign, "+=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpAdd, "+");
						pos++;
					}
					break;
				case '-':
					if (nextChar == '-')
					{
						InsertToken(TokenType::OpDec, "--");
						pos += 2;
					}
					else if (nextChar == '=')
					{
						InsertToken(TokenType::OpSubAssign, "-=");
						pos += 2;
					}
					else if (nextChar == '>')
					{
						InsertToken(TokenType::RightArrow, "->");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpSub, "-");
						pos++;
					}
					break;
				case '*':
					if (nextChar == '=')
					{
						InsertToken(TokenType::OpMulAssign, "*=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpMul, "*");
						pos++;
					}
					break;
				case '/':
					if (nextChar == '=')
					{
						InsertToken(TokenType::OpDivAssign, "/=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpDiv, "/");
						pos++;
					}
					break;
				case '%':
					if (nextChar == '=')
					{
						InsertToken(TokenType::OpModAssign, "%=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpMod, "%");
						pos++;
					}
					break;
				case '|':
					if (nextChar == '|')
					{
						InsertToken(TokenType::OpOr, "||");
						pos += 2;
					}
					else if (nextChar == '=')
					{
						InsertToken(TokenType::OpOrAssign, "|=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitOr, "|");
						pos++;
					}
					break;
				case '&':
					if (nextChar == '&')
					{
						InsertToken(TokenType::OpAnd, "&&");
						pos += 2;
					}
					else if (nextChar == '=')
					{
						InsertToken(TokenType::OpAndAssign, "&=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitAnd, "&");
						pos++;
					}
					break;
				case '^':
					if (nextChar == '=')
					{
						InsertToken(TokenType::OpXorAssign, "^=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitXor, "^");
						pos++;
					}
					break;
				case '>':
					if (nextChar == '>')
					{
						if (nextNextChar == '=')
						{
							InsertToken(TokenType::OpShrAssign, ">>=");
							pos += 3;
						}
						else
						{
							InsertToken(TokenType::OpRsh, ">>");
							pos += 2;
						}
					}
					else if (nextChar == '=')
					{
						InsertToken(TokenType::OpGeq, ">=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpGreater, ">");
						pos++;
					}
					break;
				case '<':
					if (nextChar == '<')
					{
						if (nextNextChar == '=')
						{
							InsertToken(TokenType::OpShlAssign, "<<=");
							pos += 3;
						}
						else
						{
							InsertToken(TokenType::OpLsh, "<<");
							pos += 2;
						}
					}
					else if (nextChar == '=')
					{
						InsertToken(TokenType::OpLeq, "<=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpLess, "<");
						pos++;
					}
					break;
				case '=':
					if (nextChar == '=')
					{
						InsertToken(TokenType::OpEql, "==");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpAssign, "=");
						pos++;
					}
					break;
				case '!':
					if (nextChar == '=')
					{
						InsertToken(TokenType::OpNeq, "!=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpNot, "!");
						pos++;
					}
					break;
				case '?':
					InsertToken(TokenType::QuestionMark, "?");
					pos++;
					break;
				case '@':
					InsertToken(TokenType::At, "@");
					pos++;
					break;
                case '#':
                    if (nextChar == '#')
					{
                        InsertToken(TokenType::PoundPound, "##");
						pos += 2;
					}
					else
					{
                        InsertToken(TokenType::Pound, "#");
						pos++;
					}
					pos++;
					break;
				case ':':
					InsertToken(TokenType::Colon, ":");
					pos++;
					break;
				case '~':
					InsertToken(TokenType::OpBitNot, "~");
					pos++;
					break;
				case ';':
					InsertToken(TokenType::Semicolon, ";");
					pos++;
					break;
				case ',':
					InsertToken(TokenType::Comma, ",");
					pos++;
					break;
				case '.':
					InsertToken(TokenType::Dot, ".");
					pos++;
					break;
				case '{':
					InsertToken(TokenType::LBrace, "{");
					pos++;
					break;
				case '}':
					InsertToken(TokenType::RBrace, "}");
					pos++;
					break;
				case '[':
					InsertToken(TokenType::LBracket, "[");
					pos++;
					break;
				case ']':
					InsertToken(TokenType::RBracket, "]");
					pos++;
					break;
				case '(':
					InsertToken(TokenType::LParent, "(");
					pos++;
					break;
				case ')':
					InsertToken(TokenType::RParent, ")");
					pos++;
					break;
				}
			}
		}

		List<Token> TokenizeText(const String & fileName, const String & text, Procedure<TokenizeErrorType, CodePosition> errorHandler)
		{
			int lastPos = 0, pos = 0;
			int line = 1, col = 0;
			String file = fileName;
			State state = State::Start;
			StringBuilder tokenBuilder;
			int tokenLine, tokenCol;
			List<Token> tokenList;
			LexDerivative derivative = LexDerivative::None;
            TokenFlags tokenFlags = TokenFlag::AtStartOfLine;
			auto InsertToken = [&](TokenType type)
			{
				derivative = LexDerivative::None;
				tokenList.Add(Token(type, tokenBuilder.ToString(), tokenLine, tokenCol, pos, file, tokenFlags));
                tokenFlags = 0;
				tokenBuilder.Clear();
			};
			auto ProcessTransferChar = [&](char nextChar)
			{
				switch (nextChar)
				{
				case '\\':
				case '\"':
				case '\'':
					tokenBuilder.Append(nextChar);
					break;
				case 't':
					tokenBuilder.Append('\t');
					break;
				case 's':
					tokenBuilder.Append(' ');
					break;
				case 'n':
					tokenBuilder.Append('\n');
					break;
				case 'r':
					tokenBuilder.Append('\r');
					break;
				case 'b':
					tokenBuilder.Append('\b');
					break;
				}
			};
			while (pos <= text.Length())
			{
				char curChar = (pos < text.Length() ? text[pos] : ' ');
				char nextChar = (pos < text.Length() - 1) ? text[pos + 1] : '\0';
				if (lastPos != pos)
				{
					if (curChar == '\n')
					{
						line++;
						col = 0;
					}
					else
						col++;
					lastPos = pos;
				}

				switch (state)
				{
				case State::Start:
					if (IsLetter(curChar))
					{
						state = State::Identifier;
						tokenLine = line;
						tokenCol = col;
					}
					else if (IsDigit(curChar))
					{
						state = State::Int;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == '\'')
					{
						state = State::Char;
						pos++;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == '"')
					{
						state = State::String;
						pos++;
						tokenLine = line;
						tokenCol = col;
					}
                    else if (curChar == '\r' || curChar == '\n')
                    {
                        tokenFlags |= TokenFlag::AtStartOfLine | TokenFlag::AfterWhitespace;
                        pos++;
                    }
					else if (curChar == ' ' || curChar == '\t' || curChar == -62 || curChar== -96) // -62/-96:non-break space
                    {
                        tokenFlags |= TokenFlag::AfterWhitespace;
						pos++;
                    }
					else if (curChar == '/' && nextChar == '/')
					{
						state = State::SingleComment;
						pos += 2;
					}
					else if (curChar == '/' && nextChar == '*')
					{
						pos += 2;
						state = State::MultiComment;
					}
					else if (curChar == '.')
					{
						if (IsDigit(nextChar))
						{
							state = State::Int;
							tokenLine = line;
							tokenCol = col;
						}
						else
						{
							state = State::Operator;
							tokenLine = line;
							tokenCol = col;

							tokenBuilder.Append(curChar);
							pos++;
						}
					}
					else if (IsPunctuation(curChar))
					{
						state = State::Operator;
						tokenLine = line;
						tokenCol = col;
					}
					else
					{
						errorHandler(TokenizeErrorType::InvalidCharacter, CodePosition(line, col, pos, file));
						pos++;
					}
					break;
				case State::Identifier:
					if (IsLetter(curChar) || IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						auto tokenStr = tokenBuilder.ToString();
#if 0
						if (tokenStr == "#line_reset#")
						{
							line = 0;
							col = 0;
							tokenBuilder.Clear();
						}
						else if (tokenStr == "#line")
						{
							derivative = LexDerivative::Line;
							tokenBuilder.Clear();
						}
						else if (tokenStr == "#file")
						{
							derivative = LexDerivative::File;
							tokenBuilder.Clear();
							line = 0;
							col = 0;
						}
						else
#endif
							InsertToken(TokenType::Identifier);
						state = State::Start;
					}
					break;
				case State::Operator:
					if (IsPunctuation(curChar) && !((curChar == '/' && nextChar == '/') || (curChar == '/' && nextChar == '*')))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						//do token analyze
						ParseOperators(tokenBuilder.ToString(), tokenList, tokenFlags, tokenLine, tokenCol, pos - tokenBuilder.Length(), file);
						tokenBuilder.Clear();
						state = State::Start;
					}
					break;
				case State::Int:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == '.')
					{
						state = State::Fixed;
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == 'e' || curChar == 'E')
					{
						state = State::Double;
						tokenBuilder.Append(curChar);
						if (nextChar == '-' || nextChar == '+')
						{
							tokenBuilder.Append(nextChar);
							pos++;
						}
						pos++;
					}
					else if (curChar == 'x')
					{
						state = State::Hex;
						tokenBuilder.Append(curChar);
						pos++;
					}
					// Allow `u` or `U` suffix to indicate unsigned literal
					// TODO(tfoley): handle more general suffixes on literals
					else if (curChar == 'u' || curChar == 'U')
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						if (derivative == LexDerivative::Line)
						{
							derivative = LexDerivative::None;
							line = StringToInt(tokenBuilder.ToString()) - 1;
							col = 0;
							tokenBuilder.Clear();
						}
						else
						{
							InsertToken(TokenType::IntLiterial);
						}
						state = State::Start;
					}
					break;
				case State::Hex:
					if (IsDigit(curChar) || (curChar>='a' && curChar <= 'f') || (curChar >= 'A' && curChar <= 'F'))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						InsertToken(TokenType::IntLiterial);
						state = State::Start;
					}
					break;
				case State::Fixed:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == 'e' || curChar == 'E')
					{
						state = State::Double;
						tokenBuilder.Append(curChar);
						if (nextChar == '-' || nextChar == '+')
						{
							tokenBuilder.Append(nextChar);
							pos++;
						}
						pos++;
					}
					else
					{
						if (curChar == 'f')
							pos++;
						InsertToken(TokenType::DoubleLiterial);
						state = State::Start;
					}
					break;
				case State::Double:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						if (curChar == 'f')
							pos++;
						InsertToken(TokenType::DoubleLiterial);
						state = State::Start;
					}
					break;
				case State::String:
					if (curChar != '"')
					{
						if (curChar == '\\')
						{
							ProcessTransferChar(nextChar);
							pos++;
						}
						else
							tokenBuilder.Append(curChar);
					}
					else
					{
						if (derivative == LexDerivative::File)
						{
							derivative = LexDerivative::None;
							file = tokenBuilder.ToString();
							tokenBuilder.Clear();
						}
						else
						{
							InsertToken(TokenType::StringLiterial);
						}
						state = State::Start;
					}
					pos++;
					break;
				case State::Char:
					if (curChar != '\'')
					{
						if (curChar == '\\')
						{
							ProcessTransferChar(nextChar);
							pos++;
						}
						else
							tokenBuilder.Append(curChar);
					}
					else
					{
						if (tokenBuilder.Length() > 1)
							errorHandler(TokenizeErrorType::InvalidEscapeSequence, CodePosition(line, col - tokenBuilder.Length(), pos, file));

						InsertToken(TokenType::CharLiterial);
						state = State::Start;
					}
					pos++;
					break;
				case State::SingleComment:
					if(curChar == '\r' || curChar == '\n')
					{
						state = State::Start;
						tokenFlags |= TokenFlag::AtStartOfLine | TokenFlag::AfterWhitespace;
					}
					pos++;
					break;
				case State::MultiComment:
					if (curChar == '*' && nextChar == '/')
					{
						state = State::Start;
						pos += 2;
					}
					else
						pos++;
					break;
				}
			}
			return tokenList;
		}
		List<Token> TokenizeText(const String & fileName, const String & text)
		{
			return TokenizeText(fileName, text, [](TokenizeErrorType, CodePosition) {});
		}
		List<Token> TokenizeText(const String & text)
		{
			return TokenizeText("", text, [](TokenizeErrorType, CodePosition) {});
		}

		String EscapeStringLiteral(String str)
		{
			StringBuilder sb;
			sb << "\"";
			for (int i = 0; i < str.Length(); i++)
			{
				switch (str[i])
				{
				case ' ':
					sb << "\\s";
					break;
				case '\n':
					sb << "\\n";
					break;
				case '\r':
					sb << "\\r";
					break;
				case '\t':
					sb << "\\t";
					break;
				case '\v':
					sb << "\\v";
					break;
				case '\'':
					sb << "\\\'";
					break;
				case '\"':
					sb << "\\\"";
					break;
				case '\\':
					sb << "\\\\";
					break;
				default:
					sb << str[i];
					break;
				}
			}
			sb << "\"";
			return sb.ProduceString();
		}

		String UnescapeStringLiteral(String str)
		{
			StringBuilder sb;
			for (int i = 0; i < str.Length(); i++)
			{
				if (str[i] == '\\' && i < str.Length() - 1)
				{
					switch (str[i + 1])
					{
					case 's':
						sb << " ";
						break;
					case 't':
						sb << '\t';
						break;
					case 'n':
						sb << '\n';
						break;
					case 'r':
						sb << '\r';
						break;
					case 'v':
						sb << '\v';
						break;
					case '\'':
						sb << '\'';
						break;
					case '\"':
						sb << "\"";
						break;
					case '\\':
						sb << "\\";
						break;
					default:
						i = i - 1;
						sb << str[i];
					}
					i++;
				}
				else
					sb << str[i];
			}
			return sb.ProduceString();
		}


		String TokenTypeToString(TokenType type)
		{
			switch (type)
			{
			case TokenType::EndOfFile:
				return "end of file";
			case TokenType::Unknown:
				return "UnknownToken";
			case TokenType::Identifier:
				return "identifier";
			case TokenType::IntLiterial:
				return "integer literal";
			case TokenType::DoubleLiterial:
				return "floating-point literal";
			case TokenType::StringLiterial:
				return "string literal";
			case TokenType::CharLiterial:
				return "character literal";
			case TokenType::QuestionMark:
				return "'?'";
			case TokenType::Colon:
				return "':'";
			case TokenType::Semicolon:
				return "';'";
			case TokenType::Comma:
				return "','";
			case TokenType::LBrace:
				return "'{'";
			case TokenType::RBrace:
				return "'}'";
			case TokenType::LBracket:
				return "'['";
			case TokenType::RBracket:
				return "']'";
			case TokenType::LParent:
				return "'('";
			case TokenType::RParent:
				return "')'";
			case TokenType::At:
				return "'@'";
			case TokenType::OpAssign:
				return "'='";
			case TokenType::OpAdd:
				return "'+'";
			case TokenType::OpSub:
				return "'-'";
			case TokenType::OpMul:
				return "'*'";
			case TokenType::OpDiv:
				return "'/'";
			case TokenType::OpMod:
				return "'%'";
			case TokenType::OpNot:
				return "'!'";
			case TokenType::OpLsh:
				return "'<<'";
			case TokenType::OpRsh:
				return "'>>'";
			case TokenType::OpAddAssign:
				return "'+='";
			case TokenType::OpSubAssign:
				return "'-='";
			case TokenType::OpMulAssign:
				return "'*='";
			case TokenType::OpDivAssign:
				return "'/='";
			case TokenType::OpModAssign:
				return "'%='";
			case TokenType::OpEql:
				return "'=='";
			case TokenType::OpNeq:
				return "'!='";
			case TokenType::OpGreater:
				return "'>'";
			case TokenType::OpLess:
				return "'<'";
			case TokenType::OpGeq:
				return "'>='";
			case TokenType::OpLeq:
				return "'<='";
			case TokenType::OpAnd:
				return "'&&'";
			case TokenType::OpOr:
				return "'||'";
			case TokenType::OpBitXor:
				return "'^'";
			case TokenType::OpBitAnd:
				return "'&'";
			case TokenType::OpBitOr:
				return "'|'";
			case TokenType::OpInc:
				return "'++'";
			case TokenType::OpDec:
				return "'--'";
			case TokenType::Pound:
				return "'#'";
			case TokenType::PoundPound:
				return "'##'";
			default:
				return "";
			}
		}

		List<String> Split(String text, char c)
		{
			List<String> result;
			StringBuilder sb;
			for (int i = 0; i < text.Length(); i++)
			{
				if (text[i] == c)
				{
					auto str = sb.ToString();
					if (str.Length() != 0)
						result.Add(str);
					sb.Clear();
				}
				else
					sb << text[i];
			}
			auto lastStr = sb.ToString();
			if (lastStr.Length())
				result.Add(lastStr);
			return result;
		}

	}
}