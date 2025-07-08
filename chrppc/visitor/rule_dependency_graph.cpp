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

#include <unordered_set>
#include <visitor/rule.hh>
#include <visitor/body.hh>
#include <visitor/expression.hh>
#include <ast/expression.hh>
#include <ast/body.hh>

namespace chr::compiler::visitor
{
	void RuleDependencyGraph::populate(ast::Rule& r)
	{
		r.accept( *this );
	}

	std::string RuleDependencyGraph::to_string() const
	{
		return _graph.to_string();
	}

	const DependencyGraph& RuleDependencyGraph::graph() const
	{
		return _graph;
	}

	void RuleDependencyGraph::do_populate(ast::Rule& r)
	{
		using namespace ast;
		std::vector< Node > dst_nodes;
		auto add_dest_node = [&](Body& b)
		{
			auto p0 = dynamic_cast< ChrConstraintCall* >(&b);
			if (p0 != nullptr)
			{
				dst_nodes.emplace_back( Node(p0->constraint()) );
				return true;
			}
			auto p1 = dynamic_cast< Unification* >(&b);
			if (p1 != nullptr)
			{
				dst_nodes.emplace_back( Node(*p1->expression()) );
				return true;
			}

			auto add_dest_node_exp = [&](Expression& e)
			{
				auto pp0 = dynamic_cast< BuiltinConstraint* >(&e);
				if (pp0 != nullptr)
				{
					dst_nodes.emplace_back( Node(*pp0) );
					return true;
				}
				auto pp1 = dynamic_cast< ChrConstraint* >(&e);
				if (pp1 != nullptr)
				{
					dst_nodes.emplace_back( Node(*pp1) );
					return true;
				}
				return true;
			};
			visitor::ExpressionApply eav;
			auto p2 = dynamic_cast< CppExpression* >(&b);
			if (p2 != nullptr)
			{
				eav.apply(*p2->expression(), add_dest_node_exp);
				return true;
			}
			auto p3 = dynamic_cast< CppDeclAssignment* >(&b);
			if (p3 != nullptr)
				eav.apply(*p3->expression(), add_dest_node_exp);
			return true;
		};
		visitor::BodyApply ap;
		ap.apply(*r.body(),add_dest_node);

		auto deal_with = [&](PtrChrConstraintCall& c)
		{
			for (auto& dst_n : dst_nodes)
				_graph.add_edge(Node(c->constraint()), dst_n);

			auto add_partner = [&](PtrChrConstraintCall& cc)
			{
				if (c == cc) return;
				_graph.add_partner(Node(c->constraint()),Node(cc->constraint()));
			};
			std::for_each(r.head_keep().begin(), r.head_keep().end(), add_partner);
			std::for_each(r.head_del().begin(), r.head_del().end(), add_partner);
		};

		std::for_each(r.head_keep().begin(), r.head_keep().end(), deal_with);
		std::for_each(r.head_del().begin(), r.head_del().end(), deal_with);
	}

	void RuleDependencyGraph::visit(ast::Rule& r)
	{
		do_populate(r);
	}

	void RuleDependencyGraph::visit(ast::PropagationRule& r)
	{
		do_populate(r);
	}

	void RuleDependencyGraph::visit(ast::PropagationNoHistoryRule& r)
	{
		do_populate(r);
	}

	void RuleDependencyGraph::visit(ast::SimplificationRule& r)
	{
		do_populate(r);
	}

	void RuleDependencyGraph::visit(ast::SimpagationRule& r)
	{
		do_populate(r);
	}
}
