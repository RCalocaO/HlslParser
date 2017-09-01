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
		Plus,
		Subtract,
		Mul,
		UnaryMinus,
	} Type;

	FBase* LHS;
	FBase* RHS;

	virtual FOperator* AsOperator() override { return this; }
	virtual void Write() override
	{
		switch (Type)
		{
		case Plus:
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
		case UnaryMinus:
			printf(" -");
			LHS->Write();
			break;
		default:
			assert(0);
			break;
		}
	}
};
