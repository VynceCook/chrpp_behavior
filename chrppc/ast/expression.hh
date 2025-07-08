/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Apache License, Version 2.0.
 *
 *  Copyright:
 *     2022, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include <tao/pegtl.hpp>
#include <parse_error.hh>

namespace chr::compiler::visitor {
	// Forward declaration
	struct ExpressionVisitor;
};

namespace chr::compiler::ast
{
	// Forward declaration
	class ChrConstraintCall;
	struct ChrConstraintDecl;

	/**
	 * @brief Root class of the expression AST
	 *
	 * An expression may be a constant numeric value, a CPP variable,
	 * a logical variable, a CPP or CHR function call or a prefix,
	 * infix, postfix expression.
	 */
	class Expression
	{
	public:
		/**
		 * Initialize expression at given position.
		 * @param pos The position of the element
		 */
		Expression(PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Expression(const Expression &o) =default;

		/**
		 * Return the position of the expression in the input source file.
		 * @return The position of the element
		 */
		PositionInfo position() const;

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		virtual Expression* clone() const = 0;

		/**
		 * Accept ExpressionVisitor
		 */
		virtual void accept(visitor::ExpressionVisitor&) = 0;

		/**
		 * Virtual destructor.
		 */
		virtual ~Expression() =default;

	protected:
		PositionInfo _pos; ///< Position of the element in the input source fille
	};

	/**
	 * Shortcut to unique ptr of Expression
	 */
	using PtrExpression = std::unique_ptr< Expression >;

	/**
	 * @brief An identifier (variable name, function name, ...)
	 */
	class Identifier : public Expression
	{
	public:
		/**
		 * Initialize a leaf with an identifier
		 * @param name The value of the identifier
		 * @param pos The position of the element
		 */
		Identifier(std::string_view value, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Identifier(const Identifier &o) =default;

		/**
		 * Return the identifier value
		 * @return The value
		 */
		std::string& value();

		/**
		 * Return the identifier value
		 * @return The value
		 */
		const std::string& value() const;

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) override;

	protected:
		std::string _value;	///< Value of identifier
	};

	/**
	 * @brief A logical variable (leaf of an expression)
	 *
	 * A logical variable seems like a Prolog variable,
	 * for parsing, it must start with a capital letter.
	 */
	class LogicalVariable : public Identifier
	{
	public:
		/**
		 * Initialize a leaf with a logical variable.
		 * @param name The name of logical variable
		 * @param pos The position of the element
		 */
		LogicalVariable(std::string_view name, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		LogicalVariable(const LogicalVariable &o) =default;

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;
	};

	/**
	 * @brief A CPP variable (leaf of an expression)
	 *
	 * A CPP variable name is considered as a constant from the CHR point
	 * of view.
	 */
	class CppVariable : public Identifier
	{
	public:
		/**
		 * Initialize a leaf with a cpp variable.
		 * @param name The name of the cpp variable
		 * @param pos The position of the element
		 */
		CppVariable(std::string_view name, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		CppVariable(const CppVariable &o) =default;

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;
	};

	/**
	 * @brief A literal (leaf of an expression)
	 *
	 * A literal is a number value or a CPP constant value (string, char, ...)
	 * The only purpose of a constant is to be printed, not computed
	 * As a consequence, a constant will be stored as a string.
	 * by the CHR compiler.
	 */
	class Literal : public Expression
	{
	public:
		/**
		 * Initialize a leaf with a constant.
		 * @param value The value of the constant
		 * @param pos The position of the element
		 */
		Literal(std::string_view value, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Literal(const Literal &o) =default;

		/**
		 * Return the literal value
		 * @return The value
		 */
		std::string& value();

		/**
		 * Return the literal value
		 * @return The value
		 */
		const std::string& value() const;

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;

	protected:
		std::string _value;		///< String representation of literal
	};

	/**
	 * @brief An unary expression
	 *
	 * An unary expression is a classical user friendly
	 * expression (prefix or postfix) composed of usual
	 * operators (!, ^, ++, ...) that are coming from the host
	 * language, here CPP.
	 */
	class UnaryExpression : public Expression
	{
	public:
		/**
		 * Initialize a node with an unary operator.
		 * @param op The name of the operator
		 * @param e The sub-expression
		 * @param pos The position of the element
		 */
		UnaryExpression(std::string_view op, PtrExpression e, PositionInfo pos);

		/**
		 * Return expression operator
		 * @return The operator
		 */
		std::string_view op() const;

		/**
		 * Return the encapsulated expression
		 * @return The sub-expression
		 */
		PtrExpression& child();

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		UnaryExpression(const UnaryExpression &o);

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v);

	protected:
		std::string _op;		///< Name of the operator
		PtrExpression _child;	///< The child
	};

	/**
	 * @brief A Prefix unary expression
	 */
	class PrefixExpression : public UnaryExpression
	{
	public:
		/**
		 * Initialize a node with an unary prefix operator.
		 * @param op The name of the operator
		 * @param e The sub-expression
		 * @param pos The position of the element
		 */
		PrefixExpression(std::string_view op, PtrExpression e, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		PrefixExpression(const PrefixExpression &o);

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;
	};

	/**
	 * @brief A Postfix unary expression
	 */
	class PostfixExpression : public UnaryExpression
	{
	public:
		/**
		 * Initialize a node with an unary prefix operator.
		 * @param op The name of the operator
		 * @param e The sub-expression
		 * @param pos The position of the element
		 */
		PostfixExpression(std::string_view op, PtrExpression e, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		PostfixExpression(const PostfixExpression &o);

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;
	};

	/**
	 * @brief Infix binary expression
	 *
	 * An infix binary expression is a classical user friendly expression
	 * composed of usual operators (+, -, *, /, &&, ||, etc.)
	 * that are coming from the host language, here CPP.
	 */
	class InfixExpression : public Expression
	{
	public:
		/**
		 * Initialize a node with a binary infix expression.
		 * @param op The name of the operator
		 * @param l The left argument of the binary expression
		 * @param r The right argument of the binary expression
		 * @param pos The position of the element
		 */
		InfixExpression(std::string_view op, PtrExpression l, PtrExpression r, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		InfixExpression(const InfixExpression &o);

		/**
		 * Return expression operator
		 * @return The operator
		 */
		std::string_view op() const;

		/**
		 * Return the left encapsulated expression
		 * @return The left sub-expression
		 */
		PtrExpression& l_child();

		/**
		 * Return the right encapsulated expression
		 * @return The right sub-expression
		 */
		PtrExpression& r_child();

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;

	protected:
		std::string _op;		///< Name of the operator
		PtrExpression _l_child;	///< The left child
		PtrExpression _r_child;	///< The right child
	};

	/**
	 * @brief Ternary expression
	 *
	 * A ternary expression is the ?: C++ operator.
	 */
	class TernaryExpression : public Expression
	{
	public:
		/**
		 * Initialize a node with a ternary expression.
		 * @param op1 The name of the operator
		 * @param op2 The name of the second operator
		 * @param e1 The first element of the ternary expression
		 * @param e2 The first element of the ternary expression
		 * @param e3 The first element of the ternary expression
		 * @param pos The position of the element
		 */
		TernaryExpression(std::string_view op1, std::string_view op2, PtrExpression e1, PtrExpression e2, PtrExpression e3, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		TernaryExpression(const TernaryExpression &o);

		/**
		 * Return the first expression operator
		 * @return The first operator
		 */
		std::string_view op1() const;

		/**
		 * Return the second expression operator
		 * @return The second operator
		 */
		std::string_view op2() const;

		/**
		 * Return the first encapsulated expression
		 * @return The first sub-expression
		 */
		PtrExpression& child1();

		/**
		 * Return the second encapsulated expression
		 * @return The second sub-expression
		 */
		PtrExpression& child2();

		/**
		 * Return the third encapsulated expression
		 * @return The third sub-expression
		 */
		PtrExpression& child3();

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;

	private:
		std::string _op1;	///< Name of the first operator
		std::string _op2;	///< Name of the second operator
		PtrExpression _e1;	///< The first element
		PtrExpression _e2;	///< The second element
		PtrExpression _e3;	///< The third element
	};

	/**
	 * @brief A built-in constraint
	 *
	 * A built-in constraint is a function of the host language.
	 * It may be any CPP function call.
	 */
	class BuiltinConstraint : public Expression
	{
	public:
		/**
		 * Initialize a node with a built-in constraint.
		 * @param name The name of the constraint
		 * @param l_delim The left delimiter for parameters
		 * @param r_delim The right delimiter for parameters
		 * @param args The arguments of the constraint
		 * @param pos The position of the element
		 */
		BuiltinConstraint(PtrExpression name, std::string_view l_delim, std::string_view r_delim, std::vector< PtrExpression >& args, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		BuiltinConstraint(const BuiltinConstraint &o);

		/**
		 * Return constraint call name
		 * @return The name
		 */
		const PtrExpression& name() const;

		/**
		 * Return the left delimiter of constraint parameters
		 * @return The left delimiter
		 */
		std::string_view l_delim() const;

		/**
		 * Return the right delimiter of constraint parameters
		 * @return The name
		 */
		std::string_view r_delim() const;

		/**
		 * Return the encapsulated expressions
		 * @return The sub-expressions
		 */
		std::vector< PtrExpression >& children();

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;

	protected:
		PtrExpression _name;						///< Name of the built-in function
		std::string _l_delim;						///< Left delimiter for parameters
		std::string _r_delim;						///< Right delimiter for parameters
		std::vector< PtrExpression > _children;		///< The list of children
	};

	/**
	 * @brief A CHR constraint
	 *
	 * A CHR constraint is a CHR function call. It must start with
	 * an identifier.
	 */
	class ChrConstraint : public Expression
	{
	public:
		/**
		 * Initialize a node with a chr constraint.
		 * @param name The name of the constraint
		 * @param args The arguments of the constraint
		 * @param pos The position of the element
		 */
		ChrConstraint(std::unique_ptr<Identifier> name, std::vector< PtrExpression >& args, PositionInfo pos);

		/**
		 * Initialize a node with a chr constraint.
		 * @param decl The declaration of the constraint
		 * @param args The arguments of the constraint
		 * @param pos The position of the element
		 */
		ChrConstraint(std::shared_ptr< ChrConstraintDecl > decl, std::vector< PtrExpression >& args, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrConstraint(const ChrConstraint &o);

		/**
		 * Return the constraint name.
		 * @return The name
		 */
		const std::unique_ptr< Identifier >& name() const;

		/**
		 * Return the constraint declaration.
		 * @return The declaration
		 */
		const std::shared_ptr< ChrConstraintDecl >& decl() const;

		/**
		 * Return the encapsulated expressions
		 * @return The sub-expressions
		 */
		std::vector< PtrExpression >& children();

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;

	protected:
		std::unique_ptr< Identifier> _name;			///< Name of the built-in function
		std::vector< PtrExpression > _children;		///< The list of children
		std::shared_ptr< ChrConstraintDecl > _decl;	///< Ref to the constraint declaration
	};

	/**
	 * Shortcut to unique ptr of ChrConstraint
	 */
	using PtrChrConstraint = std::unique_ptr< ChrConstraint >;

	/**
	 * @brief A chr_count constraint
	 *
	 * A chr_count constraint is a reserved CHR constraint call to count the number
	 * of CHR constraints of a store.
	 */
	class ChrCount : public Expression
	{
	public:
		/**
		 * Initialize a node with a chr_count
		 * @param use_index The number of the index or -1 if no index has to be used
		 * @param arg The CHR constraint argument for the chr_count
		 * @param pos The position of the element
		 */
		ChrCount(int use_index, PtrChrConstraint arg, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrCount(const ChrCount &o);

		/**
		 * Return the number of index to use (or -1 if no index has to be used).
		 * @return The use_index value
		 */
		int use_index() const;

		/**
		 * Return the encapsulated CHR constraint
		 * @return The sub-expressions
		 */
		PtrChrConstraint& constraint();

		/**
		 * Recursively clone the current expression
		 * @return A new fresh cloned expression
		 */
		Expression* clone() const override;

		/**
		 * Accept ExpressionVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::ExpressionVisitor& v) final;

	protected:
		int _use_index;					///< The number of the index or -1 if no index has to be used
		PtrChrConstraint _constraint;	///< The CHR constraint
	};

} // namespace chr::compiler::ast
