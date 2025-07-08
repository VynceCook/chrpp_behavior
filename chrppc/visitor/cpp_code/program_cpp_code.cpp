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

#include <visitor/program.hh>
#include <visitor/occ_rule.hh>
#include <visitor/body.hh>
#include <visitor/expression.hh>
#include <set>

namespace chr::compiler::visitor
{
	ProgramCppCode::ProgramCppCode(std::ostream& os)
		: ProgramAbstractCode(os)
	{ }

	ProgramCppCode::ProgramCppCode(std::ostream& os_ds, std::ostream& os_rc)
		: ProgramAbstractCode(os_ds,os_rc)
	{ }

	template< typename TT, size_t... I >
	void ProgramCppCode::loop_to_ostream(std::ostream& os, const TT& args, std::index_sequence<I...>)
	{
		(..., (os << (I == 0? "" : ", ") << std::get<I>(args)));
	}

	template< typename... T >
	void ProgramCppCode::write_trace_statement(std::ostream& os, std::string_view c_name, const char* flag, const std::tuple< T... >& args)
	{
		if (chr::compiler::Compiler_options::TRACE)
		{
			os << prefix() << "TRACE( chr::Log::trace_constraint(chr::Log::" << flag << R"_STR(,"",")_STR" << c_name << R"_STR(",-1, std::make_tuple()_STR";
			loop_to_ostream(os, args, std::make_index_sequence<sizeof...(T)>());
			os << ")); )\n";
		}
	}

	void ProgramCppCode::begin_program(ast::ChrProgram& p)
	{
		if (chr::compiler::Compiler_options::TRACE)
			_os_ds << "#define ENABLE_TRACE 1\n";

		if (chr::compiler::Compiler_options::LINE_ERROR)
		{
			_os_ds << prefix() << "#define __CHRPPC_MAJOR__ " << chr::compiler::Compiler_options::CHRPPC_MAJOR << "\n";
			_os_ds << prefix() << "#define __CHRPPC_MINOR__ " << chr::compiler::Compiler_options::CHRPPC_MINOR << "\n";
			_os_ds << prefix() << R"_STR(#line 1 ")_STR" << p.input_file_name() << "\"\n";
		}

		_os_ds << prefix() << "#include <chrpp.hh>\n";
		if (!p.template_parameters().empty())
		{
			_os_ds << prefix() << "template< ";
			bool first = true;
			for (const auto& x : p.template_parameters())
			{
				_os_ds << (first?"":",") << std::get<0>(x) << " " << std::get<1>(x);
				first = false;
			}
			_os_ds << " >\n";
		}
		_os_ds << prefix() << "class " << p.name() << " {\n";
		++_depth;
		_os_ds << prefix() << "unsigned long int next_free_constraint_id = 1;\n";
	}

	void ProgramCppCode::end_program(ast::ChrProgram& p)
	{
		chr::compiler::visitor::ExpressionPrint ve;
		--_depth;
		_os_ds << prefix() << "public:\n";
		++_depth;
		_os_ds << prefix() << "unsigned int _ref_use_count  = 0;///< Count of shared references\n";
		_os_ds << prefix() << "unsigned int _ref_weak_count = 0;///< Count of weak references + (#shared != 0)\n";

		// Members
		for (const auto& x : p.parameters())
			_os_ds << prefix() << std::get<0>(x) << " " << std::get<1>(x) << ";\n";

		// Constructor
		_os_ds << prefix() << p.name() << "(";
		bool first = true;
		for (const auto& x : p.parameters())
		{
			_os_ds << (first?"":",") << std::get<0>(x) << " " << std::get<1>(x) << "0";
			first = false;
		}
		_os_ds << ") ";
		first = true;
		for (const auto& x : p.parameters())
		{
			_os_ds << (first?": ":", ") << std::get<1>(x) << "(" << std::get<1>(x) << "0)";
			first = false;
		}
		_os_ds << "{ }\n";

		// Destructor
		_os_ds << prefix() << "~" << p.name() << "() {\n";
		++_depth;
		for (auto& c : p.chr_constraints())
			if (!c->_never_stored)
			{
				auto c_name = std::string( c->_c->constraint()->name()->value() );
				_os_ds << prefix() << c_name << "_constraint_store.release();\n";
			}
		--_depth;
		_os_ds << prefix() << "}\n";


		// Create member function
		_os_ds << prefix() << "static chr::Shared_obj<" << p.name() << "> create(";
		first = true;
		for (const auto& x : p.parameters())
		{
			_os_ds << (first?"":",") << std::get<0>(x) << " " << std::get<1>(x);
			first = false;
		}
		_os_ds << ") { return chr::make_shared<" << p.name() << ">(";
		first = true;
		for (const auto& x : p.parameters())
		{
			_os_ds << (first?"":",") << std::get<1>(x);
			first = false;
		}
		_os_ds << "); }\n";

		// -----------------------------------------------------------------
		// GENERATE ITERATOR OVER CONSTRAINT STORES
		_os_ds << prefix() << "chr::Constraint_stores_iterator<";
		unsigned int i = 0;
		for (auto& c : p.chr_constraints())
			if (!c->_never_stored)
			{
				auto c_name = std::string( c->_c->constraint()->name()->value() );
				_os_ds << (i==0?"":",") << "chr::Shared_obj<typename " << c_name << "::Constraint_store_t>";
				++i;
			}
		_os_ds << "> chr_store_begin() { ";
		_os_ds << "return chr::Constraint_stores_iterator(";
		i = 0;
		for (auto& c : p.chr_constraints())
			if (!c->_never_stored)
			{
				auto c_name = std::string( c->_c->constraint()->name()->value() );
				_os_ds << (i==0?"":",") << c_name << "_constraint_store";
				++i;
			}
		_os_ds << "); }\n";

		// -----------------------------------------------------------------
		// GENERATE CONSTRAINT STORES GETTER
		for (auto& c : p.chr_constraints())
			if (!c->_never_stored)
			{
				auto c_name = std::string( c->_c->constraint()->name()->value() );
				_os_ds << prefix() << "typename " << c_name << "::Constraint_store_t& get_" << c_name << "_store() { return *" << c_name << "_constraint_store; }\n";
			}

		// -----------------------------------------------------------------
		// GENERATE HISTORY
		std::set< std::pair<unsigned int, unsigned int> > rule_history;
		for (auto& r : p.rules())
		{
			auto pr = dynamic_cast< ast::PropagationRule* >( r.get() );
			if (pr != nullptr)
			{
				unsigned int h_count = 0;
				for (auto& c : pr->head())
					if (std::find(c->pragmas().begin(), c->pragmas().end(), Pragma::no_history) == c->pragmas().end())
						++h_count;
				if (h_count != 0)
					rule_history.insert( std::make_pair(r->id(), h_count) );
			}
		}
		if (!rule_history.empty())
		{
			_os_ds << prefix() << "struct History {\n";
			++_depth;
			for (auto& h : rule_history)
				_os_ds << prefix() << "typename chr::Shared_obj< chr::History_dyn< " << h.second << " > > rule_" << h.first << "{ chr::make_shared< typename chr::History_dyn< " << h.second << " > >() };\n";
			--_depth;
			_os_ds << prefix() << "};\n";
			_os_ds << prefix() << "History _history;\n";
		}

		// -----------------------------------------------------------------
		// GENERATE CONSTRAINT CALLS
		chr::compiler::visitor::ExpressionPrintNoParent venp;
		for (auto& c : p.chr_constraints())
		{
			auto c_name = std::string( c->_c->constraint()->name()->value() );
			// Constraint call function declaration
			if (!c->_never_stored)
				_os_ds << prefix() << "chr::ES_CHR do_" << c_name << "(typename " << c_name << "::Type c_args, typename " << c_name << "::Constraint_store_t::iterator c_it);\n";
			else
				_os_ds << prefix() << "chr::ES_CHR do_" << c_name << "(typename " << c_name << "::Type c_args);\n";

			unsigned int n_arg = 0;
			_os_ds << prefix() << "chr::ES_CHR " << c_name << "(";
			bool first = true;
			for (auto& t : c->_c->constraint()->children())
			{
				auto pt = dynamic_cast< ast::UnaryExpression* >( t.get() );
				assert(pt != nullptr);
				if (pt->op() == "+") _os_ds << (first?"":",") << " chr::Logical_var_ground< ";
				else if (pt->op() == "?") _os_ds <<  (first?"":",") << " chr::Logical_var< ";
				else if (pt->op() == "-") _os_ds <<  (first?"":",") << " chr::Logical_var_mutable< ";
				else assert(false);
				_os_ds << venp.string_from(*pt->child()) << " > arg" << n_arg;
				first=false;
				++n_arg;
			}
			_os_ds << ") {\n";
			++_depth;
			_os_ds << prefix() << "assert(!chr::failed() && (_ref_use_count >= 1));\n";
			_os_ds << prefix() << "auto c_args = std::make_tuple(next_free_constraint_id++";
			for (unsigned int i=0; i < n_arg; ++i)
				_os_ds << ", arg" << i;
			_os_ds << ");\n";
			write_trace_statement(_os_ds, c_name, "GOAL", std::make_tuple(R"_STR("Goal constraint: )_STR" + std::string(c_name) + R"_STR(")_STR", "c_args"));
			if (!c->_never_stored)
				_os_ds << prefix() << "return do_" << c_name << "(std::move(c_args), " << c_name << "_constraint_store->end());\n";
			else
				_os_ds << prefix() << "return do_" << c_name << "(std::move(c_args));\n";
			--_depth;
			_os_ds << prefix() << "}\n";
		}

		--_depth;
		_os_ds << prefix() << "};\n";

		// Include hpp if it is a template C++ class
		if (!p.include_hpp_file().empty())
			_os_ds << prefix() << "#include <" << p.include_hpp_file() << ">\n";
	}

	void ProgramCppCode::constraint_store(ast::ChrProgram& p, unsigned int i)
	{
		chr::compiler::visitor::BodyPrint vb;
		auto& c = p.chr_constraints()[i];
		_os_ds << prefix() << "//(constraint store) " << vb.string_from(*c->_c);
		if (!c->_indexes.empty())
		{
			bool first1 = true;
			_os_ds << ", indexes: {";
			for (auto& index : c->_indexes)
			{
				if (!first1)
					_os_ds <<  ", <";
				else {
					_os_ds <<  " <";
					first1 = false;
				}
				bool first2 = true;
				for (auto& i : index)
					if (!first2)
						_os_ds <<  "," << i;
					else {
						_os_ds << i;
						first2 = false;
					}
				_os_ds << ">";
			}
			_os_ds << " }";
		}
		auto pragmas = c->_c->pragmas();
		std::size_t n_pragmas = pragmas.size();
		if (n_pragmas > 0)
		{
			_os_ds << ", ";
			if (n_pragmas > 1)
				_os_ds << "{ ";
			for( std::size_t i = 0; i < n_pragmas; ++i ) {
				if( i > 0 )
					_os_ds << ", ";
				_os_ds <<  StrPragma.at( (int)pragmas[i] );
			}
			if (n_pragmas > 1)
				_os_ds << " }";
		}
		_os_ds << "\n";

		chr::compiler::visitor::ExpressionPrint ve;
		chr::compiler::visitor::ExpressionPrintNoParent venp;
		std::string_view c_name = c->_c->constraint()->name()->value();
		--_depth;
		_os_ds << prefix() << "public:\n";
		++_depth;
		_os_ds << prefix() << "struct " << c_name << " {\n";
		++_depth;

		// Print constraint type
		_os_ds << prefix() << "using Type = typename std::tuple< unsigned long int";
		for (auto& t : c->_c->constraint()->children())
		{
			auto pt = dynamic_cast< ast::UnaryExpression* >( t.get() );
			assert(pt != nullptr);
			if (pt->op() == "+") _os_ds << ", chr::Logical_var_ground< ";
			else if (pt->op() == "?") _os_ds << ", chr::Logical_var< ";
			else if (pt->op() == "-") _os_ds << ", chr::Logical_var_mutable< ";
			else assert(false);
			_os_ds << venp.string_from(*pt->child()) << " >";
		}
		_os_ds << " >;\n";

		// Exit if store will be never stored
		if (c->_never_stored)
		{
			--_depth;
			_os_ds << prefix() << "};\n";
			return;
		}

		if (chr::compiler::Compiler_options::CONSTRAINT_STORE_INDEX && !c->_indexes.empty()) {
			_os_ds << prefix() << "using Constraint_store_t = typename chr::Constraint_store_index< Type, std::tuple<";
			bool first1 = true;
			for (auto index : c->_indexes)
			{
				_os_ds << (first1?"":",") << " chr::LNS::Index<";
				first1 = false;
				bool first2 = true;
				for (auto idx : index)
				{
					_os_ds << (first2?"":",") << idx;
					first2 = false;
				}
				_os_ds << ">";
			}
			if (std::find(pragmas.begin(), pragmas.end(), Pragma::persistent) != pragmas.end())
				_os_ds << " >, false";
			else
				_os_ds << " >, true";
			_os_ds << " >;\n";
		} else {
			_os_ds << prefix() << "using Constraint_store_t = typename chr::Constraint_store_simple< Type";
			if (std::find(pragmas.begin(), pragmas.end(), Pragma::persistent) != pragmas.end())
				_os_ds << ", false";
			else
				_os_ds << ", true";
			_os_ds << " >;\n";
		}

		// Print constraint callback
		_os_ds << prefix() << "class Constraint_callback : public chr::Logical_var_imp_observer_constraint {\n";
		_os_ds << prefix() << "public:\n";
		++_depth;
		_os_ds << prefix() << "Constraint_callback(" << p.name() << "* space, typename Constraint_store_t::iterator& it) : _space(space), _it( std::move(it) ) { assert((space != nullptr) && _it.alive()); _it.lock(); }\n";
		_os_ds << prefix() << "Constraint_callback(const Constraint_callback&) =delete;\n";
		_os_ds << prefix() << "void operator=(const Constraint_callback&) =delete;\n",
		_os_ds << prefix() << "~Constraint_callback() { if (!_space.expired() && _space->" << c_name << "_constraint_store && _space->" << c_name << "_constraint_store->depth() >= chr::Backtrack::depth()) _it.unlock(); }\n";
		_os_ds << prefix() << "unsigned char run() override {\n";
		++_depth;
		_os_ds << prefix() << "if (_space.expired()) return 0;\n";
		_os_ds << prefix() << "if (!_it.alive()) return 0;\n";
		_os_ds << prefix() << "auto& cc = const_cast< Type& >(*_it);\n";
		write_trace_statement(_os_ds, c_name, "WAKE", std::make_tuple(R"_STR("Reactivate constraint: )_STR" + std::string(c_name) + R"_STR(")_STR", "cc"));
		_os_ds << prefix() << "if ( _space->do_" << c_name << "(cc, _it) == chr::ES_CHR::FAILURE ) { return 2; }\n";
		_os_ds << prefix() << "return 1;\n";
		--_depth;
		_os_ds << prefix() << "}\n";
		--_depth;
		_os_ds << prefix() << "private:\n";
		++_depth;
		_os_ds << prefix() << "chr::Weak_obj< " << p.name() << " > _space;\n";
		_os_ds << prefix() << "typename Constraint_store_t::iterator _it;\n";
		--_depth;
		_os_ds << prefix() << "};\n";
	
		// End of constaint structure
		--_depth;
		_os_ds << prefix() << "};\n";

		--_depth;
		_os_ds << prefix() << "private:\n";
		++_depth;
		// Constraint store declaration and initialization
		_os_ds << prefix() << "chr::Shared_obj< typename " << c_name << "::Constraint_store_t > " << c_name << "_constraint_store{ chr::make_shared< typename " << c_name << R"_STR(::Constraint_store_t >(")_STR" << c_name << R"_STR(") };)_STR" << "\n";
	}

	void ProgramCppCode::generate_rule_header(ast::ChrProgram& p, ast::ChrConstraintDecl& cdecl)
	{
		auto c_name = std::string( cdecl._c->constraint()->name()->value() );
		if (!p.template_parameters().empty())
		{
			_os_rc << prefix() << "template <";
			bool first = true;
			for (const auto& x : p.template_parameters())
			{
				_os_rc << (first?"":",") << std::get<0>(x) << " " << std::get<1>(x);
				first = false;
			}
			_os_rc << ">\n";
		}
		_os_rc << prefix() << "chr::ES_CHR " << p.name();
		if (!p.template_parameters().empty())
		{
			_os_rc << prefix() << "<";
			bool first = true;
			for (const auto& x : p.template_parameters())
			{
				_os_rc << (first?"":",") << std::get<1>(x);
				first = false;
			}
			_os_rc << ">";
		}
		_os_rc << "::do_" << c_name;
		if (!cdecl._never_stored)
		{
			_os_rc << "(typename " << c_name << "::Type c_args, typename " << c_name << "::Constraint_store_t::iterator c_it) {\n";
			++_depth;
			_os_rc << prefix() << "bool c_stored_before = !c_it.at_end();\n";
		} else {
			_os_rc << "(";
			if (cdecl._c->constraint()->children().size() == 0)
				_os_rc << "[[maybe_unused]] ";
			_os_rc << "typename " << c_name << "::Type c_args) {\n";
			++_depth;
		}
		_os_rc << prefix() << "chr::Statistics::update_call_stack();\n";
		_os_rc << prefix() << "[[maybe_unused]] " << c_name << "_call:\n";
		write_trace_statement(_os_rc, "", "CALL", std::make_tuple(R"_STR("Call constraint: )_STR" + std::string(c_name) + R"_STR(")_STR", "c_args"));
	}

	void ProgramCppCode::store_active_constraint(ast::ChrProgram&, ast::ChrConstraintDecl& cdecl, std::vector< unsigned int > schedule_var_idx)
	{
		auto c_name = std::string( cdecl._c->constraint()->name()->value() );
		_os_rc << prefix() << "// Store constraint\n";
		_os_rc << prefix() << "[[maybe_unused]] " << c_name << "_store:\n";
		if (!cdecl._never_stored)
		{
			++_depth;
			write_trace_statement(_os_rc, "", "EXIT", std::make_tuple(R"_STR("Exit constraint: )_STR" + c_name + R"_STR(")_STR", "c_args" ));
			_os_rc << prefix() << "if (!c_stored_before) {\n";
			++_depth;
			write_trace_statement(_os_rc, "", "INSERT", std::make_tuple(R"_STR("New constraint inserted: )_STR" + c_name + R"_STR(")_STR", "c_args"));
			if (schedule_var_idx.empty())
				_os_rc << prefix() << "(void) " << c_name << "_constraint_store->add( std::move(c_args) );\n";
			else {
				_os_rc << prefix() << "c_it = " << c_name << "_constraint_store->add( c_args );\n";
				_os_rc << prefix() << "auto ccb = chr::Shared_x_obj< chr::Logical_var_imp_observer_constraint >(new typename " << c_name << "::Constraint_callback(this,c_it));\n";
			}
			for (auto i : schedule_var_idx)
			{
#ifndef NDEBUG
				auto pt = dynamic_cast< ast::UnaryExpression* >( cdecl._c->constraint()->children()[i].get() );
				assert((pt != nullptr) && (pt->op() != "+"));
#endif
				_os_rc << prefix() << "chr::schedule_constraint_callback(std::get<" << i+1 << ">(c_args), ccb);\n";
			}
			--_depth;
			_os_rc << prefix() << "}\n";
			--_depth;
		}
		_os_rc << prefix() << "return chr::ES_CHR::SUCCESS;\n";
		--_depth;
		_os_rc << prefix() << "}\n";
	}

	void ProgramCppCode::generate_occurrence_rule(ast::ChrProgram& p, ast::OccRule& r, std::string str_next_occurrence, const std::vector< unsigned int >& schedule_var_idx)
	{
		chr::compiler::visitor::OccRuleCppCode vor(_os_rc,p.input_file_name(),p.auto_catch_failure());
		vor.generate( r, _depth, str_next_occurrence, schedule_var_idx);
	}

}
