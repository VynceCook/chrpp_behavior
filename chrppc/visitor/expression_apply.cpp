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

#include <visitor/expression.hh>

namespace chr::compiler::visitor
{
	void ExpressionApply::apply(ast::Expression& e, std::function< bool (ast::Expression&) > f)
	{
		_f = std::move( f );
		e.accept( *this );
	}

	void ExpressionApply::visit(ast::Identifier& e)
	{
		_f(e);
	}

	void ExpressionApply::visit(ast::CppVariable& e)
	{
		_f(e);
	}

	void ExpressionApply::visit(ast::LogicalVariable& e)
	{
		_f(e);
	}

	void ExpressionApply::visit(ast::Literal& e)
	{
		_f(e);
	}

	void ExpressionApply::visit(ast::PrefixExpression& e)
	{
		if (_f(e)) e.child()->accept(*this);
	}

	void ExpressionApply::visit(ast::PostfixExpression& e)
	{
		if (_f(e)) e.child()->accept(*this);
	}

	void ExpressionApply::visit(ast::InfixExpression& e)
	{
		if (!_f(e)) return;
		e.l_child()->accept(*this);
		e.r_child()->accept(*this);
	}

	void ExpressionApply::visit(ast::TernaryExpression& e)
	{
		if (!_f(e)) return;
		e.child1()->accept(*this);
		e.child2()->accept(*this);
		e.child3()->accept(*this);
	}

	void ExpressionApply::visit(ast::BuiltinConstraint& e)
	{
		if (!_f(e)) return;
		std::size_t args = e.children().size();
		e.name()->accept(*this);
		for( std::size_t i = 0; i < args; ++i )
			e.children()[i]->accept(*this);
	}

	void ExpressionApply::visit(ast::ChrConstraint& e)
	{
		if (!_f(e)) return;
		std::size_t args = e.children().size();
		e.name()->accept(*this);
		for( std::size_t i = 0; i < args; ++i )
			e.children()[i]->accept(*this);
	}

	void ExpressionApply::visit(ast::ChrCount& e)
	{
		if (_f(e)) e.constraint()->accept(*this);
	}
}
