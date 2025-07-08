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

#include <visitor/body.hh>
#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	std::string_view BodyPrint::string_from(ast::Body& e)
	{
		string_stack.clear();
		e.accept( *this );
		return string_stack.at( 0 );
	}

	void BodyPrint::visit(ast::Body&)
	{
		string_stack.emplace_back( std::string() );
	}

	void BodyPrint::visit(ast::ChrKeyword& k)
	{
		string_stack.emplace_back( k.name() );
	}

	void BodyPrint::visit(ast::CppExpression& e)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *e.expression() ));
		auto pragmas = e.pragmas();
		std::size_t n_pragmas = pragmas.size();
		if (n_pragmas > 0)
		{
			str += " # ";
			if (n_pragmas > 1)
				str += "{ ";
			for( std::size_t i = 0; i < n_pragmas; ++i ) {
				if( i > 0 )
					str += ", ";
				str += StrPragma.at( (int)pragmas[i] );
			}
			if (n_pragmas > 1)
				str += " }";
		}
		string_stack.emplace_back( str );
	}

	void BodyPrint::visit(ast::CppDeclAssignment& a)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *a.variable() ));
		str += " = ";
		str += std::string(v.string_from( *a.expression() ));
		string_stack.emplace_back( str );
	}

	void BodyPrint::visit(ast::Unification& a)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *a.variable() ));
		str += " %= ";
		str += std::string(v.string_from( *a.expression() ));
		string_stack.emplace_back( str );
	}

	void BodyPrint::visit(ast::ChrConstraintCall& c)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *c.constraint() ));
		string_stack.emplace_back( str );
	}

	void BodyPrint::visit(ast::Sequence& s)
	{
		std::size_t args = s.children().size();

		for( std::size_t i = 0; i < args; ++i )
			s.children()[i]->accept(*this);
		assert( string_stack.size() >= args );

		std::string tmp = "( ";
		for( std::size_t i = 0; i < args; ++i ) {
			if( i > 0 ) {
				tmp += std::string(s.op()) + " ";
			}
			tmp += *( string_stack.end() - args + i );
		}
		tmp += " )";
		string_stack.resize( string_stack.size() - args + 1 );
		string_stack.back() = std::move( tmp );
	}

	void BodyPrint::visit(ast::ChrBehavior& b)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto s_stop_cond = std::string(v.string_from( *b.stop_cond() ));
		auto s_final_status = std::string(v.string_from( *b.final_status() ));

		b.on_succeeded_alt()->accept(*this);
		b.on_failed_alt()->accept(*this);
		b.on_succeeded_status()->accept(*this);
		b.on_failed_status()->accept(*this);
		b.behavior_body()->accept(*this);
		assert( string_stack.size() >= 5 );

		auto s_on_succeeded_alt = *( string_stack.end() - 5 );
		auto s_on_failed_alt = *( string_stack.end() - 4 );
		auto s_on_succeeded_status = *( string_stack.end() - 3 );
		auto s_on_failed_status = *( string_stack.end() - 2 );
		auto s_behavior_body = *( string_stack.end() - 1 );

		std::string tmp = "behavior( ";
		tmp += s_stop_cond + ", ";
		tmp += s_on_succeeded_alt + ", ";
		tmp += s_on_failed_alt + ", ";
		tmp += s_final_status + ", ";
		tmp += s_on_succeeded_status + ", ";
		tmp += s_on_failed_status + ", ";
		tmp += s_behavior_body + ")";
		string_stack.resize( string_stack.size() - 4 );
		string_stack.back() = std::move( tmp );
	}

	void BodyPrint::visit(ast::ChrTry& t)
	{
		t.body()->accept(*this);
		auto str = std::string( t.do_backtrack()?"try_bt( ":"try( ");

		chr::compiler::visitor::ExpressionPrint v;
		str += std::string(v.string_from( *t.variable() ));

		str += ", ";
		str += *(string_stack.end() - 1);
		str += ")";
		string_stack.back() = std::move(str);
	}

}
