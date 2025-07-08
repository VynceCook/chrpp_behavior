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
#include <unordered_set>
#include <unordered_map>
#include <cassert>

#include <ast/rule.hh>
#include <ast/program.hh>

namespace chr::compiler::visitor
{
	/**
	 * @brief Root class of the occurrence rule visitor
	 *
	 * An OccRule visitor visits an occurrence rule.
	 */
	struct OccRuleVisitor {
		/**
		 * Root visit function for OccRule.
		 */
		virtual void visit(ast::OccRule&) = 0;

		/**
		 * Default virtual destructor
		 */
		virtual ~OccRuleVisitor() = default;
	};

	/**
	 * @brief OccRule visitor which build a string representation of an occurrence rule
	 */
	struct OccRulePrint : OccRuleVisitor {
		/**
		 * Build a string representation of the occurrence rule \a r.
		 * @param r The occurrence rule
		 */
		std::string_view string_from(ast::OccRule& r);

	private:
		void visit(ast::OccRule&);

		std::string _str_rule;	///< String representation of a rule
	};

	/**
	 * @brief OccRule visitor which reorders the constraints of the head
	 *
	 * Visitor who reorders the constraints in the head of the ruler
	 * to place the ones that will be matched the fastest first.
	 * Constraints with constants are placed first followed by those
	 * with variables in common with constraints already matched.
	 */
	struct OccRuleHeadReorder : OccRuleVisitor {
		/**
		 * Reorder head constraints.
		 * @param r The occurrence rule
		 */
		void reorder(ast::OccRule& r);

	private:
		void visit(ast::OccRule&) final;
	};

	/**
	 * @brief OccRule visitor which reorders part of the guard
	 *
	 * Visitor which reorders the parts of the ruler's guard according
	 * to the constraints of the head. The parts of the guard that
	 * will be solved with the already identified partner constraints
	 * are placed first.
	 */
	struct OccRuleGuardReorder : OccRuleVisitor {
		/**
		 * Reorder the guard
		 * @param r The occurrence rule
		 */
		void reorder(ast::OccRule& r);

	private:
		void visit(ast::OccRule&) final;
	};

	/**
	 * @brief OccRule visitor which update indexes of head constraints
	 *
	 * Visitor that walks through the constraints of the head calculating
	 * the necessary indexes to speed up the search for matches.
	 * It updates the constraint stores accordingly.
	 */
	struct OccRuleUpdateConstraintStoreIndexes : OccRuleVisitor {
		/**
		 * Update the indexes of the constraint stores involved in the head.
		 * @param r The occurrence rule
		 */
		void update_indexes(ast::OccRule& r);

	private:
		void visit(ast::OccRule&) final;
	};

	/**
	 * @brief OccRule visitor which generates the abstract imperative code of the occurrence rule
	 *
	 * Visitor that generates the abstract imperative code of the rule.
	 * It is in charge of matching partner constraints, checking custody and history.
	 * Finally, it calls the code generation for the rule body.
	 */
	struct OccRuleAbstractCode : OccRuleVisitor {
		/**
		 * Context data structure used to gather elements which are part of context
		 * of statements of the body. The context contains variable usages and
		 * declaration. It will be enlarged in the visitor body.

		 */
		struct Context {
			std::unordered_map< std::string, std::tuple<ast::PtrSharedChrConstraintDecl,int> > _logical_variables;	///< Logical variables (and declaration) introduced by the occurrence rule
			std::unordered_set< std::string > _local_variables;														///< Local variables introduced by the occurrence rule
		};

		/**
		 * Default constructor.
		 * @param os The output stream used to send the generated code.
		 */
		OccRuleAbstractCode(std::ostream& os);

		/**
		 * Generate the imperative code of occurrence rule \a r.
		 * @param r The occurrence rule
		 * @param prefix_w The initial prefix width used for alignment
		 * @param next_on_inapplicable The label to go if the rule can't be applied
		 * @param schedule_var_idx The list of variable indexes to be scheduled on constraint storage
		 */
		void generate(ast::OccRule& r, std::size_t prefix_w, std::string_view next_on_inapplicable, std::vector< unsigned int > schedule_var_idx);

	protected:
		void visit(ast::OccRule&) final;

		/**
		 * Generates the code for the beginning of the occurrence rule.
		 * @param r The occurrence rule
		 */
		virtual void begin_occ_rule(ast::OccRule& r);

		/**
		 * Generates the code for the end of the occurrence rule.
		 * @param r The occurrence rule
		 */
		virtual void end_occ_rule(ast::OccRule& r);

		/**
		 * Generates the code when an occurrence rule must exist with success.
		 * @param r The occurrence rule
		 */
		virtual void exit_success(ast::OccRule& r);

		/**
		 * Generates the verification code of the \a i part of the guard.
		 * @param r The occurrence rule
		 * @param i The guard part number in the array of guard parts
		 * @param ctxt The context to update
		 */
		virtual void begin_guard(ast::OccRule& r, unsigned int i, Context& ctxt);

		/**
		 * Generates the code of closure of the \a i part of the guard.
		 * @param r The occurrence rule
		 * @param i The guard part number in the array of guard parts
		 */
		virtual void end_guard(ast::OccRule& r, unsigned int i);

		/**
		 * Returns the key, constructed from the active constraint,
		 * to be used for the history.
		 * @param r The occurrence rule
		 * @return The string corresponding to key
		 */
		virtual std::string history_key_from_active_constraint(ast::OccRule& r);

		/**
		 * Returns the key, constructed from the partner constraint \a i,
		 * to be used for the history.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 * @return The string corresponding to key
		 */
		virtual std::string history_key_from_partner(ast::OccRule& r, unsigned int i);

		/**
		 * Generates the verification code of history for occurrence rule \a r.
		 * @param r The occurrence rule
		 * @param key The key to search for in history
		 */
		virtual void begin_history(ast::OccRule& r, std::vector< std::string > key);

		/**
		 * Generates the code of closure of the history check.
		 * @param r The occurrence rule
		 */
		virtual void end_history(ast::OccRule& r);

		/**
		 * The rule will be executed, nothing more prevents it.
		 * @param r The occurrence rule
		 */
		virtual void commit_rule(ast::OccRule& r);

		/**
		 * Generates the code to check if the active constraint is still alived and go
		 * to _next_on_inapplicable if it is not.
		 * @param r The occurrence rule
		 */
		virtual void check_alived_active_constraint(ast::OccRule& r);

		/**
		 * Generates the code to check if a partner is alived and go to next matching
		 * partner loop if it is not.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		virtual void check_alived_partner(ast::OccRule& r, unsigned int i);

		/**
		 * Generates the code to go to the next loop of the matching of the partner
		 * \a i.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		virtual void goto_next_matching_partner(ast::OccRule& r, unsigned int i);

		/**
		 * Generates the code to check that partner \a i and active constraint are differents.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		virtual void partner_must_be_different_from_active_constraint(ast::OccRule& r, unsigned int i);

		/**
		 * Generates the code to check that partner \a i and partner \a j are differents.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 * @param j The other partner number in the array of partners
		 */
		virtual void partner_must_be_different_from(ast::OccRule& r, unsigned int i, unsigned int j);

		/**
		 * Generates the code for the end of the match search for
		 * a partner constraint.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 * @param index The values to use with the corresponding index
		 */
		virtual void begin_partner(ast::OccRule& r, unsigned int i, std::vector <std::string> index);

		/**
		 * Generates the code for the end of the match search for
		 * a partner constraint.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		virtual void end_partner(ast::OccRule& r, unsigned int i);

		/**
		 * Generates the code for removing a partner constraint from the
		 * store.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		virtual void remove_partner(ast::OccRule& r, unsigned int i);

		/**
		 * Generates the code for storing the active constraint.
		 * @param r The occurrence rule
		 */
		virtual void store_active_constraint(ast::OccRule& r);

		/**
		 * Generates the code for removing the active constraint.
		 * @param r The occurrence rule
		 */
		virtual void remove_active_constraint(ast::OccRule& r);

		/**
		 * Generates the code for the body of the rule.
		 * @param r The occurrence rule
		 * @param ctxt The context 
		 */
		virtual void body(ast::OccRule& r, Context ctxt);

		/**
		 * Return the prefix to print for each line, depending on the _depth.
		 * @return The prefix
		 */
		std::string prefix();

		std::size_t _prefix_w = 0;						///< Initial prefix width for alignment
		std::size_t _depth = 0;							///< Recursive depth
		std::string _next_on_inapplicable;				///< Next label to go after the rule succeeds
		std::vector< unsigned int > _schedule_var_idx;	///< List of variable idx to be scheduled on constraint storage
		std::ostream& _os;								///< Output stream used to send the generated code
	};

	/**
	 * @brief OccRule visitor which generates the CPP code of the occurrence rule
	 *
	 * Visitor that generates the CPP code of the rule.
	 * It is in charge of matching partner constraints, checking custody and history.
	 * Finally, it calls the code generation for the rule body.
	 */
	struct OccRuleCppCode : OccRuleAbstractCode {
		/**
		 * Default constructor.
		 * @param os The output stream used to send the generated code
		 * @param input_file_name The name of the input file used to built this rule
		 * @param auto_catch_failure True if CHR program must try to automatically catch failure, false otherwise
		 */
		OccRuleCppCode(std::ostream& os, std::string_view input_file_name, bool auto_catch_failure);

	protected:
		/**
		 * Generates the code for the beginning of the occurrence rule.
		 * @param r The occurrence rule
		 */
		void begin_occ_rule(ast::OccRule& r) override final;

		/**
		 * Generates the code for the end of the occurrence rule.
		 * @param r The occurrence rule
		 */
		void end_occ_rule(ast::OccRule& r) override final;

		/**
		 * Generates the code when an occurrence rule must exist with success.
		 * @param r The occurrence rule
		 */
		void exit_success(ast::OccRule& r) override final;

		/**
		 * Generates the verification code of the \a i part of the guard.
		 * @param r The occurrence rule
		 * @param i The guard part number in the array of guard parts
		 * @param ctxt The context to update
		 */
		void begin_guard(ast::OccRule& r, unsigned int i, Context& ctxt) override final;

		/**
		 * Generates the code of closure of the \a i part of the guard.
		 * @param r The occurrence rule
		 * @param i The guard part number in the array of guard parts
		 */
		void end_guard(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Returns the key, constructed from the active constraint,
		 * to be used for the history.
		 * @param r The occurrence rule
		 * @return The string corresponding to key
		 */
		std::string history_key_from_active_constraint(ast::OccRule& r) override final;

		/**
		 * Returns the key, constructed from the partner constraint \a i,
		 * to be used for the history.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 * @return The string corresponding to key
		 */
		std::string history_key_from_partner(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Generates the verification code of history for occurrence rule \a r.
		 * @param r The occurrence rule
		 * @param key The key to search for in history
		 */
		void begin_history(ast::OccRule& r, std::vector< std::string > key) override final;

		/**
		 * Generates the code of closure of the history check.
		 * @param r The occurrence rule
		 */
		void end_history(ast::OccRule& r) override final;

		/**
		 * The rule will be executed, nothing more prevents it.
		 * @param r The occurrence rule
		 */
		void commit_rule(ast::OccRule& r) override final;

		/**
		 * Generates the code to check if the active constraint is still alived and go
		 * to _next_on_inapplicable if it is not.
		 * @param r The occurrence rule
		 */
		void check_alived_active_constraint(ast::OccRule& r) override final;

		/**
		 * Generates the code to check if a partner is alived and go to next matching
		 * partner loop if it is not.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		void check_alived_partner(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Generates the code to go to the next loop of the matching of the partner
		 * \a i.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		void goto_next_matching_partner(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Generates the code to check that partner \a i and active constraint are differents.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		void partner_must_be_different_from_active_constraint(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Generates the code to check that partner \a i and partner \a j are differents.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 * @param j The other partner number in the array of partners
		 */
		void partner_must_be_different_from(ast::OccRule& r, unsigned int i, unsigned int j) override final;

		/**
		 * Generates the code for the end of the match search for
		 * a partner constraint.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 * @param index The values to use with the corresponding index
		 */
		void begin_partner(ast::OccRule& r, unsigned int i, std::vector <std::string> index) override final;

		/**
		 * Generates the code for the end of the match search for
		 * a partner constraint.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		void end_partner(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Generates the code for removing a partner constraint from the
		 * store.
		 * @param r The occurrence rule
		 * @param i The partner number in the array of partners
		 */
		void remove_partner(ast::OccRule& r, unsigned int i) override final;

		/**
		 * Generates the code for storing the active constraint.
		 * @param r The occurrence rule
		 */
		void store_active_constraint(ast::OccRule& r) override final;

		/**
		 * Generates the code for removing the active constraint.
		 * @param r The occurrence rule
		 */
		void remove_active_constraint(ast::OccRule& r) override final;

		/**
		 * Generates the code for the body of the rule.
		 * @param r The occurrence rule
		 * @param ctxt The context 
		 */
		void body(ast::OccRule& r, Context ctxt) override final;

	private:
		/**
		 * Remove or replace spaces, ( and ) symbols from \a str
		 * @param str The string to parse and modify
		 */
		void strip_symbols(std::string& str);

		/**
		 * Loop over all element of args and send each args[I] to ostream. 
		 * @param args The tuple of elements to browse
		 * @param I Index sequence of the recursive loop
		 */
		template< typename TT, size_t... I >
		void loop_to_ostream(const TT& args, std::index_sequence<I...>);

		/**
		 * Generates the code corresponding to a debug message.
		 * @param r The occurrence rule
		 * @param flag The flag for this message
		 * @param args The tuple of elements corresponding to the message to write
		 */
		template< typename... T >
		void write_trace_statement(ast::OccRule& r, const char* flag, const std::tuple< T... >& args);

		/**
		 * Generate a string for the iterator of partner \a c of rank \a i
		 * @param r The occurrence rule
		 * @param c The constraint partner to match
		 * @param i the rank of partner \a c in the occurrence rule
		 */
		std::string str_partner_it(ast::OccRule& r, ast::Body& c, unsigned int i);

		std::string _input_file_name;	///< Name of the file used to built this rule
		bool _auto_catch_failure;		///< True if CHR program must try to automatically catch failure, false otherwise
	};
}
