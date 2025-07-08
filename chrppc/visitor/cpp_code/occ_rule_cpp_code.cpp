/*
 *  Thisprogram is free software; you can redistribute it and/or
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

#include <visitor/occ_rule.hh>
#include <visitor/body.hh>
#include <visitor/expression.hh>
#include <ast/program.hh>
#include <unordered_set>

namespace chr::compiler::visitor
{
	OccRuleCppCode::OccRuleCppCode(std::ostream& os, std::string_view input_file_name, bool auto_catch_failure)
		: OccRuleAbstractCode(os), _input_file_name(input_file_name), _auto_catch_failure(auto_catch_failure)
	{ }

	void OccRuleCppCode::strip_symbols(std::string& str)
	{
		unsigned int offset = 0;
		for (unsigned int i=0; i < str.length(); ++i)
		{
			char c = str[i];
			if (c == ' ') ++offset;
			else if ((c == '(') || (c == ')') || (c == ',')) str[i-offset] = '_';
			else str[i-offset] = str[i];
		}
		str.resize(str.length() - offset);
	}

	template< typename TT, size_t... I >
	void OccRuleCppCode::loop_to_ostream(const TT& args, std::index_sequence<I...>)
	{
		(..., (_os << (I == 0? "" : ", ") << std::get<I>(args)));
	}

	template< typename... T >
	void OccRuleCppCode::write_trace_statement(ast::OccRule& r, const char* flag, const std::tuple< T... >& args)
	{
		if (chr::compiler::Compiler_options::TRACE)
		{
			_os << prefix() << "TRACE( chr::Log::trace_constraint(chr::Log::" << flag << R"_STR(,")_STR" << std::string(r.name()) << R"_STR(",")_STR" << r.active_constraint().constraint()->name()->value() << R"_STR(",)_STR" << r.active_constraint_occurrence() << ", std::make_tuple(";
			loop_to_ostream(args, std::make_index_sequence<sizeof...(T)>());
			_os << ")); )\n";
		}
	}

	std::string OccRuleCppCode::str_partner_it(ast::OccRule& r,ast::Body&, unsigned int i)
	{
		chr::compiler::visitor::BodyPrint body_v;
		std::string str_it = "it" + std::to_string(r.active_constraint_occurrence()) + "_" + std::to_string(i);
		// std::string(body_v.string_from(c))
		strip_symbols(str_it);
		return str_it;
	}

	void OccRuleCppCode::begin_occ_rule(ast::OccRule& r)
	{
		using namespace ast;
		chr::compiler::visitor::BodyPrint body_v;
		auto c_name = std::string( r.active_constraint().constraint()->name()->value() );
		_os << prefix() << "// ***************************************************\n";
		_os << prefix() << "// " << c_name << "_" << r.active_constraint_occurrence();
		_os << " <=> Rule ";
		if (!r.name().empty())
			_os << std::string(r.name());
		else 
			_os << "NO_NAME";
		_os << ", active constraint " << body_v.string_from( r.active_constraint() );
		_os << ", occurrence " << r.active_constraint_occurrence() + 1 << "\n";

		_os << prefix() << "[[maybe_unused]] " << r.active_constraint().constraint()->name()->value() << "_" << r.active_constraint_occurrence() << ":\n";
		_os << prefix() << "{\n";
		++_depth;

		write_trace_statement(r,"TRY",std::make_tuple(R"_STR("Try occurrence )_STR" + std::to_string(r.active_constraint_occurrence()+1) + " for active constraint: " + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args" ));

		// -----------------------------------------------------------------
		// Add active constraint if it is a bang constraint or if any partner of same
		// name is bangged.
		if (std::find(r.active_constraint().pragmas().begin(), r.active_constraint().pragmas().end(), Pragma::bang) != r.active_constraint().pragmas().end())
			store_active_constraint(r);
		else {
			const auto& cname = r.active_constraint().constraint()->name()->value();
			for (unsigned int i=0; i < r.partners().size(); ++i)
			{
				auto& partner = r.partners()[i];
				if ((cname == partner._c->constraint()->name()->value()) &&
						(std::find(partner._c->pragmas().begin(), partner._c->pragmas().end(), Pragma::bang) != partner._c->pragmas().end()))
				{
					store_active_constraint(r);
					break;
				}
			}
		}

		// -----------------------------------------------------------------
		// Check empty stores
		for (unsigned int i=0; i < r.partners().size(); ++i)
		{
			auto& partner = r.partners()[i];
			_os << prefix() << "if (" << partner._c->constraint()->name()->value();
			_os << "_constraint_store->empty()) goto " << _next_on_inapplicable << ";\n";
		}

		chr::compiler::visitor::ExpressionApply expression_apply_v;
		chr::compiler::visitor::ExpressionPrint expression_print_v;
		// -----------------------------------------------------------------
		// Check active constraint parameters for constant values and duplicate
		// variable names
		unsigned int i_param = 0;
		std::unordered_set< std::string > seen_variables;
		auto& c_decl = *r.active_constraint().constraint()->decl()->_c->constraint();
		auto f_assign_active_constraint_parameters = [&](const ast::Expression& e) {
			auto pL = dynamic_cast< const Literal* >(&e);
			auto pV = dynamic_cast< const CppVariable* >(&e);
			auto pLV = dynamic_cast< const LogicalVariable* >(&e);
			// Check constants in active constraint
			if ((pL != nullptr) || (pV != nullptr))
			{
				// If parameter is a Logical_var<> we must check that it is grounded
				auto pt = dynamic_cast< ast::UnaryExpression* >( c_decl.children()[i_param].get() );
				assert(pt != nullptr);
				if (pt->op() == "?")
					_os << prefix() << "if (!std::get<" << i_param+1 << ">(c_args).ground() || (std::get<" << i_param+1 << ">(c_args) != " << expression_print_v.string_from(const_cast<ast::Expression&>(e)) << ")) goto " << _next_on_inapplicable << ";\n";
				else
					_os << prefix() << "if (std::get<" << i_param+1 << ">(c_args) != " << expression_print_v.string_from(const_cast<ast::Expression&>(e)) << ") goto " << _next_on_inapplicable << ";\n";
				++i_param;
			}
			if (pLV != nullptr)
			{
				std::string v_name = pLV->value();
				if (v_name != "_")
				{
					// Check duplicate name variables in active constraint
					if (seen_variables.find(v_name) != seen_variables.end())
					{
						_os << prefix() << "if (" << v_name << " != std::get<" << i_param+1 << ">(c_args)) goto " << _next_on_inapplicable << ";\n";
					} else {
						(void) seen_variables.insert(v_name);
						_os << prefix() << "auto& " << expression_print_v.string_from(const_cast<ast::Expression&>(e)) << " = std::get<" << i_param+1 << ">(c_args);\n";
					}
				}
				++i_param;
			}
			return true;
		};
		expression_apply_v.apply(*r.active_constraint().constraint(), f_assign_active_constraint_parameters);
	}

	void OccRuleCppCode::end_occ_rule(ast::OccRule&)
	{
		--_depth;
		_os << prefix() << "}\n";
	}

	void OccRuleCppCode::exit_success(ast::OccRule& r)
	{
		write_trace_statement(r, "EXIT", std::make_tuple(R"_STR("Exit constraint: )_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args" ));
		_os << prefix() << "return chr::ES_CHR::SUCCESS;\n";
	}

	void OccRuleCppCode::begin_guard(ast::OccRule& r, unsigned int i, Context& ctxt)
	{
		using namespace ast;
		chr::compiler::visitor::ExpressionPrint expression_print_v;
		auto it_guard = r.guard_parts()[i].begin();
		auto it_guard_end = r.guard_parts()[i].end();
		// First, print all assignments
		while (it_guard != it_guard_end)
		{
			auto pA = dynamic_cast< InfixExpression* >((*it_guard).get());
			if ((pA != nullptr) && (pA->op() == "="))
			{
				_os << prefix() << std::string(expression_print_v.string_from( **it_guard )) << "\n";
				(void) ctxt._local_variables.insert( std::string(expression_print_v.string_from( *pA->l_child() )) );
			} else
				break;
			++it_guard;
		}
		// Then, gather and print all conditions
		bool guard_cond = false;
		bool first = true;
		if (it_guard != it_guard_end)
		{
			if (Compiler_options::LINE_ERROR)
				_os <<  "#line " << r.position().line << R"_STR( ")_STR" << _input_file_name <<  R"_STR(")_STR" << "\n";
			_os << prefix() << "// Begin guard\n";
			_os << prefix() << "if (\n";
			++_depth;
			guard_cond = true;
		}
		while (it_guard != it_guard_end)
		{
#ifndef NDEBUG
			auto pA = dynamic_cast< InfixExpression* >((*it_guard).get());
#endif
			assert((pA == nullptr) || (pA->op() != "="));
			if (!first) _os << prefix() << "&& ";
			else _os << prefix();
			first = false;
			_os << std::string(expression_print_v.string_from( **it_guard )) << "\n";
			++it_guard;
		}
		if (guard_cond)
		{
			--_depth;
			_os << prefix() << ") {\n";
			++_depth;
		}
		if (i > 0)
		{
			auto& partner = r.partners()[i-1];
			std::string str_it = str_partner_it(r,*partner._c,i-1);
			write_trace_statement(r, "PARTNER", std::make_tuple(R"_STR("New partner constraint for )_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args", R"_STR(" found: )_STR" + partner._c->constraint()->name()->value() + R"_STR(")_STR", "*" + str_it ));
		}
	}

	void OccRuleCppCode::end_guard(ast::OccRule& r, unsigned int i)
	{
		using namespace ast;

		// Check that we must end a previously open guard
		if (!r.guard_parts()[i].empty())
		{
			auto rit_guard = r.guard_parts()[i].rbegin();
			assert(rit_guard != r.guard_parts()[i].rend());
			auto pA = dynamic_cast< InfixExpression* >((*rit_guard).get());
			if ((pA == nullptr) || (pA->op() != "="))
				--_depth;
			_os << prefix() << "} // End guard\n";
		}
	}

	std::string OccRuleCppCode::history_key_from_active_constraint(ast::OccRule&)
	{
		return std::string("std::get<0>(c_args)");
	}

	std::string OccRuleCppCode::history_key_from_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		std::string str_it = str_partner_it(r,*partner._c,i);
		return std::string("std::get<0>(*") + str_it + std::string(")");
	}

	void OccRuleCppCode::begin_history(ast::OccRule& r, std::vector< std::string > key)
	{
		write_trace_statement(r,"HISTORY",std::make_tuple(R"_STR("History check triggered by: )_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args" ));
		_os << prefix() << "// Check history\n";
		_os << prefix() << "if (_history.rule_" << r.rule()->id() << "->check( {{";
		bool first = true;
		for (auto& s : key)
		{
			if (!first) _os << ",";
			first = false;
			_os << s;
		}
		_os << "}} )) {\n";
		++_depth;
	}

	void OccRuleCppCode::end_history(ast::OccRule& r)
	{
		--_depth;
		_os << prefix() << "} // End history" << std::endl;
        if (!r.partners().empty())
        {
            auto& partner = r.partners().back();
            std::string str_it = str_partner_it(r,*partner._c,r.partners().size()-1);
            _os << prefix() << "else {" << std::endl;
            ++_depth;
            _os << prefix() << "++" << str_it << ";" << std::endl;
            if (!r.guard_parts()[r.partners().size()].empty())
                _os << prefix() << "goto " << str_it << "_next;\n";
            --_depth;
            _os << prefix() << "}\n";
        }
	}

	void OccRuleCppCode::commit_rule(ast::OccRule& r)
	{
		std::string str_partners;
		for (unsigned int i=0; i < r.partners().size(); ++i)
			str_partners += R"_STR(,", )_STR" + r.partners()[i]._c->constraint()->name()->value() + R"_STR(",*)_STR" + str_partner_it(r,*r.partners()[i]._c,i);
		write_trace_statement(r, "COMMIT", std::make_tuple(R"_STR("Commit rule with: )_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args" + str_partners ));

		if (r.keep_active_constraint())
		{
			for (unsigned int i = 0; i < r.partners().size(); ++i)
			{
				auto& partner = r.partners()[i];
				std::string str_it = str_partner_it(r,*partner._c,i);
				if (r.partners()[i]._keep)
					_os << prefix() << str_it << ".lock();\n";
				else {
					_os << prefix() << str_it << ".lock();\n";
					break; // No more locked iterators after
				}
			}
		}

		// We must lock iterators of bang partner constraints which will be removed
		for (unsigned int i = 0; i < r.partners().size(); ++i)
		{
			auto& partner_i = r.partners()[i];
			auto c_i_name = std::string( partner_i._c->constraint()->name()->value() );
			std::string str_it = str_partner_it(r,*partner_i._c,i);
			if (!r.partners()[i]._keep)
			{
				auto c_name = std::string( r.active_constraint().constraint()->name()->value() );
				// Check if another constraint of same name is bang
				auto pragmas = r.active_constraint().pragmas();
				bool pragma_bang = (c_name == c_i_name) && (std::find(pragmas.begin(), pragmas.end(), Pragma::bang) != pragmas.end());
				for (unsigned int j=0; !pragma_bang && (j < r.partners().size()) ; ++j)
				{
					auto& pj = r.partners()[j];
					auto c_j_name = std::string( pj._c->constraint()->name()->value() );
					pragmas = pj._c->pragmas();
					pragma_bang = (c_i_name == c_j_name) && (std::find(pragmas.begin(), pragmas.end(), Pragma::bang) != pragmas.end());
				}

				if (pragma_bang)
					_os << prefix() << str_it << ".lock(); // because of #bang\n";
			}
		}
	}

	void OccRuleCppCode::check_alived_partner(ast::OccRule& r, unsigned int i)
	{
		chr::compiler::visitor::BodyPrint body_v;
		auto& partner = r.partners()[i];
		std::string str_it = str_partner_it(r,*partner._c,i);
		_os << prefix() << "if (!" << str_it << ".alive()) {\n";
		++_depth;
		for (unsigned int j=i+1; j < r.partners().size(); ++j)
		{
			auto& partner = r.partners()[j];
			std::string str_it_j = str_partner_it(r,*partner._c,j);
			_os << prefix() << str_it_j << ".unlock();\n";
			if (!partner._keep) break; // No more locked iterators after
		}
		_os << prefix() << str_it << ".next_and_unlock();\n";
		_os << prefix() << "goto " << str_it << "_next;\n";
		--_depth;
		_os << prefix() << "}\n";
		_os << prefix() << str_it << ".unlock();\n";
	}

	void OccRuleCppCode::partner_must_be_different_from_active_constraint(ast::OccRule& r, unsigned int i)
	{
		auto& pc_partner1 = r.partners()[i]._c;
		std::string str_it1 = str_partner_it(r,*pc_partner1,i);
		// If active constraint is not stored it cannot be equal
		// to a previous one.
		r.guard_parts()[i+1].emplace( r.guard_parts()[i+1].begin(),
				std::make_unique< ast::InfixExpression >(
					"!=",
					std::make_unique<ast::Literal>("std::get<0>(*" + str_it1 + ")",pc_partner1->position()),
					std::make_unique<ast::Literal>("std::get<0>(c_args)",pc_partner1->position()),
					pc_partner1->position()
					) );
	}

	void OccRuleCppCode::partner_must_be_different_from(ast::OccRule& r, unsigned int i, unsigned int j)
	{
		auto& pc_partner1 = r.partners()[i]._c;
		auto& pc_partner2 = r.partners()[j]._c;
		std::string str_it1 = str_partner_it(r,*pc_partner1,i);
		std::string str_it2 = str_partner_it(r,*pc_partner2,j);
		r.guard_parts()[i+1].emplace( r.guard_parts()[i+1].begin(),
				std::make_unique< ast::InfixExpression >(
					"!=",
					std::make_unique<ast::Literal>("std::get<0>(*" + str_it1 + ")",pc_partner1->position()),
					std::make_unique<ast::Literal>("std::get<0>(*" + str_it2 + ")",pc_partner2->position()),
					pc_partner1->position()
					) );
	}

	void OccRuleCppCode::goto_next_matching_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		std::string str_it = str_partner_it(r,*partner._c,i);
		_os << prefix() << str_it << ".next_and_unlock();\n";
		// For last partner, if guard empty, the previous call to .next_and_unlock() do the job
		if (!((i == (r.partners().size() - 1)) && r.guard_parts()[i+1].empty()))
			_os << prefix() << "goto " << str_it << "_next;\n";
	}

	void OccRuleCppCode::begin_partner(ast::OccRule& r, unsigned int i, std::vector <std::string> index)
	{
		auto& partner = r.partners()[i];
		std::string str_it = str_partner_it(r,*partner._c,i);

		if (not(chr::compiler::Compiler_options::CONSTRAINT_STORE_INDEX) or partner._use_index == -1)
			_os << prefix() << "auto " << str_it << " = " << partner._c->constraint()->name()->value() << "_constraint_store->begin();\n";
		else {
			_os << prefix() << "auto " << str_it << " = " << partner._c->constraint()->name()->value() << "_constraint_store->template begin<" << partner._use_index << ">(";
			bool first = true;
			for (auto& s : index)
			{
				if (!first) _os << ",";
				first = false;
				_os << s;
			}
			_os << ");\n";
		}

		// Go over all assignments to search for first real guard test
		auto it_guard = r.guard_parts()[i+1].begin();
		auto it_guard_end = r.guard_parts()[i+1].end();
		// Go to the first test in the guard (go over all assignments)
		while (it_guard != it_guard_end)
		{
			auto pA = dynamic_cast< ast::InfixExpression* >((*it_guard).get());
			if ((pA == nullptr) || (pA->op() != "="))
				break;
			++it_guard;
		}

		// If we are at the last partner matching and if we delete the
		// active constraint and if we have no guard, no need to go to next partner
		// as we are not alive anymore. Just use the iterator!
		if ( (it_guard == it_guard_end) 
				&& !r.keep_active_constraint()
				&& (i == (r.partners().size() - 1)) )
			_os << prefix() << "if ( !" << str_it << ".at_end() ) {\n";
		else
			_os << prefix() << "while ( !" << str_it << ".at_end() ) {\n";

		chr::compiler::visitor::ExpressionApply expression_apply_v;
		chr::compiler::visitor::ExpressionPrint expression_print_v;
		// -----------------------------------------------------------------
		// Assign constraint parameters parameters
		unsigned int i_param = 0;
		auto f_assign_partner_constraint_parameters = [&](const ast::Expression& e) {
			auto pL = dynamic_cast< const ast::Literal* >(&e);
			auto pV = dynamic_cast< const ast::CppVariable* >(&e);
			auto pLV = dynamic_cast< const ast::LogicalVariable* >(&e);
			if ((pL != nullptr) || (pV != nullptr))
				++i_param;
			if (pLV != nullptr)
			{
				if (pLV->value() != "_")
					// If variable is in index, we don't need to declare a variable
					if (std::find(index.begin(),index.end(),pLV->value()) == index.end())
						_os << prefix() << "auto " << expression_print_v.string_from(const_cast<ast::Expression&>(e)) << "(std::get<" << i_param+1 << ">(*" << str_it << "));\n";
				++i_param;
			}
			return true;
		};

		++_depth;
		expression_apply_v.apply(*partner._c->constraint(), f_assign_partner_constraint_parameters);
		--_depth;
	}

	void OccRuleCppCode::end_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		std::string str_it = str_partner_it(r,*partner._c,i);

		// Go over all assignments to search for first real guard test
		auto it_guard = r.guard_parts()[i+1].begin();
		auto it_guard_end = r.guard_parts()[i+1].end();
		// Go to the first test in the guard (go over all assignments)
		while (it_guard != it_guard_end)
		{
			auto pA = dynamic_cast< ast::InfixExpression* >((*it_guard).get());
			if ((pA == nullptr) || (pA->op() != "="))
				break;
			++it_guard;
		}

		// If we are at the last partner matching and if we delete the
		// active constraint and if we have no guard, no need to go to next partner
		// as we are not alive anymore. Just use the iterator!
		if ( (it_guard == it_guard_end) 
				&& !r.keep_active_constraint()
				&& (i == (r.partners().size() - 1)) )
		{
			_os << prefix() << "}\n";
		} else {
			++_depth;
			// For last partner, if guard empty, the previous call to .next_and_unlock() do the job
			if (!((i == (r.partners().size() - 1)) && r.guard_parts()[i+1].empty()))
			{
				_os << prefix() << "++" << str_it << ";\n";
				bool drop_one_partner = false;
				for (unsigned int j=0; j < i; ++j)
					if (!r.partners()[j]._keep)
					{
						drop_one_partner = true;
						break;
					}
				if (r.keep_active_constraint() && !drop_one_partner)
					_os << prefix() << str_it << "_next:;\n";
			}
			--_depth;
			_os << prefix() << "}\n";
		}
	}

	void OccRuleCppCode::remove_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		auto c_name = std::string( r.active_constraint().constraint()->name()->value() );
		auto c_i_name = std::string( r.partners()[i]._c->constraint()->name()->value() );
		std::string str_it = str_partner_it(r,*partner._c,i);

		// Check if another constraint of same name is bang
		auto pragmas = r.active_constraint().pragmas();
		bool pragma_bang = (c_name == c_i_name) && (std::find(pragmas.begin(), pragmas.end(), Pragma::bang) != pragmas.end());
		for (unsigned int j=0; !pragma_bang && (j < r.partners().size()) ; ++j)
		{
			auto& pj = r.partners()[j];
			auto c_j_name = std::string( pj._c->constraint()->name()->value() );
			pragmas = pj._c->pragmas();
			pragma_bang = (c_i_name == c_j_name) && (std::find(pragmas.begin(), pragmas.end(), Pragma::bang) != pragmas.end());
		}

		// If bang constraint, check that constraint is alived before remove it
		if (pragma_bang)
		{
			_os << prefix() << "if (" << str_it << ".alive()) { // because of #bang\n";
			++_depth;
		}
		write_trace_statement(r, "REMOVE", std::make_tuple(R"_STR("Remove constraint: )_STR" + partner._c->constraint()->name()->value() + R"_STR(")_STR", "*" + str_it ));
		_os << prefix() << str_it << ".kill();\n";
		if (pragma_bang)
		{
			--_depth;
			_os << prefix() << "}\n";
			_os << prefix() << str_it << ".unlock(); // because of #bang\n";
		}
	}

	void OccRuleCppCode::check_alived_active_constraint(ast::OccRule& r)
	{
		chr::compiler::visitor::BodyPrint body_v;
		
		_os << prefix() << "assert(c_stored_before);\n";
		_os << prefix() << "if (!c_it.alive()) {\n";
		++_depth;
		_os << prefix() << "c_it.unlock();\n";
		for (unsigned int i=0; i < r.partners().size(); ++i)
		{
			auto& partner = r.partners()[i];
			std::string str_it = str_partner_it(r,*partner._c,i);
			_os << prefix() << str_it << ".unlock();\n";
			if (!partner._keep) break; // No more locked iterators after
		}
		exit_success(r);
		--_depth;
		_os << prefix() << "}\n";
		if (r.keep_active_constraint())
			_os << prefix() << "c_it.unlock();\n";
	}

	void OccRuleCppCode::store_active_constraint(ast::OccRule& r)
	{
		_os << prefix() << "if (!c_stored_before) {\n";
		++_depth;
		write_trace_statement(r,"INSERT",std::make_tuple(R"_STR("New constraint inserted: )_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args" ));
		_os << prefix() << "c_it = " << r.active_constraint().constraint()->name()->value() << "_constraint_store->add(c_args);\n";
		if (!_schedule_var_idx.empty())
		{
			_os << prefix() << "auto ccb = chr::Shared_x_obj< chr::Logical_var_imp_observer_constraint >(new typename " << r.active_constraint().constraint()->name()->value() << "::Constraint_callback(this,c_it));\n";
		}
		for (auto i : _schedule_var_idx)
		{
			auto pLV = dynamic_cast< ast::LogicalVariable* >(r.active_constraint().constraint()->children()[i].get());
			if (pLV != nullptr)
				_os << prefix() << "chr::schedule_constraint_callback(std::get<" << i+1 << ">(c_args), ccb);\n";
		}
		_os << prefix() << "c_stored_before = true;\n";
		--_depth;
		_os << prefix() << "}\n";

		// Lock active constraint
		if (r.keep_active_constraint())
			_os << prefix() << "c_it.lock();\n";
	}

	void OccRuleCppCode::remove_active_constraint(ast::OccRule& r)
	{
		// No need to kill a never stored constraint
		if (!r.active_constraint().constraint()->decl()->_never_stored)
		{
			_os << prefix() << "if (c_stored_before) {\n";
			++_depth;
			write_trace_statement(r, "REMOVE", std::make_tuple(R"_STR("Remove constraint: )_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(")_STR", "c_args" ));
			_os << prefix() << "c_it.kill();\n";
			--_depth;
			_os << prefix() << "}\n";
		}
	}

	void OccRuleCppCode::body(ast::OccRule& r, Context ctxt)
	{
		assert(!Compiler_options::LINE_ERROR || !_input_file_name.empty());

		std::string trace_prefix;
		trace_prefix = R"_STR(")_STR" + std::string(r.name()) + R"_STR(",")_STR" + r.active_constraint().constraint()->name()->value() + R"_STR(",)_STR" + std::to_string(r.active_constraint_occurrence());

		chr::compiler::visitor::BodyCppCode vb(_os,_input_file_name,trace_prefix,_auto_catch_failure);
		_os << prefix() << "// Body\n";
		_os << prefix() << "chr::Statistics::inc_nb_rules();\n";

		auto c_name = std::string( r.active_constraint().constraint()->name()->value() );
		vb.generate(*r.body(),prefix().length(),c_name,!r.keep_active_constraint(),ctxt._logical_variables,ctxt._local_variables);
		_os << "\n";
	}
} // namespace chr::compiler::visitor
