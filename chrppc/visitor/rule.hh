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

#include <ast/rule.hh>
#include <dependency_graph.hh>

namespace chr::compiler::visitor
{
	/**
	 * @brief Root class of the rule visitor
	 *
	 * An rule visitor visits all type of rules.
	 */
	struct RuleVisitor {
		/**
		 * Root visit function for Rule.
		 */
		virtual void visit(ast::Rule&) = 0;
		virtual void visit(ast::PropagationRule&) = 0;
		virtual void visit(ast::PropagationNoHistoryRule&) = 0;
		virtual void visit(ast::SimplificationRule&) = 0;
		virtual void visit(ast::SimpagationRule&) = 0;

		/**
		 * Default virtual destructor
		 */
		virtual ~RuleVisitor() = default;
	};

	/**
	 * @brief Rule visitor which build a string representation of a rule
	 */
	struct RulePrint : RuleVisitor {
		/**
		 * Build a string representation of the rule \a r.
		 * @param r The rule
		 */
		std::string_view string_from(ast::Rule&);

	private:
		void visit(ast::Rule&) final { assert(false); }
		void visit(ast::PropagationRule& r) final;
		void visit(ast::PropagationNoHistoryRule& r) final;
		void visit(ast::SimplificationRule& r) final;
		void visit(ast::SimpagationRule& r) final;

		/**
		 * Build a string representation of the rule \a r (internal).
		 * @param r The rule
		 * @param op The operator for this rule
		 */
		void rule_string_build(ast::Rule& r, std::string_view op);
	
		std::string _str_rule;	///< String representation of a rule
	};

	/**
	 * @brief Rule visitor which build a string representation of a rule
	 *
	 * This visitor add more details when the rule is printing. AS
	 * a result, the rule cannot be parsed again.
	 */
	struct RuleFullPrint : RuleVisitor {
		/**
		 * Build a string representation of the rule \a r.
		 * @param r The rule
		 * @param prefix The space alignment width
		 */
		std::string_view string_from(ast::Rule&, std::size_t prefix = 0);

	private:
		void visit(ast::Rule&) final { assert(false); }
		void visit(ast::PropagationRule& r) final;
		void visit(ast::PropagationNoHistoryRule& r) final;
		void visit(ast::SimplificationRule& r) final;
		void visit(ast::SimpagationRule& r) final;

		/**
		 * Build a string representation of the rule \a r (internal).
		 * @param r The rule
		 * @param op The operator for this rule
		 */
		void rule_string_build(ast::Rule& r, std::string_view op);

		std::size_t _prefix = 0;	///< The space alignment with
		std::string _str_rule;		///< String representation of a rule
	};

	/**
	 * @brief Rule visitor which builds a dependcy graph
	 *
	 * This visitor browses rules and builds a dependency graph of chr constraints.
	 */
	struct RuleDependencyGraph : RuleVisitor {
		/**
		 * Populate the graph with the rule \a r.
		 * @param r The rule
		 */
		void populate(ast::Rule&);

		/**
		 * Build and return a string representation of the dependency graph.
		 * @return The string representation of the graph.
		 */
		std::string to_string() const;

		/**
		 * Return the dependcy graph previously built.
		 * @return The previously built graph
		 */
		const DependencyGraph& graph() const;

	private:
		/**
		 * Populate the graph with the rule \a r.
		 * @param r The rule
		 */
		void do_populate(ast::Rule&);

		void visit(ast::Rule&) final;
		void visit(ast::PropagationRule& r) final;
		void visit(ast::PropagationNoHistoryRule& r) final;
		void visit(ast::SimplificationRule& r) final;
		void visit(ast::SimpagationRule& r) final;

		DependencyGraph _graph;		///< The dependency graph
	};
}
