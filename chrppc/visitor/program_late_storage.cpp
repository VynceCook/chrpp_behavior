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

namespace chr::compiler::visitor
{
	void ProgramLateStorage::apply(const DependencyGraph& g, ast::ChrProgram& p)
	{
		_graph = &g;
		p.accept( *this );
	}

	void ProgramLateStorage::visit(ast::ChrProgram& p)
	{
		assert(_graph != nullptr);
		ast::PtrSharedChrConstraintDecl active_c;
		auto it = p.occ_rules().begin();
		while (it != p.occ_rules().end())	
		{
			// Check if we still have the same active constraint
			if (active_c != (*it)->active_constraint().constraint()->decl())
				active_c = (*it)->active_constraint().constraint()->decl();

			// Check if we have an active constraint to keep
			if ((*it)->keep_active_constraint())
			{
				// we check if the constraint is not observed
				if (!_graph->observed((*it)->active_constraint().constraint()))
					(*it)->set_store_active_constraint(false);
			}
			++it;
		}
	}
} // namespace chr::compiler::visitor
