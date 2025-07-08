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

#include <vector>
#include <string>
#include <unordered_set>
#include <ast/body.hh>
#include <ast/rule.hh>
#include <visitor/expression.hh>

namespace chr::compiler::parser::ast_builder
{
	/**
	 * @brief Ast rule building
	 *
	 * AstRuleBuilder contains all element that form a rule:
	 *		the name, the head, the guard and the body
	 */
	struct AstRuleBuilder
	{
		/**
		 * Default constructort
		 * @param id The unique id of the rule which be built
		 * @param chr_constraints0 List of declared CHR constraints
		 */
		AstRuleBuilder(unsigned int id, std::vector< ast::PtrSharedChrConstraintDecl >& chr_constraints0)
			: rule_id(id), chr_constraints(chr_constraints0)
		{}

		/**
		 * Build rule from gathered data
		 * @return The new rule object
		 */
		ast::PtrRule build_rule() 
		{
			assert(!rule_op.empty());
			if (rule_op	== "==>")
			{
				assert(head_keep.size() > 0);
				assert(head_del.size() == 0);
				assert(!split_head);
				assert(rule_op_pos);
				return std::make_unique<ast::PropagationRule>(rule_id, rule_name, std::move(head_keep), std::move(guard), std::move(body), *rule_op_pos);
			} else if (rule_op == "=>>")
			{
				assert(head_keep.size() > 0);
				assert(head_del.size() == 0);
				assert(!split_head);
				assert(rule_op_pos);
				return std::make_unique<ast::PropagationNoHistoryRule>(rule_id, rule_name, std::move(head_keep), std::move(guard), std::move(body), *rule_op_pos);
			} else if (rule_op == "<=>")
			{
				assert(rule_op_pos);
				assert(head_del.size() > 0);
				if (split_head)
				{
					assert(head_keep.size() > 0);
					return std::make_unique<ast::SimpagationRule>(rule_id, rule_name, std::move(head_keep), std::move(head_del), std::move(guard), std::move(body), *rule_op_pos);
				} else {
					assert(head_keep.size() == 0);
					return std::make_unique<ast::SimplificationRule>(rule_id, rule_name, std::move(head_del), std::move(guard), std::move(body), *rule_op_pos);
				}
			} else {
				throw ParseError("parse error during rule building", *rule_op_pos);
			}
		}

		/**
		 * Function to set the name of the rule
		 * @param s The name of the rule
		 */
		void set_name(std::string_view s)
		{
			rule_name = std::string(s);
		}

		/**
		 * Function to clear the name of the rule
		 */
		void clear_name()
		{
			rule_name = std::string();
		}

		/**
		 * Function to set the operator of the rule
		 * @param s The operator of the rule
		 */
		void set_rule_operator(std::string_view s, PositionInfo p)
		{
			rule_op = std::string(s);
			rule_op_pos = std::make_unique<PositionInfo>(p);
			if (split_head && ((rule_op == "==>") || (rule_op == "=>>")))
				throw ParseError("parse error, propagation rule can't have any constraint to delete", *split_head);
		}

		/**
		 * Function to add a new constraint call to the head
		 * @param c The constraint call
		 */
		void add_head(ast::PtrExpression e)
		{
			// Check that expression c is a CHR constraint call
			auto* pC = dynamic_cast< ast::ChrConstraint* >(e.get());
			if (pC == nullptr)
				throw ParseError("parse error matching head, CHR constraint expected", e->position());
			e.release();
			ast::PtrChrConstraint c(pC);

			// Build ChrConstraintCall from c
			PositionInfo pos = c->position();
			if (split_head)
				head_del.emplace_back( std::make_unique<ast::ChrConstraintCall>(std::move(c), pos) );
			else
				head_keep.emplace_back( std::make_unique<ast::ChrConstraintCall>(std::move(c), pos) );
		}

		/**
		 * Function to add a pragma to the last added head constraint
		 * @param c The constraint call
		 */
		void add_head_pragma(Pragma p)
		{
			if (split_head)
				head_del.at( head_del.size() - 1)->add_pragma(p);
			else
				head_keep.at( head_keep.size() - 1)->add_pragma(p);
		}

		/**
		 * Function to set the position of the split operator of the head.
		 * @param p The position of the split operator
		 */
		void set_split_head(PositionInfo p)
		{
			assert(!split_head);
			split_head = std::make_unique<PositionInfo>(p);
		}

		/**
		 * Function to add a new element to the guard
		 * @param c The constraint call
		 */
		void add_guard_part(ast::PtrExpression c)
		{
			auto ptr = dynamic_cast< ast::InfixExpression* >( c.get() );
			if ((ptr != nullptr) && (ptr->op() == "="))
			{
				try {
					ast::CppVariable& cpp_v = dynamic_cast< ast::CppVariable& >( *ptr->l_child() );
					(void) cpp_v;
				} catch (std::bad_cast&) {
					throw ParseError("parse error matching local variable identifier of assignment", ptr->position());
				}

			}
			guard.emplace_back( std::move(c) );
		}

		/**
		 * Function to check that the guard is valid
		 */
		void valid_guard()
		{
			visitor::ExpressionFullCheck check_visitor;
			// Check that guard doesn't contain any forbidden_ops
			std::unordered_set< std::string_view > forbidden_ops = { "++", "--", "%=", "+=", "-=", "*=", "/=", "<<=", ">>=", "&=", "|=", "^=" };
			auto f1 = [forbidden_ops](ast::Expression& e) {
				auto ptr1 = dynamic_cast< const ast::InfixExpression* >( &e );
				if ((ptr1 != nullptr) && (forbidden_ops.find(ptr1->op()) != forbidden_ops.end()))
					throw ParseError("parse error matching non const operator in guard", e.position());
				auto ptr2 = dynamic_cast< const ast::UnaryExpression* >( &e );
				if ((ptr2 != nullptr) && (forbidden_ops.find(ptr2->op()) != forbidden_ops.end()))
					throw ParseError("parse error matching non const operator in guard", e.position());
				auto ptr3 = dynamic_cast< const ast::ChrConstraint* >( &e );
				if (ptr3 != nullptr)
					throw ParseError("parse error matching CHR constraint in guard", e.position());
				return false;
			};

			// Check that guard doesn't contain any CPP or CHR keywords
			auto f2 = [&](ast::Expression& e) {
				auto ptr1 = dynamic_cast< const ast::Identifier* >( &e );
				if (ptr1 != nullptr)
				{
					if (std::find(CHR_KEYWORDS.begin(), CHR_KEYWORDS.end(), ptr1->value()) != CHR_KEYWORDS.end())
						throw ParseError("parse error matching reserved CHR keyword in expression", e.position());
					if (std::find(CPP_KEYWORDS.begin(), CPP_KEYWORDS.end(), ptr1->value()) != CPP_KEYWORDS.end())
						throw ParseError("parse error matching reserved CPP keyword in expression", e.position());
				}
				return false;
			};

			for (unsigned int i=0; i < (guard.size() - 1); ++i)
			{
				// Check that previous element is an assignment expression
				// to introduce new local variable
				try {
					ast::InfixExpression& s = dynamic_cast< ast::InfixExpression& >( *guard.at( i ) );
					if (s.op() != "=")
						throw ParseError("parse error matching guard, all but the last one must be local variable declarations", guard.at( i )->position());
					(void)check_visitor.check( *guard.at(i), f2 );
				} catch (std::bad_cast&) {
					throw ParseError("parse error matching guard, all but the last one must be local variable declarations", guard.at( i )->position());
				}
			}
			(void)check_visitor.check( *guard.back(), f1 );
		}

		/**
		 * Function to set the body of the rule
		 * @param b The body
		 */
		void set_body(ast::PtrBody b)
		{
            chr::compiler::visitor::BodyApply bav;
            // Check CHR constraint arity usefull only when body is not build from a sequence, beacause CHR constraint arity has already been tested on each part of a sequence.
            auto pChrConstraint = dynamic_cast< ast::ChrConstraint* >( b.get() );
            if (pChrConstraint != nullptr)
            {
                ast::PtrSharedChrConstraintDecl found;
                for (auto& c : chr_constraints)
                    if (c->_c->constraint()->name()->value() == pChrConstraint->name()->value())
                    {
                        // Check CHR constraint arity
                        if (pChrConstraint->children().size() != found->_c->constraint()->children().size())
                            throw ParseError("parse error matching CHR constraint, wrong number of arguments", pChrConstraint->position());
                        break;
                    }
            }
			body = std::move(b);
		}
	
		unsigned int rule_id;							///< The id of the rule
		std::string rule_name;							///< The name of the rule
		std::string rule_op;							///< The operator of the rule
		std::unique_ptr< PositionInfo > rule_op_pos;	///< Position of the rule operator
		std::unique_ptr< PositionInfo > split_head;		///< To know if head is split in two parts
		std::vector< ast::PtrChrConstraintCall > head_keep;		///< The constraints of the head that must be kept
		std::vector< ast::PtrChrConstraintCall > head_del;		///< The constraints of the head that must be deleted
		std::vector< ast::PtrExpression > guard;				///< The guard of the rule
		ast::PtrBody body;										///< The body of the rule

		std::vector< ast::PtrSharedChrConstraintDecl >& chr_constraints;	///< Reference to the recorded CHR constraints
	};
} // namespace chr::compiler::parser::ast_builder
