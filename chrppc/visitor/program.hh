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
#include <cassert>

#include <ast/program.hh>
#include <dependency_graph.hh>

namespace chr::compiler::visitor
{
	/**
	 * @brief Root class of the program visitor
	 *
	 */
	struct ProgramVisitor {
		/**
		 * Root visit function for ChrProgram.
		 */
		virtual void visit(ast::ChrProgram&) = 0;

		/**
		 * Default virtual destructor
		 */
		virtual ~ProgramVisitor() = default;
	};

	/**
	 * @brief Program visitor which build a string representation of the program
	 */
	struct ProgramPrint : ProgramVisitor {
		/**
		 * Build a string representation of the program \a p.
		 * @param p The program
		 */
		std::string_view string_from(ast::ChrProgram& p);

	private:
		void visit(ast::ChrProgram&);

		std::string _str_program;	///< String representation of the program
	};

	/**
	 * @brief Program visitor which add the pragma persistent to all store of constraints
	 */
	struct ProgramSetAllPersistent : ProgramVisitor {
		/**
		 * Set all stores of constraint persistent.
		 * @param p The program
		 */
		void apply(ast::ChrProgram& p);

	private:
		void visit(ast::ChrProgram&);
	};

	/**
	 * @brief Program visitor which build the occurrences rules from the existing rules
	 */
	struct ProgramBuilOccRules : ProgramVisitor {
		/**
		 * Build the occurrences rules.
		 * @param p The program
		 */
		void build(ast::ChrProgram& p);

	private:
		void visit(ast::ChrProgram&);
	};

	/**
	 * @brief Program visitor which search for never stored constraint
	 */
	struct ProgramNeverStored : ProgramVisitor {
		/**
		 * Search for and set the never stored constraints.
		 * @param p The program
		 */
		void apply(ast::ChrProgram& p);

	private:
		void visit(ast::ChrProgram&);
	};

	/**
	 * @brief Program visitor which perform late storage analysis on occurence rules
	 */
	struct ProgramLateStorage : ProgramVisitor {
		/**
		 * Analyse the occurence rules to deduce where late storage must be applied
		 * @param g The graph of dependencies between constraints
		 * @param p The program
		 */
		void apply(const DependencyGraph& g, ast::ChrProgram& p);

	private:
		void visit(ast::ChrProgram&);

		const DependencyGraph* _graph = nullptr;
	};

	/**
	 * @brief Program visitor which generates the abstract imperative code of the CHR program
	 *
	 * Visitor that generates the abstract imperative code of the program.
	 * It is in charge of generation constraint store definitions, rules source code
	 * and other needed data structures and functions.
	 */
	struct ProgramAbstractCode : ProgramVisitor {
		/**
		 * Default constructor.
		 * Use same output for data structures and rule code.
		 * @param os The output stream used to send the generated code.
		 */
		ProgramAbstractCode(std::ostream& os);

		/**
		 * Default constructor.
		 * Use two different outputs for data structures and rule code.
		 * @param os The output stream used to send the generated code.
		 */
		ProgramAbstractCode(std::ostream& os_ds, std::ostream& os_rc);

		/**
		 * Generate the imperative code of the whole program \a p.
		 * @param p The CHR program
		 * @param data_structures Is true if the data structures which must be in header files has to be generated
		 * @param rule_code Is true if the output code corresponding to the program of rules has to be generated
		 */
		void generate(ast::ChrProgram& p, bool data_structures = true, bool rule_code = true);

	protected:
		void visit(ast::ChrProgram&);

		/**
		 * Generates the code for the beginning of the program.
		 * @param p The CHR program
		 */
		virtual void begin_program(ast::ChrProgram& p);

		/**
		 * Generates the code for the end of the program.
		 * @param p The CHR program
		 */
		virtual void end_program(ast::ChrProgram& p);

		/**
		 * Generates the code for the \a i constraint store of the program.
		 * @param p The CHR program
		 * @param i The number of CHR constraint in the list
		 */
		virtual void constraint_store(ast::ChrProgram& p, unsigned int i);

		/**
		 * Generates the code at begining of a serie of occurrence rules (i.e. the
		 * first one).
		 * @param p The CHR program
		 * @param cd The constraint declaration that leads to all occurence rule
		 */
		virtual void generate_rule_header(ast::ChrProgram& p, ast::ChrConstraintDecl& cd);

		/**
		 * Generates the code for storing the active constraint of rule \a r
		 * in the constraint store.
		 * @param p The CHR program
		 * @param cdecl The constraint declaration that leads to all occurence rule
		 * @param schedule_var_idx The list of variable indexes to be scheduled
		 */
		virtual void store_active_constraint(ast::ChrProgram&, ast::ChrConstraintDecl& cdecl, std::vector< unsigned int > schedule_var_idx);

		/**
		 * Generates the code for occurrence of rule \a r.
		 * @param p The CHR program
		 * @param r The rule to be converted to code
		 * @param str_next_occurrence The label of the next occurence rule or next rule if \a r is the last rule
		 * @param schedule_var_idx The list of variable indexes to be scheduled on constraint storage
		 */
		virtual void generate_occurrence_rule(ast::ChrProgram& p, ast::OccRule& r, std::string str_next_occurrence, const std::vector< unsigned int >& schedule_var_idx);

		/**
		 * Return the prefix to print for each line, depending on the _depth.
		 * @return The prefix
		 */
		std::string prefix();

		std::size_t _prefix_w = 0;			///< Initial prefix width for alignment
		std::size_t _depth = 0;				///< Recursive depth
		bool _data_structures = true;		///< If true, the data structures are generated
		bool _rule_code = true;				///< If true, the code of rules (program) is generated
		std::ostream& _os_ds;				///< Output stream used to send the generated code of data structures
		std::ostream& _os_rc;				///< Output stream used to send the generated code of rules
	};

	/**
	 * @brief Program visitor which generates the CPP code of the CHR program
	 *
	 * Visitor that generates the CPP code of the program.
	 * It is in charge of generation constraint store definitions, rules source code
	 * and other needed data structures and functions.
	 */
	struct ProgramCppCode : ProgramAbstractCode {
		/**
		 * Default constructor.
		 * Use same output for data structures and rule code.
		 * @param os The output stream used to send the generated code.
		 */
		ProgramCppCode(std::ostream& os);

		/**
		 * Default constructor.
		 * Use two different outputs for data structures and rule code.
		 * @param os The output stream used to send the generated code.
		 */
		ProgramCppCode(std::ostream& os_ds, std::ostream& os_rc);

	protected:
		/**
		 * Generates the code for the beginning of the program.
		 * @param p The CHR program
		 */
		void begin_program(ast::ChrProgram& p) override final;

		/**
		 * Generates the code for the end of the program.
		 * @param p The CHR program
		 */
		void end_program(ast::ChrProgram& p) override final;

		/**
		 * Generates the code for the \a i constraint store of the program.
		 * @param p The CHR program
		 * @param i The number of CHR constraint in the list
		 */
		void constraint_store(ast::ChrProgram& p, unsigned int i) override final;

		/**
		 * Generates the code at begining of a serie of occurrence rules (i.e. the
		 * first one).
		 * @param p The CHR program
		 * @param cdecl The constraint declaration that leads to all occurence rule
		 */
		void generate_rule_header(ast::ChrProgram& p, ast::ChrConstraintDecl& cdecl) override final;

		/**
		 * Generates the code for storing the active constraint of rule \a r
		 * in the constraint store.
		 * @param p The CHR program
		 * @param cdecl The constraint declaration that leads to all occurence rule
		 * @param schedule_var_idx The list of variable indexes to be scheduled
		 */
		void store_active_constraint(ast::ChrProgram& p, ast::ChrConstraintDecl& cdecl, std::vector< unsigned int > schedule_var_idx) override final;

		/**
		 * Generates the code for occurrence of rule \a r.
		 * @param p The CHR program
		 * @param r The rule to be converted to code
		 * @param str_next_occurrence The label of the next occurence rule or next rule if \a r is the last rule
		 * @param schedule_var_idx The list of variable indexes to be scheduled on constraint storage
		 */
		void generate_occurrence_rule(ast::ChrProgram& p, ast::OccRule& r, std::string str_next_occurrence, const std::vector< unsigned int >& schedule_var_idx) override final;

	private:
		/**
		 * Loop over all element of args and send each args[I] to ostream. 
		 * @param os The output stream to use
		 * @param args The tuple of elements to browse
		 * @param I Index sequence of the recursive loop
		 */
		template< typename TT, size_t... I >
		void loop_to_ostream(std::ostream& os, const TT& args, std::index_sequence<I...>);

		/**
		 * Generates the code corresponding to a debug message.
		 * @param os The output stream to use
		 * @param c_name The constraint name
		 * @param flag The flag for this message
		 * @param args The tuple of elements corresponding to the message to write
		 */
		template< typename... T >
		void write_trace_statement(std::ostream& os, std::string_view c_name, const char* flag, const std::tuple< T... >& args);
	};
}
