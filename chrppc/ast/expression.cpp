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

#include <ast/expression.hh>
#include <ast/body.hh>
#include <ast/program.hh>
#include <visitor/expression.hh>

namespace chr::compiler::ast
{
	/*
	 * Expression
	 */
	Expression::Expression(PositionInfo pos)
		: _pos(std::move(pos))
	{ }

	PositionInfo Expression::position() const
	{
		return _pos;
	}

	/*
	 * Identifier
	 */

	Identifier::Identifier(std::string_view value, PositionInfo pos)
		: Expression(std::move(pos)), _value(std::move(value))
	{ }

	std::string& Identifier::value()
	{
		return _value;
	}

	const std::string& Identifier::value() const
	{
		return _value;
	}

	Expression* Identifier::clone() const
	{
		return new Identifier(*this);
	}

	void Identifier::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Logical variable
	 */

	LogicalVariable::LogicalVariable(std::string_view name, PositionInfo pos)
		: Identifier(std::move(name),std::move(pos))
	{ }

	Expression* LogicalVariable::clone() const
	{
		return new LogicalVariable(*this);
	}

	void LogicalVariable::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * CPP variable
	 */

	CppVariable::CppVariable(std::string_view name, PositionInfo pos)
		: Identifier(std::move(name),std::move(pos))
	{ }

	Expression* CppVariable::clone() const
	{
		return new CppVariable(*this);
	}

	void CppVariable::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Literal
	 */

	Literal::Literal(std::string_view value, PositionInfo pos)
		: Expression(std::move(pos)), _value(std::move(value))
	{ }

	std::string& Literal::value()
	{
		return _value;
	}

	const std::string& Literal::value() const
	{
		return _value;
	}

	Expression* Literal::clone() const
	{
		return new Literal(*this);
	}

	void Literal::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Unary expression
	 */

	UnaryExpression::UnaryExpression(std::string_view op, PtrExpression e, PositionInfo pos)
		: Expression(std::move(pos)), _op(std::move(op)), _child(std::move(e))
	{ }

	UnaryExpression::UnaryExpression(const UnaryExpression& o)
		: Expression(o), _op(o._op), _child( PtrExpression( o._child->clone()) )
	{ }

	std::string_view UnaryExpression::op() const
	{
		return _op;
	}

	PtrExpression& UnaryExpression::child()
	{
		return _child;
	}

	void UnaryExpression::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Prefix unary expression
	 */

	PrefixExpression::PrefixExpression(std::string_view op, PtrExpression e, PositionInfo pos)
		: UnaryExpression(op,std::move(e),std::move(pos))
	{ }

	PrefixExpression::PrefixExpression(const PrefixExpression& o)
		: UnaryExpression(o)
	{ }

	Expression* PrefixExpression::clone() const
	{
		return new PrefixExpression(*this);
	}

	void PrefixExpression::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Postfix unary expression
	 */

	PostfixExpression::PostfixExpression(std::string_view op, PtrExpression e, PositionInfo pos)
		: UnaryExpression(op,std::move(e),std::move(pos))
	{ }

	PostfixExpression::PostfixExpression(const PostfixExpression& o)
		: UnaryExpression(o)
	{ }

	Expression* PostfixExpression::clone() const
	{
		return new PostfixExpression(*this);
	}

	void PostfixExpression::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Infix binary expression
	 */

	InfixExpression::InfixExpression(std::string_view op, PtrExpression l, PtrExpression r, PositionInfo pos)
		: Expression(std::move(pos)), _op(op), _l_child(std::move(l)), _r_child(std::move(r))
	{ }

	InfixExpression::InfixExpression(const InfixExpression& o)
		: Expression(o), _op(o._op), _l_child( PtrExpression( o._l_child->clone()) ), _r_child( PtrExpression( o._r_child->clone()) )
	{ }

	std::string_view InfixExpression::op() const
	{
		return _op;
	}

	PtrExpression& InfixExpression::l_child()
	{
		return _l_child;
	}

	PtrExpression& InfixExpression::r_child()
	{
		return _r_child;
	}

	Expression* InfixExpression::clone() const
	{
		return new InfixExpression(*this);
	}

	void InfixExpression::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Ternary expression
	 */

	TernaryExpression::TernaryExpression(std::string_view op1, std::string_view op2, PtrExpression e1, PtrExpression e2, PtrExpression e3, PositionInfo pos)
		: Expression(std::move(pos)), _op1(op1), _op2(op2), _e1(std::move(e1)), _e2(std::move(e2)), _e3(std::move(e3))
	{ }

	TernaryExpression::TernaryExpression(const TernaryExpression& o)
		: Expression(o), _op1(o._op1), _op2(o._op2), _e1( PtrExpression( o._e1->clone()) ), _e2( PtrExpression( o._e2->clone()) ), _e3( PtrExpression( o._e3->clone()) )
	{ }

	std::string_view TernaryExpression::op1() const
	{
		return _op1;
	}

	std::string_view TernaryExpression::op2() const
	{
		return _op2;
	}

	PtrExpression& TernaryExpression::child1()
	{
		return _e1;
	}

	PtrExpression& TernaryExpression::child2()
	{
		return _e2;
	}

	PtrExpression& TernaryExpression::child3()
	{
		return _e2;
	}

	Expression* TernaryExpression::clone() const
	{
		return new TernaryExpression(*this);
	}

	void TernaryExpression::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * Built-in constraint expression
	 */

	BuiltinConstraint::BuiltinConstraint(PtrExpression name, std::string_view l_delim, std::string_view r_delim, std::vector< PtrExpression >& args, PositionInfo pos)
		: Expression(std::move(pos)), _name(std::move(name)), _l_delim(l_delim=="<["?"<":l_delim), _r_delim(r_delim=="]>"?">":r_delim)
	{
		_children.reserve( args.size() ); // Optional, improve performance
		for(auto& a : args)
			_children.emplace_back( std::move(a) );
	}

	BuiltinConstraint::BuiltinConstraint(const BuiltinConstraint& o)
		: Expression(o), _name(o._name->clone()), _l_delim(o._l_delim), _r_delim(o._r_delim)
	{
		_children.reserve( o._children.size() ); // Optional, improve performance
		for(auto& child : o._children)
			_children.emplace_back( PtrExpression( child->clone() ) );
	}

	const PtrExpression& BuiltinConstraint::name() const
	{
		return _name;
	}

	std::string_view BuiltinConstraint::l_delim() const
	{
		return _l_delim;
	}

	std::string_view BuiltinConstraint::r_delim() const
	{
		return _r_delim;
	}

	std::vector< PtrExpression >& BuiltinConstraint::children()
	{
		return _children;
	}

	Expression* BuiltinConstraint::clone() const
	{
		return new BuiltinConstraint(*this);
	}

	void BuiltinConstraint::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}


	/*
	 * CHR constraint
	 */

	ChrConstraint::ChrConstraint(std::unique_ptr<Identifier> name, std::vector< PtrExpression >& args, PositionInfo pos)
		: Expression(std::move(pos)), _name(std::move(name))
	{
		_children.reserve( args.size() ); // Optional, improve performance
		for(auto& a : args)
			_children.emplace_back( std::move(a) );
	}

	ChrConstraint::ChrConstraint(std::shared_ptr<ChrConstraintDecl> decl, std::vector< PtrExpression >& args, PositionInfo pos)
		: Expression(std::move(pos)), _name( new Identifier(*decl->_c->constraint()->name()) ), _decl(decl)
	{
		_children.reserve( args.size() ); // Optional, improve performance
		for(auto& a : args)
			_children.emplace_back( std::move(a) );
	}

	ChrConstraint::ChrConstraint(const ChrConstraint& o)
		: Expression(o), _name( new Identifier(*o._name) ), _decl(o._decl) 
	{
		_children.reserve( o._children.size() ); // Optional, improve performance
		for(auto& child : o._children)
			_children.emplace_back( PtrExpression( child->clone() ) );
	}

	const std::unique_ptr<Identifier>& ChrConstraint::name() const
	{
		return _name;
	}

	const std::shared_ptr<ChrConstraintDecl>& ChrConstraint::decl() const
	{
		return _decl;
	}

	std::vector< PtrExpression >& ChrConstraint::children()
	{
		return _children;
	}

	Expression* ChrConstraint::clone() const
	{
		return new ChrConstraint(*this);
	}

	void ChrConstraint::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

	/*
	 * chr_count
	 */

	ChrCount::ChrCount(int use_index, PtrChrConstraint constraint, PositionInfo pos)
		: Expression(std::move(pos)), _use_index(use_index), _constraint( std::move(constraint) )
	{
	}

	ChrCount::ChrCount(const ChrCount& o)
		: Expression(o), _use_index(o._use_index), _constraint( new ChrConstraint(*o._constraint) ) 
	{
	}

	int ChrCount::use_index() const
	{
		return _use_index;
	}

	PtrChrConstraint& ChrCount::constraint()
	{
		return _constraint;
	}

	Expression* ChrCount::clone() const
	{
		return new ChrCount(*this);
	}

	void ChrCount::accept(visitor::ExpressionVisitor& v)
	{
		v.visit(*this);
	}

} // namespace chr::compiler::ast
