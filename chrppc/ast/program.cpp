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

#include <string_view>
#include <unordered_map>

#include <ast/program.hh>
#include <visitor/program.hh>

namespace chr::compiler::ast
{
	/*
	 * ChrConstraintDecl
	 */
	ChrConstraintDecl::ChrConstraintDecl(PtrChrConstraintCall c)
		: _c(std::move(c)), _never_stored(false)
	{ }

	/*
	 * ChrProgram
	 */

	ChrProgram::ChrProgram(const ChrProgram& o)
		: _name(o._name), _input_file_name(o._input_file_name), _prg_parameters(o._prg_parameters), _tpl_parameters(o._tpl_parameters), _include_hpp_file(o._include_hpp_file), _auto_persistent(o._auto_persistent), _auto_catch_failure(o._auto_catch_failure), _chr_constraints(o._chr_constraints), _rules(o._rules)
	{
		std::transform(o._occ_rules.cbegin(), o._occ_rules.cend(), std::back_inserter(_occ_rules), [](const PtrOccRule& r) {
				return PtrOccRule( new OccRule(*r) );
			});
	}

	void ChrProgram::set_name(std::string_view name)
	{
		_name = name;
	}

	std::string_view ChrProgram::name() const
	{
		return _name;
	}

	void ChrProgram::set_input_file_name(std::string_view file_name)
	{
		_input_file_name = file_name;
	}

	std::string_view ChrProgram::input_file_name() const
	{
		return _input_file_name;
	}

	void ChrProgram::set_parameters(std::vector< std::tuple<std::string,std::string> > parameters)
	{
		_prg_parameters = std::move(parameters);
	}

	const std::vector< std::tuple<std::string,std::string> >& ChrProgram::parameters() const
	{
		return _prg_parameters;
	}

	void ChrProgram::set_template_parameters(std::vector< std::tuple<std::string,std::string> > parameters)
	{
		_tpl_parameters = std::move(parameters);
	}

	const std::vector< std::tuple<std::string,std::string> >& ChrProgram::template_parameters() const
	{
		return _tpl_parameters;
	}

	std::string ChrProgram::include_hpp_file() const
	{
		return _include_hpp_file;
	}

	void ChrProgram::set_include_hpp_file(std::string name)
	{
		_include_hpp_file = name;
	}

	bool ChrProgram::auto_persistent() const
	{
		return _auto_persistent;
	}

	void ChrProgram::set_auto_persistent(bool b)
	{
		_auto_persistent = b;
	}

	bool ChrProgram::auto_catch_failure() const
	{
		return _auto_catch_failure;
	}

	void ChrProgram::set_auto_catch_failure(bool b)
	{
		_auto_catch_failure = b;
	}

	std::vector< PtrSharedChrConstraintDecl >& ChrProgram::chr_constraints()
	{
		return _chr_constraints;
	}

	void ChrProgram::add_chr_constraint(PtrChrConstraintCall new_c)
	{
		std::string_view new_c_name = new_c->constraint()->name()->value();

		// Compare with all previous constraint name
		for (auto& c : _chr_constraints)
		{
			if (new_c_name == c->_c->constraint()->name()->value())
			{
				std::string msg = R"_STR(parse error, CHR constraint duplicate name ")_STR" + std::string(new_c_name) + R"_STR(")_STR";
				throw ParseError(msg.c_str(), new_c->position());
			}

		}
		_chr_constraints.emplace_back( std::make_shared<ChrConstraintDecl>(std::move(new_c)) );
	}

	std::vector< PtrSharedRule >& ChrProgram::rules()
	{
		return _rules;
	}

	std::vector< PtrOccRule >& ChrProgram::occ_rules()
	{
		return _occ_rules;
	}

	void ChrProgram::add_rule(PtrSharedRule new_r)
	{
		std::string_view new_r_name(new_r->name());
		// Compare with all previous rules name
		if (!new_r_name.empty())
			for (auto& r : _rules)
			{
				if (r->name() == new_r_name)
				{
					std::string msg = R"_STR(parse error, rule duplicate name ")_STR" + std::string(new_r_name) + R"_STR(")_STR";
					throw ParseError(msg.c_str(), new_r->position());
				}
			}
		_rules.emplace_back( std::move(new_r) );
	}

	void ChrProgram::accept(visitor::ProgramVisitor& v)
	{
		v.visit(*this);
	}

} // namespace chr::compiler::ast
