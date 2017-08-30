#pragma once

struct FConstant;
struct FOperator;

struct FBase
{
	~FBase() {}
	virtual FConstant* AsConstant() { return nullptr; }
	virtual FOperator* AsOperator() { return nullptr; }
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

	virtual FConstant* AsConstant() { return this; }
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

	virtual FOperator* AsOperator() { return this; }
};
