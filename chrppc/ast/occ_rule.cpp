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
#include <visitor/expression.hh>
#include <visitor/occ_rule.hh>

namespace chr::compiler::ast
{
	/*
	 * OccRule
	 */
	OccRule::OccRule(const PtrSharedRule& rule, unsigned int occurrence, int active_c_idx, bool keep)
		: _rule(rule), _active_c( {keep, -1, std::make_unique<ChrConstraintCall>(keep?*rule->head_keep().at(active_c_idx):*rule->head_del().at(active_c_idx))} ), _occurrence(occurrence), _store_active_constraint(keep?true:false)
	{
		int idx = 0;
		for (auto& c : rule->head_del())
		{
			if (keep || (idx != active_c_idx))
				_partners.emplace_back( HeadChrConstraint({ false, -1, std::make_unique<ChrConstraintCall>(*c)}) );
			++idx;
		}
		idx = 0;
		for (auto& c : rule->head_keep())
		{
			if (!keep || (idx != active_c_idx))
				_partners.emplace_back( HeadChrConstraint({ true, -1, std::make_unique<ChrConstraintCall>(*c)}) );
			++idx;
		}
		_guard_parts.resize( _partners.size() + 1);

		for (auto& e : rule->guard())
			_guard_parts.back().emplace_back( PtrExpression(e->clone()) );

		// Split guard according to &&
		if (!rule->guard().empty())
		{
			auto last_guard = std::move( _guard_parts.back().back() );
			_guard_parts.back().resize( _guard_parts.back().size() - 1 );

			visitor::ExpressionApply v;
			auto deal_with = [&](Expression& e)
			{
				auto p0 = dynamic_cast< InfixExpression* >(&e);
				if ((p0 != nullptr) && ((p0->op() == "&&") || (p0->op() == "and")))
					return true;
				else
				{
					_guard_parts.back().emplace_back( PtrExpression(e.clone()) );
					return false;
				}
			};
			v.apply(*last_guard,deal_with);
		}
	}

	OccRule::OccRule(const OccRule& o)
		: _rule(o._rule), _occurrence(o._occurrence), _store_active_constraint(o._store_active_constraint)
	{ 
		_active_c._keep = o._active_c._keep;
		_active_c._use_index = o._active_c._use_index;
		_active_c._c.reset( static_cast<ChrConstraintCall*>(o._active_c._c->clone()) );

		std::transform(o._partners.cbegin(), o._partners.cend(), std::back_inserter(_partners), [](const HeadChrConstraint& hc) {
				HeadChrConstraint copy;
				copy._keep = hc._keep;
				copy._use_index = hc._use_index;
				copy._c.reset( static_cast<ChrConstraintCall*>(hc._c->clone()) );
				return copy;
			});

		std::transform(o._guard_parts.cbegin(), o._guard_parts.cend(), std::back_inserter(_guard_parts), [](const std::vector< PtrExpression >& vect) {
				std::vector< PtrExpression > copy;
				std::transform(vect.cbegin(), vect.cend(), std::back_inserter(copy), [](const PtrExpression& c) {
						return PtrExpression( c->clone() );
					});
				return copy;
			});
	}

	PositionInfo OccRule::position() const
	{
		return _rule->position();
	}

	std::string_view OccRule::name() const
	{
		return _rule->name();
	}

	PtrSharedRule OccRule::rule()
	{
		return _rule;
	}

	unsigned int OccRule::active_constraint_occurrence()
	{
		return _occurrence;
	}

	bool OccRule::keep_active_constraint()
	{
		return _active_c._keep;
	}

	bool OccRule::store_active_constraint()
	{
		return _store_active_constraint;
	}

	void OccRule::set_store_active_constraint(bool value)
	{
		assert(keep_active_constraint() == true);
		_store_active_constraint = value;
	}

	ChrConstraintCall& OccRule::active_constraint()
	{
		return *_active_c._c;
	}

	std::vector< OccRule::HeadChrConstraint >& OccRule::partners()
	{
		return _partners;
	}

	std::vector< PtrExpression >& OccRule::guard()
	{
		return _rule->guard();
	}

	std::vector< std::vector< PtrExpression > >& OccRule::guard_parts()
	{
		return _guard_parts;
	}

	PtrBody& OccRule::body()
	{
		return _rule->body();
	}

	void OccRule::accept(visitor::OccRuleVisitor& v)
	{
		v.visit(*this);
	}

} // namespace chr::compiler::ast
