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
#include <visitor/expression.hh>
#include <visitor/body.hh>

namespace chr::compiler::parser::ast_builder
{
	/**
	 * @brief Ast body tree building
	 *
	 * AstBodyBuilder contains a stack of atomic elements constituting a syntactically
	 * correct chr body. Its various functions allow to add parsed elements
	 * to the stack, but also to unpack and build the abstract syntax tree of the body. 
	 */
	struct AstBodyBuilder
	{
		/**
		 * Default constructort
		 */
		AstBodyBuilder(std::vector< ast::PtrSharedChrConstraintDecl >& chr_constraints0)
			: chr_constraints(chr_constraints0)
		{}

        void check_chr_constraint_arity( std::size_t i )
        {

            chr::compiler::visitor::BodyApply bav;
            auto pChrConstraint = dynamic_cast< ast::ChrConstraint* >( body_stack.at(i).get() );
            if (pChrConstraint != nullptr)
            {
                ast::PtrSharedChrConstraintDecl found;
                for (auto& c : chr_constraints)
                    if (c->_c->constraint()->name()->value() == pChrConstraint->name()->value())
                    {
                        // Check CHR constraint arity
                        if (pChrConstraint->children().size() != found->_c->constraint()->children().size())
                            throw ParseError("parse error matching CHR constraint, wrong number of arguments", pChrConstraint->position());
                        return;
                    }
            }
        }

		/**
		 * Function to construct and stack an infixed binary expression.
		 * @param op The binary operator to use
		 * @param p The position of element in source
		 */
		void sequence( const std::string_view op, PositionInfo p )
		{
			assert( body_stack.size() >= 2 );
			ast::Sequence* ptr = nullptr;
			ptr = dynamic_cast< ast::Sequence* >( body_stack.at( body_stack.size() - 2 ).get() );
			// If the left element of the sequence is of same type, we merge
			if ((ptr != nullptr) && (ptr->op() == op))
			{
				cast_identifier_to_var( body_stack.size() - 1 );
                // Check CHR constraint arity as soon as possible
				check_chr_constraint_arity( body_stack.size() - 1 );
				ptr->add_child( std::move(body_stack.at(body_stack.size() - 1)) );
				body_stack.pop_back();
			} else {
				cast_identifier_to_var( body_stack.size() - 2 );
                // Check CHR constraint arity as soon as possible
				check_chr_constraint_arity( body_stack.size() - 2 );
				cast_identifier_to_var( body_stack.size() - 1 );
                // Check CHR constraint arity as soon as possible
				check_chr_constraint_arity( body_stack.size() - 1 );
				auto tmp = std::make_unique< ast::Sequence >(op, p);
				tmp->add_child( std::move(body_stack.at( body_stack.size() - 2)) );
				tmp->add_child( std::move(body_stack.at( body_stack.size() - 1)) );
				body_stack.pop_back();
				body_stack.back() = std::move( tmp );
			}
		}

		/**
		 * Function to add a pragma to last cpp expression parsed
		 * @param pragma The pragma to add
		 * @param p The position of pragma in source
		 */
		void add_pragma( Pragma pragma, PositionInfo p )
		{
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 1 ) );
				s.add_pragma( pragma );
			} catch (std::bad_cast&) {
				throw ParseError("parse error trying adding pragma",p);
			}
		}

		/**
		 * Function to stack a parsed chr keyword.
		 * @param s The name of the keyword
		 * @param p The position of element in source
		 */
		void chr_keyword( std::string k, PositionInfo p )
		{
			std::string kw(k);
			std::string::size_type i = kw.find_first_of('(');
			if (i != std::string::npos)
				kw.erase( i );
			body_stack.emplace_back( std::make_unique<ast::ChrKeyword>(kw,p) );
		}

		/**
		 * Function which checks that \a e is not a CHR statement.
		 * It throws an error, if such a statement is found.
		 * @param e The expression to check
		 * @return false in order to be compatible with ExpressionCheck requirements
		 */
		static bool no_chr_in_expression( ast::Expression& e )
		{
			auto ptr1 = dynamic_cast< const ast::InfixExpression* >( &e );
			if ((ptr1 != nullptr) && (ptr1->op() == "%="))
				throw ParseError("parse error, unification cannot be nested in another expression", e.position());
			auto ptr2 = dynamic_cast< const ast::UnaryExpression* >( &e );
			if ((ptr2 != nullptr) && (ptr2->op() == "%="))
				throw ParseError("parse error, unification cannot be nested in another expression", e.position());
			auto ptr3 = dynamic_cast< const ast::ChrConstraint* >( &e );
			if (ptr3 != nullptr)
					throw ParseError("parse error, CHR constraint cannot be nested in another expression", e.position());
			return false;
		}

		/**
		 * Function which checks that \a s is not a CHR statement.
		 * It throws an error, if such a statement is found.
		 * @param s The statement to check
		 * @param authorize_assignment True if C++ assignment are authorized
		 */
		void check_no_chr_statement( ast::PtrBody& s, bool authorize_assignment )
		{
			if (!authorize_assignment && (dynamic_cast< ast::CppDeclAssignment* >(s.get()) != nullptr))
				throw ParseError("parse error, assignment cannot be encountered here", s->position());
			if (dynamic_cast< ast::Unification* >(s.get()) != nullptr)
				throw ParseError("parse error, unification cannot be encountered here", s->position());
			if (dynamic_cast< ast::ChrConstraintCall* >(s.get()) != nullptr)
				throw ParseError("parse error, CHR constraint call cannot be encountered here", s->position());
			if ((dynamic_cast< ast::CppDeclAssignment* >(s.get()) == nullptr) && (dynamic_cast< ast::ChrExpression* >(s.get()) != nullptr))
				throw ParseError("parse error, CHR reserved expression cannot be encountered here", s->position());
			if (dynamic_cast< ast::ChrBehavior* >(s.get()) != nullptr)
				throw ParseError("parse error, behavior, exists and forall cannot be encountered here", s->position());
		}

		/**
		 * Function to stack a parsed cpp expression.
		 * @param e The expression to add
		 * @param p The position of element in source
		 */
		void cpp_expression( ast::PtrExpression e, PositionInfo p )
		{
			visitor::ExpressionFullCheck check_visitor1;
			visitor::ExpressionLightCheck check_visitor2;
			// Check that name doesn't contain any CPP or CHR keywords
			auto f = [&](ast::Expression& e) {
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
			(void)check_visitor1.check( *e, f );

			// Check if expression is an CppDeclAssignment
			{
				ast::InfixExpression* ptr = nullptr;
				ptr = dynamic_cast< ast::InfixExpression* >( e.get() );
				if ((ptr != nullptr) && (ptr->op() == "="))
				{
					std::unique_ptr< ast::CppVariable > cpp_variable;
					try {
						ast::CppVariable& s = dynamic_cast< ast::CppVariable& >( *ptr->l_child() );
						ptr->l_child().release();
						cpp_variable.reset(&s);
					} catch (std::bad_cast&) {
						throw ParseError("parse error matching local variable identifier of assignment", ptr->position());
					}
					(void)check_visitor2.check( *ptr->r_child(), no_chr_in_expression );
					body_stack.emplace_back( std::make_unique<ast::CppDeclAssignment>(std::move(cpp_variable), std::move(ptr->r_child()), ptr->position()) );
					return;
				}
			}

			// Check if expression is an unification
			{
				ast::InfixExpression* ptr = nullptr;
				ptr = dynamic_cast< ast::InfixExpression* >( e.get() );
				if ((ptr != nullptr)  and (ptr->op() == "%="))
				{
					std::unique_ptr< ast::LogicalVariable > logic_variable;
					try {
						ast::LogicalVariable& plv = dynamic_cast< ast::LogicalVariable& >( *ptr->l_child() );
						ptr->l_child().release();
						logic_variable.reset(&plv);
					} catch (std::bad_cast&) {
						throw ParseError("parse error matching logical variable identifier", ptr->position());
					}
					(void)check_visitor2.check( *ptr->r_child(), no_chr_in_expression );
					body_stack.emplace_back( std::make_unique<ast::Unification>(std::move(logic_variable), std::move(ptr->r_child()), ptr->position()) );
					return;
				}
			}

			// Check if expression is a ChrConstraint
			{
				auto ptr = dynamic_cast< ast::ChrConstraint* >( e.get() );
				if (ptr != nullptr)
				{
					e.release();
					std::unique_ptr< ast::ChrConstraint > chr_c( ptr );
					for (auto& c : chr_c->children())
						(void)check_visitor2.check( *c, no_chr_in_expression );
					body_stack.emplace_back( std::make_unique<ast::ChrConstraintCall>( std::move(chr_c), p) );
					return;
				}
			}
			
			// Default behavior
			(void)check_visitor2.check( *e, no_chr_in_expression );
			body_stack.emplace_back( std::make_unique<ast::CppExpression>(std::move(e),p) );
		}
	
		/**
		 * Function to construct and stack an empty body
		 * @param p The position of element in source
		 */
		void empty_body( PositionInfo p )
		{
			body_stack.emplace_back( std::make_unique<ast::Body>(p) );
		}
	
		/**
		 * Function to construct and stack a chr try statement
		 * @param backtrack True if operator must backtrack even if succeeded, false otherwise
		 * @param p The position of element in source
		 */
		void chr_try( bool backtrack, PositionInfo p )
		{
			assert( body_stack.size() >= 2 );
	
			// Convert to CppExpression
			cast_identifier_to_var( body_stack.size() - 2 );
			ast::PtrExpression ptr_tmp;
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 2 ) );
				ptr_tmp.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching result variable of CHR try",body_stack.at( body_stack.size() - 2 )->position());
			}

			// Cast to CppVariable
			std::unique_ptr< ast::CppVariable > cpp_variable;
			try {
				ast::CppVariable& s = dynamic_cast< ast::CppVariable& >( *ptr_tmp );
				ptr_tmp.release();
				cpp_variable.reset(&s);
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching local variable identifier", ptr_tmp->position());
			}

			auto tmp = std::make_unique< ast::ChrTry >(
					backtrack,
					std::move(cpp_variable),
					std::move(body_stack.at( body_stack.size() - 1)),
					p);
			body_stack.resize( body_stack.size() - 1 );
			body_stack.back() = std::move( tmp );
		}

		/**
		 * Function to construct and stack a behavior chr statement
		 * @param p The position of element in source
		 */
		void behavior( PositionInfo p )
		{
			assert( body_stack.size() >= 7 );

			ast::PtrExpression stop_cond;
			check_no_chr_statement( body_stack.at( body_stack.size() - 7 ), false );
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 7 ) );
				stop_cond.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching stop condition of behavior",body_stack.at( body_stack.size() - 7 )->position());
			}

			check_no_chr_statement( body_stack.at( body_stack.size() - 6 ), true );
			auto on_succeeded_alt = std::move(body_stack.at( body_stack.size() - 6));
			check_no_chr_statement( body_stack.at( body_stack.size() - 5 ), true );
			auto on_failed_alt = std::move(body_stack.at( body_stack.size() - 5));

			ast::PtrExpression final_status;
			check_no_chr_statement( body_stack.at( body_stack.size() - 4 ), false );
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 4 ) );
				final_status.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching final status boolean expression of behavior",body_stack.at( body_stack.size() - 4 )->position());
			}

			auto on_succeeded_status = std::move(body_stack.at( body_stack.size() - 3));
			auto on_failed_status = std::move(body_stack.at( body_stack.size() - 2));
			auto behavior_body = std::move(body_stack.at( body_stack.size() - 1));

			auto tmp = std::make_unique< ast::ChrBehavior >(
					std::move(stop_cond),
					std::move(on_succeeded_alt),
					std::move(on_failed_alt),
					std::move(final_status),
					std::move(on_succeeded_status),
					std::move(on_failed_status),
					std::move(behavior_body),
					p);
			body_stack.resize( body_stack.size() - 6 );
			body_stack.back() = std::move( tmp );
		}

		/**
		 * Function to construct and stack an exists and for_all chr statement
		 * @param p The position of element in source
		 */
		void exists_forall(std::string_view name, PositionInfo p )
		{
			assert( body_stack.size() >= 4 );

			// Convert first expression to CppVariable
			cast_identifier_to_var( body_stack.size() - 4 );
			ast::PtrExpression ptr_tmp;
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 4 ) );
				ptr_tmp.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching local variable identifier", body_stack.at( body_stack.size() - 4 )->position() );
			}
			std::unique_ptr< ast::CppVariable > cpp_variable;
			try {
				ast::CppVariable& s = dynamic_cast< ast::CppVariable& >( *ptr_tmp );
				ptr_tmp.release();
				cpp_variable.reset(&s);
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching local variable identifier", ptr_tmp->position());
			}

			// Convert second expression (lower bound) to PtrExpression
			check_no_chr_statement( body_stack.at( body_stack.size() - 3 ), false );
			ast::PtrExpression lower_bound;
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 3 ) );
				lower_bound.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching lower bound", body_stack.at( body_stack.size() - 3 )->position());
			}

			// Convert second expression (upper bound) to PtrExpression
			check_no_chr_statement( body_stack.at( body_stack.size() - 2 ), false );
			ast::PtrExpression upper_bound;
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 2 ) );
				upper_bound.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching upper bound", body_stack.at( body_stack.size() - 2 )->position());
			}

			// Create stop condition
			unsigned int n = body_stack.size();
			ast::PtrExpression stop_cond;
			if (name == "exists")
				stop_cond = std::make_unique< ast::InfixExpression >(
					std::string("||"),
						std::make_unique< ast::CppVariable >(
								std::string("__local_success__" + std::to_string(n)),
								upper_bound->position()),
						std::make_unique< ast::InfixExpression >(
							std::string(">"),
							ast::PtrExpression(cpp_variable->clone()),
							ast::PtrExpression(upper_bound->clone()),
							upper_bound->position()),
						upper_bound->position());
			else
				stop_cond = std::make_unique< ast::InfixExpression >(
					std::string("||"),
						std::make_unique< ast::PrefixExpression >(
								std::string("!"),
								std::make_unique< ast::CppVariable >(
										std::string("__local_success__" + std::to_string(n)),
										upper_bound->position()),
								upper_bound->position()),
						std::make_unique< ast::InfixExpression >(
							std::string(">"),
							ast::PtrExpression(cpp_variable->clone()),
							ast::PtrExpression(upper_bound->clone()),
							upper_bound->position()),
						upper_bound->position());

			// Create final_status
			auto final_status = std::make_unique< ast::CppVariable >(
							std::string("__local_success__" + std::to_string(n)),
							upper_bound->position());

			// Create on_succeeded_alt
			auto on_succeeded_alt = std::make_unique< ast::CppDeclAssignment >(
								std::make_unique< ast::CppVariable >(
										std::string("__local_success__" + std::to_string(n)),
										lower_bound->position()),
								std::make_unique< ast::Literal >(
										(name=="exists"?std::string("true"):std::string("false")),
										p),
								p);

			// Create on_failed_alt
			auto on_failed_alt = std::make_unique< ast::CppExpression >(
					std::make_unique< ast::PrefixExpression >(
						std::string("++"),
						ast::PtrExpression(cpp_variable->clone()),
						p),
					p);

			// Create init
			// Lower bound init
			std::unique_ptr< ast::CppExpression > init_lower_bound =
				std::make_unique< ast::CppDeclAssignment >(
						std::make_unique< ast::CppVariable >(
								std::string(cpp_variable->value()),
								lower_bound->position()),
						ast::PtrExpression(lower_bound->clone()),
						lower_bound->position());

			// Local_success init
			std::unique_ptr< ast::CppExpression > init_local_success =
				std::make_unique< ast::CppDeclAssignment >(
						std::make_unique< ast::CppVariable >(
							std::string("__local_success__" + std::to_string(n)),
							lower_bound->position()),
						std::make_unique< ast::Literal >(
							(name=="exists"?std::string("false"):std::string("true")),
							lower_bound->position()),
						lower_bound->position());

			auto body = std::move(body_stack.at( body_stack.size() - 1));
	
			ast::PtrSequence tmp = std::make_unique< ast::Sequence >(std::string(","), p);
			tmp->add_child( std::move(init_lower_bound) );
			tmp->add_child( std::move(init_local_success) );
			if (name == "exists")
				tmp->add_child( 
						std::make_unique< ast::ChrBehavior >(
							std::move( stop_cond ),
							std::move( on_succeeded_alt ),
							std::move( on_failed_alt ),
							std::move( std::move(final_status) ),
							std::make_unique< ast::Body >(p),
							std::make_unique< ast::Body >(p),
							std::move(body),
							p) );
			else
				tmp->add_child( 
						std::make_unique< ast::ChrBehavior >(
							std::move( stop_cond ),
							std::move( on_failed_alt ), // SWAP 
							std::move( on_succeeded_alt ), // SWAP
							std::move( std::move(final_status) ),
							std::make_unique< ast::Body >(p),
							std::make_unique< ast::Body >(p),
							std::move(body),
							p) );
	
			body_stack.resize( body_stack.size() - 3 );
			body_stack.back() = std::move( tmp );
		}

		/**
		 * Function to construct and stack an exists and for_all (on container)
		 * chr statement
		 * @param p The position of element in source
		 */
		void exists_forall_it(std::string_view name, PositionInfo p )
		{
			assert( body_stack.size() >= 3 );

			// Convert first expression to CppVariable
			cast_identifier_to_var( body_stack.size() - 3 );
			ast::PtrExpression ptr_tmp;
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 3 ) );
				ptr_tmp.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching local variable identifier", body_stack.at( body_stack.size() - 3 )->position() );
			}
			std::unique_ptr< ast::CppVariable > cpp_variable;
			try {
				ast::CppVariable& s = dynamic_cast< ast::CppVariable& >( *ptr_tmp );
				ptr_tmp.release();
				cpp_variable.reset(&s);
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching local variable identifier", ptr_tmp->position());
			}
			std::string s_it = std::string("__it_") + std::string(cpp_variable->value());
			std::string s_it_end = std::string("__it_end_") + std::string(cpp_variable->value());

			// Convert second expression (container) to PtrExpression
			check_no_chr_statement( body_stack.at( body_stack.size() - 2 ), false );
			ast::PtrExpression container;
			try {
				ast::CppExpression& s = dynamic_cast< ast::CppExpression& >( *body_stack.at( body_stack.size() - 2 ) );
				container.swap(s.expression());
			} catch (std::bad_cast&) {
				throw ParseError("parse error matching container expression", body_stack.at( body_stack.size() - 2 )->position());
			}

			// Create stop condition
			unsigned int n = body_stack.size();
			ast::PtrExpression stop_cond;
			if (name == "exists")
				stop_cond = std::make_unique< ast::InfixExpression >(
					std::string("||"),
						std::make_unique< ast::CppVariable >(
								std::string("__local_success__" + std::to_string(n)),
								container->position()),
						std::make_unique< ast::InfixExpression >(
								std::string("=="),
								std::make_unique< ast::CppVariable >(
											s_it,
											container->position()),
								std::make_unique< ast::CppVariable >(
											s_it_end,
											container->position()),
							container->position()),
						container->position());
			else
				stop_cond = std::make_unique< ast::InfixExpression >(
					std::string("||"),
						std::make_unique< ast::PrefixExpression >(
								std::string("!"),
								std::make_unique< ast::CppVariable >(
										std::string("__local_success__" + std::to_string(n)),
										container->position()),
								container->position()),
						std::make_unique< ast::InfixExpression >(
								std::string("=="),
								std::make_unique< ast::CppVariable >(
											s_it,
											container->position()),
								std::make_unique< ast::CppVariable >(
											s_it_end,
											container->position()),
							container->position()),
						container->position());

			// Create final_status
			ast::PtrExpression final_status = std::make_unique< ast::CppVariable >(
							std::string("__local_success__" + std::to_string(n)),
							container->position());

			// Create on_succeeded_alt
			auto on_succeeded_alt = std::make_unique< ast::CppDeclAssignment >(
					std::make_unique< ast::CppVariable >(
						std::string("__local_success__" + std::to_string(n)),
						container->position()),
					std::make_unique< ast::Literal >(
						(name=="exists"?std::string("true"):std::string("false")),
						p),
					p);

			// Create on_failed_alt
			auto on_failed_alt = std::make_unique<ast::CppExpression>(
					std::make_unique< ast::PrefixExpression >(
						std::string("++"),
						std::make_unique< ast::CppVariable >(
							s_it,
							container->position()),
						p),
					p);

			// Create init
			std::vector< ast::PtrExpression > empty_vector;
			// iterator begin
			ast::PtrExpression container_begin =
				std::make_unique< ast::InfixExpression >(
						std::string("."),
						ast::PtrExpression(container->clone()),
						std::make_unique< ast::BuiltinConstraint >(
								std::make_unique< ast::Identifier >(
									std::string("begin"),
									container->position()),
								std::string("("), std::string(")"),
								empty_vector,
								container->position()),
						container->position());

			// iterator end
			ast::PtrExpression container_end =
				std::make_unique< ast::InfixExpression >(
						std::string("."),
						ast::PtrExpression(container->clone()),
						std::make_unique< ast::BuiltinConstraint >(
								std::make_unique< ast::Identifier >(
									std::string("end"),
									container->position()),
								std::string("("), std::string(")"),
								empty_vector,
								container->position()),
						container->position());

			// iterator init begin
			std::unique_ptr< ast::CppExpression > init_it =
				std::make_unique< ast::CppDeclAssignment >(
						std::make_unique< ast::CppVariable >(
							s_it,
							container->position()),
						std::move( container_begin ),
						container->position());

			// iterator init end
			std::unique_ptr< ast::CppExpression > init_it_end =
				std::make_unique< ast::CppDeclAssignment >(
						std::make_unique< ast::CppVariable >(
							s_it_end,
							container->position()),
						std::move( container_end ),
						container->position());

			// Local_success init
			std::unique_ptr< ast::CppExpression > init_local_success =
				std::make_unique< ast::CppDeclAssignment >(
						std::make_unique< ast::CppVariable >(
							std::string("__local_success__" + std::to_string(n)),
							container->position()),
						std::make_unique< ast::Literal >(
							(name=="exists"?std::string("false"):std::string("true")),
							container->position()),
						container->position());

			// Body sequence with ref
			std::unique_ptr< ast::CppDeclAssignment > decl_ass =
				std::make_unique< ast::CppDeclAssignment >(
						std::unique_ptr<ast::CppVariable>( static_cast<ast::CppVariable*>(cpp_variable->clone()) ),
						std::make_unique< ast::PrefixExpression >(
								std::string("*"),
								std::make_unique< ast::CppVariable >(
									s_it,
									container->position()),
								container->position()),
						container->position());
			auto body = std::move(body_stack.at( body_stack.size() - 1));

			auto body_sequence = std::make_unique< ast::Sequence >(std::string(","), p);
			body_sequence->add_child( std::move(decl_ass) );
			auto ptr_body = dynamic_cast<ast::Sequence*>( body.get() );
            if ((ptr_body != nullptr) and (ptr_body->op() == ","))
			{
				for (auto& child : ptr_body->children())
					body_sequence->add_child( std::move(child) );
			} else
				body_sequence->add_child( std::move(body) );
	
			ast::PtrSequence tmp = std::make_unique< ast::Sequence >(std::string(","), p);
			tmp->add_child( std::move(init_it) );
			tmp->add_child( std::move(init_it_end) );
			tmp->add_child( std::move(init_local_success) );

			if (name == "exists")
				tmp->add_child(std::make_unique< ast::ChrBehavior >(
							std::move( stop_cond ),
							std::move(on_succeeded_alt),
							std::move(on_failed_alt),
							std::move( std::move(final_status) ),
							std::make_unique< ast::Body >(p),
							std::make_unique< ast::Body >(p),
							std::move(body_sequence),
							p) );
			else
				tmp->add_child(std::make_unique< ast::ChrBehavior >(
							std::move( stop_cond ),
							std::move(on_failed_alt), // SWAP
							std::move(on_succeeded_alt), //SWAP
							std::move( std::move(final_status) ),
							std::make_unique< ast::Body >(p),
							std::make_unique< ast::Body >(p),
							std::move(body_sequence),
							p) );
	
			body_stack.resize( body_stack.size() - 2 );
			body_stack.back() = std::move( tmp );
		}

		/**
		 * Function to convert a body reduced to an identifier
		 * to a variable (CppVariable or LogicalVariable)
		 * @param i The index of body in stack
		 */
		void cast_identifier_to_var( std::size_t i )
		{
			ast::CppExpression* p_exp = dynamic_cast< ast::CppExpression* >(body_stack.at(i).get());
			if (p_exp != nullptr)
			{
				ast::Identifier* p_id = dynamic_cast< ast::Identifier* >( p_exp->expression().get() );
				if (p_id != nullptr)
				{
					if (p_id->value()[0] == '_')
						throw ParseError("parse error, a variable cannot start with '_'", p_id->position());
					else if (isupper(p_id->value()[0]))
						p_exp->expression() = std::make_unique<ast::LogicalVariable>(p_id->value(),p_id->position());
					else
						p_exp->expression() = std::make_unique<ast::CppVariable>(p_id->value(),p_id->position());
				}
			}
		}

		std::string last_op;					///< The last operator used
		std::vector< ast::PtrBody > body_stack;	///< The stack of body parts

		std::vector< ast::PtrSharedChrConstraintDecl >& chr_constraints;	///< Reference to the recorded CHR constraints
	};
} // namespace chr::compiler::parser::ast_builder
