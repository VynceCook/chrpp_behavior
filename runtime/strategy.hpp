/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Apache License, Version 2.0.
 *
 *  Copyright:
 *     2016, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
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

#ifndef RUNTIME_STRATEGY_HH_
#define RUNTIME_STRATEGY_HH_

#include <list>
#include <cassert>
#include <cstring>

#include <shared_obj.hh>
#include <backtrack.hh>
#include <utils.hpp>

namespace chr
{
	/**
	 * @brief Manage a tree which backtrack in order to store strategy(s)
	 *
	 * Backtrackable trees stores trees. The main use is to store solutions. The
	 * root is a fake one in order to be able to store a forest or a simple tree
	 * (the children of the root contains the list of the trees).
	 * It stores the backtrack depth of each node and rewind to the parent node
	 * of new depth when the CHR program backtrack.
	 * It is usefull to gather the strategy of a quantified or optimisation problem.
	 */
	template < typename T >
	class Backtrackable_trees : public chr::Backtrack_observer
	{
	public:
		class Node
		{
			friend class Backtrackable_trees<T>;

			T _e;											///< The value stored in the node
			chr::Depth_t _backtrack_depth;					///< The backtrack depth used when the node was created
			Node* _p_parent;								///< Parent Node
			std::list< std::shared_ptr< Node > > _children;	///< Children nodes
	
		public:
			/**
			 * Initialize.
			 */
			Node(const T& e, chr::Depth_t depth, Node * parent)
				: _e(e), _backtrack_depth(depth), _p_parent(parent)
			{}

			/**
			 * Initialize (fake root, no value for _e).
			 */
			Node(chr::Depth_t depth, Node * parent)
				: _backtrack_depth(depth), _p_parent(parent)
			{}

			/**
			 * Check if current node is the fake root or not
			 * @return True if node is the fake root, false otherwise
			 */
			bool is_root() const
			{
				return _p_parent == nullptr;
			}

			/**
			 * Return the value of the node
			 * @return A const reference to the value of the node
			 */
			const T& value() const
			{
				assert(_p_parent != nullptr); /* Can't get value from fake root */
				return _e;
			}

			/**
			 * Return the parent of the node
			 * @return A const reference to the parent of the node
			 */
			const Node& parent() const { assert(_p_parent != nullptr); return *_p_parent; }
	
			/**
			 * Return the children of the node
			 * @return A const reference to the list of children of the node
			 */
			const std::list< std::shared_ptr< Node > >& children() const { return _children; }
	
			/**
			 * Needed to be print a constraint store.
			 * @return A string representation of value of current node
			 */
			std::string to_string() const
			{
				if (_children.empty()) {
					return std::string(chr::TIW::to_string(_e));
				} else {
					std::string str;
					str += "(" + chr::TIW::to_string(_e) + ",[";
	
					bool first = true;
					for (auto& p : _children)
					{
						if (!first) str += ",";
						first = false;
						str += p->to_string();
					}
					str += "])";
					return str;
				}
			}

			/**
			 * Convert current sub-tree to a std::string
			 * @param d The decorator used to format Node
			 * @param depth The depth of this node in the tree
			 * @return A string representation of current sub-strategy (subtree)
			 */
			template< typename Decorator > std::string to_string(Decorator d, unsigned int depth) const
			{
				if (_children.empty()) {
					return d.decorate_leaf(*this,depth);
				} else {
					std::string str;
					str += d.decorate(*this,depth);
					str += d.left(*this,depth);
	
					unsigned int n_idx = 0;
					for (auto& p : _children)
					{
						if (n_idx != 0) str += d.sep(*this,depth);
						str += p->to_string(d,depth+1);
						++n_idx;
					}
					str += d.right(*this,depth);
					return str;
				}
			}
		};
	
		/**
		 * Initialize.
		 */
		Backtrackable_trees()
			: chr::Backtrack_observer()
		{
			_scheduled = false;
			// Add a fake root in order to call drop subtree whatever the backtrack level
			_root = std::make_unique< Node >(chr::Backtrack::depth(),nullptr);
			_p_current = _root.get();
		}
	
		/**
		 * Copy constructor: deleted.
		 * @param o The source variable
		 */
		Backtrackable_trees(const Backtrackable_trees& o) =delete;
	
		/**
		 * Virtual destructor
		 */
		virtual ~Backtrackable_trees() {}
	
		/**
		 * Deleted assignment operator.
		 */
		Backtrackable_trees& operator=(const Backtrackable_trees& o) =delete;
	
		/**
		 * Return the root of the backtrackable tree
		 * @return A const reference to the root
		 */
		const Node& root() const
		{
			return *_root;
		}
	
		/**
		 * Return the current node of the tree
		 * @return A const reference to the current node
		 */
		const Node& current_node() const
		{
			assert(_p_current);
			return *_p_current;
		}
	
		/**
		 * Add a new child to the tree and set the current node to this new
		 * child.
		 * @param e the new element to store
		 */
		void add_child(const T& e)
		{
			assert(_p_current);
			if (!_scheduled)
			{
				_scheduled = true;
				chr::Backtrack::schedule( this ); // Assume that this is a *naked* Shared_obj
			}
	
			auto new_node = std::make_shared< Node >(e,chr::Backtrack::depth(),_p_current);
			Node* p = new_node.get();
			_p_current->_children.push_back( new_node );
			_p_current = p;
		}
	
		/**
		 * Drop all the subtree started with the current node
		 */
		void drop_subtree()
		{
			assert(_p_current);
			_p_current->_children.clear();
		}
	
		/**
		 * Convert current strategy to std::string
		 * @return A string representation of current strategy
		 */
		std::string to_string() const
		{
			assert( _root );
			assert( _root->_children.size() == 1);
			return _root->_children.front()->to_string();
		}
	
		/**
		 * Convert current strategy to std::string using a user Decorator
		 * @param d The Decorator to use to format elements of the tree
		 * @return A string representation of current strategy
		 */
		template< typename Decorator > std::string to_string(Decorator d) const
		{
			assert( _root );
			assert( _root->_children.size() == 1);
			return _root->_children.front()->to_string(d,1);
		}
	
	private:
		/**
		 * Rewind the current node in the backtrackable tree to the node of depth
		 * \a new_depth of the current branch.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return True if the callback can be removed from the wake up list, false otherwise
		 */
		bool rewind(chr::Depth_t, chr::Depth_t new_depth) override
		{
			assert(_p_current);
			while ( (_p_current->_p_parent != nullptr)
					&& (_p_current->_backtrack_depth > new_depth) )
			{
				_p_current = _p_current->_p_parent;
			}
			return true;
		}
	
		std::unique_ptr< Node > _root;	///< Root node
		Node* _p_current;				///< Current Node
		bool _scheduled;				///< Flag to know if it has been already scheduled
	};
	
	/**
	 * Wrapper class for the most common use of a Backtrackable_trees in a
	 * CHR program
	 */
	template< typename T >
	class Strategy
	{
	public:
		using Node = typename Backtrackable_trees<T>::Node;

		/**
		 * Class used to iterate over the first branch of the tree.
		 * When the problem is not a quantified one, it simplifies the
		 * access to the strategy
		 */
		class First_branch
		{
		public:
			/**
			 * The iterator on the first branch
			 */
			class iterator
			{
				friend class First_branch; 
			public:
				/**
				 * Test if current iterator is different from \a o
				 * @return True if o != this, false otherwise
				 */
				bool operator!=(const iterator& o) const
				{
					return _p_current != o._p_current;
				}

				/**
				 * Test if current iterator is equal to \a o
				 * @return True if o == this, false otherwise
				 */
				bool operator==(const iterator& o) const
				{
					return _p_current == o._p_current;
				}

				/**
				 * Return value given by current iterator
				 * @return A const reference to the underlying value
				 */
				const T& operator*() const
				{
					return _p_current->value();
				}

				/**
				 * Move the iterator to the next node (element).
				 * @return A reference to the current iterator
				 */
				iterator& operator++()
				{
					assert(_p_current != nullptr);
					if (_p_current->children().empty())
						_p_current = nullptr;
					else
					{
						auto& n = _p_current->children().front();
						_p_current = n.get();
					}
					return *this;
				}

			private:
				/**
				 * Initialize.
				 * @param the strategy to use
				 */
				iterator(const chr::Shared_obj< Backtrackable_trees<T> >& stgy)
				{
					if (stgy->root().children().empty())
						_p_current = nullptr;
					else
						_p_current = stgy->root().children().front().get();
				}

				/**
				 * Initialize to an end iterator.
				 * 
				 */
				iterator() : _p_current(nullptr) { }

				Node* _p_current;	///< Current Node
			};

			/**
			 * Initialize.
			 * @param stgy The strategy to use for initialize first branch
			 */
			First_branch(const chr::Shared_obj< Backtrackable_trees<T> >& stgy)
				: _stgy(stgy)
			{ }

			/**
			 * Return an iterator to the first element of first branch of strategy.
			 * @return an iterator to first element
			 */
			iterator begin() const { return iterator(_stgy); }

			/**
			 * Return an iterator at end of  first branch of strategy.
			 * @return an iterator at end of first branch
			 */
			iterator end() const { return iterator(); }
	
		private:
			chr::Shared_obj< Backtrackable_trees<T> > _stgy; ///< The strategy object
		};

		/**
		 * Initialize.
		 */
		Strategy() : _stgy(chr::make_shared< Backtrackable_trees< T > >())
		{ }

		/**
		 * Copy constructor: deleted.
		 * @param o The source variable
		 */
		Strategy(const Strategy& o) =delete;
	
		/**
		 * Deleted assignment operator.
		 */
		Strategy& operator=(const Strategy& o) =delete;
	
		/**
		 * Return the wrapper to the first branch of strategy
		 * @return An objet which represents the first branch of strategy
		 */
		First_branch first_branch() const
		{
			return First_branch(_stgy);
		}
	
		/**
		 * Return the root of the strategy (first element)
		 * @return A const reference to the root element
		 */
		const Node& root() const
		{
			return _stgy->root();
		}
	
		/**
		 * Return the current element of the strategy
		 * @return A const reference to the current element (a Node)
		 */
		const Node& current_element() const
		{
			return _stgy->current_node();
		}
	
		/**
		 * Add a new child to the current node of strategy.
		 * @param e the new element to add
		 */
		void add_child(const T& e)
		{
			_stgy->add_child(e);
		}
	
		/**
		 * Drop all the subtree started with the current node in strategy
		 */
		void drop_subtree()
		{
			_stgy->drop_subtree();
		}
	
		/**
		 * Convert current strategy to std::string using DefaultDecorator
		 * @return A string representation of current strategy
		 */
		std::string to_string() const
		{
			return _stgy->to_string();
		}
	
		/**
		 * Convert current strategy to std::string using a user Decorator
		 * @param d The Decorator to use to format elements of the tree
		 * @return A string representation of current strategy
		 */
		template< typename Decorator > std::string to_string(Decorator d) const
		{
			return _stgy->to_string(d);
		}

	private:
		chr::Shared_obj< Backtrackable_trees<T> > _stgy; ///< The strategy object
	};
}

#endif /* RUNTIME_STRATEGY_HH_ */
