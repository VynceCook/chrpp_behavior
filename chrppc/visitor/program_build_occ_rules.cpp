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
#include <ast/rule.hh>
#include <unordered_map>

namespace chr::compiler::visitor
{
	void ProgramBuilOccRules::build(ast::ChrProgram& p)
	{
		p.accept( *this );
	}

	void ProgramBuilOccRules::visit(ast::ChrProgram& p)
	{
		std::unordered_map< std::string_view, unsigned int > occurrences;
		for (auto& r : p.rules())
		{
			unsigned int idx = 0;
			std::vector< ast::PtrChrConstraintCall > *rule_set1;
			std::vector< ast::PtrChrConstraintCall > *rule_set2;
			bool set1_keep, set2_keep;

			if (chr::compiler::Compiler_options::OCCURRENCES_REORDER)
			{
				rule_set1 = &r->head_del();
				set1_keep = false;
				rule_set2 = &r->head_keep();
				set2_keep = true;
			} else {
				rule_set1 = &r->head_keep();
				set1_keep = true;
				rule_set2 = &r->head_del();
				set2_keep = false;
			}

			for (auto& c : *rule_set1)
			{
				// If pragma passive, do not store this occurence rule
				if (std::find(c->pragmas().begin(), c->pragmas().end(), Pragma::passive) == c->pragmas().end())
				{
					auto ret = occurrences.insert( {c->constraint()->name()->value(), 0} );
					if (!ret.second) ++(*ret.first).second;
					p.occ_rules().emplace_back( std::make_unique<ast::OccRule>(r, (*ret.first).second, idx, set1_keep) );
				}
				++idx;
			}
			idx = 0;
			for (auto& c : *rule_set2)
			{
				// If pragma passive, do not store this occurence rule
				if (std::find(c->pragmas().begin(), c->pragmas().end(), Pragma::passive) == c->pragmas().end())
				{
					auto ret = occurrences.insert( {c->constraint()->name()->value(), 0} );
					if (!ret.second) ++(*ret.first).second;
					p.occ_rules().emplace_back( std::make_unique<ast::OccRule>(r, (*ret.first).second, idx, set2_keep) );
				}
				++idx;
			}
		}
		// Sort occurrences rules
		std::sort(p.occ_rules().begin(), p.occ_rules().end(), [](const std::unique_ptr<ast::OccRule>& r1, const std::unique_ptr<ast::OccRule> &r2)
				{
					auto r1_name = r1->active_constraint().constraint()->name()->value();
					auto r2_name = r2->active_constraint().constraint()->name()->value();
					return ((r1_name < r2_name) || ((r1_name == r2_name) && (r1->active_constraint_occurrence() < r2->active_constraint_occurrence())));
				});
	}
} // namespace chr::compiler::visitor
