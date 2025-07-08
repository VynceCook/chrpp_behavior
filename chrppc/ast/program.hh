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

#pragma once

#include <string>
#include <vector>
#include <memory>

#include <tao/pegtl.hpp>
#include <common.hh>
#include <parse_error.hh>
#include <ast/rule.hh>
#include <ast/body.hh>
#include <ast/expression.hh>

namespace chr::compiler::visitor {
	struct ProgramVisitor;
};

namespace chr::compiler::ast
{
	/**
	 * @brief Model a CHR constraint store
	 *
	 * This structure represents a shop of constraints.
	 * It contains a link to the constraint declaration
	 * combined with other important information.
	 */
	struct ChrConstraintDecl {
		PtrSharedChrConstraintCall _c;	///< Ref to constraint store definition
		bool _never_stored;				///< True if the constraint will be never stored
		std::vector< std::vector< unsigned int > > _indexes;	///< The indexes needed for the corresponding constraint store

		/**
		 * Default constructor.
		 * @param c The constraint declaration
		 */
		ChrConstraintDecl(PtrChrConstraintCall c);	
	};

	/**
	 * Shortcut to shared ptr of ChrConstraintDecl
	 */
	using PtrSharedChrConstraintDecl = std::shared_ptr< ChrConstraintDecl >;

	/**
	 * @brief CHR program class
	 *
	 * The ChrProgram class gather all elements of a CHR program (CHR constraints, rules,
	 * program parameters, ...).
	 * It stores the parsed rules, the optimized occurrence rules and all internal
	 * representations of the CHR program.
	 */
	class ChrProgram
	{
	public:
		/**
		 * Default constructor.
		 */
		ChrProgram() = default;

		/**
		 * Copy constructor. The occurrences rules are duplicated,
		 * but other shared data structures are shared with the new ChrProgram.
		 * @param o the other element
		 */
		ChrProgram(const ChrProgram& o);

		/**
		 * Return the name of the program.
		 * @return The name of the program
		 */
		std::string_view name() const;

		/**
		 * Set the name of the program
		 * @param name The name of the program
		 */
		void set_name(std::string_view name);

		/**
		 * Return the file name used for building this program.
		 * @return The file name
		 */
		std::string_view input_file_name() const;

		/**
		 * Set the parameters of this program (local C++ object)
		 * @param parameters The parameters
		 */
		void set_parameters(std::vector< std::tuple<std::string,std::string> > parameters);

		/**
		 * Return the template parameters of this program.
		 * @return The template parameters
		 */
		const std::vector< std::tuple<std::string,std::string> >& template_parameters() const;

		/**
		 * Set the template parameters of this program (C++ template parameters for class)
		 * @param parameters The template parameters
		 */
		void set_template_parameters(std::vector< std::tuple<std::string,std::string> > parameters);

		/**
		 * Return the parameters of this program.
		 * @return The parameters
		 */
		const std::vector< std::tuple<std::string,std::string> >& parameters() const;

		/**
		 * Set the input file name used for building this program.
		 * @param name The file name
		 */
		void set_input_file_name(std::string_view file_name);

		/**
		 * Return the hpp output file name used for putting all function definitions.
		 * The result is not empty only when the template_parameters is not empty.
		 * @return The file name
		 */
		std::string include_hpp_file() const;

		/**
		 * Set the hpp output file name used for putting all function definitions.
		 * Used only when template_parameters is not empty and that we need the file
		 * name for generating the code.
		 * @param name The file name
		 */
		void set_include_hpp_file(std::string name);

		/**
		 * Return if stores of the program can be set persistent if no backtrack
		 * operator is found.
		 * @return True if the stores can be set persistent, false otherwise
		 */
		bool auto_persistent() const;

		/**
		 * Set the value to know if stores of the program can be set persistent if no
		 * backtrack operator is found.
		 * @param b The boolean value
		 */
		void set_auto_persistent(bool b);

		/**
		 * Return true if CHR program must try to detect failure of builtins constraints.
		 * @return True if the CHR program must try to detect the failures, false otherwise
		 */
		bool auto_catch_failure() const;

		/**
		 * Set to true if CHR program must try to detect failure of builtins constraints.
		 * @param b The boolean value
		 */
		void set_auto_catch_failure(bool b);

		/**
		 * Return the CHR constraints declared in the program.
		 * @return The CHR constraints
		 */
		std::vector< PtrSharedChrConstraintDecl >& chr_constraints();

		/**
		 * Add a new CHR constraint to the set of declared CHR constraints.
		 * @param new_c The CHR constraint to add
		 */
		void add_chr_constraint(PtrChrConstraintCall new_c);

		/**
		 * Return the rules of the program.
		 * @return The rules
		 */
		std::vector< PtrSharedRule >& rules();

		/**
		 * Return the rules of the program.
		 * @return The rules
		 */
		std::vector< PtrOccRule >& occ_rules();

		/**
		 * Add a new rule to the list of rules.
		 * @param new_r The rule to add
		 */
		void add_rule(PtrSharedRule new_r);

		/**
		 * Accept ProgramVisitor
		 */
		void accept(visitor::ProgramVisitor&);

	protected:
		std::string _name = "_CHR_PRG_";										///< Name of the program
		std::string _input_file_name = "";										///< Name of the input file used to built this program
		std::vector< std::tuple<std::string, std::string> > _prg_parameters;	///< Parameters of the program
		std::vector< std::tuple<std::string, std::string> > _tpl_parameters;	///< Template parameters of the program
		std::string _include_hpp_file = "";										///< Name of the output file used to put function definitions
		bool _auto_persistent = true;											///< True if constaint stores can be automatically set persistent, false otherwise
		bool _auto_catch_failure = true;										///< True if CHR program try to automatically catch failure of builtins constraints, false otherwise 
		std::vector< PtrSharedChrConstraintDecl > _chr_constraints;				///< CHR constraints declared int the program
		std::vector< PtrSharedRule > _rules;									///< Rules of the program
		std::vector< PtrOccRule > _occ_rules;									///< Occurrence rules of the program
	};
} // namespace chr::compiler::ast:input
