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
#include <visitor/expression.hh>
#include <unordered_set>
#include <ast/expression.hh>
#include <ast/body.hh>

namespace chr::compiler::visitor
{
	void OccRuleGuardReorder::reorder(ast::OccRule& r)
	{
		r.accept( *this );
	}

	void OccRuleGuardReorder::visit(ast::OccRule& r)
	{
		using namespace ast;
		std::unordered_set< std::string > not_decl_head_variables;
		std::unordered_set< std::string > not_decl_assignment_variables;
	
		// Populate with head variables
		for (auto& c : r.partners())
		{
			// Compute with all constraints arguments
			for (auto& cc : c._c->constraint()->children())
			{
				auto pV = dynamic_cast< LogicalVariable* >(cc.get());
				if (pV != nullptr)
				{
					std::string v_name = pV->value();
					if (v_name != "_")
						(void) not_decl_head_variables.insert(v_name);
				}
			}
		}
		// Remove active constraint head variables
		for (auto& cc : r.active_constraint().constraint()->children())
		{
			auto pV = dynamic_cast< LogicalVariable* >(cc.get());
			if (pV != nullptr)
			{
				std::string v_name = pV->value();
				if (v_name != "_")
					(void) not_decl_head_variables.erase(v_name);
			}
		}

		// Populate with guard assign left variables
		for (auto& e : r.guard_parts().back())
		{
			auto pA = dynamic_cast< InfixExpression* >(e.get());
			if ((pA != nullptr) and (pA->op() == "="))
			{
				auto pV = dynamic_cast< CppVariable* >(pA->l_child().get());
				assert(pV != nullptr);
				std::string v_name = pV->value();
				if (v_name != "_")
					(void) not_decl_assignment_variables.insert(v_name);
			}
		}

		// Visitor to check if a variable of an expression occurs in the
		// not_decl_* variables
		ExpressionFullCheck vis;
		auto f = [&](const Expression& e) {
			auto p0 = dynamic_cast< const LogicalVariable* >(&e);
			if (p0 != nullptr)
				return not_decl_head_variables.find(p0->value()) != not_decl_head_variables.end();
			auto p1 = dynamic_cast< const CppVariable* >(&e);
			if (p1 != nullptr)
				return not_decl_assignment_variables.find(p1->value()) != not_decl_assignment_variables.end();
			return false;
		};
		for (unsigned int i = 0; i < r.guard_parts().size()-1; ++i)
		{
			// Gather head variables of next partner constraint
			std::unordered_set< std::string > head_variables;
			if (i < r.partners().size())
			{
				for (auto& v : r.partners()[i]._c->constraint()->children())
				{
					auto pV = dynamic_cast< LogicalVariable* >(v.get());
					if (pV != nullptr)
					{
						std::string v_name = pV->value();
						if (v_name != "_")
							(void) head_variables.insert(v_name);
					}
				}
			}

			// Search for guard parts
			auto it = r.guard_parts().back().begin();
			while (it != r.guard_parts().back().end())
			{
				auto pA = dynamic_cast< InfixExpression* >((*it).get());
				bool ret;
				if ((pA != nullptr) && (pA->op() == "="))
					ret = vis.check(*pA->r_child(),f);
				else 
					ret = vis.check(**it,f);
				if (!ret)
				{
					// Update not_decl_assignment_variables with variables of
					// current assignment expression
					if ((pA != nullptr) && (pA->op() == "="))
					{
						auto pV = dynamic_cast< CppVariable* >(pA->l_child().get());
						assert(pV != nullptr);
						std::string v_name = pV->value();
						(void) not_decl_assignment_variables.erase(v_name);
					}

					r.guard_parts()[i].emplace_back( std::move(*it) );
					it = r.guard_parts().back().erase( it );
				}
				else
					++it;
			}

			// Update not_decl_head_variables with variables of next partner constraint
			for (auto& v : head_variables)
				(void) not_decl_head_variables.erase(v);
		}
	}
}
