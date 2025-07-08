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
#include <unordered_set>
#include <ast/expression.hh>
#include <ast/body.hh>
#include <ast/program.hh>

namespace chr::compiler::visitor
{
	void OccRuleUpdateConstraintStoreIndexes::update_indexes(ast::OccRule& r)
	{
		r.accept( *this );
	}

	void OccRuleUpdateConstraintStoreIndexes::visit(ast::OccRule& r)
	{
		using namespace ast;
		std::unordered_set< std::string > seen_head_variables;
		// Update with variables of active constraint
		for (auto& cc : r.active_constraint().constraint()->children())
		{
			PositionInfo pos = cc->position();
			auto pV = dynamic_cast< LogicalVariable* >(cc.get());
			if (pV != nullptr)
			{
				std::string v_name = pV->value();
				if (v_name != "_")
					(void) seen_head_variables.insert(v_name);
			}
		}
	
		auto it1 = r.partners().begin();
		auto it_end = r.partners().end();
		while (it1 != it_end)
		{
			std::vector< unsigned int > index;
			std::unordered_set< std::string > head_variables;
			// Compute with all constraints arguments
			unsigned int i = 0;
			for (auto& cc : (*it1)._c->constraint()->children())
			{
				PositionInfo pos = cc->position();
				auto pL = dynamic_cast< Literal* >(cc.get());
				auto pV = dynamic_cast< CppVariable* >(cc.get());
				auto pLV = dynamic_cast< LogicalVariable* >(cc.get());
				if ((pL == nullptr) && (pV == nullptr) && (pLV == nullptr))
					throw ParseError("Unexpected argument in CHR constraint (should not occur)",cc->position());
				if (pL != nullptr)
				{
					assert((pLV == nullptr) && (pV == nullptr));
					index.emplace_back(i);
				}
				if (pV != nullptr)
				{
					assert((pLV == nullptr) && (pL == nullptr));
					index.emplace_back(i);
				}
				if (pLV != nullptr)
				{
					assert((pV == nullptr) && (pL == nullptr));
					std::string v_name = pLV->value();
					if (v_name != "_")
					{
						(void) head_variables.insert(v_name);
						if (seen_head_variables.find(v_name) != seen_head_variables.end())
							index.emplace_back(i);
					}
				}
				++i;
			}

			// Update seen head variables
			for (auto& v : head_variables)
				(void) seen_head_variables.insert(v);

			// Update index of *it1
			if (!index.empty())
			{
				assert((*it1)._use_index == -1);
				auto& indexes = (*it1)._c->constraint()->decl()->_indexes;
				auto ret = std::find(indexes.begin(),indexes.end(),index);
				// Add index only if it not already here
				if (ret == indexes.end())
				{
					(*it1)._use_index = indexes.size();
					(*it1)._c->constraint()->decl()->_indexes.emplace_back( index );
				} else {
					(*it1)._use_index = ret - indexes.begin();
				}
			}

			++it1;
		}
	}
}
