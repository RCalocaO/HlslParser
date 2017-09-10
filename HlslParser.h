
#pragma once

enum class EToken
{
	Error,
	Semicolon,
	Integer,
	HexInteger,
	Float,
	Plus,
	PlusPlus,
	Multiply,
	Divide,
	Mod,
	Minus,
	MinusMinus,
	LeftParenthesis,
	RightParenthesis,
	Question,
	Colon,
	Exclamation,
	Ampersand,
	AmpersandAmpersand,
	Pipe,
	PipePipe,
	Greater,
	GreaterGreater,
	GreaterEqual,
	Lower,
	LowerLower,
	LowerEqual,
	EqualEqual,
	ExclamationEqual,
	Caret,
	Tilde,
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

	std::map<char, std::map<char, EToken> > SimpleTokens;

	FLexer(std::string InSource)
		: Source(InSource)
	{
		Data = Source.c_str();
		End = Data + Source.length();

		SimpleTokens['+'][0] = EToken::Plus;
		SimpleTokens['+']['+'] = EToken::PlusPlus;
		SimpleTokens['-'][0] = EToken::Minus;
		SimpleTokens['-']['-'] = EToken::MinusMinus;
		SimpleTokens['*'][0] = EToken::Multiply;
		SimpleTokens['/'][0] = EToken::Divide;
		SimpleTokens['%'][0] = EToken::Mod;
		SimpleTokens['('][0] = EToken::LeftParenthesis;
		SimpleTokens[')'][0] = EToken::RightParenthesis;
		SimpleTokens[';'][0] = EToken::Semicolon;
		SimpleTokens[':'][0] = EToken::Colon;
		SimpleTokens['?'][0] = EToken::Question;
		SimpleTokens['!'][0] = EToken::Exclamation;
		SimpleTokens['&'][0] = EToken::Ampersand;
		SimpleTokens['&']['&'] = EToken::AmpersandAmpersand;
		SimpleTokens['|'][0] = EToken::Pipe;
		SimpleTokens['|']['|'] = EToken::PipePipe;
		SimpleTokens['>'][0] = EToken::Greater;
		SimpleTokens['>']['>'] = EToken::GreaterGreater;
		SimpleTokens['>']['='] = EToken::GreaterEqual;
		SimpleTokens['<'][0] = EToken::Lower;
		SimpleTokens['<']['<'] = EToken::LowerLower;
		SimpleTokens['<']['='] = EToken::LowerEqual;
		SimpleTokens['^'][0] = EToken::Caret;
		SimpleTokens['~'][0] = EToken::Tilde;
		SimpleTokens['=']['='] = EToken::EqualEqual;
		SimpleTokens['!']['='] = EToken::ExclamationEqual;
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
			auto Found = SimpleTokens.find(c);
			if (Found != SimpleTokens.end())
			{
				++Data;
				auto DoubleFound = Found->second.find(*Data);
				if (DoubleFound != Found->second.end())
				{
					++Data;
				}
				else
				{
					DoubleFound = Found->second.find(0);
				}

				assert(DoubleFound != Found->second.end());
				Token.TokenType = DoubleFound->second;
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
	case EToken::Caret:
		return FOperator::Xor;
	case EToken::Greater:
		return FOperator::Greater;
	case EToken::GreaterGreater:
		return FOperator::ShitRight;
	case EToken::GreaterEqual:
		return FOperator::GreaterEqual;
	case EToken::Lower:
		return FOperator::Lower;
	case EToken::LowerLower:
		return FOperator::ShitLeft;
	case EToken::LowerEqual:
		return FOperator::LowerEqual;
	case EToken::EqualEqual:
		return FOperator::Equals;
	case EToken::ExclamationEqual:
		return FOperator::NotEquals;
	case EToken::Ampersand:
		return FOperator::BitAnd;
	case EToken::AmpersandAmpersand:
		return FOperator::LogicalAnd;
	case EToken::Pipe:
		return FOperator::BitOr;
	case EToken::PipePipe:
		return FOperator::LogicalOr;
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
	case EToken::Exclamation:
		return FOperator::Not;
	case EToken::Tilde:
		return FOperator::Neg;
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
