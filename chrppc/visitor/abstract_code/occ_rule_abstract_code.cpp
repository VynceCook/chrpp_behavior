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

#include <visitor/occ_rule.hh>
#include <visitor/body.hh>
#include <visitor/expression.hh>
#include <ast/program.hh>
#include <unordered_set>

namespace chr::compiler::visitor
{
	OccRuleAbstractCode::OccRuleAbstractCode(std::ostream& os)
		: _prefix_w(0), _depth(0), _os(os) 
	{ }

	void OccRuleAbstractCode::generate(ast::OccRule& r, std::size_t prefix_w, std::string_view next_on_inapplicable, std::vector< unsigned int > schedule_var_idx)
	{
		_prefix_w = prefix_w;
		_depth = 0;
		_next_on_inapplicable = std::string(next_on_inapplicable);
		_schedule_var_idx = std::move( schedule_var_idx );
		r.accept( *this );
	}

	std::string OccRuleAbstractCode::prefix()
	{
		return std::string(_prefix_w + _depth, '\t');
	}

	void OccRuleAbstractCode::begin_occ_rule(ast::OccRule& r)
	{
		using namespace ast;
		chr::compiler::visitor::BodyPrint body_v;
		_os << prefix() << "// Rule ";
		if (!r.name().empty())
			_os << std::string(r.name());
		else 
			_os << "NO_NAME";
		_os << ", active constraint " << body_v.string_from( r.active_constraint() );
		_os << ", occurrence " << r.active_constraint_occurrence() << "\n";

		_os << prefix() << "Begin ";
		_os << r.active_constraint().constraint()->name()->value();
		_os << "_" << r.active_constraint_occurrence() << "\n";

		// -----------------------------------------------------------------
		// CHECK EMPTY STORES
		for (unsigned int i=0; i < r.partners().size(); ++i)
		{
			auto& partner = r.partners()[i];
			chr::compiler::visitor::BodyPrint body_v;
			_os << prefix() << "If empty store " << body_v.string_from( *partner._c );
			_os << " Then goto " << _next_on_inapplicable << "\n";
		}
		++_depth;
	}

	void OccRuleAbstractCode::end_occ_rule(ast::OccRule&)
	{
		--_depth;
		_os << prefix() << "End\n";
	}

	void OccRuleAbstractCode::exit_success(ast::OccRule&)
	{
		_os << prefix() << "goto next goal constraint\n";
	}

	void OccRuleAbstractCode::begin_guard(ast::OccRule& r, unsigned int i, Context& ctxt)
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
			_os << prefix() << "If guard\n";
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
			_os << prefix() << "Then guard\n";
			++_depth;
		}
	}

	void OccRuleAbstractCode::end_guard(ast::OccRule& r, unsigned int i)
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
			_os << prefix() << "End guard\n";
		}
	}

	std::string OccRuleAbstractCode::history_key_from_active_constraint(ast::OccRule& r)
	{
		chr::compiler::visitor::BodyPrint v;
		return std::string(v.string_from( r.active_constraint() ));
	}

	std::string OccRuleAbstractCode::history_key_from_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		chr::compiler::visitor::BodyPrint v;
		return std::string(v.string_from( *partner._c ));
	}

	void OccRuleAbstractCode::begin_history(ast::OccRule&, std::vector< std::string > key)
	{
		_os << prefix() << "If history <";
		bool first = true;
		for (auto& s : key)
		{
			if (!first) _os << ",";
			first = false;
			_os << s;
		}
		_os << "> Then\n";
		++_depth;
	}

	void OccRuleAbstractCode::end_history(ast::OccRule&)
	{
		--_depth;
		_os << prefix() << "End history" << std::endl;
	}

	void OccRuleAbstractCode::commit_rule(ast::OccRule&)
	{ }

	void OccRuleAbstractCode::check_alived_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		chr::compiler::visitor::BodyPrint body_v;
		_os << prefix() << "If not alived " << body_v.string_from( *partner._c );
		_os << " Then goto next matching of " << body_v.string_from( *partner._c ) << "\n";
	}

	void OccRuleAbstractCode::partner_must_be_different_from_active_constraint(ast::OccRule& r, unsigned int i)
	{
		auto& pc_partner1 = r.partners()[i]._c;
		r.guard_parts()[i+1].emplace_back(
				std::make_unique< ast::InfixExpression >(
					"!=",
					std::make_unique<ast::ChrConstraint>(*pc_partner1->constraint()),
					std::make_unique<ast::ChrConstraint>(*r.active_constraint().constraint()),
					pc_partner1->position()
					) );
	}

	void OccRuleAbstractCode::partner_must_be_different_from(ast::OccRule& r, unsigned int i, unsigned int j)
	{
		auto& pc_partner1 = r.partners()[i]._c;
		auto& pc_partner2 = r.partners()[j]._c;
		r.guard_parts()[i+1].emplace_back(
				std::make_unique< ast::InfixExpression >(
					"!=",
					std::make_unique<ast::ChrConstraint>(*pc_partner1->constraint()),
					std::make_unique<ast::ChrConstraint>(*pc_partner2->constraint()),
					pc_partner1->position()
					) );
	}

	void OccRuleAbstractCode::goto_next_matching_partner(ast::OccRule& r, unsigned int i)
	{
		auto& partner = r.partners()[i];
		chr::compiler::visitor::BodyPrint body_v;
		_os << prefix() << "goto next matching of " << body_v.string_from( *partner._c ) << "\n";
	}

	void OccRuleAbstractCode::begin_partner(ast::OccRule& r, unsigned int i, std::vector <std::string> index)
	{
		chr::compiler::visitor::BodyPrint body_v;
		auto& partner = r.partners()[i];
		_os << prefix() << "Matching partner " << body_v.string_from( *partner._c );
		if (chr::compiler::Compiler_options::CONSTRAINT_STORE_INDEX and partner._use_index != -1)
		{
			_os << " with idx#" << partner._use_index << "<";
			bool first = true;
			for (auto& s : index)
			{
				if (!first) _os << ",";
				first = false;
				_os << s;
			}
			_os << ">";
		}
		_os << "\n";
	}

	void OccRuleAbstractCode::end_partner(ast::OccRule& r, unsigned int i)
	{
		chr::compiler::visitor::BodyPrint body_v;
		auto& partner = r.partners()[i];
		_os << prefix() << "End matching partner " << body_v.string_from( *partner._c ) << "\n";
	}

	void OccRuleAbstractCode::remove_partner(ast::OccRule& r, unsigned int i)
	{
		chr::compiler::visitor::BodyPrint body_v;
		auto& partner = r.partners()[i];
		_os << prefix() << "remove constraint " << body_v.string_from( *partner._c ) << "\n";
	}

	void OccRuleAbstractCode::check_alived_active_constraint(ast::OccRule& r)
	{
		chr::compiler::visitor::BodyPrint body_v;
		
		_os << prefix() << "If not alived " << body_v.string_from( r.active_constraint() );
		_os << " Then goto next goal constraint\n";
	}

	void OccRuleAbstractCode::store_active_constraint(ast::OccRule& r)
	{
		chr::compiler::visitor::BodyPrint body_v;
		_os << prefix() << "store constraint " << body_v.string_from( r.active_constraint() ) << "\n";
	}

	void OccRuleAbstractCode::remove_active_constraint(ast::OccRule& r)
	{
		chr::compiler::visitor::BodyPrint body_v;
		_os << prefix() << "remove constraint " << body_v.string_from( r.active_constraint() ) << "\n";
	}

	void OccRuleAbstractCode::body(ast::OccRule& r, Context ctxt)
	{
		chr::compiler::visitor::BodyAbstractCode vb(_os);
		auto c_name = std::string( r.active_constraint().constraint()->name()->value() );
		vb.generate(*r.body(),prefix().length(),c_name,true,ctxt._logical_variables,ctxt._local_variables);
		_os << "\n";
	}

	void OccRuleAbstractCode::visit(ast::OccRule& r)
	{
		using namespace ast;
		Context context;
		std::vector< ast::ChrConstraintCall* > seen_head_constraints;
		seen_head_constraints.emplace_back(&r.active_constraint());

		// -----------------------------------------------------------------
		// BEGIN OCCURENCE RULE
		begin_occ_rule(r);

		// -----------------------------------------------------------------
		// UPDATE HISTORY WITH ACTIVE CONSTRAINT
		bool is_propagation_rule = true;
		std::vector< std::string > history;
		if (r.keep_active_constraint())
		{
			auto& pragmas = r.active_constraint().pragmas();
			if (std::find(pragmas.begin(), pragmas.end(), Pragma::no_history) == pragmas.end())
				history.emplace_back( history_key_from_active_constraint(r) );
		} else
			is_propagation_rule = false;

		// -----------------------------------------------------------------
		// PARSE ACTIVE CONSTRAINT AND UPDATE seen_head_variables
		unsigned int next_free_variable_id = 0;
		std::unordered_set< std::string > seen_head_variables;
		// Update with variables of active constraint
		for (auto& cc : r.active_constraint().constraint()->children())
		{
			PositionInfo pos = cc->position();
			auto pLV = dynamic_cast< LogicalVariable* >(cc.get());
			if (pLV != nullptr)
			{
				std::string v_name = pLV->value();
				if (v_name != "_") {
					(void) seen_head_variables.insert(v_name);
					(void) context._logical_variables.insert({v_name,std::make_tuple(ast::PtrSharedChrConstraintDecl(),-1)});
				}
			}
		}

		// -----------------------------------------------------------------
		// CHECK IF ACTIVE CONSTRAINT IS BANG
		auto& pragmas_active_c = r.active_constraint().pragmas();
		bool bang_active_constraint = std::find(pragmas_active_c.begin(), pragmas_active_c.end(), Pragma::bang) != pragmas_active_c.end();

		// -----------------------------------------------------------------
		// GUARD - FRONT
		begin_guard(r,0,context);

		// -----------------------------------------------------------------
		// PARTNERS MATCHING - START
		std::vector< unsigned int > partners_to_delete;
		for (unsigned int i = 0; i < r.partners().size(); ++i )
		{
			// -----------------------------------------------------------------
			// UPDATE INDEX
			std::vector< std::string > index;
			int n_index = r.partners()[i]._use_index;
            if (chr::compiler::Compiler_options::CONSTRAINT_STORE_INDEX and n_index != -1)
			{
				for (auto idx : r.partners()[i]._c->constraint()->decl()->_indexes[n_index])
				{
					auto& cc = r.partners()[i]._c->constraint()->children()[idx];
					auto pL = dynamic_cast< Literal* >(cc.get());
					auto pV = dynamic_cast< CppVariable* >(cc.get());
					auto pLV = dynamic_cast< LogicalVariable* >(cc.get());
					assert((pL != nullptr) || (pV != nullptr) || (pLV != nullptr));
					index.emplace_back( (pL!=nullptr?pL->value():(pV!=nullptr?pV->value():pLV->value())) );
				}
			}

			// -----------------------------------------------------------------
			// CHECK CONSTANTS AND DUPLICATE VARIABLE NAMES OF PARTNER CONSTRAINT
			// Update with all constraints arguments
			for (auto& cc : r.partners()[i]._c->constraint()->children())
			{
				auto pL = dynamic_cast< Literal* >(cc.get());
				auto pV = dynamic_cast< CppVariable* >(cc.get());
				auto pLV = dynamic_cast< LogicalVariable* >(cc.get());
				assert((pL != nullptr) || (pV != nullptr) || (pLV != nullptr));
				if ((pL != nullptr) || (pV != nullptr))
				{
					std::string& value = (pL != nullptr)?pL->value():pV->value();
					// Rename variable and add expression matching to guard
					std::string new_value = "_LV_" + std::to_string(next_free_variable_id) + "_";

					// If variable is in index, we don't rename neither add the equality constraint
					if (std::find(index.begin(),index.end(),value) == index.end())
					{
						PositionInfo pos(cc->position());
						r.guard_parts()[i+1].emplace_back(
								std::make_unique< InfixExpression >(
									"==",
									std::make_unique<Identifier>(new_value,pos),
									std::make_unique<Identifier>(value,pos),
									pos
								) );
						value = new_value;
						++next_free_variable_id;
					}
				}
				if (pLV != nullptr)
				{
					std::string& v_name = pLV->value();
					if (v_name != "_")
					{
						// Rename variable and add expression matching to guard
						if (seen_head_variables.find(v_name) != seen_head_variables.end())
						{
							std::string new_name = "_LV_" + std::to_string(next_free_variable_id) + "_";

							// If variable is in index, we don't rename neither add the equality constraint
							if (std::find(index.begin(),index.end(),v_name) == index.end())
							{
								PositionInfo pos(cc->position());
								r.guard_parts()[i+1].emplace_back(
										std::make_unique< InfixExpression >(
											"==",
											std::make_unique<Identifier>(new_name,pos),
											std::make_unique<Identifier>(v_name,pos),
											pos
										) );
								v_name = new_name;
								++next_free_variable_id;
							}
						} else {
							(void) seen_head_variables.insert(v_name);
							(void) context._logical_variables.insert({v_name,std::make_tuple(ast::PtrSharedChrConstraintDecl(),-1)});
						}
					}
				}
			}

			// -----------------------------------------------------------------
			// CHECK ALREADY SEEN CONSTRAINT
			auto& pragmas = r.partners()[i]._c->pragmas();
			if (std::find(pragmas.begin(), pragmas.end(), Pragma::bang) == pragmas.end())
			{
				for ( int j = seen_head_constraints.size(); j-- ; )
				{
					auto pc_head = seen_head_constraints[j];
					if (pc_head->constraint()->decl() == r.partners()[i]._c->constraint()->decl())
					{
						assert(j <= (int)i);
						if (j == 0) {
							// Add test only if active constraint is not bang
							if (!bang_active_constraint)
								partner_must_be_different_from_active_constraint(r,i);
						} else {
							// Add test only if partner j-1 is not bang
							auto& pragmas_p = r.partners()[j-1]._c->pragmas();
							if (std::find(pragmas_p.begin(), pragmas_p.end(), Pragma::bang) == pragmas_p.end())
								partner_must_be_different_from(r,i,j-1);
						}
					}
				}
				seen_head_constraints.emplace_back(r.partners()[i]._c.get());
			}

			// -----------------------------------------------------------------
			// UPDATE HISTORY WITH PARTNER CONSTRAINT
			if (r.partners()[i]._keep)
			{
				auto& pragmas = r.partners()[i]._c->pragmas();
				if (std::find(pragmas.begin(), pragmas.end(), Pragma::no_history) == pragmas.end())
					history.emplace_back( history_key_from_partner(r,i) );
			} else
				is_propagation_rule = false;

			begin_partner(r,i,std::move(index));
			if (!r.partners()[i]._keep)
				partners_to_delete.emplace_back(i);
			++_depth;

			// -----------------------------------------------------------------
			// GUARD - ACCORDING TO PARTNER i
			begin_guard(r,i+1,context);
		}

		// -----------------------------------------------------------------
		// CHECK HISTORY - BEGIN
		bool close_history = false;
		if (is_propagation_rule && !history.empty())
		{
			begin_history(r,std::move(history));
			close_history = true;
		}

		// -----------------------------------------------------------------
		// COMMIT RULE
		commit_rule(r);

		// -----------------------------------------------------------------
		// STORE / DELETE CONSTRAINTS
		if (r.store_active_constraint())
			store_active_constraint(r);
		else if (!r.keep_active_constraint())
			remove_active_constraint(r);
		for (auto n : partners_to_delete)
			remove_partner(r,n);

		// -----------------------------------------------------------------
		// BODY
		body(r,context);

		// -----------------------------------------------------------------
		// CHECK ALIVED CONSTRAINT STORES
		if (r.keep_active_constraint())
		{
			if (r.store_active_constraint())
				check_alived_active_constraint(r);
			for (unsigned int i = 0; i < r.partners().size(); ++i)
				if (r.partners()[i]._keep && (i != (r.partners().size() - 1)))
					check_alived_partner(r,i);
				else {
					goto_next_matching_partner(r,i);
					break;
				}
		} else
			exit_success(r);

		// -----------------------------------------------------------------
		// CHECK HISTORY - END
		if (close_history)
			end_history(r);


		// -----------------------------------------------------------------
		// PARTNERS MATCHING - END
		for (int i = r.partners().size(); i-- ; )
		{
			// -----------------------------------------------------------------
			// END GUARD - ACCORDING TO PARTNER i
			end_guard(r,i+1);

			--_depth;
			end_partner(r,i);
		}

		// -----------------------------------------------------------------
		// END GUARD - FRONT
		end_guard(r,0);

		// -----------------------------------------------------------------
		// END OCCURENCE RULE
		end_occ_rule(r);
	}
} // namespace chr::compiler::visitor
