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

namespace chr::compiler::visitor
{
	void OccRuleHeadReorder::reorder(ast::OccRule& r)
	{
		r.accept( *this );
	}

	void OccRuleHeadReorder::visit(ast::OccRule& r)
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
			std::unordered_set< std::string > max_head_variables;
			unsigned int max_weight = 0;
			auto max_it = it1;
			auto it2 = it1;
			// Search for the hightest priority partner
			while (it2 != it_end)
			{
				std::unordered_set< std::string > head_variables;
				unsigned int weight = 0;
				// Compute with all constraints arguments
				for (auto& cc : (*it2)._c->constraint()->children())
				{
					PositionInfo pos = cc->position();
					auto pL = dynamic_cast< Literal* >(cc.get());
					auto pV = dynamic_cast< CppVariable* >(cc.get());
					auto pLV = dynamic_cast< LogicalVariable* >(cc.get());
					if ((pL == nullptr) && (pV == nullptr) && (pLV == nullptr))
						throw ParseError("Unexpected argument in CHR constraint (should not occur)",cc->position());
					if ((pL != nullptr) || (pV != nullptr))
					{
						weight += 100;
					}
					if (pLV != nullptr)
					{
						std::string v_name = pLV->value();
						if (v_name != "_")
						{
							(void) head_variables.insert(v_name);
							if (seen_head_variables.find(v_name) != seen_head_variables.end())
								weight += 10;
						}
					}
				}

				if (weight > max_weight)
				{
					max_head_variables = std::move( head_variables );
					max_weight = weight;
					max_it = it2;
				}
				++it2;
			}

			// Update seen head variables
			for (auto& v : max_head_variables)
				(void) seen_head_variables.insert(v);
			// Swap head elements
			if (it1 != max_it)
				std::swap(*it1, *max_it);
			++it1;
		}
	}
}
