// HlslParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

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
		assert(c);
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

static void Parse(const std::string& Data)
{
	FLexer Lexer(Data);
	std::vector<FToken> Tokens;
	while (Lexer.HasMoreTokens())
	{
		FToken Token = Lexer.NextToken();
		if (Token.TokenType == EToken::Error)
		{
			break;
		}
		Tokens.push_back(Token);
	}
}

int main()
{
	std::string Data = Load(Filename.c_str());

	Parse(Data);

	return 0;
}
