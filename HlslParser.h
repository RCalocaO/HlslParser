
#pragma once

enum class EToken
{
	Error,
	Semicolon,
	Integer,
	HexInteger,
	Float,
	Plus,
	Multiply,
	Divide,
	Mod,
	Minus,
	LeftParenthesis,
	RightParenthesis,
	Question,
	Colon,
};

struct FToken
{
	FToken(EToken InTokenType = EToken::Error)
		: TokenType(InTokenType)
	{
	}

	EToken TokenType;
	std::string Literal;
};


struct FLexer
{
	const char* Data;
	const char* End;
	std::string Source;

	FLexer(std::string InSource)
		: Source(InSource)
	{
		Data = Source.c_str();
		End = Data + Source.length();
	}

	void SkipWhiteSpace()
	{
		char c = *Data;
		FToken Token;

		while (Data < End && (c == ' ' || c == '\n' || c == '\t' || c == '\r'))
		{
			++Data;
			c = *Data;
		}
	}

	bool HasMoreTokens()
	{
		SkipWhiteSpace();
		if (Data >= End)
		{
			assert(Data == End);
			return false;
		}
		return true;
	}

	bool TryParseHexInteger(FToken& Token)
	{
		const char* TempData = Data + 1;
		if (*TempData == 'x' || *TempData == 'X')
		{
			uint32_t Value = 0;
			++TempData;
			while (TempData < End)
			{
				if (isdigit(*TempData))
				{
					Value *= 16;
					Value += *TempData - '0';
					++TempData;
				}
				else if (*TempData >= 'a' && *TempData <= 'f')
				{
					Value *= 16;
					Value += *TempData - 'a' + 10;
					++TempData;
				}
				else if (*TempData >= 'A' && *TempData <= 'F')
				{
					Value *= 16;
					Value += *TempData - 'A' + 10;
					++TempData;
				}
				else
				{
					break;
				}
			}

			Token.TokenType = EToken::HexInteger;
			do
			{
				Token.Literal += (Value % 10) + '0';
				Value /= 10;
			}
			while (Value > 0);
			std::reverse(Token.Literal.begin(), Token.Literal.end());
			Data = TempData;
			return true;
		}

		return false;
	}

	FToken NextToken()
	{
		FToken Token;
		SkipWhiteSpace();

		char c = *Data;
		assert(c);
		c = *Data;
		if (isdigit(c))
		{
			if (!TryParseHexInteger(Token))
			{
				do
				{
					Token.Literal += *Data;
					++Data;
				} while (isdigit(*Data));

				Token.TokenType = EToken::Integer;

				if (*Data == '.')
				{
					Token.Literal += *Data++;
					Token.TokenType = EToken::Float;
					while (isdigit(*Data))
					{
						Token.Literal += *Data;
						++Data;
					}
				}

				if (*Data == 'e' || *Data == 'E')
				{
					Token.Literal += *Data++;
					if (*Data == '-' || *Data == '+')
					{
						Token.Literal += *Data++;
					}
					while (isdigit(*Data))
					{
						Token.Literal += *Data;
						++Data;
					}
				}
			}
		}
		else
		{
			switch (c)
			{
			case '+': ++Data; Token.TokenType = EToken::Plus; break;
			case '-': ++Data; Token.TokenType = EToken::Minus; break;
			case '*': ++Data; Token.TokenType = EToken::Multiply; break;
			case '/': ++Data; Token.TokenType = EToken::Divide; break;
			case '%': ++Data; Token.TokenType = EToken::Mod; break;
			case '(': ++Data; Token.TokenType = EToken::LeftParenthesis; break;
			case ')': ++Data; Token.TokenType = EToken::RightParenthesis; break;
			case ';': ++Data; Token.TokenType = EToken::Semicolon; break;
			case ':': ++Data; Token.TokenType = EToken::Colon; break;
			case '?': ++Data; Token.TokenType = EToken::Question; break;
			default:
				break;
			}
		}

		return Token;
	}
};

struct FTokenizer
{
	std::vector<FToken> Tokens;
	FBase* Expression = nullptr;

	FTokenizer(const std::vector<FToken>& InTokens)
		: Tokens(InTokens)
	{
	}

	uint32_t Current = 0;

	static uint32_t ConvertHex(const char* String)
	{
		return 0;
	}

	bool HasMoreTokens() const
	{
		return Current < Tokens.size();
	}

	void Advance()
	{
		if (HasMoreTokens())
		{
			++Current;
		}
	}

	bool Match(EToken Token)
	{
		if (HasMoreTokens())
		{
			if (Tokens[Current].TokenType == Token)
			{
				++Current;
				return true;
			}
		}

		return false;
	}

	EToken PeekToken() const
	{
		if (HasMoreTokens())
		{
			return Tokens[Current].TokenType;
		}

		return EToken::Error;
	}

	const std::string& PreviousTokenString() const
	{
		assert(Current > 0);
		return Tokens[Current - 1].Literal;
	}

	EToken PreviousTokenType() const
	{
		assert(Current > 0);
		return Tokens[Current - 1].TokenType;
	}
};


inline FOperator::EType TokenToBinaryOperator(EToken Token, bool bAssertIfNotOperator)
{
	switch (Token)
	{
	case EToken::Plus:
		return FOperator::Add;
	case EToken::Minus:
		return FOperator::Subtract;
	case EToken::Multiply:
		return FOperator::Mul;
	case EToken::Divide:
		return FOperator::Divide;
	case EToken::Mod:
		return FOperator::Remainder;
	default:
		if (bAssertIfNotOperator)
		{
			assert(0);
		}
		break;
	}
	return FOperator::Error;
}

inline bool IsBinaryOperator(EToken Token)
{
	return TokenToBinaryOperator(Token, false) != FOperator::Error;
}

inline FOperator::EType TokenToUnaryOperator(EToken Token, bool bAssertIfNotOperator)
{
	switch (Token)
	{
	case EToken::Minus:
		return FOperator::UnaryMinus;
	case EToken::Plus:
		return FOperator::UnaryPlus;
	default:
		if (bAssertIfNotOperator)
		{
			assert(0);
		}
		break;
	}
	return FOperator::Error;
}

inline bool IsUnaryOperator(EToken Token)
{
	return TokenToUnaryOperator(Token, false) != FOperator::Error;
}
