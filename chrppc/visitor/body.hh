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
#include <unordered_set>
#include <unordered_map>
#include <cassert>

#include <ast/body.hh>
#include <ast/program.hh>

namespace chr::compiler::visitor
{
	/**
	 * @brief Root class of the body visitor
	 *
	 * An body visitor visits node of a body tree.
	 */
	struct BodyVisitor {
		/**
		 * Root visit function for Body
		 */
		virtual void visit(ast::Body&) = 0;
		virtual void visit(ast::ChrKeyword&) = 0;
		virtual void visit(ast::CppExpression&) = 0;
		virtual void visit(ast::CppDeclAssignment&) = 0;
		virtual void visit(ast::Unification&) = 0;
		virtual void visit(ast::ChrConstraintCall&) = 0;
		virtual void visit(ast::Sequence&) = 0;
		virtual void visit(ast::ChrBehavior&) = 0;
		virtual void visit(ast::ChrTry&) = 0;

		/**
		 * Default virtual destructor
		 */
		virtual ~BodyVisitor() = default;
	};

	/**
	 * @brief Body visitor which prints a body tree
	 */
	struct BodyPrint : BodyVisitor {
		/**
		 * Build a string representation of the body \a b
		 * @param b The body
		 * @return The generated string
		 */
		std::string_view string_from(ast::Body&);

	private:
		void visit(ast::Body&) final;
		void visit(ast::ChrKeyword& k) final;
		void visit(ast::CppExpression& e) final;
		void visit(ast::CppDeclAssignment& e) final;
		void visit(ast::Unification& e) final;
		void visit(ast::ChrConstraintCall& e) final;
		void visit(ast::Sequence& s) final;
		void visit(ast::ChrBehavior& b) final;
		void visit(ast::ChrTry& b) final;

		std::vector< std::string > string_stack;	///< The result string which contains the convertion of a body tree to a string
	};

	/**
	 * @brief Body visitor which prints a body tree
	 *
	 * In the visitor, the sequence() constraint print the
	 * involved variables of each part.
	 */
	struct BodyFullPrint : BodyVisitor {
		/**
		 * Build a string representation of the body \a b.
		 * @param b The body
		 * @return The generated string
		 */
		std::string_view string_from(ast::Body&, std::size_t prefix = 0);

	private:
		std::string prefix();

		void visit(ast::Body&) final;
		void visit(ast::ChrKeyword& k) final;
		void visit(ast::CppExpression& e) final;
		void visit(ast::CppDeclAssignment& e) final;
		void visit(ast::Unification& e) final;
		void visit(ast::ChrConstraintCall& e) final;
		void visit(ast::Sequence& s) final;
		void visit(ast::ChrBehavior& b) final;
		void visit(ast::ChrTry& b) final;

		std::size_t _prefix_w = 0;	///< Prefix for width alignment
		std::size_t _depth = 0;		///< Recursive depth
		std::vector< std::string > string_stack;	///< The result string which contains the convertion of a body tree to a string
	};

	/**
	 * @brief Body visitor which check if body is an empty one
	 *
	 * Visitor visits the first node and checks if it is an empty body.
	 */
	struct BodyEmpty : BodyVisitor {
		/**
		 * Check if body is only an empty body.
		 * @param b The body
		 * @return True if body is empty, false otherwise
		 */
		bool is_empty(ast::Body& b);

	private:
		void visit(ast::Body&) final;
		void visit(ast::ChrKeyword& k) final;
		void visit(ast::CppExpression& e) final;
		void visit(ast::CppDeclAssignment& e) final;
		void visit(ast::Unification& e) final;
		void visit(ast::ChrConstraintCall& e) final;
		void visit(ast::Sequence& s) final;
		void visit(ast::ChrBehavior& b) final;
		void visit(ast::ChrTry& b) final;

		bool res; ///< The result value
	};

	/**
	 * @brief Body visitor which apply a lambda function
	 *
	 * The visitor visits nodes of the tree and apply the lambda
	 * function on each node. The recursion of a branch is stopped
	 * if the lambda function f return false.
	 */
	struct BodyApply : BodyVisitor {
		/**
		 * Apply function \a f to each node of the body.
		 * @param b The body
		 * @param f The function to apply
		 */
		void apply(ast::Body& b, std::function< bool (ast::Body&) > f);

	private:
		void visit(ast::Body&) final;
		void visit(ast::ChrKeyword& k) final;
		void visit(ast::CppExpression& e) final;
		void visit(ast::CppDeclAssignment& e) final;
		void visit(ast::Unification& e) final;
		void visit(ast::ChrConstraintCall& e) final;
		void visit(ast::Sequence& s) final;
		void visit(ast::ChrBehavior& b) final;
		void visit(ast::ChrTry& b) final;

		std::function< bool (ast::Body&) > _f;	///< The lambda to apply
	};

	/**
	 * @brief Body visitor which apply a lambda function on each body or expression node
	 *
	 * The visitor visits nodes of the tree and apply the lambda
	 * function on each node. If the node embeds an expression, the function _f_expr
	 * is applied on each node of the expression. an The recursion of a branch is stopped
	 * if the lambda function f return false.
	 */
	struct BodyExpressionApply : BodyVisitor {
		/**
		 * Apply function \a f to each node of the body.
		 * @param b The body
		 * @param f The function to apply
		 * @param f_expr The function to apply on Expression nodes
		 */
		void apply(ast::Body& b, std::function< bool (ast::Body&) > f, std::function< bool (ast::Expression&) > f_expr);

	private:
		void visit(ast::Body&) final;
		void visit(ast::ChrKeyword& k) final;
		void visit(ast::CppExpression& e) final;
		void visit(ast::CppDeclAssignment& e) final;
		void visit(ast::Unification& e) final;
		void visit(ast::ChrConstraintCall& e) final;
		void visit(ast::Sequence& s) final;
		void visit(ast::ChrBehavior& b) final;
		void visit(ast::ChrTry& b) final;

		std::function< bool (ast::Body&) > _f;				///< The lambda to apply on body
		std::function< bool (ast::Expression&) > _f_expr;	///< The lambda to apply on expression
	};

	/**
	 * @brief Body visitor which generates the abstract imperative code of the body
	 *
	 * Visitor that generates the abstract imperative code of the body.
	 */
	struct BodyAbstractCode : BodyVisitor {
		/**
		 * Context data structure used to gather elements which are part of context
		 * of statements of the body. The context contains variable usages and
		 * declaration.
		 */
		struct Context {
			std::unordered_map< std::string, std::tuple<ast::PtrSharedChrConstraintDecl,int> > _logical_variables;	///< Logical variables (and declaration) used
			std::unordered_set< std::string > _local_variables;														///< Local variables used
		};

		/**
		 * Default constructor.
		 * @param os The output stream used to send the generated code.
		 */
		BodyAbstractCode(std::ostream& os);

		/**
		 * Generate the imperative code of body \a e.
		 * @param e The body
		 * @param prefix_w The initial prefix width used for alignment
		 * @param active_constraint_name The name of the active constraint of the rule which involved this body
		 * @param last_statement True if the body to generate is the last statement of the rule which involved this body
		 * @param ctxt_logical_var Logical variables introduced by occurrence rule
		 * @param ctxt_local_var Local variables introduced by occurrence rule
		 */
		void generate(ast::Body& e, std::size_t prefix_w, std::string active_constraint_name, bool last_statement, std::unordered_map<std::string,std::tuple<ast::PtrSharedChrConstraintDecl,int>> ctxt_logical_var, std::unordered_set<std::string> ctxt_local_var);

	protected:
		virtual void visit(ast::Body&);
		virtual void visit(ast::ChrKeyword& k);
		virtual void visit(ast::CppExpression& e);
		virtual void visit(ast::CppDeclAssignment& e);
		virtual void visit(ast::Unification& e);
		virtual void visit(ast::ChrConstraintCall& e);
		virtual void visit(ast::Sequence& s);
		virtual void visit(ast::ChrBehavior& b);
		virtual void visit(ast::ChrTry& b);

		/**
		 * Remove spaces at the beginning of string \a s.
		 * @return The left trimmed string \a s
		 */
		std::string ltrim(std::string_view s);

		/**
		 * Return the prefix to print for each line, depending on the _depth.
		 * @return The prefix
		 */
		std::string prefix();

		Context _context;					///< Context of the body
		std::size_t _prefix_w = 0;			///< Initial prefix width for alignment
		std::size_t _depth = 0;				///< Recursive depth
		std::size_t _num_sequence = 0;		///< Number of ";" encountered
		bool _last_statement = true;		///< True if the current statement is the last one
		std::string _active_constraint_name;///< The name of the active constraint
		std::string _next_on_inapplicable;	///< Next label to go after the rule succeeds
		std::ostream& _os;					///< Output stream used to send the generated code
	};

	/**
	 * @brief Body visitor which generates the CPP code of the body
	 *
	 * Visitor that generates the CPP code of the body.
	 */
	struct BodyCppCode : BodyAbstractCode {
		/**
		 * Default constructor.
		 * @param os The output stream used to send the generated code.
		 * @param input_file_name The name of the input file used to built this body
		 * @param trace_prefix The prefix used to write trace statements
		 * @param auto_catch_failure True if CHR program must try to automatically catch failure, false otherwise
		 */
		BodyCppCode(std::ostream& os, std::string_view input_file_name, std::string_view trace_prefix, bool auto_catch_failure);

	protected:
		void visit(ast::Body&) override final;
		void visit(ast::ChrKeyword& k) override final;
		void visit(ast::CppExpression& e) override final;
		void visit(ast::CppDeclAssignment& e) override final;
		void visit(ast::Unification& e) override final;
		void visit(ast::ChrConstraintCall& e) override final;
		void visit(ast::Sequence& s) override final;
		void visit(ast::ChrBehavior& b) override final;
		void visit(ast::ChrTry& b) override final;
	
	private:
		/**
		 * Loop over all element of args and send each args[I] to ostream. 
		 * @param args The tuple of elements to browse
		 * @param I Index sequence of the recursive loop
		 */
		template< typename TT, size_t... I >
		void loop_to_ostream(const TT& args, std::index_sequence<I...>);

		/**
		 * Generates the code corresponding to a debug message.
		 * @param flag The flag for this message
		 * @param args The tuple of elements corresponding to the message to write
		 */
		template< typename... T >
		void write_trace_statement(const char* flag, const std::tuple< T... >& args);

		/**
		 * Generate the imperative code in order to declare all undeclared logical
		 * variables of expression \a e0.
		 * @param e0 The expression
		 */
		void declare_undeclared_logical_variable(ast::Expression& e0);

		/**
		 * Generate the imperative code in order to declare all undeclared logical
		 * variables of body \a b0.
		 * @param b0 The body
		 */
		void declare_undeclared_logical_variable(ast::Body& b0);

		std::string _input_file_name;	///< Name of the file used to built this rule
		std::string _trace_prefix;		///< Prefix for writing trace statements
		bool _auto_catch_failure;		///< True if CHR program must try to automatically catch failure, false otherwise
	};
}
