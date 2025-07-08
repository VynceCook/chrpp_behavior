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
#include <visitor/expression.hh>
#include <ast/rule.hh>
#include <parse_error.hh>
#include <iostream>

namespace chr::compiler::visitor
{
	void ProgramNeverStored::apply(ast::ChrProgram& p)
	{
		p.accept( *this );
	}

	void ProgramNeverStored::visit(ast::ChrProgram& p)
	{
		ast::PtrSharedChrConstraintDecl active_c;
		auto it1 = p.occ_rules().begin();
		bool may_be_never_stored = true;
		while (it1 != p.occ_rules().end())
		{
			// Check if we still have the same active constraint
			if (active_c != (*it1)->active_constraint().constraint()->decl())
			{
				active_c = (*it1)->active_constraint().constraint()->decl();
				assert(!active_c->_never_stored);
				may_be_never_stored = true;
			}

			// Check if we have a rule after a never stored rule
			if (active_c->_never_stored)
			{
				if (chr::compiler::Compiler_options::WARNING_UNUSED_RULE)
				{
					PositionInfo pos = (*it1)->position();
					std::string msg;
					msg = "warning: an unused occurence of rule";
					if (!(*it1)->name().empty())
						msg += " named '" + std::string((*it1)->name()) + "'";
					msg += " has been detected and removed";
					std::cerr << ParseError(msg.c_str(),pos).what() << std::endl;
				}
				// Drop the rule, as it will be never triggered
				it1 = p.occ_rules().erase( it1 );
				continue;
			}

			visitor::ExpressionFullCheck v;
			std::unordered_set< std::string > seen_variables;
			auto f = [&](ast::Expression& e) {
				auto p0 = dynamic_cast< const ast::Literal* >(&e);
				auto p1 = dynamic_cast< const ast::CppVariable* >(&e);
				auto p2 = dynamic_cast< const ast::LogicalVariable* >(&e);
				if (p2 != nullptr)
				{
					std::string v_name = p2->value();
					if (v_name != "_")
					{
						// Check duplicate name variables in active constraint
						if (seen_variables.find(v_name) != seen_variables.end())
							return true;
						else
							(void) seen_variables.insert(v_name);
					}
					return false;
				}
				else
					return ((p0 != nullptr) || (p1 != nullptr));
			};

			if ((*it1)->keep_active_constraint())
				may_be_never_stored = false;

			// Check if the constraint will be never stored
			if (may_be_never_stored
					&&!(*it1)->keep_active_constraint()
					&& (*it1)->partners().empty()
					&& (*it1)->guard().empty()
					&& !v.check(*(*it1)->active_constraint().constraint(),f))
				active_c->_never_stored = true;

			++it1;
		}

		// Remove rules occurrences when a partner is a never stored constraint
		auto it2 = p.occ_rules().begin();
		while (it2 != p.occ_rules().end())	
		{
			// Check if we still have the same active constraint
			if (active_c != (*it2)->active_constraint().constraint()->decl())
				active_c = (*it2)->active_constraint().constraint()->decl();

			// Check if we have a never triggered rule because a partner is
			// a never stored constraint
			bool found = false;
			for (auto& c : (*it2)->partners())
			{
				if (c._c->constraint()->decl()->_never_stored)
				{
					found = true;
					break;
				}
			}

			if (found)
			{
				if (chr::compiler::Compiler_options::WARNING_UNUSED_RULE)
				{
					PositionInfo pos = (*it2)->position();
					std::string msg;
					msg = "info: an unused occurence of rule";
					if (!(*it2)->name().empty())
						msg += " named '" + std::string((*it2)->name()) + "'";
					msg += " has been detected and automatically removed";
					std::cerr << ParseError(msg.c_str(),pos).what() << std::endl;
				}
				it2 = p.occ_rules().erase( it2 );
			} else
				++it2;
		}

	}
} // namespace chr::compiler::visitor
