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

#pragma once

#include <algorithm>
#include <cassert>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

#include <ast/expression.hh>

namespace chr::compiler {

	/**
	 * A node of the dependency graph which can be a CHR or Built-in constraint.
	 * A built-in constraint node can only be found at the end of an edge
	 * as it can't be found in the head of a rule.
	 */
	class Node
	{
	public:
	    /**
	     * @brief Type of the node
	     */
		enum class Type : int {
	        CHR = 0,	//!< CHR constraint node
	        BUILTIN		//!< Built-in constraint node
	    };

		/**
		 * Initialize node with a built-in constraint.
		 */
		Node();

		/**
		 * Initialize node with an expression (which is associated to a built-in constraint).
		 * @param c The constraint
		 */
		Node(const ast::Expression& e);

		/**
		 * Initialize node with a built-in constraint.
		 * @param c The constraint
		 */
		Node(const ast::BuiltinConstraint& c);

		/**
		 * Initialize node with a built-in constraint.
		 * @param c The constraint
		 */
		Node(const std::unique_ptr<ast::BuiltinConstraint>& c);

		/**
		 * Initialize node with a CHR constraint.
		 * @param c The constraint
		 */
		Node(const ast::ChrConstraint& c);

		/**
		 * Initialize node with a CHR constraint.
		 * @param c The constraint
		 */
		Node(const std::unique_ptr<ast::ChrConstraint>& c);

		/**
		 * Override of operator==.
		 * @param n The other node
		 * @return True if the two nodes are identical, false otherwise
		 */
		bool operator==(const Node& n) const;

		/**
		 * Override of operator!=.
		 * @param n The other node
		 * @return True if the two nodes are different, false otherwise
		 */
		bool operator!=(const Node& n) const;

		/**
		 * Convert the node to a string. It uses the label and the type
		 * to know if we had to write the chr id in it.
		 * @return A string corresponding to the node
		 */
		std::string to_string() const;

		/**
		 * Return the name of the node.
		 * @return The name of the node
		 */
		std::string_view name() const { return _name; }

		/**
		 * Return the type of the node.
		 * @return The type of the node
		 */
		Type type() const { return _type; }

	private:
		Type _type;					///< The type of the node
		std::string_view _name;		///< Name (identifier) of the constraint of the node
		const ast::Expression* _c;	///< The constraint of the node
	};

} // namespace chr::compiler

namespace std {
	/**
	 * Specialization of hash function for Node to be used in unordered_set
	 */
	template <>
	struct hash< chr::compiler::Node >
	{
	    inline size_t operator()(const chr::compiler::Node& n) const
	    {
	        std::hash< std::string_view > str_hasher;
	        return str_hasher(n.name());
	    }
	};
}

namespace chr::compiler {

	/**
	 * A dependency graph for a CHR program.
	 * It's an oriented graph which represent the dependencies between the head
	 * and the body of a rule. The guard is not relevant as we are only interested
	 * in constraint that may update the program (constraint store, variables, etc.).
	 */
	class DependencyGraph
	{
		/**
		 * Oriented edges coming from one source node
		 */
		struct OrientedEdges
		{
			std::unordered_set< Node > _partners;	///< The partners nodes (CHR constraints) of the _src_node
			std::unordered_set< Node > _dst_nodes;	///< The destinations nodes of the edge
		};

	public:
		/**
		 * Add a new source node in the graph.
		 * @param node The new node to add
		 * @return An iterator to the newly inserted node or to the already existing node
		 */
		std::unordered_map<Node, OrientedEdges>::iterator add_node(Node node);

		/**
		 * Add a new egde in the graph.
		 * @param src_node The source node of the edge (must be a CHR constraint node)
		 * @param dst_node The destination node
		 */
		void add_edge(Node src_node, Node dst_node);

		/**
		 * Add a new partner to the src_node of an edge of the graph.
		 * @param src_node The source node of the edge (must be a CHR constraint node)
		 * @param partner_node The partner node
		 */
		void add_partner(Node src_node, Node partner_node);

		/**
		 * Test if a CHR constraint is observed by another one.
		 * @param constraint The constraint searched for
		 * @return True if the constraint is observed, false otherwise
		 */
		bool observed(const ast::PtrChrConstraint& c) const;

		/**
		 * Convert the graph to a string representation (human readable).
		 * @return The string corresponding to this dependency graph
		 */
		std::string to_string() const;

	private:
		std::unordered_map< Node, OrientedEdges > graph;	///< The edges of the graph.
															///< Each element of the set corresponds to a CHR constraint
	};

} // namespace chr::compiler

