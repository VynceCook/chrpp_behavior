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
	bool BodyEmpty::is_empty(ast::Body& e)
	{
		e.accept( *this );
		return res;
	}

	void BodyEmpty::visit(ast::Body&)
	{
		res = true;
	}

	void BodyEmpty::visit(ast::ChrKeyword&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::CppExpression&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::CppDeclAssignment&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::Unification&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::ChrConstraintCall&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::Sequence&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::ChrBehavior&)
	{
		res = false;
	}

	void BodyEmpty::visit(ast::ChrTry&)
	{
		res = false;
	}
}
