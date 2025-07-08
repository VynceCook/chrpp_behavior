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
	std::string_view BodyFullPrint::string_from(ast::Body& e, std::size_t prefix)
	{
		string_stack.clear();
		_prefix_w = prefix;
		_depth = 0;
		e.accept( *this );
		return string_stack.at( 0 );
	}
	
	std::string BodyFullPrint::prefix()
	{
		return std::string(_prefix_w + 4 * ((_depth>0)?((int)_depth-1):0), ' ');
	}

	void BodyFullPrint::visit(ast::Body&)
	{
		string_stack.emplace_back( prefix() );
	}

	void BodyFullPrint::visit(ast::ChrKeyword& k)
	{
		string_stack.emplace_back( prefix() + std::string(k.name()) );
	}

	void BodyFullPrint::visit(ast::CppExpression& e)
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
		string_stack.emplace_back( prefix() + str );
	}

	void BodyFullPrint::visit(ast::CppDeclAssignment& a)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *a.variable() ));
		str += " <- ";
		str += std::string(v.string_from( *a.expression() ));
		string_stack.emplace_back( prefix() + str );
	}

	void BodyFullPrint::visit(ast::Unification& a)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *a.variable() ));
		str += " %= ";
		str += std::string(v.string_from( *a.expression() ));
		string_stack.emplace_back( prefix() + str );
	}

	void BodyFullPrint::visit(ast::ChrConstraintCall& c)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto str = std::string(v.string_from( *c.constraint() ));
		string_stack.emplace_back( prefix() + str );
	}

	void BodyFullPrint::visit(ast::Sequence& s)
	{
		std::size_t args = s.children().size();

		++_depth;
		for( std::size_t i = 0; i < args; ++i )
			s.children()[i]->accept(*this);
		--_depth;
		assert( string_stack.size() >= args );

		std::string tmp;
		for( std::size_t i = 0; i < args; ++i ) {
			tmp += *( string_stack.end() - args + i );
			if( i < (args-1) ) {
				tmp += std::string(s.op()) + "\n";
			}
		}
		string_stack.resize( string_stack.size() - args + 1 );
		string_stack.back() = std::move( tmp );
	}

	void BodyFullPrint::visit(ast::ChrBehavior& b)
	{
		chr::compiler::visitor::ExpressionPrint v;
		auto s_stop_cond = std::string(v.string_from( *b.stop_cond() ));
		auto s_final_status = std::string(v.string_from( *b.final_status() ));

		++_depth;
		b.on_succeeded_alt()->accept(*this);
		b.on_failed_alt()->accept(*this);
		b.on_succeeded_status()->accept(*this);
		b.on_failed_status()->accept(*this);
		b.behavior_body()->accept(*this);
		--_depth;
		assert( string_stack.size() >= 5 );

		auto s_on_succeeded_alt = *( string_stack.end() - 5 );
		auto s_on_failed_alt = *( string_stack.end() - 4 );
		auto s_on_succeeded_status = *( string_stack.end() - 3 );
		auto s_on_failed_status = *( string_stack.end() - 2 );
		auto s_behavior_body = *( string_stack.end() - 1 );

		std::string tmp = prefix() + "behavior(\n";
		++_depth;
		tmp += prefix() + s_stop_cond + ",\n";
		--_depth;
		tmp += s_on_succeeded_alt + ",\n";
		tmp += s_on_failed_alt + ",\n";
		++_depth;
		tmp += prefix() + s_final_status + ",\n";
		--_depth;
		tmp += s_on_succeeded_status + ",\n";
		tmp += s_on_failed_status + ", (\n";
		tmp += s_behavior_body + "\n";
		tmp += prefix() + ") )";
		string_stack.resize( string_stack.size() - 4 );
		string_stack.back() = std::move( tmp );
	}

	void BodyFullPrint::visit(ast::ChrTry& t)
	{
		++_depth;
		t.body()->accept(*this);
		--_depth;
		auto str = std::string( prefix() + (t.do_backtrack()?"try_bt( ":"try( "));

		chr::compiler::visitor::ExpressionPrint v;
		str += std::string(v.string_from( *t.variable() ));

		str += ", (\n";
		str += *(string_stack.end() - 1) + "\n";
		str += prefix() + ") )";
		string_stack.back() = std::move(str);
	}
}
