#pragma once

namespace IR
{
	struct FExpression
	{
		virtual ~FExpression() {}
		virtual FOperator* AsOperator() { return nullptr; }
		virtual FConstant* AsConstant() { return nullptr; }
	};

	struct FType
	{
		enum EBaseType
		{
			Integer,
			Float,
		} BaseType;
	};


	struct FConstant : public FExpression
	{
		virtual FConstant* AsConstant() { return this; }
	};

	struct FScalarConstant : public FConstant
	{
	};

	struct FOperator : public FExpression
	{
		FType* Type;

		virtual FOperator* AsOperator() { return this; }
	};

	struct FUnaryOperator : public FOperator
	{
		enum EOperator
		{
			Minus,
		} Op;

		FExpression* LHS;
	};

	struct FBinaryOperator : public FOperator
	{
		enum EOperator
		{
			Add,
			Subtract,
			Multiply,
			Divide,
			Remainder,
		} Op;

		FExpression* LHS;
		FExpression* RHS;
	};
}
