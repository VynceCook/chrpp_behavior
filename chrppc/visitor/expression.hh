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
#include <cassert>
#include <functional>

#include <ast/expression.hh>

namespace chr::compiler::visitor
{
	/**
	 * @brief Root class of the expression visitor
	 *
	 * An expression visitor visits node of an expression tree.
	 */
	struct ExpressionVisitor {
		/**
		 * Root visit function for Expression
		 */
		virtual void visit(ast::Expression&) = 0;
		virtual void visit(ast::Identifier&) = 0;
		virtual void visit(ast::CppVariable&) = 0;
		virtual void visit(ast::LogicalVariable&) = 0;
		virtual void visit(ast::Literal&) = 0;
		virtual void visit(ast::UnaryExpression&) = 0;
		virtual void visit(ast::PrefixExpression&) = 0;
		virtual void visit(ast::PostfixExpression&) = 0;
		virtual void visit(ast::InfixExpression&) = 0;
		virtual void visit(ast::TernaryExpression&) = 0;
		virtual void visit(ast::BuiltinConstraint&) = 0;
		virtual void visit(ast::ChrConstraint&) = 0;
		virtual void visit(ast::ChrCount&) = 0;

		/**
		 * Default virtual destructor
		 */
		virtual ~ExpressionVisitor() = default;
	};

	/**
	 * @brief Expression visitor which build a string representation of an expression tree
	 */
	struct ExpressionPrint : ExpressionVisitor {
		/**
		 * Build a string representation of the expression \a e.
		 * @param e The expression
		 */
		std::string_view string_from(ast::Expression&);

	private:
		void visit(ast::Expression&) final { assert(false); }
		void visit(ast::Identifier& e) final;
		void visit(ast::CppVariable& e) final;
		void visit(ast::LogicalVariable& e) final;
		void visit(ast::Literal& e) final;
		void visit(ast::UnaryExpression&) final { assert(false); }
		void visit(ast::PrefixExpression& e) final;
		void visit(ast::PostfixExpression& e) final;
		void visit(ast::InfixExpression& e) final;
		void visit(ast::TernaryExpression& e) final;
		void visit(ast::BuiltinConstraint& e) final;
		void visit(ast::ChrConstraint& e) final;
		void visit(ast::ChrCount&) final;

	private:
		std::vector< std::string > string_stack;	///< The result string which contains the convertion of an expression tree to a string
	};

	/**
	 * @brief Expression visitor which build a string representation of an expression tree but
	 * omit parenthesis in infix expression
	 */
	struct ExpressionPrintNoParent : ExpressionVisitor {
		/**
		 * Build a string representation of the expression \a e.
		 * @param e The expression
		 */
		std::string_view string_from(ast::Expression&);

	private:
		void visit(ast::Expression&) final { assert(false); }
		void visit(ast::Identifier& e) final;
		void visit(ast::CppVariable& e) final;
		void visit(ast::LogicalVariable& e) final;
		void visit(ast::Literal& e) final;
		void visit(ast::UnaryExpression&) final { assert(false); }
		void visit(ast::PrefixExpression& e) final;
		void visit(ast::PostfixExpression& e) final;
		void visit(ast::InfixExpression& e) final;
		void visit(ast::TernaryExpression& e) final;
		void visit(ast::BuiltinConstraint& e) final;
		void visit(ast::ChrConstraint& e) final;
		void visit(ast::ChrCount&) final;

	private:
		std::vector< std::string > string_stack;	///< The result string which contains the convertion of an expression tree to a string
	};

	/**
	 * @brief Expression visitor which check a lambda function
	 *
	 * The visitor check all nodes of the tree and stop as soon as
	 * the boolean function answers true. The result is stored in
	 * a local member in order to be checked later.
	 */
	struct ExpressionFullCheck : ExpressionVisitor {
		/**
		 * Build a string representation of the expression \a e.
		 * @param e The expression
		 * @param f The fonction to check
		 * @return True if the \a f function has answered true, false otherwise
		 */
		bool check(ast::Expression&, std::function< bool (ast::Expression&) > f);

		/**
		 * Returns the first element to have validated the check function. The function
		 * assumes that the previous check() call answered true. Otherwise, the behavior
		 * is undefined.
		 * @return The result of the last check call
		 */
		ast::Expression& res();

	protected:
		void visit(ast::Expression&) final { assert(false); }
		void visit(ast::Identifier& e) final;
		void visit(ast::CppVariable& e) final;
		void visit(ast::LogicalVariable& e) final;
		void visit(ast::Literal& e) final;
		void visit(ast::UnaryExpression&) final { assert(false); }
		void visit(ast::PrefixExpression& e) final;
		void visit(ast::PostfixExpression& e) final;
		void visit(ast::InfixExpression& e) final;
		void visit(ast::TernaryExpression& e) final;
		void visit(ast::BuiltinConstraint& e) final;
		void visit(ast::ChrConstraint& e) final;
		void visit(ast::ChrCount&);

		std::function< bool (ast::Expression&) > _f;	///< The lambda to check
		ast::Expression* _res = nullptr;				///< The result of the check
	};

	/**
	 * @brief Expression visitor which check a lambda function
	 *
	 * The visitor check all nodes but the chr_count ones of the tree
	 * and stop as soon as the boolean function answers true.
	 * Usefull for some 'check' functions which must go deep on a chr_count node.
	 */
	struct ExpressionLightCheck : ExpressionFullCheck {
		void visit(ast::ChrCount&) final;
	};

	/**
	 * @brief Expression visitor which apply a lambda function
	 *
	 * The visitor visits nodes of the tree and apply the lambda
	 * function on each node. The recursion of a branch is stopped
	 * if the lambda function f return false. 
	 */
	struct ExpressionApply : ExpressionVisitor {
		/**
		 * Apply function \a f to each node of the expression.
		 * @param e The expression
		 * @param f The function to apply
		 */
		void apply(ast::Expression& e, std::function< bool (ast::Expression&) > f);

	protected:
		void visit(ast::Expression&) final { assert(false); }
		void visit(ast::Identifier& e) final;
		void visit(ast::CppVariable& e) final;
		void visit(ast::LogicalVariable& e) final;
		void visit(ast::Literal& e) final;
		void visit(ast::UnaryExpression&) final { assert(false); }
		void visit(ast::PrefixExpression& e) final;
		void visit(ast::PostfixExpression& e) final;
		void visit(ast::InfixExpression& e) final;
		void visit(ast::TernaryExpression& e) final;
		void visit(ast::BuiltinConstraint& e) final;
		void visit(ast::ChrConstraint& e) final;
		void visit(ast::ChrCount&);

		std::function< bool (ast::Expression&) > _f;	///< The lambda to apply
	};

	/**
	 * @brief Expression visitor which build a CPP string representation of an expression tree
	 */
	struct ExpressionCppCode : ExpressionVisitor {
		/**
		 * Build a CPP string representation of the expression \a e.
		 * @param e The expression
		 */
		std::string_view string_from(ast::Expression&);

	private:
		void visit(ast::Expression&) final { assert(false); }
		void visit(ast::Identifier& e) final;
		void visit(ast::CppVariable& e) final;
		void visit(ast::LogicalVariable& e) final;
		void visit(ast::Literal& e) final;
		void visit(ast::UnaryExpression&) final { assert(false); }
		void visit(ast::PrefixExpression& e) final;
		void visit(ast::PostfixExpression& e) final;
		void visit(ast::InfixExpression& e) final;
		void visit(ast::TernaryExpression& e) final;
		void visit(ast::BuiltinConstraint& e) final;
		void visit(ast::ChrConstraint& e) final;
		void visit(ast::ChrCount&) final;

	private:
		std::vector< std::string > string_stack;	///< The result string which contains the convertion of an expression tree to a CPP string
	};
}
