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

#include <ast/program.hh>
#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	std::string_view ExpressionCppCode::string_from(ast::Expression& e)
	{
		string_stack.clear();
		e.accept( *this );
		return string_stack.at( 0 );
	}

	void ExpressionCppCode::visit(ast::Identifier& e)
	{
		string_stack.emplace_back( e.value() );
	}

	void ExpressionCppCode::visit(ast::CppVariable& e)
	{
		string_stack.emplace_back( e.value() );
	}

	void ExpressionCppCode::visit(ast::LogicalVariable& e)
	{
		string_stack.emplace_back( e.value() );
	}

	void ExpressionCppCode::visit(ast::Literal& e)
	{
		string_stack.emplace_back( e.value() );
	}

	void ExpressionCppCode::visit(ast::PrefixExpression& e)
	{
		e.child()->accept(*this);
		assert( string_stack.size() >= 1 );

		std::string tmp = "(" +  std::string( e.op() ) + string_stack.at( string_stack.size() - 1 ) + " )";
		string_stack.back() = std::move( tmp );
	}

	void ExpressionCppCode::visit(ast::PostfixExpression& e)
	{
		e.child()->accept(*this);
		assert( string_stack.size() >= 1 );
	
		std::string tmp = "( " + string_stack.at( string_stack.size() - 1 ) + std::string( e.op() ) + " )";
		string_stack.back() = std::move( tmp );
	}

	void ExpressionCppCode::visit(ast::InfixExpression& e)
	{
		e.l_child()->accept(*this);
		e.r_child()->accept(*this);
		assert( string_stack.size() >= 1 );

		std::string tmp;
		if (e.op() == ".")
			tmp = string_stack.at( string_stack.size() - 2 ) + " " + std::string( e.op() ) + " " + string_stack.at( string_stack.size() - 1 );
		else
			tmp = "( " + string_stack.at( string_stack.size() - 2 ) + " " + std::string( e.op() ) + " " + string_stack.at( string_stack.size() - 1 ) + " )";
		string_stack.pop_back();
		string_stack.back() = std::move( tmp );
	}

	void ExpressionCppCode::visit(ast::TernaryExpression& e)
	{
		e.child1()->accept(*this);
		e.child2()->accept(*this);
		e.child3()->accept(*this);
		assert( string_stack.size() >= 2 );

		std::string tmp = "( " + string_stack.at( string_stack.size() - 3 ) + " " + std::string( e.op1() ) + " " + string_stack.at( string_stack.size() - 2 ) + " " + std::string( e.op2() ) + " " + string_stack.at( string_stack.size() - 1 ) + " )";
		string_stack.pop_back();
		string_stack.pop_back();
		string_stack.back() = std::move( tmp );
	}

	void ExpressionCppCode::visit(ast::BuiltinConstraint& e)
	{
		std::size_t args = e.children().size();

		e.name()->accept(*this);
		for( std::size_t i = 0; i < args; ++i )
			e.children()[i]->accept(*this);
		assert( string_stack.size() > args );

		std::string tmp = *( string_stack.end() - args - 1 ) + std::string( e.l_delim() ) + " ";
		for( std::size_t i = 0; i < args; ++i ) {
			if( i > 0 ) {
				tmp += ", ";
			}
			tmp += *( string_stack.end() - args + i );
		}
		tmp += " " + std::string( e.r_delim() );
		string_stack.resize( string_stack.size() - args );
		string_stack.back() = std::move( tmp );
	}

	void ExpressionCppCode::visit(ast::ChrConstraint& e)
	{
		std::size_t args = e.children().size();

		e.name()->accept(*this);
		for( std::size_t i = 0; i < args; ++i )
			e.children()[i]->accept(*this);
		assert( string_stack.size() > args );

		std::string tmp = *( string_stack.end() - args - 1 ) + "( ";
		for( std::size_t i = 0; i < args; ++i ) {
			if( i > 0 ) {
				tmp += ", ";
			}
			tmp += *( string_stack.end() - args + i );
		}
		tmp += " )";
		string_stack.resize( string_stack.size() - args );
		string_stack.back() = std::move( tmp );
	}

	void ExpressionCppCode::visit(ast::ChrCount& e)
	{
		std::string tmp;
		auto& pc = e.constraint();
		auto c_name = e.constraint()->name()->value();
		if (e.use_index() == -1)
		{
			tmp += c_name + "_constraint_store->size()";
		} else {
			auto index = pc->decl()->_indexes[e.use_index()];
			tmp += c_name + "_constraint_store->size<" + std::to_string(e.use_index()) + ">(";
			bool first = true;
			for (auto x : index)
			{
				pc->children()[x]->accept(*this);
				assert( string_stack.size() >= 1 );
				tmp += (first?"":",") + string_stack.at( string_stack.size() - 1 );
				string_stack.pop_back();
				first = false;
			}
			tmp += ")";
		}
		string_stack.emplace_back( std::move( tmp ) );
	}
}
