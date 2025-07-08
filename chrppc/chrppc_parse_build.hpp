/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Apache License, Version 2.0.
 *
 *  Copyright:
 *     2023, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
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

#include <tao/pegtl.hpp>

#include <parse_error.hh>
#include <parser/grammar.hpp>
#include <visitor/body.hh>
#include <visitor/rule.hh>
#include <visitor/occ_rule.hh>
#include <visitor/program.hh>

#include <iostream>
#include <iomanip>

namespace chr::compiler {
	/**
	 * Parse the input \a in and search for all CHR programs.
	 * @param in The input to parse
	 * @param chr_prg_results The vector of programs to populate
	 * @param input_file_name The input file name
	 * @param os_err The output stream for error messages
	 * @return 0 if the parsing succeeded, 1 otherwise
	 */
	template< typename T >
	int parse_chr_input(T& in, std::vector< chr::compiler::parser::ast_builder::AstProgramBuilder >& chr_prg_results, std::string input_file_name, std::ostream& os_err)
	{
		while (true) {
			try {
				// Create new program result object
				chr_prg_results.push_back( chr::compiler::parser::ast_builder::AstProgramBuilder() );
				auto& prg_res = chr_prg_results.back();
				if (!input_file_name.empty())
					prg_res.prg.set_input_file_name(input_file_name);
	
				TAO_PEGTL_NAMESPACE::parse< chr::compiler::parser::grammar::program, chr::compiler::parser::action, chr::compiler::parser::error_control >( in, prg_res );
	
				// No program found, break from the loop
				if (prg_res._empty)
				{
					chr_prg_results.pop_back();
					break;
				}
	
				auto& chr_prg = prg_res.prg;
				if (chr_prg.name() == "_CHR_PRG_") chr_prg.set_name( std::string(chr_prg.name()) + std::to_string(chr_prg_results.size() - 1) );
				// Check for duplicate program names
				for (unsigned int i=0; i < (chr_prg_results.size()-1); ++i)
					if (chr_prg_results[i].prg.name() == chr_prg.name())
					{
						auto error_pos = in.position();
						error_pos.line = 0;
						error_pos.column = 0;
						throw chr::compiler::ParseError(std::string("duplicate CHR program name " + std::string(chr_prg.name())).c_str(), error_pos);
					}
	
				// Automatic detection of all #persistent stores
				bool apply_all_persistent = false;
				if (chr_prg.auto_persistent())
				{
					apply_all_persistent = true;
					chr::compiler::visitor::BodyApply bav;
					auto f_body = [&apply_all_persistent](chr::compiler::ast::Body& b) {
						auto pSequence = dynamic_cast< chr::compiler::ast::Sequence* >(&b);
						auto pBehavior = dynamic_cast< chr::compiler::ast::ChrBehavior* >(&b);
						auto pTry = dynamic_cast< chr::compiler::ast::ChrTry* >(&b);
						if ((pBehavior != nullptr) || (pTry != nullptr) ||
								((pSequence != nullptr) && (pSequence->op() == ";")))
						{
							apply_all_persistent = false;
							return false;
						}
						return true;
					};
					for (auto& r : chr_prg.rules())
						bav.apply(*r->body(),f_body);
				}
	
				// Apply program parameter all_persistent
				if (apply_all_persistent)
				{
					chr::compiler::visitor::ProgramSetAllPersistent psapv;
					psapv.apply(chr_prg);
				}
	
				// Compute dependency graph
				chr::compiler::visitor::RuleDependencyGraph vrdg1;
				for (auto& r : chr_prg.rules())
					vrdg1.populate( *r );
	
				// Debug dependency graph
				prg_res._str_dependency_graph = vrdg1.to_string();
	
				// Build occurrence rules from the parsed rules
				chr::compiler::visitor::ProgramBuilOccRules vp1;
				vp1.build(chr_prg);
	
				// Apply optimization on occurrence rules
				chr::compiler::visitor::OccRuleHeadReorder vor1;
				chr::compiler::visitor::OccRuleGuardReorder vor2;
				chr::compiler::visitor::OccRuleUpdateConstraintStoreIndexes vor3;
				for (auto& r : chr_prg.occ_rules())
				{
					if (chr::compiler::Compiler_options::HEAD_REORDER)
						vor1.reorder( *r );
					if (chr::compiler::Compiler_options::GUARD_REORDER)
						vor2.reorder( *r );
					vor3.update_indexes( *r );
				}
	
				// Set some constraint stores as NEVER_STORED
				if (chr::compiler::Compiler_options::NEVER_STORED)
				{
					chr::compiler::visitor::ProgramNeverStored vp2;
					vp2.apply(chr_prg);
				}
	
				// Apply late storage from previously computed graph
				chr::compiler::visitor::ProgramLateStorage vp3;
				vp3.apply(vrdg1.graph(), chr_prg);
			}
			catch( const TAO_PEGTL_NAMESPACE::parse_error& e ) {
				const auto p = e.positions().front();
				std::string_view line = in.line_at(p);
				size_t n = std::count(line.begin(), line.begin() + p.column, '\t');
				os_err << e.what() << std::endl
					<< in.line_at(p) << std::endl;
				for (size_t i=n; i--;) os_err << '\t';
				os_err << std::setw( p.column - n ) << '^' << std::endl;
				return 1;
			}
			catch( const chr::compiler::ParseError& e ) {
				const auto p = e.positions().front();
				os_err << e.what() << std::endl;
				if ((p.line != 0) || (p.column != 0))
				{
					std::string_view line = in.line_at(p);
					size_t n = std::count(line.begin(), line.begin() + p.column, '\t');
					if (e.in_line().empty()) {
						os_err << in.line_at(p) << std::endl;
					} else {
						os_err << e.in_line() << std::endl;
					}
					for (size_t i=n; i--;) os_err << '\t';
					os_err << std::setw( p.column - n ) << '^' << std::endl;
				}
				return 1;
			}
			catch(...)
			{
				os_err << "error: chr: unknown error when parsing" << std::endl;
				return 1;
			}
		}
		return 0;
	}

	/**
	 * Parse the input \a in and search for all CHR programs.
	 * All error messages are sent to std::cerr.
	 * @param in The input to parse
	 * @param chr_prg_results The vector of programs to populate
	 * @param input_file_name The input file name
	 * @return 0 if the parsing succeeded, 1 otherwise
	 */
	template< typename T >
	int parse_chr_input(T& in, std::vector< chr::compiler::parser::ast_builder::AstProgramBuilder >& chr_prg_results, std::string input_file_name = "")
	{
		return parse_chr_input(in,chr_prg_results,input_file_name,std::cerr);
	}
} // namespace chr::compiler

