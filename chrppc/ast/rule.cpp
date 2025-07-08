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

#include <ast/rule.hh>
#include <visitor/rule.hh>

namespace chr::compiler::ast
{
	/*
	 * Rule
	 */

	Rule::Rule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head_keep, std::vector< PtrChrConstraintCall > head_del, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos)
		: _id(id), _pos(std::move(pos)), _name(name), _head_keep(std::move(head_keep)), _head_del(std::move(head_del)), _guard(std::move(guard)), _body(std::move(body))
	{ }

	Rule::Rule(const Rule& o)
		: _id(o._id), _pos(o._pos), _name(o._name), _body( PtrBody(o._body->clone()) )
	{ 
		std::transform(o._head_keep.cbegin(), o._head_keep.cend(), std::back_inserter(_head_keep), [](const PtrChrConstraintCall& c) { return PtrChrConstraintCall( static_cast<ChrConstraintCall*>(c->clone()) ); });
		std::transform(o._head_del.cbegin(), o._head_del.cend(), std::back_inserter(_head_del), [](const PtrChrConstraintCall& c) { return PtrChrConstraintCall( static_cast<ChrConstraintCall*>(c->clone()) ); });
		std::transform(o._guard.cbegin(), o._guard.cend(), std::back_inserter(_guard), [](const PtrExpression& c) { return PtrExpression( c->clone() ); });
	}

	unsigned int Rule::id() const
	{
		return _id;
	}

	std::string_view Rule::name() const
	{
		return _name;
	}

	std::vector< PtrChrConstraintCall >& Rule::head_keep()
	{
		return _head_keep;
	}

	std::vector< PtrChrConstraintCall >& Rule::head_del()
	{
		return _head_del;
	}

	std::vector< PtrExpression >& Rule::guard()
	{
		return _guard;
	}

	PtrBody& Rule::body()
	{
		return _body;
	}

	PositionInfo Rule::position() const
	{
		return _pos;
	}

	/*
	 * PropagationRule
	 */

	PropagationRule::PropagationRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos)
		: Rule(id, std::move(name), std::move(head), std::vector< PtrChrConstraintCall >(), std::move(guard), std::move(body), std::move(pos))
	{ }

	PropagationRule::PropagationRule(const PropagationRule& o)
		: Rule(o)
	{ }

	std::vector< PtrChrConstraintCall >& PropagationRule::head()
	{
		return _head_keep;
	}

	void PropagationRule::accept(visitor::RuleVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * PropagationNoHistoryRule
	 */

	PropagationNoHistoryRule::PropagationNoHistoryRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos)
		: PropagationRule(id, std::move(name), std::move(head), std::move(guard), std::move(body), std::move(pos))
	{
		// Add pragma no_history to all constraint of the head (only keep)
		for (auto& cc : _head_keep)
			if (std::find(cc->pragmas().begin(), cc->pragmas().end(), Pragma::no_history)
					== cc->pragmas().end())
				cc->add_pragma(Pragma::no_history);
	}

	PropagationNoHistoryRule::PropagationNoHistoryRule(const PropagationNoHistoryRule& o)
		: PropagationRule(o)
	{ }

	void PropagationNoHistoryRule::accept(visitor::RuleVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * SimplificationRule
	 */

	SimplificationRule::SimplificationRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos)
		: Rule(id, std::move(name), std::vector< PtrChrConstraintCall >(), std::move(head), std::move(guard), std::move(body), std::move(pos))
	{ }

	SimplificationRule::SimplificationRule(const SimplificationRule& o)
		: Rule(o)
	{ }

	std::vector< PtrChrConstraintCall >& SimplificationRule::head()
	{
		return _head_del;
	}

	void SimplificationRule::accept(visitor::RuleVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * SimpagationRule
	 */

	SimpagationRule::SimpagationRule(unsigned int id, std::string_view name, std::vector< PtrChrConstraintCall > head_keep, std::vector< PtrChrConstraintCall > head_del, std::vector< PtrExpression > guard, PtrBody body, PositionInfo pos)
		: Rule(id, std::move(name), std::move(head_keep), std::move(head_del), std::move(guard), std::move(body), std::move(pos))
	{ }

	SimpagationRule::SimpagationRule(const SimpagationRule& o)
		: Rule(o)
	{ }

	void SimpagationRule::accept(visitor::RuleVisitor& v)
	{
		v.visit(*this);
	}
} // namespace chr::compiler::ast
