
#pragma once

enum class EToken
{
	Error,
	Integer,
	Float,
	Plus,
	Multiply,
	Minus,
	LeftParenthesis,
	RightParenthesis,
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

	FToken NextToken()
	{
		FToken Token;
		SkipWhiteSpace();

		char c = *Data;
		assert(c);
		c = *Data;
		if (isdigit(c))
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
		else
		{
			switch (c)
			{
			case '+': ++Data; Token.TokenType = EToken::Plus; break;
			case '*': ++Data; Token.TokenType = EToken::Multiply; break;
			case '-': ++Data; Token.TokenType = EToken::Minus; break;
			case '(': ++Data; Token.TokenType = EToken::LeftParenthesis; break;
			case ')': ++Data; Token.TokenType = EToken::RightParenthesis; break;
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
	case EToken::Minus:
		return FOperator::Subtract;
	case EToken::Multiply:
		return FOperator::Mul;
	case EToken::Plus:
		return FOperator::Plus;
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