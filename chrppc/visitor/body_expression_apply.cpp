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

#include <visitor/body.hh>
#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	void BodyExpressionApply::apply(ast::Body& b, std::function< bool (ast::Body&) > f, std::function< bool (ast::Expression&) > f_expr)
	{
		_f = std::move( f );
		_f_expr = std::move( f_expr );
		b.accept( *this );
	}
	
	void BodyExpressionApply::visit(ast::Body&)
	{ }

	void BodyExpressionApply::visit(ast::ChrKeyword& k)
	{
		_f(k);
	}

	void BodyExpressionApply::visit(ast::CppExpression& e)
	{
		if (_f(e)) {
			visitor::ExpressionApply eav;
			eav.apply(*e.expression(), _f_expr);
		}
	}

	void BodyExpressionApply::visit(ast::CppDeclAssignment& a)
	{
		if (_f(a)) {
			visitor::ExpressionApply eav;
			eav.apply(*a.variable(), _f_expr);
			eav.apply(*a.expression(), _f_expr);
		}
	}

	void BodyExpressionApply::visit(ast::Unification& a)
	{
		if (_f(a)) {
			visitor::ExpressionApply eav;
			eav.apply(*a.variable(), _f_expr);
			eav.apply(*a.expression(), _f_expr);
		}
	}

	void BodyExpressionApply::visit(ast::ChrConstraintCall& c)
	{
		if (_f(c)) {
			visitor::ExpressionApply eav;
			eav.apply(*c.constraint(), _f_expr);
		}
	}

	void BodyExpressionApply::visit(ast::Sequence& s)
	{
		if (!_f(s)) return;
		std::size_t args = s.children().size();
		for( std::size_t i = 0; i < args; ++i )
			s.children()[i]->accept(*this);
	}

	void BodyExpressionApply::visit(ast::ChrBehavior& b)
	{
		if (!_f(b)) return;
		visitor::ExpressionApply eav;
		eav.apply(*b.stop_cond(), _f_expr);
		eav.apply(*b.final_status(), _f_expr);
		b.on_succeeded_alt()->accept(*this);
		b.on_failed_alt()->accept(*this);
		b.on_succeeded_status()->accept(*this);
		b.on_failed_status()->accept(*this);
		b.behavior_body()->accept(*this);
	}

	void BodyExpressionApply::visit(ast::ChrTry& t)
	{
		if (!_f(t)) return;
		visitor::ExpressionApply eav;
		eav.apply(*t.variable(), _f_expr);
		t.body()->accept(*this);
	}

}
