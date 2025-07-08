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

#include <ast/body.hh>
#include <visitor/body.hh>

namespace chr::compiler::ast
{
	/*
	 * Body
	 */
	Body::Body(PositionInfo pos)
		: _pos(std::move(pos))
	{ }

	PositionInfo Body::position() const
	{
		return _pos;
	}

	Body* Body::clone() const
	{
		return new Body(*this);
	}

	void Body::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}


	/*
	 * ChrKeyword
	 */

	ChrKeyword::ChrKeyword(std::string_view name, PositionInfo pos)
		: Body(std::move(pos)), _name(name)
	{ }

	ChrKeyword::ChrKeyword(const ChrKeyword& o)
		: Body(o), _name(o._name)
	{ }

	std::string_view ChrKeyword::name() const
	{
		return _name;
	}

	Body* ChrKeyword::clone() const
	{
		return new ChrKeyword(*this);
	}

	void ChrKeyword::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * CppExpression
	 */

	CppExpression::CppExpression(PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos)
		: Body(std::move(pos)), _e(std::move(e)), _pragmas(std::move(pragmas))
	{ }

	CppExpression::CppExpression(PtrExpression e, PositionInfo pos)
		: Body(std::move(pos)), _e(std::move(e))
	{ }

	CppExpression::CppExpression(const CppExpression& o)
		: Body(o), _e( PtrExpression( o._e->clone()) )
	{
		_pragmas.reserve( o._pragmas.size() ); // Optional, improve performance
		for(auto& p : o._pragmas)
			_pragmas.emplace_back( p );
	}

	PtrExpression& CppExpression::expression()
	{
		return _e;
	}

	const std::vector< Pragma >& CppExpression::pragmas() const
	{
		return _pragmas;
	}

	void CppExpression::add_pragma(Pragma p)
	{
		_pragmas.emplace_back( p );
	}

	Body* CppExpression::clone() const
	{
		return new CppExpression(*this);
	}

	void CppExpression::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * ChrExpression
	 */

	ChrExpression::ChrExpression(PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos)
		: CppExpression(std::move(e), std::move(pragmas), std::move(pos))
	{ }

	ChrExpression::ChrExpression(PtrExpression e, PositionInfo pos)
		: CppExpression(std::move(e), std::move(pos))
	{ }

	ChrExpression::ChrExpression(const ChrExpression& o)
		: CppExpression(o)
	{ }

	void ChrExpression::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * CppDeclAssignment
	 */

	CppDeclAssignment::CppDeclAssignment(std::unique_ptr<CppVariable> v, PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos)
		: ChrExpression(std::move(e), std::move(pragmas), std::move(pos)), _v(std::move(v))
	{ }

	CppDeclAssignment::CppDeclAssignment(std::unique_ptr<CppVariable> v, PtrExpression e, PositionInfo pos)
		: ChrExpression(std::move(e), std::move(pos)), _v(std::move(v))
	{ }

	CppDeclAssignment::CppDeclAssignment(const CppDeclAssignment& o)
		: ChrExpression(o), _v( static_cast<CppVariable*>(o._e->clone()) )
	{ }

	std::unique_ptr<CppVariable>& CppDeclAssignment::variable()
	{
		return _v;
	}

	Body* CppDeclAssignment::clone() const
	{
		return new CppDeclAssignment(*this);
	}

	void CppDeclAssignment::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Unification
	 */

	Unification::Unification(std::unique_ptr<LogicalVariable> v, PtrExpression e, std::vector< Pragma > pragmas, PositionInfo pos)
		: ChrExpression(std::move(e), std::move(pragmas), std::move(pos)), _v(std::move(v))
	{ }

	Unification::Unification(std::unique_ptr<LogicalVariable> v, PtrExpression e, PositionInfo pos)
		: ChrExpression(std::move(e), std::move(pos)), _v(std::move(v))
	{ }

	Unification::Unification(const Unification& o)
		: ChrExpression(o), _v( static_cast<LogicalVariable*>(o._e->clone()) )
	{ }

	std::unique_ptr<LogicalVariable>& Unification::variable()
	{
		return _v;
	}

	Body* Unification::clone() const
	{
		return new Unification(*this);
	}

	void Unification::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * ChrConstraintCall
	 */

	ChrConstraintCall::ChrConstraintCall(PtrChrConstraint c, std::vector< Pragma > pragmas, PositionInfo pos)
		: ChrExpression(std::make_unique<Literal>("#",pos), std::move(pragmas), std::move(pos)), _c(std::move(c))
	{ }

	ChrConstraintCall::ChrConstraintCall(PtrChrConstraint c, PositionInfo pos)
		: ChrExpression(std::make_unique<Literal>("#",pos), std::move(pos)), _c(std::move(c))
	{ }

	ChrConstraintCall::ChrConstraintCall(const ChrConstraintCall& o)
		: ChrExpression(o), _c( static_cast<ChrConstraint*>(o._c->clone()) )
	{ }

	PtrChrConstraint& ChrConstraintCall::constraint()
	{
		return _c;
	}

	Body* ChrConstraintCall::clone() const
	{
		return new ChrConstraintCall(*this);
	}

	void ChrConstraintCall::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Sequence
	 */

	Sequence::Sequence(std::string_view op, PositionInfo pos)
		: Body(std::move(pos)),  _op(op)
	{ }

	Sequence::Sequence(std::string_view op, std::vector< PtrBody >& args, PositionInfo pos)
		: Body(std::move(pos)),  _op(op)
	{
		_children.reserve( args.size() ); // Optional, improve performance
		for(auto& a : args)
			_children.emplace_back( std::move(a) );
	}

	Sequence::Sequence(const Sequence& o)
		: Body(o), _op(o._op)
	{
		_children.reserve( o._children.size() ); // Optional, improve performance
		for(auto& child : o._children)
			_children.emplace_back( PtrBody( child->clone() ) );
	}

	std::string_view Sequence::op() const
	{
		return _op;
	}

	std::vector< PtrBody >& Sequence::children()
	{
		return _children;
	}

	void Sequence::add_child(PtrBody b)
	{
		_children.emplace_back( std::move(b) );
	}

	Body* Sequence::clone() const
	{
		return new Sequence(*this);
	}

	void Sequence::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * ChrBehavior
	 */

	ChrBehavior::ChrBehavior(PtrExpression stop_cond, PtrBody on_succeeded_alt, PtrBody on_failed_alt, PtrExpression final_status, PtrBody on_succeeded_status, PtrBody on_failed_status, PtrBody behavior_body, PositionInfo pos)
		: Body(std::move(pos)),  _stop_cond( std::move(stop_cond) ),
		_final_status( std::move(final_status) ),
		_on_succeeded_alt( std::move(on_succeeded_alt) ),
		_on_failed_alt( std::move(on_failed_alt) ),
		_on_succeeded_status( std::move(on_succeeded_status) ),
		_on_failed_status( std::move(on_failed_status) ),
		_behavior_body( std::move(behavior_body) )
	{ }

	ChrBehavior::ChrBehavior(const ChrBehavior& o)
		: Body(o), _stop_cond( PtrExpression(o._stop_cond->clone()) ),
		_final_status( PtrExpression( o._final_status->clone()) ),
		_on_succeeded_alt( o._on_succeeded_alt->clone() ),
		_on_failed_alt( o._on_failed_alt->clone() ),
		_on_succeeded_status( o._on_succeeded_status->clone() ),
		_on_failed_status( o._on_failed_status->clone() ),
		_behavior_body( o._behavior_body->clone() )
	{ }

	PtrExpression& ChrBehavior::stop_cond()
	{
		return _stop_cond;
	}

	PtrExpression& ChrBehavior::final_status()
	{
		return _final_status;
	}

	PtrBody& ChrBehavior::on_succeeded_alt()
	{
		return _on_succeeded_alt;
	}

	PtrBody& ChrBehavior::on_failed_alt()
	{
		return _on_failed_alt;
	}

	PtrBody& ChrBehavior::on_succeeded_status()
	{
		return _on_succeeded_status;
	}

	PtrBody& ChrBehavior::on_failed_status()
	{
		return _on_failed_status;
	}

	PtrBody& ChrBehavior::behavior_body()
	{
		return _behavior_body;
	}

	Body* ChrBehavior::clone() const
	{
		return new ChrBehavior(*this);
	}

	void ChrBehavior::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * ChrTry
	 */

	ChrTry::ChrTry(bool backtrack, std::unique_ptr<CppVariable> variable, PtrBody body, PositionInfo pos)
		: Body(std::move(pos)), _backtrack(backtrack), _variable( std::move(variable) ), _body( std::move(body) )
	{ }

	ChrTry::ChrTry(const ChrTry& o)
		: Body(o),
		_backtrack(o._backtrack),
		_variable( static_cast<CppVariable*>(o._variable->clone()) ),
		_body( o._body->clone() )
	{ }

	bool ChrTry::do_backtrack() const
	{
		return _backtrack;
	}

	std::unique_ptr<CppVariable>& ChrTry::variable()
	{
		return _variable;
	}

	PtrBody& ChrTry::body()
	{
		return _body;
	}

	Body* ChrTry::clone() const
	{
		return new ChrTry(*this);
	}

	void ChrTry::accept(visitor::BodyVisitor& v)
	{
		v.visit(*this);
	}

} // namespace chr::compiler::ast
