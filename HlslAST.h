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
		Float,
	} Type;

	union
	{
		uint32_t IntValue;
		float FloatValue;
	};

	virtual FConstant* AsConstant() override { return this; }
	virtual void Write() override
	{
		switch (Type)
		{
		case Integer:
			printf("%d ", IntValue);
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
		Add,
		Subtract,
		Mul,
		Divide,
		Remainder,
		UnaryMinus,
		UnaryPlus,

		NumTypes,
		Sentinel = NumTypes,
	} Type;

	static bool IsUnary(EType Type)
	{
		switch (Type)
		{
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
			return true;
		default:
			break;
		}
		return false;
	}

	FBase* LHS;
	FBase* RHS;

	virtual FOperator* AsOperator() override { return this; }
	virtual void Write() override
	{
		printf(" (");
		switch (Type)
		{
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
		case UnaryPlus:
			printf(" +");
			LHS->Write();
			break;
		case UnaryMinus:
			printf(" -");
			LHS->Write();
			break;
		default:
			assert(0);
			break;
		}
		printf(") ");
	}
};
