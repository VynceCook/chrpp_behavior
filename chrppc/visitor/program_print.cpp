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
#include <visitor/occ_rule.hh>
#include <visitor/rule.hh>
#include <visitor/body.hh>

namespace chr::compiler::visitor
{
	std::string_view ProgramPrint::string_from(ast::ChrProgram& p)
	{
		_str_program.clear();
		p.accept( *this );
		return _str_program;
	}

	void ProgramPrint::visit(ast::ChrProgram& p)
	{
		chr::compiler::visitor::BodyPrint body_v;
		chr::compiler::visitor::RulePrint rule_v;
		chr::compiler::visitor::OccRulePrint occ_rule_v;
		std::string tmp;
		for (auto& c : p.chr_constraints())
		{
			_str_program += "(constraint) ";
			if (c->_never_stored)
				_str_program += "##";
			_str_program += std::string( body_v.string_from(*c->_c) );
			if (!c->_indexes.empty())
			{
				bool first1 = true;
				_str_program += ", indexes: {";
				for (auto& index : c->_indexes)
				{
					if (!first1)
						_str_program += ", <";
					else {
						_str_program += " <";
						first1 = false;
					}
					bool first2 = true;
					for (auto& i : index)
						if (!first2)
							_str_program += "," + std::to_string(i);
						else {
							_str_program += std::to_string(i);
							first2 = false;
						}
					_str_program += ">";
				}
				_str_program += " }";
			}
			auto pragmas = c->_c->pragmas();
			std::size_t n_pragmas = pragmas.size();
			if (n_pragmas > 0)
			{
				_str_program += ", ";
				if (n_pragmas > 1)
					_str_program += "{ ";
				for( std::size_t i = 0; i < n_pragmas; ++i ) {
					if( i > 0 )
						_str_program += ", ";
					_str_program += StrPragma.at( (int)pragmas[i] );
				}
				if (n_pragmas > 1)
					_str_program += " }";
			}
			_str_program += "\n";
		}
		for (auto& r : p.rules())
			_str_program += "(rule) " + std::string( rule_v.string_from(*r) ) + "\n";
		for (auto& r : p.occ_rules())
			_str_program += "(occ rule) " + std::string( occ_rule_v.string_from(*r) ) + "\n";
	}
} // namespace chr::compiler::visitor
