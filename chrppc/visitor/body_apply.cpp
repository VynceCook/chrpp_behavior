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

namespace chr::compiler::visitor
{
	void BodyApply::apply(ast::Body& b, std::function< bool (ast::Body&) > f)
	{
		_f = std::move( f );
		b.accept( *this );
	}
	
	void BodyApply::visit(ast::Body&)
	{ }

	void BodyApply::visit(ast::ChrKeyword& k)
	{
		_f(k);
	}

	void BodyApply::visit(ast::CppExpression& e)
	{
		_f(e);
	}

	void BodyApply::visit(ast::CppDeclAssignment& a)
	{
		_f(a);
	}

	void BodyApply::visit(ast::Unification& a)
	{
		_f(a);
	}

	void BodyApply::visit(ast::ChrConstraintCall& c)
	{
		_f(c);
	}

	void BodyApply::visit(ast::Sequence& s)
	{
		if (!_f(s)) return;
		std::size_t args = s.children().size();
		for( std::size_t i = 0; i < args; ++i )
			s.children()[i]->accept(*this);
	}

	void BodyApply::visit(ast::ChrBehavior& b)
	{
		if (!_f(b)) return;
		b.on_succeeded_alt()->accept(*this);
		b.on_failed_alt()->accept(*this);
		b.on_succeeded_status()->accept(*this);
		b.on_failed_status()->accept(*this);
		b.behavior_body()->accept(*this);
	}

	void BodyApply::visit(ast::ChrTry& t)
	{
		if (_f(t)) t.body()->accept(*this);
	}

}
