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

#include <unordered_map>
#include <visitor/program.hh>
#include <visitor/occ_rule.hh>
#include <visitor/body.hh>

namespace chr::compiler::visitor
{
	ProgramAbstractCode::ProgramAbstractCode(std::ostream& os)
		: _prefix_w(0), _depth(0), _os_ds(os), _os_rc(os) 
	{ }

	ProgramAbstractCode::ProgramAbstractCode(std::ostream& os_ds, std::ostream& os_rc)
		: _prefix_w(0), _depth(0), _os_ds(os_ds), _os_rc(os_rc) 
	{ }

	void ProgramAbstractCode::generate(ast::ChrProgram& p, bool data_structures, bool rule_code)
	{
		_data_structures = data_structures;
		_rule_code = rule_code;
		p.accept( *this );
	}

	std::string ProgramAbstractCode::prefix()
	{
		return std::string(_prefix_w + _depth, '\t');
	}

	void ProgramAbstractCode::begin_program(ast::ChrProgram&)
	{ }

	void ProgramAbstractCode::end_program(ast::ChrProgram&)
	{ }

	void ProgramAbstractCode::constraint_store(ast::ChrProgram& p, unsigned int i)
	{
		chr::compiler::visitor::BodyPrint vb;
		auto& c = p.chr_constraints()[i];
		_os_ds << prefix() << "(constraint store) " << vb.string_from(*c->_c);
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
	}

	void ProgramAbstractCode::generate_rule_header(ast::ChrProgram&, ast::ChrConstraintDecl&)
	{ }

	void ProgramAbstractCode::store_active_constraint(ast::ChrProgram&, ast::ChrConstraintDecl& cdecl, std::vector< unsigned int > schedule_var_idx)
	{
		auto c_name = std::string( cdecl._c->constraint()->name()->value() );
		_os_rc << prefix() << "// Fail through\n";
		_os_rc << prefix() << "Begin " << c_name << "_store\n";
		++_depth;
		_os_rc << prefix() << "Store constraint " << c_name << "\n";

		for (auto i : schedule_var_idx)
		{
#ifndef NDEBUG
			auto pt = dynamic_cast< ast::UnaryExpression* >( cdecl._c->constraint()->children()[i].get() );
			assert((pt != nullptr) && (pt->op() != "+"));
#endif
			_os_rc << prefix() << "Schedule constraint " << c_name << " with variable index " << i << "\n";
		}
		--_depth;
		_os_rc << prefix() << "Goto next goal constraint\n";
	}

	void ProgramAbstractCode::generate_occurrence_rule(ast::ChrProgram&, ast::OccRule& r, std::string str_next_occurrence, const std::vector< unsigned int >& schedule_var_idx)
	{
		chr::compiler::visitor::OccRuleAbstractCode vor(_os_rc);
		vor.generate( r, 0, str_next_occurrence, schedule_var_idx);
	}

	void ProgramAbstractCode::visit(ast::ChrProgram& p)
	{
		// -----------------------------------------------------------------
		// GENERATE CONSTRAINT STORE
		if (_data_structures)
		{
			begin_program(p);
			for (unsigned int i=0; i < p.chr_constraints().size(); ++i)
				constraint_store(p,i);
			end_program(p);
		}

		if (_rule_code)
		{
			// Populate constraint declarations
			std::unordered_map< std::string, const ast::PtrSharedChrConstraintDecl > constraint_decl;
			for (unsigned int i=0; i < p.chr_constraints().size(); ++i)
				constraint_decl.insert({p.chr_constraints()[i]->_c->constraint()->name()->value(),p.chr_constraints()[i]});

			ast::PtrSharedChrConstraintDecl active_decl_c;
			std::vector< unsigned int > schedule_var_idx;
			auto it = p.occ_rules().begin();
			auto it_end = p.occ_rules().end();	
			while (it != it_end)
			{
				std::string str_next_occurrence;
				// Check if we still have the same active constraint
				if (active_decl_c != (*it)->active_constraint().constraint()->decl())
				{
					// Generate fail through case where the active constraint is stored
					if (active_decl_c) // We have a previous active constraint to store
						store_active_constraint(p,*active_decl_c,schedule_var_idx);

					active_decl_c = (*it)->active_constraint().constraint()->decl();
					constraint_decl.erase(active_decl_c->_c->constraint()->name()->value());
					// Generate rule header
					generate_rule_header(p,*active_decl_c);
				}
				str_next_occurrence = active_decl_c->_c->constraint()->name()->value() + "_";

				auto it2 = it + 1;
				if ((it2 != it_end) && (active_decl_c == (*it2)->active_constraint().constraint()->decl()))
					str_next_occurrence += std::to_string((*it2)->active_constraint_occurrence());
				else
					str_next_occurrence += "store";

				// -----------------------------------------------------------------
				// COMPUTE VARIABLES IDX TO BE SCHEDULE
				schedule_var_idx.clear();
				if (!active_decl_c->_never_stored)
				{
					auto pragmas = active_decl_c->_c->pragmas();
					if (std::find(pragmas.begin(), pragmas.end(), Pragma::no_reactivate)
							== pragmas.end())
					{
						unsigned int i=0;
						for (auto& cc : active_decl_c->_c->constraint()->children())
						{
							auto pptr = dynamic_cast< ast::PrefixExpression* >(cc.get());
							assert(pptr != nullptr);
							if (pptr->op() != "+")
							{
								assert((pptr->op() == "-") || (pptr->op() == "?"));
								schedule_var_idx.push_back(i);
							}
							++i;
						}
					}
				}

				// -----------------------------------------------------------------
				// GENERATE OCCURENCE RULE
				// If no body, write only storage to constraint store
				generate_occurrence_rule(p, **it, str_next_occurrence,schedule_var_idx);

				++it;
			}
			// -----------------------------------------------------------------
			// GENERATE FAIL THROUGH CASE FOR LAST ACTIVE CONSTRAINT SEEN
			if (active_decl_c)
				store_active_constraint(p,*active_decl_c,schedule_var_idx);

			// -----------------------------------------------------------------
			// GENERATE FAIL THROUGH CASE FOR NEVER STORED CONSTRAINTS
			std::vector< unsigned int > dummy;
			for (const auto& cdecl : constraint_decl)
			{
				generate_rule_header(p,*cdecl.second);
				store_active_constraint(p,*cdecl.second,dummy);
			}
		}
	}
}
