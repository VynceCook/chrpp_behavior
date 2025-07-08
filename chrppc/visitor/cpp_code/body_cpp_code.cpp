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
#include <visitor/body.hh>
#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	BodyCppCode::BodyCppCode(std::ostream& os, std::string_view input_file_name, std::string_view trace_prefix, bool auto_catch_failure)
		: BodyAbstractCode(os), _input_file_name(input_file_name), _trace_prefix(trace_prefix), _auto_catch_failure(auto_catch_failure)
	{ }

	void BodyCppCode::visit(ast::Body&)
	{ }

	template< typename TT, size_t... I >
	void BodyCppCode::loop_to_ostream(const TT& args, std::index_sequence<I...>)
	{
		(..., (_os << (I == 0? "" : ", ") << std::get<I>(args)));
	}

	template< typename... T >
	void BodyCppCode::write_trace_statement(const char* flag, const std::tuple< T... >& args)
	{
		if (chr::compiler::Compiler_options::TRACE)
		{
			_os << prefix() << "TRACE( chr::Log::trace_constraint(chr::Log::" << flag << "," << _trace_prefix << ", std::make_tuple(";
			loop_to_ostream(args, std::make_index_sequence<sizeof...(T)>());
			_os << ")); )\n";
		}
	}

	void BodyCppCode::declare_undeclared_logical_variable(ast::Expression& e0)
	{
		chr::compiler::visitor::ExpressionCppCode v;
		// Declare undeclared logical variables
		chr::compiler::visitor::ExpressionApply vea;
		auto f_expression = [&](ast::Expression& e) {
			auto pLV = dynamic_cast< ast::LogicalVariable* >(&e);
			if (pLV != nullptr) {
				std::string str_e = std::string(v.string_from(e));
				auto it = _context._logical_variables.find(str_e);
				if ((it != _context._logical_variables.end()) && (std::get<1>((*it).second) != -1))
				{
					auto c_name = std::get<0>((*it).second)->_c->constraint()->name()->value();
					auto pt = dynamic_cast< ast::UnaryExpression* >( std::get<0>((*it).second)->_c->constraint()->children()[std::get<1>((*it).second)].get() );
					assert(pt != nullptr);
					if (pt->op() == "+") 
						_os << prefix() << "typename chr::Logical_var<typename std::tuple_element<" << std::get<1>((*it).second)+1 << ",typename " << c_name << "::Type>::type::Value_t> " << str_e << ";\n";
					else
						_os << prefix() << "typename std::tuple_element<" << std::get<1>((*it).second)+1 << ",typename " << c_name << "::Type>::type " << str_e << ";\n";
					std::get<1>((*it).second) = -1; // Set to -1 to prevent for another later declaration
				}
			}
			return true;
		};
		vea.apply(e0,f_expression);
	}

	void BodyCppCode::declare_undeclared_logical_variable(ast::Body& b0)
	{
		// Declare undeclared logical variables
		chr::compiler::visitor::BodyExpressionApply vbea;
		auto f_expression = [&](ast::Expression& e) {
			declare_undeclared_logical_variable(e);
			return true;
		};
		auto f_body = [&](ast::Body&) {
			return true;
		};
		vbea.apply(b0,f_body,f_expression);
	}

	void BodyCppCode::visit(ast::ChrKeyword& k)
	{
		if (Compiler_options::LINE_ERROR)
			_os <<  "#line " << k.position().line << R"_STR( ")_STR" << _input_file_name <<  R"_STR(")_STR" << "\n";

		if (k.name() != "success")
		{
			if (k.name() == "failure") _os << prefix() << "return chr::failure();";
			else if (k.name() == "stop") _os << prefix() << "return chr::ES_CHR::SUCCESS; // stop builtinconstraint";
			else _os << prefix() << k.name();
		}
	}

	void BodyCppCode::visit(ast::CppExpression& e)
	{
		declare_undeclared_logical_variable(*e.expression());

		if (Compiler_options::LINE_ERROR)
			_os <<  "#line " << e.position().line << R"_STR( ")_STR" << _input_file_name <<  R"_STR(")_STR" << "\n";
		chr::compiler::visitor::ExpressionCppCode v;
		auto pragmas = e.pragmas();
		if (std::find(pragmas.begin(), pragmas.end(), Pragma::catch_failure) != pragmas.end())
		{
			_os << prefix() << "if ((" << v.string_from( *e.expression() );
			_os << ") == chr::ES_CHR::FAILURE) return chr::ES_CHR::FAILURE;";
		} else {
			if (_auto_catch_failure)
				_os << prefix() << "CHECK_ES( " << v.string_from( *e.expression() ) << " );";
			else
				_os << prefix() << v.string_from( *e.expression() ) << ";";
		}

		std::size_t n_pragmas = pragmas.size();
		if (n_pragmas > 0)
		{
			_os <<  "// # ";
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

	void BodyCppCode::visit(ast::CppDeclAssignment& a)
	{
		declare_undeclared_logical_variable(*a.expression());

		if (Compiler_options::LINE_ERROR)
			_os <<  "#line " << a.position().line << R"_STR( ")_STR" << _input_file_name <<  R"_STR(")_STR" << "\n";

		chr::compiler::visitor::ExpressionCppCode v;
		std::string v_name( v.string_from(*a.variable()) );
		auto pragmas = a.pragmas();
		bool catch_f = false;
		if (std::find(pragmas.begin(), pragmas.end(), Pragma::catch_failure) != pragmas.end())
			catch_f = true;

		_os << prefix() << (catch_f?"if ((":"");
		if (_context._local_variables.find(v_name) == _context._local_variables.end())
		{
			_context._local_variables.insert(v_name);
				_os << "auto ";
		}
		_os << v_name << " = " << v.string_from( *a.expression() );
		if (catch_f)
			_os << ") == chr::ES_CHR::FAILURE) return chr::ES_CHR::FAILURE"; 
		_os << ";";
	}

	void BodyCppCode::visit(ast::Unification& a)
	{
		declare_undeclared_logical_variable(*a.variable());
		declare_undeclared_logical_variable(*a.expression());

		if (Compiler_options::LINE_ERROR)
			_os <<  "#line " << a.position().line << R"_STR( ")_STR" << _input_file_name <<  R"_STR(")_STR" << "\n";

		chr::compiler::visitor::ExpressionCppCode v;
		auto pragmas = a.pragmas();
		bool catch_f = false;
		if (_auto_catch_failure || (std::find(pragmas.begin(), pragmas.end(), Pragma::catch_failure) != pragmas.end()))
			catch_f = true;

		_os << prefix() << (catch_f?"if ((":"") << v.string_from( *a.variable() );
		_os << " %= ";
		_os << v.string_from( *a.expression() );
		if (catch_f)
			_os << ") == chr::ES_CHR::FAILURE) return chr::ES_CHR::FAILURE";
		_os << ";";
	}

	void BodyCppCode::visit(ast::ChrConstraintCall& c)
	{
		declare_undeclared_logical_variable(*c.constraint());

		if (Compiler_options::LINE_ERROR)
			_os <<  "#line " << c.position().line << R"_STR( ")_STR" << _input_file_name <<  R"_STR(")_STR" << "\n";

		chr::compiler::visitor::ExpressionCppCode v;
		auto c_name = std::string( c.constraint()->name()->value() );
		if (_last_statement && (c_name == _active_constraint_name))
		{
			bool any_not_ground_param = false;
			for (const auto& param : c.constraint()->decl()->_c->constraint()->children())
			{
				auto pt = dynamic_cast< ast::UnaryExpression* >( param.get() );
				assert(pt != nullptr);
				if (pt->op() != "+") 
				{
					any_not_ground_param = true;
					break;
				}
			}

			if (any_not_ground_param)
				_os << prefix() << "c_args = std::make_tuple(next_free_constraint_id++";
			else
				_os << prefix() << "c_args = std::forward_as_tuple(next_free_constraint_id++";

			for(auto& cc : c.constraint()->children())
				_os << ", " << v.string_from( *cc );

			_os << ");\n";
			if (!c.constraint()->decl()->_never_stored)
					_os << prefix() << "c_stored_before = false;\n";
			_os << prefix() << "goto " << c_name << "_call;\n";
		} else {
			_os << prefix() << "if (chr::ES_CHR::FAILURE == ";
			_os << v.string_from( *c.constraint() ) << ") return chr::ES_CHR::FAILURE;";
		}
	}

	void BodyCppCode::visit(ast::Sequence& s)
	{
		std::size_t args = s.children().size();
		assert(args > 1);
		if (s.op() == ",")
		{
			bool cur_last_statement = _last_statement;
			_last_statement = false;
			for( std::size_t i = 0; i < args; ++i )
			{
				if (cur_last_statement && (i == args-1))
					_last_statement = true;
				s.children()[i]->accept(*this);
				if (i < args-1)
					_os << "\n";
			}
		} else {
			assert(s.op() == ";");
			bool cur_last_statement = _last_statement;
			_last_statement = false;
			unsigned int id = _depth+1;
			auto context_backup = _context;
			std::function< void (std::size_t) > f = [&](std::size_t i) {
				if (i == 0) 
				{
					_os << prefix() << "unsigned int depth" << id << " = chr::Backtrack::depth();\n";
					_os << prefix() << "chr::Statistics::open_choice();\n";
					write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Create new node at depth ")_STR","chr::Backtrack::depth()+1"));
					_os << prefix() << "auto _try_or_" << id << "_" << i << " = [&]() {\n";
					++_depth;
					_os << prefix() << "chr::Backtrack::inc_backtrack_depth();\n";
					write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Try alternative 0 at depth ")_STR","chr::Backtrack::depth()"));
					s.children()[i]->accept(*this);
					_os << "\n";
					_os << prefix() << "return chr::ES_CHR::SUCCESS;\n";
					--_depth;
					_os << prefix() << "};\n";
				} else if (i < args-1)
				{
					_context = context_backup;
					_os << prefix() << "if (_try_or_" << id << "_" << i-1 <<"() == chr::ES_CHR::FAILURE) {\n";
					++_depth;
					_os << prefix() << "chr::reset();\n";
					_os << prefix() << "chr::Backtrack::back_to(depth" << id << ");\n";
					_os << prefix() << "auto _try_or_" << id << "_" << i << " = [&]() {\n";
					++_depth;
					_os << prefix() << "chr::Backtrack::inc_backtrack_depth();\n";
					write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Try alternative )_STR"+std::to_string(i)+R"_STR( at depth ")_STR","chr::Backtrack::depth()"));
					if (cur_last_statement && (i == args-1))
						_last_statement = true;
					s.children()[i]->accept(*this);
					_os << "\n";
					_os << prefix() << "return chr::ES_CHR::SUCCESS;\n";
					--_depth;
					_os << prefix() << "};\n";
					f(i+1);
					--_depth;
					_os << prefix() << "}\n";
				} else {
					_context = context_backup;
					_os << prefix() << "if (_try_or_" << id << "_" << i-1 <<"() == chr::ES_CHR::FAILURE) {\n";
					++_depth;
					_os << prefix() << "chr::reset();\n";
					_os << prefix() << "chr::Backtrack::back_to(depth" << id << ");\n";
					_os << prefix() << "chr::Backtrack::inc_backtrack_depth();\n";
					write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Try alternative )_STR"+std::to_string(i)+R"_STR( at depth ")_STR","chr::Backtrack::depth()"));
					s.children()[i]->accept(*this);
					_os << "\n";
					--_depth;
					_os << prefix() << "}\n";
					_os << prefix() << "chr::Statistics::close_choice();\n";
					write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Close node at depth ")_STR","chr::Backtrack::depth()"));
				}
			};
			declare_undeclared_logical_variable(s);
			_os << prefix() << "{\n";
			++_depth;
			f(0);
			f(1);
			--_depth;
			_os << prefix() << "}\n";
			++_num_sequence;
		}
	}

	void BodyCppCode::visit(ast::ChrBehavior& b)
	{
		chr::compiler::visitor::ExpressionCppCode v;
		chr::compiler::visitor::BodyEmpty vbe;
		std::string str;

		_last_statement = false;
		unsigned int id = _depth;

		_os << prefix() << "{\n";
		++_depth;
		_os << prefix() << "unsigned int depth" << id << " = chr::Backtrack::depth();\n";
		write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Create new node at depth ")_STR","chr::Backtrack::depth()+1"));
		_os << prefix() << "chr::Statistics::open_choice();\n";
		_os << prefix() << "while (!" << ltrim(v.string_from( *b.stop_cond() )) << ") {\n";
		++_depth;
		_os << prefix() << "chr::reset();\n";
		_os << prefix() << "if (depth" << id << " != chr::Backtrack::depth()) chr::Backtrack::back_to(depth" << id << ");\n";
		_os << prefix() << "chr::Backtrack::inc_backtrack_depth();\n";
		_os << prefix() << "auto _try_beha_" << id << "_ = [&]() {\n";
		++_depth;
		write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Try alternative at depth ")_STR","chr::Backtrack::depth()"));
		b.behavior_body()->accept(*this);
		_os << "\n";
		_os << prefix() << "return chr::ES_CHR::SUCCESS;\n";
		--_depth;
		_os << prefix() << "};\n";

		if (!vbe.is_empty(*b.on_succeeded_alt()) || !vbe.is_empty(*b.on_failed_alt()))
		{
			if (vbe.is_empty(*b.on_succeeded_alt()))
			{
				_os << prefix() << "if (_try_beha_" << id << "_() != chr::ES_CHR::SUCCESS) {\n";
				++_depth;
				b.on_failed_alt()->accept(*this);
				_os << "\n";
				--_depth;
				_os << prefix() << "}\n";
			} else if (vbe.is_empty(*b.on_failed_alt())) {
				_os << prefix() << "if (_try_beha_" << id << "_() == chr::ES_CHR::SUCCESS) {\n";
				++_depth;
				b.on_succeeded_alt()->accept(*this);
				_os << "\n";
				--_depth;
				_os << prefix() << "}\n";
			} else {
				_os << prefix() << "if (_try_beha_" << id << "_() == chr::ES_CHR::SUCCESS) {\n";
				++_depth;
				b.on_succeeded_alt()->accept(*this);
				_os << "\n";
				--_depth;
				_os << prefix() << "} else {\n";
				++_depth;
				b.on_failed_alt()->accept(*this);
				_os << "\n";
				--_depth;
				_os << prefix() << "}\n";
			}
		}
		--_depth;
		_os << prefix() << "}\n";
		_os << prefix() << "chr::Statistics::close_choice();\n";
	
		_os << prefix() << "chr::reset();\n";
		_os << prefix() << "if (" << ltrim(v.string_from( *b.final_status() )) << ") {\n";
		++_depth;
		_os << prefix() << "if (depth" << id << " != chr::Backtrack::depth()) chr::Backtrack::back_to(depth" << id << ");\n";
		b.on_succeeded_status()->accept(*this);
		if (!vbe.is_empty(*b.on_succeeded_status()))
			_os << "\n";
		write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Close node at depth ")_STR","chr::Backtrack::depth()+1"));
		--_depth;
		_os << prefix() << "} else {\n";
		++_depth;
		b.on_failed_status()->accept(*this);
		if (!vbe.is_empty(*b.on_failed_status()))
			_os << "\n";
		_os << prefix() << "return (chr::failed()?chr::ES_CHR::FAILURE:chr::failure());\n";
		--_depth;
		_os << prefix() << "}\n";

		--_depth;
		_os << prefix() << "}\n";
	}

	void BodyCppCode::visit(ast::ChrTry& t)
	{
		chr::compiler::visitor::ExpressionCppCode v;
		auto v_name = v.string_from( *t.variable() );
		_last_statement = false;
		_os << prefix() << "{\n";
		++_depth;
		_os << prefix() << "unsigned int depth" << _depth << " = chr::Backtrack::depth();\n";
		write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Create new node at depth ")_STR","chr::Backtrack::depth()+1"));
		_os << prefix() << "chr::Statistics::open_choice();\n";
		_os << prefix() << "chr::ES_CHR " << v_name << " = [&]() {\n";
		++_depth;
		_os << prefix() << "chr::Backtrack::inc_backtrack_depth();\n";
		t.body()->accept(*this);
		_os << "\n";
		_os << prefix() << "return chr::ES_CHR::SUCCESS;\n";
		--_depth;
		_os << prefix() << "}();\n";
		_os << prefix() << "chr::Statistics::close_choice();\n";
		write_trace_statement("BACKTRACK",std::make_tuple(R"_STR("Close node at depth ")_STR","chr::Backtrack::depth()"));
		if (t.do_backtrack())
			_os << prefix() << "chr::Backtrack::back_to(depth" << _depth << ");\n";
		else {
			_os << prefix() << "if (chr::ES_CHR::FAILURE == " << v_name << ") ";
			_os << "chr::Backtrack::back_to(depth" << _depth << ");\n";
		}
		_os << prefix() << "chr::reset();\n";
		--_depth;
		_os << prefix() << "}\n";
	}

}
