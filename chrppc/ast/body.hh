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

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include <tao/pegtl.hpp>
#include <common.hh>
#include <parse_error.hh>
#include <ast/expression.hh>

namespace chr::compiler::visitor {
	struct BodyVisitor;
};

namespace chr::compiler::ast
{
	/**
	 * @brief Root class of a body AST
	 *
	 * A body consists of a single or sequence of chr
	 * statements.
	 * Each statement is separated from the others by a separator
	 * (, or ;) or any CHR reserved built-in constraint.
	 */
	class Body
	{
	public:
		/**
		 * Initialize body element at given position.
		 * @param pos The position of the element
		 */
		Body(PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Body(const Body &o) =default;

		/**
		 * Return the position of the body element in the input source file.
		 * @return The position of the element
		 */
		PositionInfo position() const;

		/**
		 * Recursively clone the current body.
		 * @return A new fresh cloned body
		 */
		virtual Body* clone() const;

		/**
		 * Accept BodyVisitor.
		 */
		virtual void accept(visitor::BodyVisitor&);

		/**
		 * Virtual destructor.
		 */
		virtual ~Body() =default;

	protected:
		PositionInfo _pos; ///< Position of the element in the input source fille
	};

	/**
	 * Shortcut to unique ptr of Body
	 */
	using PtrBody = std::unique_ptr< Body >;

	/**
	 * @brief A CHR keyword
	 *
	 * A CHR keyword is one of the following keyword :
	 *		success, failure, stop, ...
	 */
	class ChrKeyword : public Body
	{
	public:
		/**
		 * Initialize a node with a chr keyword
		 * @param name The keyword name
		 * @param pos The position of the element
		 */
		ChrKeyword(std::string_view name, PositionInfo pos);

		/**
		 * Return the name of the keyword
		 * @return The name
		 */
		std::string_view name() const;

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrKeyword(const ChrKeyword &o);

		/**
		 * Recursively clone the current chr keyword
		 * @return A new fresh cloned of this
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) override;

	protected:
		std::string _name; ///< The name of the keyword
	};

	/**
	 * @brief A CPP expression
	 *
	 * A CPP expression is considered as a builtin constraint.
	 * It may be any cpp function call (or expression) followed by a list of pragmas.
	 * The list of pragmas is optional.
	 */
	class CppExpression : public Body
	{
	public:
		/**
		 * Initialize a node with a cpp expression.
		 * @param e The expression
		 * @param pragmas The list of pragmas
		 * @param pos The position of the element
		 */
		CppExpression(PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos);

		/**
		 * Initialize a node with a cpp expression (with no pragma).
		 * @param e The expression
		 * @param pos The position of the element
		 */
		CppExpression(PtrExpression e, PositionInfo pos);

		/**
		 * Return the encapsulated expression
		 * @return The expression
		 */
		PtrExpression& expression();

		/**
		 * Return the list of pragmas
		 * @return The pragmas
		 */
		const std::vector< Pragma >& pragmas() const;

		/**
		 * Add a new pragma to the list of pragmas
		 */
		void add_pragma(Pragma p);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		CppExpression(const CppExpression &o);

		/**
		 * Recursively clone the current cpp expression
		 * @return A new fresh cloned of this
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) override;

	protected:
		PtrExpression _e;				///< The expression
		std::vector< Pragma > _pragmas; ///< The pragmas
	};

	/**
	 * Shortcut to unique ptr of CppExpression
	 */
	using PtrCppExpression = std::unique_ptr< CppExpression >;

	/**
	 * @brief A CHR expression
	 *
	 * A CHR expression can be either a CHR constraint call or a call to
	 * a reserved constraint (such as unification, local variable declaration, ...).
	 */
	class ChrExpression : public CppExpression
	{
	public:
		/**
		 * Initialize a node with a CHR expression
		 * @param e The expression
		 * @param pragmas The list of pragmas
		 * @param pos The position of the element
		 */
		ChrExpression(PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos);

		/**
		 * Initialize a node with a CHR expression (with no pragma).
		 * @param e The expression
		 * @param pos The position of the element
		 */
		ChrExpression(PtrExpression e, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrExpression(const ChrExpression &o);

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) override;
	};

	/**
	 * @brief A CPP declaration with assignment
	 *
	 * A CPP declaration with assignment introduces a new CPP variable.
	 */
	class CppDeclAssignment : public ChrExpression
	{
	public:
		/**
		 * Initialize a node with a cpp declaration with assignment.
		 * @param v The CPP variable 
		 * @param e The expression
		 * @param pragmas The list of pragmas
		 * @param pos The position of the element
		 */
		CppDeclAssignment(std::unique_ptr<CppVariable> v, PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos);

		/**
		 * Initialize a node with a cpp variable declaration and
		 * assignment (with no pragma).
		 * @param v The CPP variable 
		 * @param e The expression
		 * @param pos The position of the element
		 */
		CppDeclAssignment(std::unique_ptr<CppVariable> v, PtrExpression e, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		CppDeclAssignment(const CppDeclAssignment &o);

		/**
		 * Return the encapsulated CPP variable
		 * @return The variable
		 */
		std::unique_ptr<CppVariable>& variable();

		/**
		 * Recursively clone the current CPP declaration with assignment.
		 * @return A new fresh cloned of this
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) override;

	protected:
		std::unique_ptr<CppVariable> _v;	///< The CPP variable
	};

	/**
	 * @brief A CPP declaration with assignment
	 *
	 * A CPP declaration with assignment introduces a new CPP variable.
	 */
	class Unification : public ChrExpression
	{
	public:
		/**
		 * Initialize a node with a logical variable unification.
		 * @param v The logical variable 
		 * @param e The expression
		 * @param pragmas The list of pragmas
		 * @param pos The position of the element
		 */
		Unification(std::unique_ptr<LogicalVariable> v, PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos);

		/**
		 * Initialize a node with a logical variable unification (with no pragma).
		 * @param v The logical variable 
		 * @param e The expression
		 * @param pos The position of the element
		 */
		Unification(std::unique_ptr<LogicalVariable> v, PtrExpression e, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Unification(const Unification &o);

		/**
		 * Return the encapsulated logical variable
		 * @return The variable
		 */
		std::unique_ptr<LogicalVariable>& variable();

		/**
		 * Recursively clone the current CPP declaration with assignment.
		 * @return A new fresh cloned of this
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) override;

	protected:
		std::unique_ptr<LogicalVariable> _v;	///< The logic variable
	};

	/**
	 * @brief A CHR constraint call
	 *
	 * A CHR constraint call
	 */
	class ChrConstraintCall : public ChrExpression
	{
	public:
		/**
		 * Initialize a node with a CHR constraint call.
		 * @param c The CHR constraint
		 * @param pragmas The list of pragmas
		 * @param pos The position of the element
		 */
		ChrConstraintCall(PtrChrConstraint c, std::vector< Pragma > pragmas, PositionInfo pos);

		/**
		 * Initialize a node with a CHR constraint call (with no pragma)
		 * @param c The CHR constraint
		 * @param pos The position of the element
		 */
		ChrConstraintCall(PtrChrConstraint c, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrConstraintCall(const ChrConstraintCall &o);

		/**
		 * Return the encapsulated expression casted to a  ChrConstraint
		 * @return The expression
		 */
		PtrChrConstraint& constraint();

		/**
		 * Recursively clone the current CHR constraint.
		 * @return A new fresh cloned of this
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) override;
	protected:
		PtrChrConstraint _c;	///< The CHR constraint
	};

	/**
	 * Shortcut to unique ptr of ChrConstraintCall
	 */
	using PtrChrConstraintCall = std::unique_ptr< ChrConstraintCall >;

	/**
	 * Shortcut to shared ptr of ChrConstraintCall
	 */
	using PtrSharedChrConstraintCall = std::shared_ptr< ChrConstraintCall >;

	/**
	 * @brief A sequence of CHR statements
	 *
	 * A sequence of CHR statement is a sequence of statement
	 * separated from others by (, or ;).
	 */
	class Sequence : public Body
	{
	public:
		/**
		 * Initialize a node with a sequence of chr statements.
		 * @param op The name of separator
		 * @param args The elements of the sequence
		 * @param pos The position of the element
		 */
		Sequence(std::string_view op, std::vector< PtrBody >& args, PositionInfo pos);

		/**
		 * Initialize a node with an empty sequence of chr statements.
		 * @param op The name of separator
		 * @param pos The position of the element
		 */
		Sequence(std::string_view op, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Sequence(const Sequence &o);

		/**
		 * Return constraint call name
		 * @return The name
		 */
		std::string_view op() const;

		/**
		 * Return the encapsulated body elements
		 * @return The elements
		 */
		std::vector< PtrBody >& children();

		/**
		 * Add a new child to the list of children
		 */
		void add_child(PtrBody b);

		/**
		 * Recursively clone the current sequence
		 * @return A new fresh cloned sequence
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) final;

	protected:
		std::string _op;					///< Name of the built-in function
		std::vector< PtrBody > _children;	///< The list of children
	};

	/**
	 * Shortcut to unique ptr of Sequence
	 */
	using PtrSequence = std::unique_ptr< Sequence >;

	/**
	 * @brief A CHR behavior constraint
	 *
	 * A CHR behavior constraint is a reserved CHR constraint name
	 */
	class ChrBehavior : public Body
	{
	public:
		/**
		 * Initialize a node with a behavior.
		 * @param _stop_cond The stop condition expression
		 * @param on_succeeded_alt The statements to run on a succeeded alternative
		 * @param on_failed_alt	The statements to run on a failed alternative
		 * @param final_status The final status expression
		 * @param on_succeeded_status The statements to run on a succeeded behavior
		 * @param on_failed_status The statements to run on a failed behavior
		 * @param behavior_body The statements to run on each alternative
		 * @param pos The position of the element
		 */
		ChrBehavior(PtrExpression stop_cond, PtrBody on_succeeded_alt, PtrBody on_failed_alt, PtrExpression final_status, PtrBody _on_succeeded_status, PtrBody _on_failed_status, PtrBody behavior_body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrBehavior(const ChrBehavior &o);

		/**
		 * Return the stop condition expression
		 * @return The stop condition
		 */
		PtrExpression& stop_cond();

		/**
		 * Return the final status condition expression
		 * @return The final_status condition
		 */
		PtrExpression& final_status();

		/**
		 * Return the statements to run on a succeeded alternative
		 * @return The statements
		 */
		PtrBody& on_succeeded_alt();

		/**
		 * Return the statements to run on a failed alternative
		 * @return The statements
		 */
		PtrBody& on_failed_alt();

		/**
		 * Return the statements to run on a succeeded behavior
		 * @return The statements
		 */
		PtrBody& on_succeeded_status();

		/**
		 * Return the statements to run on a failed behavior
		 * @return The statements
		 */
		PtrBody& on_failed_status();

		/**
		 * Return the behavior body
		 * @return The body
		 */
		PtrBody& behavior_body();

		/**
		 * Recursively clone the current behavior
		 * @return A new fresh cloned behavior
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) final;

	protected:
		PtrExpression _stop_cond;		///< The stop condition expression
		PtrExpression _final_status;	///< The final status expression
		PtrBody _on_succeeded_alt;		///< The statements to run on a succeeded alternative
		PtrBody _on_failed_alt;			///< The statements to run on a failed alternative
		PtrBody _on_succeeded_status;	///< The statements to run on a succeeded behavior
		PtrBody _on_failed_status;		///< The statements to run on a failed behavior
		PtrBody _behavior_body;			///< The statements to run on each alternative
	};

	/**
	 * @brief A CHR try constraint
	 *
	 * A CHR behavior constraint is a reserved CHR constraint name which
	 * convert a global failure to a local failure.
	 */
	class ChrTry : public Body
	{
	public:
		/**
		 * Initialize a node with a CHR try.
		 * @param backtrack True if ChrTry must backtrack even when succeeded, false otherwise
		 * @param variable The result variable
		 * @param body The body to try
		 * @param pos The position of the element
		 */
		ChrTry(bool backtrack, std::unique_ptr<CppVariable> variable, PtrBody body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		ChrTry(const ChrTry &o);

		/**
		 * Return if the ChrTry must backtrack, even when a success is encountered.
		 * @return True if ChrTry must backtrack even when succeeded, false otherwise
		 */
		bool do_backtrack() const;

		/**
		 * Return the result CPP variable which contain the result of the try.
		 * @return The result
		 */
		std::unique_ptr<CppVariable>& variable();

		/**
		 * Return the body which will be try.
		 * @return The body of the try
		 */
		PtrBody& body();

		/**
		 * Recursively clone the current try
		 * @return A new fresh cloned try
		 */
		Body* clone() const override;

		/**
		 * Accept BodyVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::BodyVisitor& v) final;

	protected:
		bool _backtrack;						///< True if ChrTry must backtrack even when succeeded, false otherwise
		std::unique_ptr<CppVariable> _variable;	///< The result variable which will contain the result of the try
		PtrBody _body;							///< The statements to try
	};

} // namespace chr::compiler::ast
