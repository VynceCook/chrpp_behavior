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

#include <vector>
#include <string>
#include <ast/program.hh>
#include <ast/rule.hh>

namespace chr::compiler::parser::ast_builder
{
	/**
	 * @brief Ast program building
	 *
	 * AstProgramBuilder contains the whole CHR program (constraints,
	 * rules, and other parameters).
	 */
	struct AstProgramBuilder
	{
		/**
		 * Function to add a new CHR constraint declaration.
		 * @param r The rule to add
		 */
		void add_chr_constraint(ast::PtrExpression e)
		{
			// Check that expression e is a CHR constraint
			ast::ChrConstraint* pchr = dynamic_cast< ast::ChrConstraint* >(e.get());
			if (pchr == nullptr)
				throw ParseError("parse error matching constraint declaration", e->position());
			PositionInfo pos = e->position();
			e.release();
			prg.add_chr_constraint( std::make_unique<ast::ChrConstraintCall>( ast::PtrChrConstraint(pchr), pos) );
		}
		/**
		 * Function to add a pragma to the last CHR constraint declared.
		 * @param p The pragma to add
		 */
		void add_chr_constraint_pragma(Pragma p)
		{
			assert(prg.chr_constraints().size() > 0);
			prg.chr_constraints().at( prg.chr_constraints().size() - 1)->_c->add_pragma(p);
		}

		/**
		 * Function to add a new rule to the program.
		 * @param r The rule to add
		 */
		void add_rule(ast::PtrRule r)
		{
			prg.add_rule( std::move(r) );
		}

		unsigned int _start_line_number = 0;///< The line number where the program started
		unsigned int _end_line_number = 0;	///< The line number where the program ended
		bool _empty = true;					///< True if no real CHR program has been matched
		ast::ChrProgram prg;				///< The CHR program
		std::string _include_file_name;		///< File name of last chr_include
		std::string _str_dependency_graph;	///< String representation of the dependency graph (for debbuging purposes)
	};
} // namespace chr::compiler::parser::ast_builder
