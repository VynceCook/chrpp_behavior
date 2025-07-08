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

#include <dependency_graph.hh>
#include <visitor/expression.hh>

namespace chr::compiler {

	Node::Node()
		: _type(Type::BUILTIN), _name("built-in"), _c(nullptr)
	{ }

	Node::Node(const ast::Expression& c)
		: _type(Type::BUILTIN), _name("built-in"), _c(&c)
	{ }

	Node::Node(const ast::BuiltinConstraint& c)
		: _type(Type::BUILTIN), _name("built-in"), _c(&c)
	{ }

	Node::Node(const std::unique_ptr<ast::BuiltinConstraint>& c)
		: _type(Type::BUILTIN), _name("built-in"), _c(c.get())
	{ }

	Node::Node(const ast::ChrConstraint& c)
		: _type(Type::CHR), _name(c.name()->value()), _c(&c)
	{ }

	Node::Node(const std::unique_ptr<ast::ChrConstraint>& c)
		: _type(Type::CHR), _name(c->name()->value()), _c(c.get())
	{ }

	bool Node::operator==(const Node& n) const
	{
		return (_type == n._type) && (_name == n._name);
	}

	bool Node::operator!=(const Node& n) const
	{
		return (_type != n._type) || (_name != n._name);
	}

	std::string Node::to_string() const
	{
		visitor::ExpressionPrint v;
		if (_c == nullptr )
			return std::string(_name);
		else
			return std::string(_name) + "<" + std::string(v.string_from(const_cast<ast::Expression&>(*_c))) + ">";
	}

	std::unordered_map<Node, DependencyGraph::OrientedEdges>::iterator DependencyGraph::add_node(Node node)
	{
		assert( node.type() == Node::Type::CHR );
		auto res = graph.insert( { std::move(node), OrientedEdges() } );
		return res.first;
	}

	void DependencyGraph::add_edge(Node src_node, Node dst_node)
	{
		assert( src_node.type() == Node::Type::CHR );
		auto it = add_node(src_node);
		if (dst_node.type() == Node::Type::CHR)
			(void) add_node(dst_node);
		(*it).second._dst_nodes.insert( dst_node );
	}

	void DependencyGraph::add_partner(Node src_node, Node partner_node)
	{
		assert( src_node.type() == Node::Type::CHR );
		assert( partner_node.type() == Node::Type::CHR );
		auto it_src = add_node(src_node);
		(*it_src).second._partners.insert( partner_node );
	}

	bool DependencyGraph::observed(const ast::PtrChrConstraint& c) const
	{
		if (graph.empty()) return false;
		Node c_node(c);
		assert( graph.find(c_node) != graph.end() );
		std::unordered_set< Node > marked;
		std::deque< Node > current_nodes;
		current_nodes.emplace_back( c_node );
		while ( !current_nodes.empty() )
		{
			auto n = current_nodes.front();
			current_nodes.pop_front();
			marked.insert( n );
			auto it = graph.find( n );
			assert( it != graph.end() );
			// Check in partners
			if ((*it).second._partners.find( c_node ) != (*it).second._partners.end())
				return true;
			// Check in body
			for (auto& dst_n : (*it).second._dst_nodes)
			{
				if (dst_n.type() == Node::Type::BUILTIN)
					return true;
				if ((dst_n.type() == Node::Type::CHR) && (marked.find(dst_n) == marked.end()))
					current_nodes.emplace_back( dst_n );
			}
		}
		return false;
	}

	std::string DependencyGraph::to_string() const
	{
		std::string str;

		for (auto& node : graph)
		{
			str += node.first.to_string();
			std::string str_partners;
			for (auto& n : node.second._partners)
				str_partners += n.to_string() + ", ";
			if (!node.second._partners.empty())
			{
				str_partners.resize(str_partners.size() - 2);
				str += " { " + str_partners + " }";
			}
			str += " -->";
			std::string str_dst_nodes;
			for (auto& n : node.second._dst_nodes)
				str_dst_nodes += " " + n.to_string() + ",";
			if (!node.second._dst_nodes.empty())
				str_dst_nodes.resize(str_dst_nodes.size() - 1);
			str += str_dst_nodes;
			str += "\n";
		}
		return str;
	}

} // namespace chr::compiler
