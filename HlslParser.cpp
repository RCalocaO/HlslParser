// HlslParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "HlslAST.h"
#include "HlslParser.h"

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

struct FBaseParseRules
{
	std::list<FBase*> Values;
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

	bool ParseTerminal()
	{
		if (Tokenizer.Match(EToken::Integer))
		{
			auto* Node = new FConstant;
			Node->Type = FConstant::Integer;
			Node->UIntValue = atoi(Tokenizer.PreviousTokenString().c_str());
			PushTerminal(Node);
			return true;
		}
		else if (Tokenizer.Match(EToken::HexInteger))
		{
			auto* Node = new FConstant;
			Node->Type = FConstant::HexInteger;
			Node->UIntValue = (uint32_t)atoll(Tokenizer.PreviousTokenString().c_str());
			PushTerminal(Node);
			return true;
		}
		else if (Tokenizer.Match(EToken::Float))
		{
			auto* Node = new FConstant;
			Node->Type = FConstant::Float;
			Node->FloatValue = (float)atof(Tokenizer.PreviousTokenString().c_str());
			PushTerminal(Node);
			return true;
		}

		return false;
	}

	virtual void PushTerminal(FBase* Node)
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
		FOperator::EType UnaryOp = FOperator::Sentinel;
		if (Tokenizer.PeekToken() == EToken::Minus || Tokenizer.PeekToken() == EToken::Plus)
		{
			UnaryOp = TokenToUnaryOperator(Tokenizer.PeekToken(), false);
			Tokenizer.Advance();
		}

		if (!ParseExpressionPrimary())
		{
			return Error();
		}

		if (UnaryOp != FOperator::Sentinel)
		{
			auto* Node = new FOperator;
			Node->Type = UnaryOp;
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

		while (Tokenizer.PeekToken() == EToken::Multiply || Tokenizer.PeekToken() == EToken::Divide || Tokenizer.PeekToken() == EToken::Mod)
		{
			Tokenizer.Advance();
			auto* Node = new FOperator;
			Node->Type = TokenToBinaryOperator(Tokenizer.PreviousTokenType(), true);
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
			Node->Type = TokenToBinaryOperator(Tokenizer.PreviousTokenType(), true);
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

	bool ParseConditionalExpression()
	{
		if (!ParseExpressionAdditive())
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

			assert(Values.size() >= 3);
			FOperator* Operator = new FOperator;
			Operator->Type = FOperator::Ternary;
			Operator->RHS = Values.back();
			Values.pop_back();
			Operator->LHS = Values.back();
			Values.pop_back();
			Operator->TernaryCondition = Values.back();
			Values.pop_back();
			Values.push_back(Operator);
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

	std::stack<FBase*> Operands;
	std::stack<FOperator::EType> Operators;

	void PopOperator()
	{
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
		else
		{
			assert(0);
		}
	}

	virtual bool ParseExpression() override
	{
		PushOperator(FOperator::Sentinel);
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

	int GetPrecedence(FOperator::EType Operator)
	{
		switch (Operator)
		{
		case FOperator::Add:
		case FOperator::Subtract:
			return 60;
		case FOperator::Mul:
		case FOperator::Divide:
		case FOperator::Remainder:
			return 70;
		case FOperator::UnaryPlus:
		case FOperator::UnaryMinus:
			return 100;
		default:
			assert(0);
			break;
		}

		return 0;
	}

	void PushOperator(FOperator::EType Operator)
	{
		if (Operator != FOperator::Sentinel)
		{
			while (!Operators.empty() && Operators.top() != FOperator::Sentinel && GetPrecedence(Operators.top()) > GetPrecedence(Operator))
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
				PushOperator(FOperator::Sentinel);
				if (!E())
				{
					return false;
				}

				assert(!Operators.empty());
				assert(Operators.top() == FOperator::Sentinel);

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
				assert(Operators.top() == FOperator::Sentinel);
				Operators.pop();

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
				break;
			}
		}


		while (!Operators.empty() && Operators.top() != FOperator::Sentinel)
		{
			PopOperator();
		}
		return true;
	}

	void PushOperand()
	{
		assert(!Values.empty());
		FBase* Operand = Values.back();
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
			PushOperator(FOperator::Sentinel);
			if (!E())
			{
				return false;
			}
			if (Tokenizer.Match(EToken::RightParenthesis))
			{
				assert(!Operators.empty());
				assert(Operators.top() == FOperator::Sentinel);
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

struct FParseRulesClassicRecursiveDescent : public FBaseParseRules
{
	FParseRulesClassicRecursiveDescent(FTokenizer& InTokenizer)
		: FBaseParseRules(InTokenizer)
	{
	}

	void MakeUnaryOp(EToken Token)
	{
		FOperator* Operator = new FOperator;
		Operator->Type = TokenToUnaryOperator(Token, false);
		assert(Values.size() >= 1);
		Operator->LHS = Values.back();
		Values.pop_back();
		Values.push_back(Operator);
	}

	void MakeBinaryOp(EToken Token)
	{
		FOperator* Operator = new FOperator;
		Operator->Type = TokenToBinaryOperator(Token, false);
		assert(Values.size() >= 2);
		Operator->RHS = Values.back();
		Values.pop_back();
		Operator->LHS = Values.back();
		Values.pop_back();
		Values.push_back(Operator);
	}

	bool E()
	{
		if (!T())
		{
			Error();
			return false;
		}
		while (Tokenizer.PeekToken() == EToken::Plus || Tokenizer.PeekToken() == EToken::Minus)
		{
			EToken Token = Tokenizer.PeekToken();
			Tokenizer.Advance();
			if (!T())
			{
				Error();
				return false;
			}
			MakeBinaryOp(Token);
		}
		return true;
	}

	bool T()
	{
		if (!F())
		{
			Error();
			return false;
		}
		while (Tokenizer.PeekToken() == EToken::Multiply || Tokenizer.PeekToken() == EToken::Divide || Tokenizer.PeekToken() == EToken::Mod)
		{
			EToken Token = Tokenizer.PeekToken();
			Tokenizer.Advance();
			if (!F())
			{
				Error();
				return false;
			}
			MakeBinaryOp(Token);
		}
		return true;
	}

	bool F()
	{
		if (!P())
		{
			Error();
			return false;
		}
		// ^
		return true;
	}

	bool P()
	{
		if (ParseTerminal())
		{
		}
		else if (Tokenizer.PeekToken() == EToken::LeftParenthesis)
		{
			Tokenizer.Advance();
			if (!C())
			{
				Error();
				return false;
			}
			if (!Tokenizer.Match(EToken::RightParenthesis))
			{
				assert(0);
				Error();
				return false;
			}
		}
		else if (Tokenizer.PeekToken() == EToken::Plus || Tokenizer.PeekToken() == EToken::Minus)
		{
			EToken Token = Tokenizer.PeekToken();
			Tokenizer.Advance();
			if (!F())
			{
				Error();
				return false;
			}
			MakeUnaryOp(Token);
		}
		else
		{
			assert(0);
			Error();
			return false;
		}
		return true;
	}

	bool C()
	{
		if (!E())
		{
			return false;
		}

		if (Tokenizer.Match(EToken::Question))
		{
			if (!E())
			{
				return false;
			}
			if (!Tokenizer.Match(EToken::Colon))
			{
				Error();
				return false;
			}
			if (!E())
			{
				return false;
			}

			FOperator* Operator = new FOperator;
			Operator->Type = FOperator::Ternary;
			assert(Values.size() >= 3);
			Operator->RHS = Values.back();
			Values.pop_back();
			Operator->LHS = Values.back();
			Values.pop_back();
			Operator->TernaryCondition = Values.back();
			Values.pop_back();
			Values.push_back(Operator);
		}

		return true;
	}

	virtual bool ParseExpression() override
	{
		return C();
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
		FBase* t = Exp(0);
		if (!t)
		{
			Error();
			return false;
		}

		Values.push_back(t);
		return true;
	}

	int Prec(FOperator::EType Operator)
	{
		switch (Operator)
		{
		case FOperator::Add:
		case FOperator::Subtract:
			return 60;
		case FOperator::Mul:
			return 70;
		case FOperator::UnaryPlus:
		case FOperator::UnaryMinus:
			return 100;
		default:
			assert(0);
			break;
		}

		return 0;
	}

	FBase* Exp(int p)
	{
		FBase* t = P();
		if (!t)
		{
			return nullptr;
		}

		while (IsBinaryOperator(Tokenizer.PeekToken()) && Prec(TokenToBinaryOperator(Tokenizer.PeekToken(), false)) >= p)
		{
			FOperator::EType Op = TokenToBinaryOperator(Tokenizer.PeekToken(), true);
			Tokenizer.Advance();
			int q = (AssociativityIsRight(Op) ? 0 : 1) + Prec(Op);
			FBase* t1 = Exp(q);
			if (!t1)
			{
				return nullptr;
			}
			t = MakeOperator(Op, t, t1);
		}

		return t;
	}

	bool AssociativityIsRight(FOperator::EType Operator)
	{
		switch (Operator)
		{
		case FOperator::UnaryPlus:
		case FOperator::UnaryMinus:
			return true;
		default:
			break;
		}

		return false;
	}

	FOperator* MakeOperator(FOperator::EType Type, FBase* LHS, FBase* RHS = nullptr)
	{
		FOperator* Operator = new FOperator;
		Operator->Type = Type;
		Operator->LHS = LHS;
		Operator->RHS = RHS;
		return Operator;
	}

	FBase* P()
	{
		if (IsUnaryOperator(Tokenizer.PeekToken()))
		{
			FOperator::EType Op = TokenToUnaryOperator(Tokenizer.PeekToken(), true);
			Tokenizer.Advance();
			int q = Prec(Op);
			FBase* t = Exp(q);
			return MakeOperator(Op, t);
		}
		else if (Tokenizer.Match(EToken::LeftParenthesis))
		{
			FBase* t = Exp(0);
			bool bMatchRP = Tokenizer.Match(EToken::RightParenthesis);
			assert(bMatchRP);
			return t;
		}
		else if (ParseTerminal())
		{
			assert(!Values.empty());
			FBase* Terminal = Values.back();
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
/*
		FBase* t = Exp(0);
		if (!t)
		{
			return false;
		}

		Values.push_back(t);
*/
		return true;
	}

	FOperator* MakeOperator(FOperator::EType Type, FBase* LHS, FBase* RHS = nullptr)
	{
		FOperator* Operator = new FOperator;
		Operator->Type = Type;
		Operator->LHS = LHS;
		Operator->RHS = RHS;
		return Operator;
	}
};

template <typename TParser>
void DoParse(std::vector<FToken>& Tokens, const char* ParserName)
{
	printf("%s\n", ParserName);

	FTokenizer Tokenizer(Tokens);
	TParser ParseRules(Tokenizer);
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
	for (auto* Value : ParseRules.Values)
	{
		Value->Write();
		printf("\n");
	}
	printf("\n");
}

static void Parse(std::vector<FToken>& Tokens)
{
	DoParse<FParseRulesRecursiveDescent>(Tokens, "Recursive Descent");
	DoParse<FParseRulesShuntingYard2>(Tokens, "Shunting Yard");
	DoParse<FParseRulesClassicRecursiveDescent>(Tokens, "Classical Recursive Descent");
	DoParse<FParseRulesPrecedenceClimbing>(Tokens, "Precedence climbing");
	//DoParse<FParseRulesPratt>(Tokens, "Pratt");
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
			break;
		}
		OutTokens.push_back(Token);
	}
	return true;
}

int main()
{
	std::string Data = Load(Filename.c_str());

	std::vector<FToken> Tokens;
	if (!Lex(Data, Tokens))
	{
		return 1;
	}

	Parse(Tokens);

	return 0;
}
