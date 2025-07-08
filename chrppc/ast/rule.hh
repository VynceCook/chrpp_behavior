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
#include <ast/body.hh>
#include <ast/expression.hh>

namespace chr::compiler::visitor {
	struct RuleVisitor;
	struct OccRuleVisitor;
};

namespace chr::compiler::ast
{
	/**
	 * @brief Root class of a rule AST
	 *
	 * A rule consists of a CHR rule which is either a propagation,
	 * simplification or simpagation rule.
	 * It is composed of a head, an optional guard and a body.
	 * A head is a list of chr constraint matching patterns, a guard is a boolean CPP
	 * expression and the body is a sequence of constraint calls.
	 */
	class Rule
	{
	public:
		/**
		 * Initialize rule element at given position.
		 * @param id The unique id of the rule
		 * @param name The name of the rule (may be empty)
		 * @param head_keep The ressources of the head that must be kept
		 * @param head_del The ressources of the head that must be deleted
		 * @param guard The guard of the rule (may be empty)
		 * @param body The body of the rule
		 * @param pos The position of the element
		 */
		Rule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head_keep, std::vector< PtrChrConstraintCall > head_del, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		Rule(const Rule &o);

		/**
		 * Return the id of the rule.
		 * @return The id of the rule
		 */
		unsigned int id() const;

		/**
		 * Return the name of the rule.
		 * @return The name of the rule
		 */
		std::string_view name() const;

		/**
		 * Return the ressources of the that must be kept.
		 * @return A part of head of the rule
		 */
		std::vector< PtrChrConstraintCall >& head_keep();

		/**
		 * Return the ressources of the that must be deleted.
		 * @return A part of head of rule
		 */
		std::vector< PtrChrConstraintCall >& head_del();

		/**
		 * Return the guard of the rule.
		 * @return The guard of the rule
		 */
		std::vector< PtrExpression >& guard();

		/**
		 * Return the body of the rule.
		 * @return The body of the rule
		 */
		PtrBody& body();

		/**
		 * Return the position of the rule in the input source file.
		 * @return The position of the rule
		 */
		PositionInfo position() const;

		/**
		 * Accept RuleVisitor
		 */
		virtual void accept(visitor::RuleVisitor&) = 0;

		/**
		 * Virtual destructor.
		 */
		virtual ~Rule() =default;

	protected:
		unsigned int _id;								///< Unique id of the rule
		PositionInfo _pos;								///< Position of the element in the input source fille
		std::string _name;								///< Name of the rule (may be empty)
		std::vector< PtrChrConstraintCall > _head_keep;	///< Ressources of the head that must be kept
		std::vector< PtrChrConstraintCall > _head_del;	///< Ressources of the head that must be deleted
		std::vector< PtrExpression > _guard;			///< Guard of the rule
		PtrBody _body;									///< Body of the rule

	};

	/**
	 * Shortcut to unique ptr of a Rule
	 */
	using PtrRule = std::unique_ptr< Rule >;

	/**
	 * Shortcut to shared ptr of a Rule
	 */
	using PtrSharedRule = std::shared_ptr< Rule >;

	/**
	 * @brief A Propagation rule
	 *
	 * A propagation rule keeps all the ressources of the head
	 */
	class PropagationRule : public Rule
	{
	public:
		/**
		 * Initialize a propagation rule
		 * @param id The unique id of the rule
		 * @param name The name of the rule (may be empty)
		 * @param head The head of the rule
		 * @param guard The guard of the rule (may be empty)
		 * @param body The body of the rule
		 * @param pos The position of the element
		 */
		PropagationRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		PropagationRule(const PropagationRule &o);

		/**
		 * Return the head of the rule.
		 * @return The head of the rule
		 */
		std::vector< PtrChrConstraintCall >& head();

		/**
		 * Accept RuleVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::RuleVisitor& v);
	};

	/**
	 * @brief A Propagation rule without history
	 *
	 * A propagation without history do not manage any history to prevent
	 * the propagation rule to be be triggered for the same set of ressources.
	 */
	class PropagationNoHistoryRule : public PropagationRule
	{
	public:
		/**
		 * Initialize a propagation rule without history
		 * @param id The unique id of the rule
		 * @param name The name of the rule (may be empty)
		 * @param head The head of the rule
		 * @param guard The guard of the rule (may be empty)
		 * @param body The body of the rule
		 * @param pos The position of the element
		 */
		PropagationNoHistoryRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		PropagationNoHistoryRule(const PropagationNoHistoryRule &o);

		/**
		 * Accept RuleVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::RuleVisitor& v);
	};

	/**
	 * @brief A Simplification rule
	 *
	 * A simplification rule deletes all the ressources of the head
	 */
	class SimplificationRule : public Rule
	{
	public:
		/**
		 * Initialize a simplification rule
		 * @param id The unique id of the rule
		 * @param name The name of the rule (may be empty)
		 * @param head The head of the rule
		 * @param guard The guard of the rule (may be empty)
		 * @param body The body of the rule
		 * @param pos The position of the element
		 */
		SimplificationRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		SimplificationRule(const SimplificationRule &o);

		/**
		 * Return the head of the rule.
		 * @return The head of the rule
		 */
		std::vector< PtrChrConstraintCall >& head();

		/**
		 * Accept RuleVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::RuleVisitor& v);
	};

	/**
	 * @brief A Simpagation rule
	 *
	 * A simpagation rule is a generalisation of propagation rules and simplification
	 * rules where some parts of the head must be kept and some other parts must be
	 * deleted.
	 */
	class SimpagationRule : public Rule
	{
	public:
		/**
		 * Initialize a simpagation rule
		 * @param id The unique id of the rule
		 * @param name The name of the rule (may be empty)
		 * @param head_keep The ressources of the head that must be kept
		 * @param head_del The ressources of the head that must be deleted
		 * @param guard The guard of the rule (may be empty)
		 * @param body The body of the rule
		 * @param pos The position of the element
		 */
		SimpagationRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head_keep, std::vector< PtrChrConstraintCall > head_del, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		SimpagationRule(const SimpagationRule &o);

		/**
		 * Accept RuleVisitor
		 * @param v Visitor to apply
		 */
		virtual void accept(visitor::RuleVisitor& v);
	};

	/**
	 * @brief Class of an occurrence for a rule
	 *
	 * An occurrence rule consists of a CHR rule where the active constraint
	 * has been identified.
	 * The head and guard are ordered to optimise the generated code.
	 */
	class OccRule
	{
	public:
		struct HeadChrConstraint {
			bool _keep;					///< True if the constraint must be kept
			int _use_index;				///< -1 if no index should be used, the number of the index otherwise
			PtrChrConstraintCall _c;	///< Reference to the CHR constraint
		};

		/**
		 * Initialize an occurrence rule from a rule.
		 * @param rule The rule 
		 * @param occurrence Occurrence of active constraint
		 * @param active_c_idx Idx of active constraint
		 * @param keep True if the active constraint is a kept one, false otherwise
		 */
		OccRule(const PtrSharedRule& rule, unsigned int occurrence, int active_c_idx, bool keep);

		/**
		 * Copy constructor.
		 * @param o the other element
		 */
		OccRule(const OccRule &o);

		/**
		 * Return the position of the rule in the input source file.
		 * @return The position of the rule
		 */
		PositionInfo position() const;

		/**
		 * Return the name of the rule.
		 * @return The name of the rule
		 */
		std::string_view name() const;

		/**
		 * Return the original rule of this occurrence rule.
		 * @return The rule
		 */
		PtrSharedRule rule();

		/**
		 * Return the occurrence number of the active constraint in the set of rules.
		 * @return The occurrence number of the active constraint
		 */
		unsigned int active_constraint_occurrence();

		/**
		 * Check if the active constraint must be kept or deleted.
		 * @return True if active constraint must be kept, false otherwise
		 */
		bool keep_active_constraint();

		/**
		 * Check if the active constraint must be stored. If the active constraint
		 * must be delete, it returns false. It the active constraint must be kept,
		 * the return value depends on the late storage analysis
		 * @return True if active constraint must be stored, false otherwise
		 */
		bool store_active_constraint();

		/**
		 * Change the value of _store_active_constraint variable. When late
		 * storage analysis concludes that the occurrence rule must not store
		 * the constraint, the \a value is set to true.
		 * @param value The new value to set
		 */
		void set_store_active_constraint(bool value);

		/**
		 * Return the active constraint of the occurrence rule.
		 * @return The active constraint
		 */
		ChrConstraintCall& active_constraint();

		/**
		 * Return the partner constraints of the occurrence rule
		 * @return A part of head of the rule
		 */
		std::vector< HeadChrConstraint >& partners();

		/**
		 * Return the guard of the rule.
		 * @return The guard of the rule
		 */
		std::vector< PtrExpression >& guard();

		/**
		 * Return the splitted guard of the rule where each part can be tested
		 * according to the corresponding partner.
		 * @return The splitted guard of the rule
		 */
		std::vector< std::vector<PtrExpression> >& guard_parts();

		/**
		 * Return the body of the rule.
		 * @return The body of the rule
		 */
		PtrBody& body();

		/**
		 * Accept OccRuleVisitor
		 */
		void accept(visitor::OccRuleVisitor&);

	private:
		PtrSharedRule _rule;						///< Original rule
		HeadChrConstraint _active_c;				///< Active constraint of rule 
		unsigned int _occurrence;					///< Occurrence of active constraint
		bool _store_active_constraint;				///< True if the active constraint must be stored
		std::vector< HeadChrConstraint > _partners;	///< Partner constraints of the occurrence rule
		std::vector< std::vector<PtrExpression> > _guard_parts;	///< Parts of the guard that can be tested according to head
	};

	/**
	 * Shortcut to unique ptr of a OccRule
	 */
	using PtrOccRule = std::unique_ptr< OccRule >;

} // namespace chr::compiler::ast
