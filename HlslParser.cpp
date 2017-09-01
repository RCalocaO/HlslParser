// HlslParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "HlslAST.h"

std::string Filename = ".\\tests\\expr.hlsl";

static std::string Load(const char* Filename)
{
	std::vector<char> Data;
	FILE* File;
	long Size = 0;
	fopen_s(&File, Filename, "rb");
	if (File)
	{
		fseek(File, 0, SEEK_END);
		Size = ftell(File);
		Data.resize(Size);
		fseek(File, 0, SEEK_SET);
		fread(&Data[0], 1, Size, File);
		fclose(File);
	}

	std::string String = &Data[0];
	String.resize(Size);
	return String;
}

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

FOperator::EType TokenToOperator(EToken Token)
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
		assert(0);
		break;
	}
	return FOperator::Error;
}


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
				if (*Data == '-' || *Data =='+')
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


struct FParseRules
{
	std::list<FBase*> Values;
//	std::map<std::vector<EToken>, std::function<bool(FParseRules&, FTokenizer& Tokenizer)>> Rules;
	FTokenizer& Tokenizer;

	FParseRules(FTokenizer& InTokenizer)
		: Tokenizer(InTokenizer)
	{
//		Rules[{EToken::Plus, EToken::Minus}] = &FParseRules::ParseExpressionAdditive;
	}
	
	bool Error()
	{
		return false;
	}

	bool ParseExpressionPrimary()
	{
		if (Tokenizer.Match(EToken::Integer))
		{
			auto* Node = new FConstant;
			Node->Type = FConstant::Integer;
			Node->IntValue = atoi(Tokenizer.PreviousTokenString().c_str());
			Values.push_back(Node);
			return true;
		}
		else if (Tokenizer.Match(EToken::Float))
		{
			auto* Node = new FConstant;
			Node->Type = FConstant::Float;
			Node->FloatValue = (float)atof(Tokenizer.PreviousTokenString().c_str());
			Values.push_back(Node);
			return true;
		}
		else if (Tokenizer.Match(EToken::LeftParenthesis))
		{
			if (!ParseExpression())
			{
				return Error();
			}

			return Tokenizer.Match(EToken::RightParenthesis);
		}
		return true;
	}

	bool ParseExpressionUnary()
	{
		bool bUnaryMinus = false;
		if (Tokenizer.PeekToken() == EToken::Minus)
		{
			bUnaryMinus = true;
			Tokenizer.Advance();
		}

		if (!ParseExpressionPrimary())
		{
			return Error();
		}

		if (bUnaryMinus)
		{
			auto* Node = new FOperator;
			Node->Type = FOperator::UnaryMinus;
			assert(!Values.empty());
			Node->LHS = Values.back();
			Values.pop_back();
			Values.push_back(Node);
		}
		return true;
	}

	bool ParseExpressionMultiplicative()
	{
		if (!ParseExpressionUnary())
		{
			return Error();
		}

		while (Tokenizer.PeekToken() == EToken::Multiply)
		{
			Tokenizer.Advance();
			auto* Node = new FOperator;
			Node->Type = TokenToOperator(Tokenizer.PreviousTokenType());
			if (!ParseExpressionUnary())
			{
				return Error();
			}

			assert(Values.size() >= 2);
			Node->RHS = Values.back();
			Values.pop_back();
			Node->LHS = Values.back();
			Values.pop_back();
			Values.push_back(Node);
		}

		return true;
	}

	bool ParseExpressionAdditive()
	{
		if (!ParseExpressionMultiplicative())
		{
			return Error();
		}

		while (Tokenizer.PeekToken() == EToken::Plus || Tokenizer.PeekToken() == EToken::Minus)
		{
			Tokenizer.Advance();
			auto* Node = new FOperator;
			Node->Type = TokenToOperator(Tokenizer.PreviousTokenType());
			if (!ParseExpressionMultiplicative())
			{
				return Error();
			}

			assert(Values.size() >= 2);
			Node->RHS = Values.back();
			Values.pop_back();
			Node->LHS = Values.back();
			Values.pop_back();
			Values.push_back(Node);
		}

		return true;
	}

	bool ParseExpression()
	{
		if (!ParseExpressionAdditive())
		{
			return Error();
		}

		return true;
	}
};

static bool Parse(std::vector<FToken>& Tokens)
{
	FTokenizer Tokenizer(Tokens);
	FParseRules ParseRules(Tokenizer);
	bool bSuccess = ParseRules.ParseExpression();

	for (auto* Value : ParseRules.Values)
	{
		Value->Write();
	}
	return bSuccess;
}

int main()
{
	std::string Data = Load(Filename.c_str());

	FLexer Lexer(Data);
	std::vector<FToken> Tokens;
	bool bError = false;
	while (Lexer.HasMoreTokens())
	{
		FToken Token = Lexer.NextToken();
		if (Token.TokenType == EToken::Error)
		{
			bError = true;
			break;
		}
		Tokens.push_back(Token);
	}

	if (bError)
	{
		return 1;
	}

	if (!Parse(Tokens))
	{
		return 1;
	}

	return 0;
}
