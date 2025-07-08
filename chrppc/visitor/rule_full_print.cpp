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

#include <visitor/rule.hh>
#include <visitor/body.hh>
#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
					return !std::isspace(ch);
					}));
	}

	std::string_view RuleFullPrint::string_from(ast::Rule& r, std::size_t prefix)
	{
		_str_rule.clear();
		_prefix = prefix;
		r.accept( *this );
		return _str_rule;
	}

	void RuleFullPrint::rule_string_build(ast::Rule& r, std::string_view op)
	{
		chr::compiler::visitor::ExpressionPrint expression_v;
		chr::compiler::visitor::BodyFullPrint body_v;
		bool first = true;
		for (auto& pc : r.head_keep())
		{
			if (first)
				first = false;
			else
				_str_rule += ", ";
			_str_rule += std::string(body_v.string_from( *pc ));
		}
		if (!(r.head_del().empty() || r.head_keep().empty()))
			_str_rule += " \\ ";
		first = true;
		for (auto& pc : r.head_del())
		{
			if (first)
				first = false;
			else
				_str_rule += ", ";
			_str_rule += std::string(body_v.string_from( *pc ));
		}
		_str_rule += " " + std::string(op) + " ";
		if (!r.guard().empty())
		{
			first = true;
			for (auto& pc : r.guard())
			{
				if (first)
					first = false;
				else
					_str_rule += ", ";
				_str_rule += std::string(expression_v.string_from( *pc ));
			}
			_str_rule += " | ";
		}

		auto tmp = std::string(body_v.string_from( *r.body(), _prefix + _str_rule.size() ));
		ltrim(tmp);
		_str_rule += tmp;
		_str_rule += " ;;";
	}

	void RuleFullPrint::visit(ast::PropagationRule& r)
	{
		rule_string_build(r,"==>");
	}

	void RuleFullPrint::visit(ast::PropagationNoHistoryRule& r)
	{
		rule_string_build(r,"=>>");
	}

	void RuleFullPrint::visit(ast::SimplificationRule& r)
	{
		rule_string_build(r,"<=>");
	}

	void RuleFullPrint::visit(ast::SimpagationRule& r)
	{
		rule_string_build(r,"<=>");
	}
}
