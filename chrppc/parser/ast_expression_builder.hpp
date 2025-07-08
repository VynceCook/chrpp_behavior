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
#include <ast/body.hh>
#include <ast/expression.hh>
#include <parser/reserved_keywords.hh>
#include <visitor/expression.hh>

namespace chr::compiler::parser::ast_builder
{
	/**
	 * @brief Ast expression tree building
	 *
	 * AstExpressionBuilder contains a stack of atomic elements constituting a syntactically
	 * correct expression. Its various functions allow to add parsed elements
	 * to the stack, but also to unpack and build the abstract syntax tree of the expression. 
	 */
	struct AstExpressionBuilder
	{
		/**
		 * Default constructort
		 */
		AstExpressionBuilder(std::vector< ast::PtrSharedChrConstraintDecl >& chr_constraints0)
			: chr_constraints(chr_constraints0)
		{}

		/**
		 * Function to construct and stack an infixed binary expression.
		 * @param op The binary operator to use
		 * @param p The position of element in source
		 */
		void infix( const std::string_view op, PositionInfo p )
		{
			assert( expression_stack.size() >= 2 );
	
            // Recast CHR constraint to builtin constraints in the right part
			if ((op == "::") or (op == "."))
            {
				auto pc = dynamic_cast< ast::ChrConstraint* >(expression_stack[expression_stack.size() - 1].get());
                if (pc != nullptr)
                {
                    ast::PtrExpression name( pc->name()->clone() );
                    auto tmp = std::make_unique< ast::BuiltinConstraint >( std::move(name), "(", ")", pc->children(), pc->position());
                    expression_stack.back() = std::move( tmp );
                }
            }

			if (op != "::")
				cast_identifier_to_var( expression_stack.size() - 2 );
			if ((op != "::") && (op != "."))
				cast_identifier_to_var( expression_stack.size() - 1 );

			ast::PtrExpression tmp = std::make_unique< ast::InfixExpression >(op, std::move(expression_stack.at( expression_stack.size() - 2)), std::move(expression_stack.at( expression_stack.size() - 1)), p);
			expression_stack.pop_back();
			expression_stack.back() = std::move( tmp );
		}
	
		/**
		 * Function to construct and stack a prefixed expression.
		 * @param op The operator to use
		 * @param p The position of element in source
		 */
		void prefix( const std::string_view op, PositionInfo p )
		{
			assert( expression_stack.size() >= 1 );  // NOLINT(readability-container-size-empty)
	
			cast_identifier_to_var( expression_stack.size() - 1 );
			ast::PtrExpression tmp = std::make_unique< ast::PrefixExpression >(op, std::move(expression_stack.at( expression_stack.size() - 1)), p);
			expression_stack.back() = std::move( tmp );
		}
	
		/**
		 * Function to construct and stack a postfixed expression.
		 * @param op The operator to use
		 * @param p The position of element in source
		 */
		void postfix( const std::string_view op, PositionInfo p )
		{
			assert( expression_stack.size() >= 1 );  // NOLINT(readability-container-size-empty)
	
			cast_identifier_to_var( expression_stack.size() - 1 );
			ast::PtrExpression tmp = std::make_unique< ast::PostfixExpression >(op, std::move(expression_stack.at( expression_stack.size() - 1)), p);
			expression_stack.back() = std::move( tmp );
		}
	
		/**
		 * Function to construct and stack a ternary expression
		 * (mainly the ? C++ ternary operator).
		 * @param op The first operator to use
		 * @param o2 The second operator to use
		 * @param p The position of element in source
		 */
		void ternary( const std::string_view op, const std::string_view o2, PositionInfo p )
		{
			assert( expression_stack.size() >= 2 );
	
			cast_identifier_to_var( expression_stack.size() - 3 );
			cast_identifier_to_var( expression_stack.size() - 2 );
			cast_identifier_to_var( expression_stack.size() - 1 );
			ast::PtrExpression tmp = std::make_unique< ast::TernaryExpression >(op, o2, std::move(expression_stack.at( expression_stack.size() - 3)), std::move(expression_stack.at( expression_stack.size() - 2)), std::move(expression_stack.at( expression_stack.size() - 1)), p);
			expression_stack.pop_back();
			expression_stack.pop_back();
			expression_stack.back() = std::move( tmp );
		}
	
		/**
		 * Function to construct and stack a function call.
		 * @param op The left delimiter
		 * @param o2 The right delimiter
		 * @param args The number of parameters to the function
		 * @param p The position of element in source
		 */
		void call( const std::string_view op, const std::string_view o2, const std::size_t args, PositionInfo p )
		{
			assert( expression_stack.size() > args );
	
			ast::PtrExpression name = std::move(*( expression_stack.end() - args - 1 ));
			std::vector< ast::PtrExpression > tmp_vect;
			tmp_vect.reserve( args );
			for( std::size_t i = 0; i < args; ++i )
			{
				cast_identifier_to_var( expression_stack.size() - args + i );
				tmp_vect.emplace_back( std::move(*( expression_stack.end() - args + i)) );
			}
			expression_stack.resize( expression_stack.size() - args );

			// Check if this is a CHR constraint
			// First we have to get only an identifier in the left part
			auto pi_name = dynamic_cast< ast::Identifier* >(name.get());
			if (pi_name != nullptr)
			{
				// Then, we search in the existing list of CHR constraints
				ast::PtrSharedChrConstraintDecl found;
				for (auto& c : chr_constraints)
					if (c->_c->constraint()->name()->value() == pi_name->value())
					{
						found = c;
						break;
					}
				if (found)
				{
					name.release();
					auto tmp = std::make_unique< ast::ChrConstraint >(found , tmp_vect, p);
					expression_stack.back() = std::move( tmp );
					return;
				}
			}

			ast::PtrExpression tmp = std::make_unique< ast::BuiltinConstraint >( std::move(name), op, o2, tmp_vect, p);
			expression_stack.back() = std::move( tmp );
		}

		/**
		 * Function to build and stack a CHR constraint declaration.
		 */
		void chr_constraint_decl()
		{
			assert( expression_stack.size() >= 1 );
			std::size_t args = (expression_stack.size() - 1) / 2;

			std::unique_ptr<ast::Identifier> name;

			auto pi = dynamic_cast< ast::Identifier* >( expression_stack.at( 0 ).get() );
			if (pi == nullptr)
				throw ParseError("parse error in constraint declaration name (should not occur)",expression_stack.at(0)->position());
			expression_stack.at( 0 ).release();
			name.reset( pi );	

			if (std::find(CHR_KEYWORDS.begin(), CHR_KEYWORDS.end(), name->value()) != CHR_KEYWORDS.end())
				throw ParseError("parse error, CHR keyword cannot be used as a CHR constraint name", name->position());
			if (std::find(CPP_KEYWORDS.begin(), CPP_KEYWORDS.end(), name->value()) != CPP_KEYWORDS.end())
				throw ParseError("parse error, CPP keyword cannot be used as a CHR constraint name", name->position());

			std::vector< ast::PtrExpression > tmp_vect;
			tmp_vect.reserve( args );
			for( std::size_t i = 0; i < args; ++i )
			{
				PositionInfo pos = (*(expression_stack.end() - 2*(args-i)))->position();
				ast::Identifier* pmode = dynamic_cast< ast::Identifier* >( (expression_stack.end() - 2*(args-i))->get() );
				if (pmode == nullptr)
					throw ParseError("parse error in constraint declaration mode (should not occur)",pos);
				tmp_vect.emplace_back(
						std::make_unique<ast::PrefixExpression>(
							pmode->value(),
							std::move( *( expression_stack.end() - 2*(args - i) + 1) ),
							pos	
							));
			}
			expression_stack.resize( 1 );
			auto tmp = std::make_unique< ast::ChrConstraint >( std::move(name), tmp_vect, name->position());
			expression_stack.back() = std::move( tmp );
		}

		/**
		 * Function to build and stack a chr_count
		 */
		void chr_count()
		{
			assert( expression_stack.size() >= 1 );
			std::unique_ptr<ast::ChrConstraint> chr_c;
			auto pc = dynamic_cast< ast::ChrConstraint* >( expression_stack.back().get() );
			if (pc == nullptr)
				throw ParseError("parse error matching CHR constraint (should not occur)",expression_stack.back()->position());
			expression_stack.back().release();
			chr_c.reset( pc );	

			std::vector< unsigned int > index;
			unsigned int i = 0;
			for (auto& cc : chr_c->children())
			{
				PositionInfo pos = cc->position();
				auto pL = dynamic_cast< ast::Literal* >(cc.get());
				auto pV = dynamic_cast< ast::CppVariable* >(cc.get());
				auto pLV = dynamic_cast< ast::LogicalVariable* >(cc.get());
				if ((pL == nullptr) && (pV == nullptr) && (pLV == nullptr))
					throw ParseError("Unexpected argument in CHR constraint (should not occur)",cc->position());
				if (pL != nullptr)
				{
					assert((pLV == nullptr) && (pV == nullptr));
					index.emplace_back(i);
				}
				if (pV != nullptr)
				{
					assert((pLV == nullptr) && (pL == nullptr));
					index.emplace_back(i);
				}
				if (pLV != nullptr)
				{
					assert((pV == nullptr) && (pL == nullptr));
					std::string v_name = pLV->value();
					if (v_name != "_")
						index.emplace_back(i);
				}
				++i;
			}
			// Update index of chr_c
			int use_index = -1;
			if (!index.empty())
			{
				auto& indexes = chr_c->decl()->_indexes;
				auto ret = std::find(indexes.begin(),indexes.end(),index);
				// Add index only if it not already here
				if (ret == indexes.end())
				{
					use_index = indexes.size();
					chr_c->decl()->_indexes.emplace_back( index );
				} else {
					use_index = ret - indexes.begin();
				}
			}
			auto tmp = std::make_unique< ast::ChrCount >( use_index, std::move(chr_c), chr_c->position());
			expression_stack.back() = std::move( tmp );
		}

		/**
		 * Function to build and stack a CHR constraint of the head.
		 */
		void chr_constraint_head()
		{
			assert( expression_stack.size() >= 1 );
			std::size_t args = (expression_stack.size() - 1);

			std::unique_ptr<ast::Identifier> name;

			auto pi = dynamic_cast< ast::Identifier* >( expression_stack.at( 0 ).get() );
			if (pi == nullptr)
				throw ParseError("parse error in chr constraint name (should not occur)",expression_stack.at( 0 )->position());
			expression_stack.at( 0 ).release();
			name.reset( pi );	

			std::vector< ast::PtrExpression > tmp_vect;
			tmp_vect.reserve( args );
			for( std::size_t i = 0; i < args; ++i )
			{
				cast_identifier_to_var( expression_stack.size() - args + i, true );
				PositionInfo pos = (*(expression_stack.end() - args + i))->position();
				auto plvar = dynamic_cast< ast::LogicalVariable* >( (expression_stack.end() - args + i)->get() );
				auto pvar = dynamic_cast< ast::CppVariable* >( (expression_stack.end() - args + i)->get() );
				auto pliteral = dynamic_cast< ast::Literal* >( (expression_stack.end() - args + i)->get() );
				if ((plvar == nullptr) && (pvar == nullptr) && (pliteral == nullptr))
					throw ParseError("parse error matching chr constraint argument, only literal or logical variables are allowed here",pos);
				tmp_vect.emplace_back( std::move( *(expression_stack.end() - args + i) ) );
			}
			expression_stack.resize( 1 );
			
			// Check that the constraint has been previously declared
			std::string c_name = name->value();
			ast::PtrSharedChrConstraintDecl found;
			for (auto& cc : chr_constraints)
				if (cc->_c->constraint()->name()->value() == c_name)
				{
					found = cc;
					break;
				}
			if (!found)
				throw ParseError("parse error matching head, undeclared CHR constraint", name->position());

			// Check CHR constraint arity
			if (args != found->_c->constraint()->children().size())
				throw ParseError("parse error matching CHR constraint, wrong number of arguments", name->position());

			auto tmp = std::make_unique< ast::ChrConstraint >( found, tmp_vect, name->position());
			expression_stack.back() = std::move( tmp );
		}

		/**
		 * Function to stack a parsed number.
		 * @param l The number to use
		 * @param p The position of element in source
		 */
		void number( std::string l, PositionInfo p )
		{
			expression_stack.emplace_back( std::make_unique<ast::Literal>(l, p) );
		}
	
		/**
		 * Function to stack a parsed identifier.
		 * @param id The string to use
		 * @param p The position of element in source
		 */
		void identifier( const std::string& id, PositionInfo p )
		{
			expression_stack.emplace_back( std::make_unique<ast::Identifier>(id,p) );
		}

		/**
		 * Function to convert an identifier to a variable (CppVariable or LogicalVariable)
		 * @param i The index of expression in stack
		 */
		void cast_identifier_to_var( std::size_t i, bool allow__ = false)
		{
			ast::Identifier* pi = dynamic_cast< ast::Identifier* >(expression_stack.at(i).get());
			if (pi != nullptr)
			{
				auto name = pi->value();
				auto pos = pi->position();
				expression_stack.at(i).reset(nullptr);
				if (allow__ && (name[0] == '_') && (name.size() == 1))
					expression_stack.at(i) = std::make_unique<ast::LogicalVariable>("_",pos);
				else if (name[0] == '_')
					throw ParseError("parse error, a variable cannot start with '_'", pos);
				else if (isupper(name[0]))
					expression_stack.at(i) = std::make_unique<ast::LogicalVariable>(name,pos);
				else
					expression_stack.at(i) = std::make_unique<ast::CppVariable>(name,pos);
			}
		}

		std::vector< ast::PtrSharedChrConstraintDecl >& chr_constraints;	///< Reference to the recorded CHR constraints
		std::vector< ast::PtrExpression > expression_stack;					///< The stack of expression parts
	};
} // namespace chr::compiler::parser::ast_builder
