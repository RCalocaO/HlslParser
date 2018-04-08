// HlslParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "HlslAST.h"
#include "HlslParser.h"
#include "HlslIR.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>

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

int GetPrecedence(AST::FOperator::EType Operator)
{
	using namespace AST;
	switch (Operator)
	{
	case FOperator::Ternary:
		return 20;
	case FOperator::LogicalOr:
		return 25;
	case FOperator::LogicalAnd:
		return 27;
	case FOperator::Xor:
		return 30;
	case FOperator::BitOr:
		return 33;
	case FOperator::BitAnd:
		return 34;
	case FOperator::Equals:
	case FOperator::NotEquals:
		return 40;
	case FOperator::Greater:
	case FOperator::GreaterEqual:
	case FOperator::Lower:
	case FOperator::LowerEqual:
		return 42;
	case FOperator::ShitLeft:
	case FOperator::ShitRight:
		return 45;
	case FOperator::Add:
	case FOperator::Subtract:
		return 50;
	case FOperator::Mul:
	case FOperator::Divide:
	case FOperator::Remainder:
		return 60;
	case FOperator::UnaryPlus:
	case FOperator::UnaryMinus:
	case FOperator::Neg:
	case FOperator::Not:
		return 70;
	default:
		assert(0);
		break;
	}

	return 0;
}


struct FBaseParseRules
{
	std::list<AST::FBase*> Values;
//	std::map<std::vector<EToken>, std::function<bool(FParseRules&, FTokenizer& Tokenizer)>> Rules;
	FTokenizer& Tokenizer;

	FBaseParseRules(FTokenizer& InTokenizer)
		: Tokenizer(InTokenizer)
	{
	//		Rules[{EToken::Plus, EToken::Minus}] = &FParseRules::ParseExpressionAdditive;
	}

	bool Error()
	{
		return false;
	}

	virtual bool ParseExpression() = 0;

	bool ParseTerminal(EToken Token, const std::string& String)
	{
		if (Token == EToken::Integer)
		{
			auto* Node = new AST::FConstant;
			Node->Type = AST::FConstant::Integer;
			Node->UIntValue = atoi(String.c_str());
			PushTerminal(Node);
			return true;
		}
		else if (Token == EToken::HexInteger)
		{
			auto* Node = new AST::FConstant;
			Node->Type = AST::FConstant::HexInteger;
			Node->UIntValue = (uint32_t)atoll(String.c_str());
			PushTerminal(Node);
			return true;
		}
		else if (Token == EToken::Float)
		{
			auto* Node = new AST::FConstant;
			Node->Type = AST::FConstant::Float;
			Node->FloatValue = (float)atof(String.c_str());
			PushTerminal(Node);
			return true;
		}

		return false;
	}

	bool ParseTerminal()
	{
		EToken Token = Tokenizer.PeekToken();
		if (ParseTerminal(Token, Tokenizer.TokenString()))
		{
			Tokenizer.Advance();
			return true;
		}

		return false;
	}

	virtual void PushTerminal(AST::FBase* Node)
	{
		Values.push_back(Node);
	}
};


// http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
struct FParseRulesRecursiveDescent : public FBaseParseRules
{
	FParseRulesRecursiveDescent(FTokenizer& InTokenizer)
		: FBaseParseRules(InTokenizer)
	{
	}

	void MakeOperator(AST::FOperator::EType Type, uint32_t NumOperands)
	{
		assert(NumOperands >= 1 && NumOperands <= 3);
		auto* Node = new AST::FOperator;
		Node->Type = Type;
		assert(Values.size() >= NumOperands);
		if (NumOperands >= 2)
		{
			Node->RHS = Values.back();
			Values.pop_back();
		}
		Node->LHS = Values.back();
		Values.pop_back();
		if (NumOperands == 3)
		{
			Node->TernaryCondition = Values.back();
			Values.pop_back();
		}
		Values.push_back(Node);

	}

	bool ParseExpressionPrimary()
	{
		if (ParseTerminal())
		{
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

		Error();
		return false;
	}

	bool ParseExpressionUnary()
	{
		AST::FOperator::EType UnaryOp = AST::FOperator::Sentinel;
		if (Tokenizer.PeekToken() == EToken::Minus || Tokenizer.PeekToken() == EToken::Plus || Tokenizer.PeekToken() == EToken::Exclamation || Tokenizer.PeekToken() == EToken::Tilde)
		{
			UnaryOp = TokenToUnaryOperator(Tokenizer.PeekToken(), false);
			Tokenizer.Advance();
		}

		if (!ParseExpressionPrimary())
		{
			return Error();
		}

		if (UnaryOp != AST::FOperator::Sentinel)
		{
			MakeOperator(UnaryOp, 1);
		}
		return true;
	}

	bool ParseExpressionMultiplicative()
	{
		if (!ParseExpressionUnary())
		{
			return Error();
		}

		while (Tokenizer.PeekToken() == EToken::Multiply || Tokenizer.PeekToken() == EToken::Divide || Tokenizer.PeekToken() == EToken::Mod)
		{
			Tokenizer.Advance();
			AST::FOperator::EType Type = TokenToBinaryOperator(Tokenizer.PreviousTokenType(), true);
			if (!ParseExpressionUnary())
			{
				return Error();
			}

			MakeOperator(Type, 2);
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
			AST::FOperator::EType Type = TokenToBinaryOperator(Tokenizer.PreviousTokenType(), true);
			if (!ParseExpressionMultiplicative())
			{
				return Error();
			}

			MakeOperator(Type, 2);
		}

		return true;
	}

	bool ParseShiftExpression()
	{
		if (!ParseExpressionAdditive())
		{
			return Error();
		}

		while (Tokenizer.PeekToken() == EToken::GreaterGreater || Tokenizer.PeekToken() == EToken::LowerLower)
		{
			Tokenizer.Advance();
			AST::FOperator::EType Type = TokenToBinaryOperator(Tokenizer.PreviousTokenType(), true);
			if (!ParseExpressionAdditive())
			{
				return Error();
			}

			MakeOperator(Type, 2);
		}

		return true;
	}

	bool ParseRelationalExpression()
	{
		if (!ParseShiftExpression())
		{
			return Error();
		}

		auto IsRelational = [](EToken Token)
		{
			switch (Token)
			{
			case EToken::Greater:
			case EToken::GreaterGreater:
			case EToken::Lower:
			case EToken::LowerEqual:
			case EToken::EqualEqual:
			case EToken::ExclamationEqual:
				return true;
			default:
				break;
			}
			return false;
		};

		if (IsRelational(Tokenizer.PeekToken()))
		{
			Tokenizer.Advance();
			AST::FOperator::EType Type = TokenToBinaryOperator(Tokenizer.PreviousTokenType(), true);
			if (!ParseShiftExpression())
			{
				return Error();
			}

			MakeOperator(Type, 2);
		}

		return true;
	}


	bool ParseBitAndExpression()
	{
		if (!ParseRelationalExpression())
		{
			return false;
		}

		if (Tokenizer.Match(EToken::Ampersand))
		{
			if (!ParseRelationalExpression())
			{
				Error();
				return false;
			}

			MakeOperator(AST::FOperator::BitAnd, 2);
		}

		return true;
	}

	bool ParseXorExpression()
	{
		if (!ParseBitAndExpression())
		{
			return false;
		}

		if (Tokenizer.Match(EToken::Caret))
		{
			if (!ParseBitAndExpression())
			{
				Error();
				return false;
			}

			MakeOperator(AST::FOperator::Xor, 2);
		}

		return true;
	}

	bool ParseBitOrExpression()
	{
		if (!ParseXorExpression())
		{
			return false;
		}

		if (Tokenizer.Match(EToken::Pipe))
		{
			if (!ParseXorExpression())
			{
				Error();
				return false;
			}

			MakeOperator(AST::FOperator::BitOr, 2);
		}

		return true;
	}

	bool ParseLogicalAndExpression()
	{
		if (!ParseBitOrExpression())
		{
			return false;
		}

		while (Tokenizer.Match(EToken::AmpersandAmpersand))
		{
			if (!ParseBitOrExpression())
			{
				Error();
				return false;
			}

			MakeOperator(AST::FOperator::LogicalAnd, 2);
		}

		return true;
	}

	bool ParseLogicalOrExpression()
	{
		if (!ParseLogicalAndExpression())
		{
			return false;
		}

		while (Tokenizer.Match(EToken::PipePipe))
		{
			if (!ParseLogicalAndExpression())
			{
				Error();
				return false;
			}

			MakeOperator(AST::FOperator::LogicalOr, 2);
		}

		return true;
	}

	bool ParseConditionalExpression()
	{
		if (!ParseLogicalOrExpression())
		{
			return Error();
		}

		if (Tokenizer.Match(EToken::Question))
		{
			if (!ParseExpression())
			{
				Error();
				return false;
			}

			if (!Tokenizer.Match(EToken::Colon))
			{
				Error();
				return false;
			}

			if (!ParseConditionalExpression())
			{
				Error();
				return false;
			}

			MakeOperator(AST::FOperator::Ternary, 3);
		}

		return true;
	}

	virtual bool ParseExpression() override
	{
		if (!ParseConditionalExpression())
		{
			return Error();
		}

		return true;
	}
};

struct FParseRulesShuntingYard2 : public FBaseParseRules
{
	FParseRulesShuntingYard2(FTokenizer& InTokenizer)
		: FBaseParseRules(InTokenizer)
	{
	}

	std::stack<AST::FBase*> Operands;
	std::stack<AST::FOperator::EType> Operators;

	void PopOperator()
	{
		using namespace AST;
		assert(!Operators.empty());
		FOperator::EType Top = Operators.top();
		Operators.pop();
		if (FOperator::IsBinary(Top))
		{
			FOperator* Operator = new FOperator;
			Operator->Type = Top;
			assert(Operands.size() >= 2);
			Operator->RHS = Operands.top();
			Operands.pop();
			Operator->LHS = Operands.top();
			Operands.pop();
			Operands.push(Operator);
		}
		else if (FOperator::IsUnary(Top))
		{
			FOperator* Operator = new FOperator;
			Operator->Type = Top;
			assert(!Operands.empty());
			Operator->LHS = Operands.top();
			Operands.pop();
			Operands.push(Operator);
		}
		else if (Top == FOperator::TernaryEnd)
		{
			FOperator* Operator = new FOperator;
			Operator->Type = FOperator::Ternary;
			assert(Operands.size() >= 3);
			Operator->RHS = Operands.top();
			Operands.pop();
			Operator->LHS = Operands.top();
			Operands.pop();
			Operator->TernaryCondition = Operands.top();
			Operands.pop();
			Operands.push(Operator);
		}
		else
		{
			assert(0);
		}
	}

	virtual bool ParseExpression() override
	{
		PushOperator(AST::FOperator::Sentinel);
		if (!E())
		{
			Error();
			return false;
		}
		while (!Operands.empty())
		{
			Values.push_back(Operands.top());
			Operands.pop();
		}

		return true;
	}

	void PushOperator(AST::FOperator::EType Operator)
	{
		if (Operator != AST::FOperator::Sentinel)
		{
			while (!Operators.empty() && Operators.top() != AST::FOperator::Sentinel && GetPrecedence(Operators.top()) > GetPrecedence(Operator))
			{
				PopOperator();
			}
		}

		Operators.push(Operator);
	}

	bool E()
	{
		if (!P())
		{
			Error();
			return false;
		}

		while (Tokenizer.HasMoreTokens())
		{
			if (IsBinaryOperator(Tokenizer.PeekToken()))
			{
				PushOperator(TokenToBinaryOperator(Tokenizer.PeekToken(), false));
				Tokenizer.Advance();
				if (!P())
				{
					Error();
					return false;
				}
			}
			else if (Tokenizer.Match(EToken::Question))
			{
				PushOperator(AST::FOperator::Ternary);
				if (!E())
				{
					return false;
				}

				assert(!Operators.empty());
				assert(Operators.top() == AST::FOperator::Ternary);

				if (!Tokenizer.Match(EToken::Colon))
				{
					Error();
					return false;
				}

				if (!E())
				{
					return false;
				}

				assert(!Operators.empty());
				assert(Operators.top() == AST::FOperator::Ternary);
				Operators.pop();
				Operators.push(AST::FOperator::TernaryEnd);
/*

				FOperator* Operator = new FOperator;
				Operator->Type = FOperator::Ternary;
				assert(Operands.size() >= 3);
				Operator->RHS = Operands.top();
				Operands.pop();
				Operator->LHS = Operands.top();
				Operands.pop();
				Operator->TernaryCondition = Operands.top();
				Operands.pop();
				Operands.push(Operator);
*/
			}
			else
			{
				break;
			}
		}


		while (!Operators.empty() && Operators.top() != AST::FOperator::Sentinel && Operators.top() != AST::FOperator::Ternary)
		{
			PopOperator();
		}
		return true;
	}

	void PushOperand()
	{
		assert(!Values.empty());
		AST::FBase* Operand = Values.back();
		Values.pop_back();
		Operands.push(Operand);
	}

	bool P()
	{
		EToken Next = Tokenizer.PeekToken();
		if (ParseTerminal())
		{
			PushOperand();
		}
		else if (Next == EToken::LeftParenthesis)
		{
			Tokenizer.Advance();
			PushOperator(AST::FOperator::Sentinel);
			if (!E())
			{
				return false;
			}
			if (Tokenizer.Match(EToken::RightParenthesis))
			{
				assert(!Operators.empty());
				assert(Operators.top() == AST::FOperator::Sentinel);
				Operators.pop();
			}
			else
			{
				assert(0);
				Error();
				return false;
			}
		}
		else if (IsUnaryOperator(Next))
		{
			PushOperator(TokenToUnaryOperator(Next, false));
			Tokenizer.Advance();
			P();
		}
		else
		{
			assert(0);
			Error();
			return false;
		}
		return true;
	}
};

struct FParseRulesPrecedenceClimbing : public FBaseParseRules
{
	FParseRulesPrecedenceClimbing(FTokenizer& InTokenizer)
		: FBaseParseRules(InTokenizer)
	{
	}

	virtual bool ParseExpression() override
	{
		AST::FBase* t = Exp(0);
		if (!t)
		{
			Error();
			return false;
		}

		Values.push_back(t);
		return true;
	}

	AST::FBase* Exp(int p)
	{
		AST::FBase* t = P();
		if (!t)
		{
			return nullptr;
		}

		while (IsBinaryOperator(Tokenizer.PeekToken()) && GetPrecedence(TokenToBinaryOperator(Tokenizer.PeekToken(), false)) >= p)
		{
			AST::FOperator::EType Op = TokenToBinaryOperator(Tokenizer.PeekToken(), true);
			Tokenizer.Advance();
			int q = (AssociativityIsRight(Op) ? 0 : 1) + GetPrecedence(Op);
			AST::FBase* t1 = Exp(q);
			if (!t1)
			{
				return nullptr;
			}
			t = MakeOperator(Op, t, t1);
		}

		if (Tokenizer.Match(EToken::Question))
		{
			AST::FBase* Condition = t;
			AST::FBase* LHS = Exp(0);
			if (!LHS)
			{
				return nullptr;
			}

			if (!Tokenizer.Match(EToken::Colon))
			{
				return nullptr;
			}

			AST::FBase* RHS = Exp(0);
			if (!RHS)
			{
				return nullptr;
			}

			t = MakeOperator(AST::FOperator::Ternary, LHS, RHS);
			(t->AsOperator())->TernaryCondition = Condition;
		}

		return t;
	}

	bool AssociativityIsRight(AST::FOperator::EType Operator)
	{
		switch (Operator)
		{
		case AST::FOperator::UnaryPlus:
		case AST::FOperator::UnaryMinus:
			return true;
		default:
			break;
		}

		return false;
	}

	AST::FOperator* MakeOperator(AST::FOperator::EType Type, AST::FBase* LHS, AST::FBase* RHS = nullptr)
	{
		AST::FOperator* Operator = new AST::FOperator;
		Operator->Type = Type;
		Operator->LHS = LHS;
		Operator->RHS = RHS;
		return Operator;
	}

	AST::FBase* P()
	{
		if (IsUnaryOperator(Tokenizer.PeekToken()))
		{
			AST::FOperator::EType Op = TokenToUnaryOperator(Tokenizer.PeekToken(), true);
			Tokenizer.Advance();
			int q = GetPrecedence(Op);
			AST::FBase* t = Exp(q);
			return MakeOperator(Op, t);
		}
		else if (Tokenizer.Match(EToken::LeftParenthesis))
		{
			AST::FBase* t = Exp(0);
			bool bMatchRP = Tokenizer.Match(EToken::RightParenthesis);
			assert(bMatchRP);
			return t;
		}
		else if (ParseTerminal())
		{
			assert(!Values.empty());
			AST::FBase* Terminal = Values.back();
			Values.pop_back();
			return Terminal;
		}
		
		return nullptr;
	}
};

struct FParseRulesPratt : public FBaseParseRules
{
	FParseRulesPratt(FTokenizer& InTokenizer)
		: FBaseParseRules(InTokenizer)
	{
	}

	virtual bool ParseExpression() override
	{
		AST::FBase* t = E(0);
		if (!t)
		{
			return false;
		}

		Values.push_back(t);
		return true;
	}

	AST::FOperator* MakeOperator(AST::FOperator::EType Type, AST::FBase* LHS, AST::FBase* RHS = nullptr, AST::FBase* Ternary = nullptr)
	{
		AST::FOperator* Operator = new AST::FOperator;
		Operator->Type = Type;
		Operator->TernaryCondition = Ternary;
		Operator->LHS = LHS;
		Operator->RHS = RHS;
		return Operator;
	}

	AST::FBase* Nud(EToken Token)
	{
		if (ParseTerminal(Token, Tokenizer.PreviousTokenString()))
		{
			assert(Values.size() >= 1);
			AST::FBase* Terminal = Values.back();
			Values.pop_back();
			return Terminal;
		}
		else if (IsUnaryOperator(Token))
		{
			AST::FOperator::EType Operator = TokenToUnaryOperator(Token, true);
			AST::FBase* LHS = E(GetPrecedence(Operator));
			return MakeOperator(Operator, LHS);
		}
		else if (Token == EToken::LeftParenthesis)
		{
			AST::FBase* Exp = E(0);
			if (!Tokenizer.Match(EToken::RightParenthesis))
			{
				return nullptr;
			}
			return Exp;
		}
		return nullptr;
	}

	int Lbp(EToken Token)
	{
		if (Token == EToken::Question)
		{
			return 20;
		}
		return GetPrecedence(TokenToBinaryOperator(Token, true));
	}

	AST::FBase* Led(EToken Token, AST::FBase* Left)
	{
		if (Token == EToken::Question)
		{
			AST::FBase* LHS = E(0);
			if (!Tokenizer.Match(EToken::Colon))
			{
				Error();
				return nullptr;
			}
			AST::FBase* RHS = E(0);
			return MakeOperator(AST::FOperator::Ternary, LHS, RHS, Left);
		}
		AST::FBase* Right = E(Lbp(Token));
		return MakeOperator(TokenToBinaryOperator(Token, false), Left, Right);
	}

	AST::FBase* E(int rbp)
	{
		AST::FBase* Left;
		if (Tokenizer.HasMoreTokens())
		{
			EToken Token = Tokenizer.PeekToken();
			Tokenizer.Advance();
			Left = Nud(Token);
			Token = Tokenizer.PeekToken();
			while (IsAnyOperator(Token) && rbp < Lbp(Token))
			{
				Tokenizer.Advance();
				Left = Led(Token, Left);
				Token = Tokenizer.PeekToken();
			}
		}
		return Left;
	}
};

struct FScopeTimer
{
	std::chrono::high_resolution_clock::time_point Begin, End;
	double Delta;
	FScopeTimer()
	{
		Begin = std::chrono::high_resolution_clock::now();
	}

	~FScopeTimer()
	{
		End = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> Diff = End - Begin;
		Delta = Diff.count();
		printf("Timer: %f ms\n", (double)Delta * 1000.0f);
	}
};

template <typename TParser>
void DoParse(std::vector<FToken>& Tokens, const char* ParserName)
{
	printf("%s\n", ParserName);

	FTokenizer Tokenizer(Tokens);
	TParser ParseRules(Tokenizer);
	{
		FScopeTimer Timer;
		while (Tokenizer.HasMoreTokens())
		{
			if (!ParseRules.ParseExpression())
			{
				printf("Failed!\n");
				break;
			}
			if (!Tokenizer.Match(EToken::Semicolon))
			{
				printf("Failed!\n");
				break;
			}
		}
	}

	static bool bWrite = !false;
	if (bWrite)
	{
		for (auto* Value : ParseRules.Values)
		{
			Value->Write();
			printf("\n");
		}
		printf("\n");
	}
}

static void Parse(std::vector<FToken>& Tokens)
{
	DoParse<FParseRulesRecursiveDescent>(Tokens, "Recursive Descent");
	DoParse<FParseRulesShuntingYard2>(Tokens, "Shunting Yard");
	DoParse<FParseRulesPrecedenceClimbing>(Tokens, "Precedence climbing");
	DoParse<FParseRulesPratt>(Tokens, "Pratt");
}

bool Lex(std::string& Data, std::vector<FToken>& OutTokens)
{
	FLexer Lexer(Data);
	bool bError = false;
	while (Lexer.HasMoreTokens())
	{
		FToken Token = Lexer.NextToken();
		if (Token.TokenType == EToken::Error)
		{
			return false;
		}
		OutTokens.push_back(Token);
	}
	return true;
}

int main()
{
	LPSTR CmdLine = ::GetCommandLineA();
	const char* Token = CmdLine;
	while (Token = strchr(Token, ' '))
	{
		++Token;
		//if (!_strnicmp(Token, "-debugger", 9))
		//{
		//}
		Filename = Token;
	}

	std::string Data = Load(Filename.c_str());

	std::vector<FToken> Tokens;
	if (!Lex(Data, Tokens))
	{
		return 1;
	}

	printf("%d Tokens\n", Tokens.size());

	Parse(Tokens);

	//Recursive
	(((3 > 5)) ? ((((1 < 2)) ? ((-1)) : (4))) : (5));
	//Shunting
	(((3 > 5)) ? ((((1 < 2)) ? ((-1)) : (4))) : (5));
	//Precedence
	(3 > ((5) ? ((1 < ((2) ? ((-1)) : (4)))) : (5)));
//	Pratt
	(((3 > 5)) ? ((((1 < 2)) ? ((-1)) : (4))) : (5));

	return 0;
}
