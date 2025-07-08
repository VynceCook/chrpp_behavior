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
#include <visitor/rule.hh>
#include <visitor/body.hh>
#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	std::string_view OccRulePrint::string_from(ast::OccRule& r)
	{
		_str_rule.clear();
		r.accept( *this );
		return _str_rule;
	}

	void OccRulePrint::visit(ast::OccRule& r)
	{
		chr::compiler::visitor::ExpressionPrint expression_v;
		chr::compiler::visitor::BodyPrint body_v;
		if (!r.name().empty())
			_str_rule += std::string(r.name()) + " @ ";
		_str_rule += "[";
		if (r.keep_active_constraint())
		{
			if (r.store_active_constraint())
				_str_rule += "+";
			else
				_str_rule += "=";
		} else
			_str_rule += "-";
		_str_rule += std::string(body_v.string_from( r.active_constraint() ));
		_str_rule += "#" + std::to_string(r.active_constraint_occurrence());
		auto pragmas = r.active_constraint().pragmas();
		std::size_t n_pragmas = pragmas.size();
		if (n_pragmas > 0)
		{
			_str_rule += "#";
			if (n_pragmas > 1)
				_str_rule += "{";
			for( std::size_t i = 0; i < n_pragmas; ++i ) {
				if( i > 0 )
					_str_rule += ",";
				_str_rule += StrPragma.at( (int)pragmas[i] );
			}
			if (n_pragmas > 1)
				_str_rule += "}";
		}
		_str_rule += "][";
		bool first = true;
		for (auto& e : r.guard_parts().front())
		{
			if (first)
				first = false;
			else
				_str_rule += ", ";
			_str_rule += std::string(expression_v.string_from( *e ));
		}
		std::size_t i = 1;
		for (auto& p : r.partners())
		{
			if (first)
				first = false;
			else
				_str_rule += ", ";
			_str_rule += p._keep?"+":"-";
			_str_rule += std::string(body_v.string_from( *p._c ));
			if (p._use_index != -1)
				_str_rule += "<idx#" + std::to_string(p._use_index) + ">";
			auto pragmas = p._c->pragmas();
			std::size_t n_pragmas = pragmas.size();
			if (n_pragmas > 0)
			{
				_str_rule += "#";
				if (n_pragmas > 1)
					_str_rule += "{";
				for( std::size_t i = 0; i < n_pragmas; ++i ) {
					if( i > 0 )
						_str_rule += ",";
					_str_rule += StrPragma.at( (int)pragmas[i] );
				}
				if (n_pragmas > 1)
					_str_rule += "}";
			}
			for (auto& e : r.guard_parts()[i])
				_str_rule += ", " + std::string(expression_v.string_from( *e ));
			++i;
		}
		_str_rule += "] --> ";
		_str_rule += std::string(body_v.string_from( *r.body() ));
		_str_rule += " ;;";
	}
} // namespace chr::compiler::visitor
