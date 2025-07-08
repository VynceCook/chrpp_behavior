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
	BodyAbstractCode::BodyAbstractCode(std::ostream& os)
		: _os(os)
	{ }

	void BodyAbstractCode::generate(ast::Body& e, std::size_t prefix_w, std::string active_constraint_name, bool last_statement, std::unordered_map<std::string,std::tuple<ast::PtrSharedChrConstraintDecl,int>> ctxt_logical_var, std::unordered_set<std::string> ctxt_local_var)
	{
		_context._logical_variables = ctxt_logical_var;
		_context._local_variables = ctxt_local_var;
		_prefix_w = prefix_w;
		_depth = 0;
		_num_sequence = 0;
		_active_constraint_name = active_constraint_name;
		_last_statement = last_statement;

		// Update context
		ExpressionPrint ve;
		// For all new logical variable search and add a declaration
		BodyExpressionApply vb;
		std::unordered_map<std::string, std::tuple<ast::PtrSharedChrConstraintDecl,int>> new_lv;
		auto f_body = [](ast::Body&) { return true; };
		auto f_expression = [&](ast::Expression& e) {
			auto pChr = dynamic_cast< ast::ChrConstraint* >(&e);
			if (pChr != nullptr) {
				std::size_t args = pChr->children().size();
				// Update context for each logical variable of the children
				for( std::size_t i = 0; i < args; ++i )
				{
					auto pLV = dynamic_cast< ast::LogicalVariable* >(pChr->children()[i].get());
					if (pLV != nullptr) {
						auto str = std::string(ve.string_from(*pChr->children()[i]));
						if (ctxt_logical_var.find(str) == ctxt_logical_var.end())
						{
							auto it = new_lv.insert({str,std::make_tuple(pChr->decl(),i)});
							if (!it.second && (std::get<1>((*it.first).second) == -1))
								(*it.first).second = std::make_tuple(pChr->decl(),i);
						}
					}
				}
			}
			auto pLV = dynamic_cast< ast::LogicalVariable* >(&e);
			if (pLV != nullptr) {
				auto str = std::string(ve.string_from(e));
				if (ctxt_local_var.find(str) == ctxt_local_var.end())
					new_lv.insert({str,std::make_tuple(ast::PtrSharedChrConstraintDecl(),-1)});
			}
			return true;
		};
		vb.apply(e,f_body,f_expression);
		
		// Add new declarations to the previous ones
		for (const auto& x : new_lv)
			_context._logical_variables.insert(x);

		e.accept( *this );
	}

	std::string BodyAbstractCode::ltrim(std::string_view s) {
		std::string s2(s);
		s2.erase(s2.begin(), std::find_if(s2.begin(), s2.end(), [](unsigned char ch) {
					return !std::isspace(ch);
					}));
		return s2;
	}

	std::string BodyAbstractCode::prefix()
	{
		return std::string(_prefix_w + _depth, '\t');
	}

	void BodyAbstractCode::visit(ast::Body&)
	{ }

	void BodyAbstractCode::visit(ast::ChrKeyword& k)
	{
		_os << prefix() << k.name();
	}

	void BodyAbstractCode::visit(ast::CppExpression& e)
	{
		chr::compiler::visitor::ExpressionPrint v;
		_os << prefix() << v.string_from( *e.expression() );
		auto pragmas = e.pragmas();
		std::size_t n_pragmas = pragmas.size();
		if (n_pragmas > 0)
		{
			_os <<  " # ";
			if (n_pragmas > 1)
				_os << "{ ";
			for( std::size_t i = 0; i < n_pragmas; ++i ) {
				if( i > 0 )
					_os << ", ";
				_os << StrPragma.at( (int)pragmas[i] );
			}
			if (n_pragmas > 1)
				_os << " }";
		}
	}

	void BodyAbstractCode::visit(ast::CppDeclAssignment& a)
	{
		chr::compiler::visitor::ExpressionPrint v;
		_os << prefix() << v.string_from( *a.variable() );
		_os << " = ";
		_os << v.string_from( *a.expression() );
	}

	void BodyAbstractCode::visit(ast::Unification& a)
	{
		chr::compiler::visitor::ExpressionPrint v;
		_os << prefix() << v.string_from( *a.variable() );
		_os << " %= ";
		_os << v.string_from( *a.expression() );
	}

	void BodyAbstractCode::visit(ast::ChrConstraintCall& c)
	{
		chr::compiler::visitor::ExpressionPrint v;
		_os << prefix() << v.string_from( *c.constraint() );
	}

	void BodyAbstractCode::visit(ast::Sequence& s)
	{
		std::size_t args = s.children().size();
		assert(args > 1);
		if (s.op() == ",")
		{
			for( std::size_t i = 0; i < args; ++i )
			{
				s.children()[i]->accept(*this);
				if (i < args-1)
					_os << "\n";
			}
		} else {
			assert(s.op() == ";");
			std::function< void (std::size_t) > f = [&](std::size_t i) {
				if (i == 0) 
				{
					_os << prefix() << "_or_" << _num_sequence << "_" << i << " <-- Try\n";
					++_depth;
					s.children()[i]->accept(*this);
					_os << "\n";
					--_depth;
					_os << prefix() << "End try\n";
				} else if (i < args-1)
				{
					_os << prefix() << "If _or_" << _num_sequence << "_" << i-1 << " is failure\n";
					++_depth;
					_os << prefix() << "_or_" << _num_sequence << "_" << i << " <-- Try\n";
					++_depth;
					s.children()[i]->accept(*this);
					_os << "\n";
					--_depth;
					_os << prefix() << "End try\n";
					f(i+1);
					--_depth;
					if (i == 1)
						_os << prefix() << "End if";
					else
						_os << prefix() << "End if\n";
				} else {
					_os << prefix() << "If _or_" << _num_sequence << "_" << i-1 << " is failure\n";
					++_depth;
					s.children()[i]->accept(*this);
					_os << "\n";
					--_depth;
					if (i == 1)
						_os << prefix() << "End if";
					else
						_os << prefix() << "End if\n";
				}
			};
			f(0);
			f(1);
			++_num_sequence;
		}
	}

	void BodyAbstractCode::visit(ast::ChrBehavior& b)
	{
		chr::compiler::visitor::ExpressionPrint v;
		std::string str;

		_os << prefix() << "Behavior\n";
		++_depth;
		_os << prefix() << "While (" << ltrim(v.string_from( *b.stop_cond() )) << ")\n";

		++_depth;
		_os << prefix() << "_ret_beha_ <-- Try\n";
		++_depth;
		b.behavior_body()->accept(*this);
		_os << "\n";
		--_depth;
		_os << prefix() << "End try\n";

		_os << prefix() << "If _ret_beha_ is success\n";
		++_depth;
		b.on_succeeded_alt()->accept(*this);
		_os << "\n";
		--_depth;
		_os << prefix() << "Else\n";
		++_depth;
		b.on_failed_alt()->accept(*this);
		_os << "\n";
		--_depth;
		_os << prefix() << "End if\n";
		--_depth;
		_os << prefix() << "End while\n";
	
		_os << prefix() << "If (" << ltrim(v.string_from( *b.final_status() )) << ")\n";
		++_depth;
		b.on_succeeded_status()->accept(*this);
		_os << "\n";
		--_depth;
		_os << prefix() << "Else\n";
		++_depth;
		b.on_failed_status()->accept(*this);
		_os << "\n";
		--_depth;
		_os << prefix() << "End if\n";
		--_depth;
		_os << prefix() << "End behavior";
	}

	void BodyAbstractCode::visit(ast::ChrTry& t)
	{
		chr::compiler::visitor::ExpressionPrint v;
		_os << prefix() << v.string_from( *t.variable() ) << " <-- Try" << (t.do_backtrack()?"_bt":"") << "\n";
		++_depth;
		t.body()->accept(*this);
		_os << "\n";
		--_depth;
		_os << prefix() << "End try" << (t.do_backtrack()?"_bt":"");
	}

}
