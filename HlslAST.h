#pragma once

struct FConstant;
struct FOperator;

struct FBase
{
	~FBase() {}
	virtual FConstant* AsConstant() { return nullptr; }
	virtual FOperator* AsOperator() { return nullptr; }
	virtual void Write() = 0;
};

struct FConstant : public FBase
{
	enum EType
	{
		Integer,
		HexInteger,
		Float,
	} Type;

	union
	{
		uint32_t UIntValue;
		float FloatValue;
	};

	virtual FConstant* AsConstant() override { return this; }
	virtual void Write() override
	{
		switch (Type)
		{
		case Integer:
			printf("%u", UIntValue);
			break;
		case HexInteger:
			printf("0x%x", UIntValue);
			break;
		case Float:
			printf("%f", FloatValue);
			break;
		default:
			assert(0);
			break;
		}
	}
};


struct FOperator : public FBase
{
	enum EType
	{
		Error = -1,
		Sentinel,
		Not,
		Neg,
		UnaryMinus,
		UnaryPlus,
		Add,
		Subtract,
		Mul,
		Divide,
		Remainder,
		LogicalOr,
		LogicalAnd,
		BitOr,
		Xor,
		BitAnd,
		ShitLeft,
		ShitRight,
		Equals,
		NotEquals,
		Greater,
		GreaterEqual,
		Lower,
		LowerEqual,

		Ternary,

		NumTypes,
		TernaryEnd = NumTypes,	// Not a real operator, only used for parsing
	} Type;

	static bool IsUnary(EType Type)
	{
		switch (Type)
		{
		case Neg:
		case Not:
		case UnaryPlus:
		case UnaryMinus:
			return true;
		default:
			break;
		}
		return false;
	}

	static bool IsBinary(EType Type)
	{
		switch (Type)
		{
		case Add:
		case Subtract:
		case Mul:
		case Divide:
		case Remainder:
		case BitAnd:
		case LogicalAnd:
		case BitOr:
		case LogicalOr:
		case Xor:
		case Equals:
		case NotEquals:
		case Greater:
		case GreaterEqual:
		case Lower:
		case LowerEqual:
			return true;
		default:
			break;
		}
		return false;
	}

	FBase* LHS = nullptr;
	FBase* RHS = nullptr;
	FBase* TernaryCondition = nullptr;

	virtual FOperator* AsOperator() override { return this; }
	virtual void Write() override
	{
		printf("(");
		switch (Type)
		{
		case LogicalOr:
			LHS->Write();
			printf(" || ");
			RHS->Write();
			break;
		case BitOr:
			LHS->Write();
			printf(" | ");
			RHS->Write();
			break;
		case LogicalAnd:
			LHS->Write();
			printf(" && ");
			RHS->Write();
			break;
		case BitAnd:
			LHS->Write();
			printf(" & ");
			RHS->Write();
			break;
		case Xor:
			LHS->Write();
			printf(" ^ ");
			RHS->Write();
			break;
		case Add:
			LHS->Write();
			printf(" + ");
			RHS->Write();
			break;
		case Subtract:
			LHS->Write();
			printf(" - ");
			RHS->Write();
			break;
		case Greater:
			LHS->Write();
			printf(" > ");
			RHS->Write();
			break;
		case GreaterEqual:
			LHS->Write();
			printf(" >= ");
			RHS->Write();
			break;
		case Lower:
			LHS->Write();
			printf(" < ");
			RHS->Write();
			break;
		case LowerEqual:
			LHS->Write();
			printf(" <= ");
			RHS->Write();
			break;
		case Equals:
			LHS->Write();
			printf(" == ");
			RHS->Write();
			break;
		case NotEquals:
			LHS->Write();
			printf(" != ");
			RHS->Write();
			break;
		case Mul:
			LHS->Write();
			printf(" * ");
			RHS->Write();
			break;
		case Divide:
			LHS->Write();
			printf(" / ");
			RHS->Write();
			break;
		case Remainder:
			LHS->Write();
			printf(" %% ");
			RHS->Write();
			break;
		case Neg:
			printf(" ~");
			LHS->Write();
			break;
		case Not:
			printf(" !");
			LHS->Write();
			break;
		case UnaryPlus:
			printf(" +");
			LHS->Write();
			break;
		case UnaryMinus:
			printf(" -");
			LHS->Write();
			break;
		case Ternary:
			printf("(");
			TernaryCondition->Write();
			printf(") ? (");
			LHS->Write();
			printf(") : (");
			RHS->Write();
			printf(")");
			break;
		default:
			assert(0);
			break;
		}
		printf(")");
	}
};
