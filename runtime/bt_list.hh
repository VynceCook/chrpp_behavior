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

#ifndef RUNTIME_BT_LIST_HH_
#define RUNTIME_BT_LIST_HH_

#include "statistics.hh"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <functional>
#include <string>

#include <type_traits>
#include <utils.hpp>
#include <backtrack.hh>

// Forward declaration
namespace tests {
	namespace bt_list {
		void insert_remove_tests();
		void iterators();
		void backtrack_tests();
		void replace_backtrack_tests();
	}
}

namespace chr {

	// Forward declaration
	template< typename T > class Logical_var_imp;
	template< typename T, bool RANGE > class Interval;
	template< typename T, bool ENABLE_BACKTRACK > class Constraint_store_simple;
	template< typename T, typename TupIndexes, bool ENABLE_BACKTRACK > class Constraint_store_index;
	template< typename T > class Bt_list_iterator;

	/**
	 * @brief Extra data structure of Bt_list
	 *
	 * Template data structure to add more members to a Bt_list
	 * depending on the value of a template parameter
	 */
	template< bool SD > struct Extra { };

	/**
	 * @brief Extra< true >
	 *
	 * Specialization for SAFE_DELETE = true. Add a new member _nb_free
	 */
	template<> struct Extra<true>
	{
		unsigned int _nb_free;	///< Number of free elements (size of the free list)

		/**
		 * Default constructor
		 */
		Extra() : _nb_free(0) { }
	};

	/**
	 * @brief Backtrackable list
	 *
	 * A backtrackable list store elements in contiguous arrays.
	 * Add and removal of an element is done in constant time.
	 * An iterator on a removed element is not valid anymore.
	 *
	 * Each Bt_list is backtrackable. Modifications of the list are recorded and undone
	 * when the list is rewind to a previous backtrack depth.
	 */
	template< typename T, bool ENABLE_BACKTRACK = true, bool SAFE_DELETE = false, unsigned int STATISTICS_T = chr::Statistics::OTHER, typename Allocator_t = std::allocator< T > >
	class Bt_list
	{
	public:
		typedef T Value_t;
		typedef std::uint32_t PID_t;
		typedef class Bt_list_iterator< Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t > > iterator;
		typedef Allocator_t _Allocator_t; // Shortcut to access template parameter from outside the class

		static constexpr bool _ENABLE_BACKTRACK = ENABLE_BACKTRACK;
		static constexpr bool _SAFE_DELETE = SAFE_DELETE;
		static constexpr unsigned int _STATISTICS_T = STATISTICS_T;
		static constexpr unsigned int NB_SPECIAL_SLOTS = 1;
		static constexpr PID_t END_LIST = ~0u;

		static const unsigned int ALIVE = 0u;
		static const unsigned int BACKTRACK = 1u;
		static const unsigned int REMOVE = 2u;
		static const unsigned int SPECIAL = 3u;
		static const unsigned int BACKTRACK_CLOSURE = 4u;

		/// Check if we need to destroy the struct
		static constexpr bool NEED_DESTROY = !std::is_scalar<T>::value && chr::Has_need_destroy<T>::value;

	private:
		friend class Constraint_store_simple< T, ENABLE_BACKTRACK >;
		template < typename TT > friend class Logical_var_imp;
		template < typename TT, bool RANGE > friend class Interval;
		template < typename TT, typename TupIndexes, bool C_ENABLE_BACKTRACK > friend class Constraint_store_index;
		friend class Bt_list_iterator< Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t > >;
		// Friend for Unit tests
		friend void tests::bt_list::insert_remove_tests();
		friend void tests::bt_list::iterators();
		friend void tests::bt_list::backtrack_tests();
		friend void tests::bt_list::replace_backtrack_tests();
	public:
		/**
		 * Bt_list is composed of linked Node(s)
		 */
		struct Node
		{
			/// Node allocator
			using Allocator = typename std::allocator_traits< Allocator_t >::template rebind_alloc< Node >;

			struct
			{
				unsigned int _status : 3;		///< Status of node (ALIVE, BACKTRACK, REMOVE, SPECIAL, BACKTRACK_CLOSURE)
				unsigned int _obs_count : 30;	///< Number of objects that watch this node
			};
			union
			{
				/**
				 * Struct used for a normal, removed or closure slot.
				 */
				struct
				{
					PID_t _prev;						///< Link to previous node
					union
					{
						/**
						 * Struct used for a normal or removed.
						 */
						struct
						{
							PID_t _next;				///< Link to next node
							T _e;						///< Element stored in the node
						};
						/**
						 * Struct used for closure slot.
						 */
						struct
						{
							PID_t _closure_next;		///< PID of the first element at the previous rollback point
							PID_t _closure_prev;		///< Size of the list at the previous rollback point
						};
					};
				};
				/**
				 * Struct used to store values used to restore a previous list state.
				 * This particular node is linked with other rewind nodes.
				 */
				struct
				{
					PID_t _next_rewind;				///< PID of the next rewind element (a rewind element is created at each rollback point)
					PID_t _first_removed_to_restore;///< PID of the first element to restore (created before rollback point, but removed after)
					Depth_t _rem_backtrack_depth;	///< Backtrack depth at the previous rollback point
				};
				/**
				 * Struct used to store values used to restore a previous list state.
				 * This particular node is not linked with others but is found at
				 * a given pid, that's why we can use other members.
				 */
				struct
				{
					PID_t _rem_first_free;			///< PID of the first free at the previous rollback point
					PID_t _rem_first;				///< PID of the first element at the previous rollback point
					PID_t _rem_size;				///< Size of the list at the previous rollback point
				};
			};

			/**
			 * Default constructor set delete.
			 */
			Node() =delete;

			/**
			 * Copy constructor set delete.
			 */
			Node(const Node& ) =delete;

			/**
			 * Move constructor.
			 * @param o The other element
			 */
			Node(Node&& o) : _status(o._status), _obs_count(o._obs_count), _prev(o._prev), _next(o._next), _e( std::move(o._e) )
			{ }

			/**
			 * Destructor.
			 */
			~Node() {}
		};

	public:
		/**
		 * Initialize.
		 */
		Bt_list()
			: _first(END_LIST),
			  _last(END_LIST),
			  _first_unused_pid(0),
			  _first_free(END_LIST),
			  _first_rewind(END_LIST),
			  _size(0),
			  _backtrack_depth(Backtrack::depth()),
			  _capacity(0),
			  _data(nullptr)
		{ }

		/**
		 * Copy constructor.
		 * @param o The other list
		 */
		Bt_list(const Bt_list& o) =delete;

		/**
		 * Move constructor.
		 * @param o The other list
		 */
		Bt_list(Bt_list&& o) noexcept
			: _first(o._first),
			  _last(o._last),
			  _first_unused_pid(o._first_unused_pid),
			  _first_free(o._first_free),
			  _first_rewind(o._first_rewind),
			  _size(o._size),
			  _backtrack_depth(o._backtrack_depth),
			  _capacity(o._capacity),
			  _data(o._data)
		{
			o._first = END_LIST;
			o._last = END_LIST;
			o._first_unused_pid = 0;
			o._first_free = END_LIST;
			o._first_rewind = END_LIST;
			o._size = 0;
			o._capacity = 0;
			o._data = nullptr;
			if constexpr (SAFE_DELETE)
			{
				extra._nb_free = o.extra._nb_free;
				o.extra._nb_free = 0;
			}
		}

		/**
		 * Destroy.
		 */
		~Bt_list();

		/**
		 * Assignment operator overload.
		 */
		Bt_list& operator=(const Bt_list& o) =delete;

		/**
		 * Move assignment operator.
		 * @param o The other list
		 */
		Bt_list& operator=(Bt_list&& o) noexcept
		{
			_first = o._first;
			_last = o._last;
			_first_unused_pid = o._first_unused_pid;
			_first_free = o._first_free;
			_first_rewind = o._first_rewind;
			_size = o._size;
			_backtrack_depth = o._backtrack_depth;
			_capacity = o._capacity;
			_data = o._data;
			o._first = END_LIST;
			o._last = END_LIST;
			o._first_unused_pid = 0;
			o._first_free = END_LIST;
			o._first_rewind = END_LIST;
			o._size = 0;
			o._capacity = 0;
			o._data = nullptr;
			return *this;
		}

		/**
		 * Clone Bt_list
		 * @return a new fresh clone of this Bt_list
		 */
		Bt_list clone() const;

		/**
		 * Check is the list is empty.
		 * Be careful, the elements that are removed but not
		 * yet free are counted!
		 * The list is empty if all elements are freed.
		 * @return True if the list is empty, false otherwise
		 */
		bool empty() const { return (_first == END_LIST); }

		/**
		 * Returns the size of the list, i.e. the number of living
		 * elements.
		 * @return The number of elements alive in the Bt_list
		 */
		std::size_t size() const {
			return _size;
		}

		/**
		 * Return the current backtrack depth.
		 * @return The current backtrack depth
		 */
		Depth_t depth() const { return _backtrack_depth; }

		/**
		 * Check is the list has a rollback point.
		 * @return True if there is a rollback point, false otherwise
		 */
		bool has_rollback_point() const { return (_first_rewind != END_LIST); }

		/**
		 * Return the current element at index \a p
		 * @param p The index of the element to retrieve
		 * @return A const reference to the element pointed
		 */
		const T& get(PID_t p) const {
			return _data[p]._e;
		}

		/**
		 * Build and return an iterator on the first element
		 * of the list.
		 * @return An iterator on first element
		 */
		iterator begin();

		/**
		 * Build and return an iterator on the end of the list
		 * of the list.
		 * @return An iterator on the end of the list
		 */
		iterator end();

		/**
		 * Insert an element at the begining of the list
		 * @param e The element to add
		 * @return An iterator to the new added element in the list
		 */
		iterator insert(T e);

		/**
		 * Remove an element from the list. When the element is removed, the iterator \a it
		 * becomes undefined. It returns an iterator to the next alive element. 
		 * @param it Iterator on the element to remove
		 * @return An iterator to the next alive element
		 */
		iterator remove(const iterator& it);

		/**
		 * Remove an element from the list. When an element is remove, the iterator becomes
		 * undefined. But you can still call the ++ operator to move to the next defined element
		 * of the list. In short, you operator* is undefined but operator++ still works!
		 * @param p The element to remove
		 */
		void remove(PID_t p);

		/**
		 * Insert an element before \a before_pid
		 * @param before_pid The pid of the element which is before \a e
		 * @param before_it Iterator on the element which is before \a e
		 * @return An iterator to the new added element in the list
		 */
		iterator insert_before(const iterator& before_it, T e);

		/**
		 * Insert an element before \a before_pid
		 * @param before_pid The pid of the element which is before \a e
		 * @param e The element to add
		 */
		void insert_before(PID_t before_pid, T e);

		/**
		 * Replace an element from the list by another one. When the element is replaced,
		 * the iterator \a it becomes undefined.
		 * It returns an iterator to the next alive element. 
		 * @param it Iterator on the element to remove
		 * @param e The element to insert in place of \a p
		 * @return An iterator to the next alive element
		 */
		iterator replace(const iterator& p, T e);

		/**
		 * Replace an element from the list by another one. As for the remove function,
		 * when an element is remove, the iterator becomes undefined. But you can still call
		 * the ++ operator to move to the next defined element of the list.
		 * In short, you operator* is undefined but operator++ still works!
		 * @param p The element to remove
		 * @param e The element to insert in place of \a p
		 */
		void replace(PID_t p, T e);

		/**
		 * Check if the element is still alive (Not set to be removed)
		 * @param p The element to check
		 * @return True is the element is alive, false otherwise
		 */
		bool alive(PID_t p) const
		{
			return _data[p]._status == ALIVE;
		}

		/**
		 * Rewind to the snapshot of the list stored at \a depth.
		 * All snapshots encountered along the path will be forgotten.
		 * The rewind() member function exists only if ENABLE_BACKTRACK is true. Otherwise,
		 * SFINAE disables it.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		template< bool EB = ENABLE_BACKTRACK >
		std::enable_if_t<EB, bool> rewind(Depth_t previous_depth, Depth_t new_depth);

		/**
		 * Return a string representation of the list. Useful for debugging
		 * purpose.
		 * @param debug Is 0 if we just have to print list values, 1 for full information, 2 for full information + capacity
		 * @return A string representation of the structure of the list
		 */
		std::string to_string(unsigned int debug=0) const;

		/**
		 * Return true if the list can be safely deleted. It can be deleted if it is
		 * empty and all slots are free (not a remaining REMOVE one). Furthermore it must
		 * not have some backtrackable state to rewind.
		 * The safe_delete() member function exists only if SAFE_DELETE is true. Otherwise,
		 * SFINAE disables it.
		 * @return True if the list can be safely deleted, false otherwise
		 */
		template< bool SD = SAFE_DELETE >
		std::enable_if_t<SD, bool> safe_delete() const {
			// Test return true only if we have no previous backtrack state.
			// We cannot safe delete if there some backtrackable elements in the list.
			// The single equality below is enough. Indeed, if a previous backtrack state
			// exists, there is some pid used to store values to remember and _first_unused_pid > _nb_free
			return _first_unused_pid == extra._nb_free;
		}
	protected:
		PID_t _first;								    ///< First node of the list
		PID_t _last;								    ///< Last node of the list
		PID_t _first_unused_pid;					    ///< First unused pid of the list (has never been ALIVE, FREE or REMOVE)
		PID_t _first_free;							    ///< First free node
		PID_t _first_rewind;						    ///< The pid of the first element to rewind
		unsigned int _size;						        ///< The number of elements in the list
		Depth_t _backtrack_depth;					    ///< The depth which is relevant to this list (snapshot)

		std::size_t _capacity;						    ///< The allocated size for data
		Node* _data;								    ///< Data (nodes) of the list

		Extra<SAFE_DELETE> extra;						///< Number of free elements (size of the free list)

		/**
		 * Return the node pointed by the pid \a p
		 * @param p The PID of the node to return
		 */
		const Node& data(PID_t p) const { return _data[p]; }

		/**
		 * Set a slot to be a free one. It may be reused for a further add.
		 * @param p The element to free
		 */
		void free(PID_t p);

		/**
		 * Reallocate list (the first unused pid have to reach the capacity of the list).
		 */
		void reallocate_list();

		/**
		 * Increment the number of observers of slot \a p.
		 * @param p The element to update
		 */
		void do_inc_obs_count(PID_t p)
		{
			assert(_data[ p ]._status == ALIVE);
			// Lock only if it is not part of backtrackable elements
			if ((_first_rewind == END_LIST) || (p > _first_rewind))
				++_data[p]._obs_count;
		}

		/**
		 * Test if we must create rollback point and
		 * increment the number of observers of slot \a p.
		 * @param p The element to update
		 */
		void inc_obs_count(PID_t p)
		{
			// Test if we need to start a new rollback point
			create_rollback_point();
			do_inc_obs_count(p);
		}

		/**
		 * Test if we must create rollback point and
		 * decrement the number of observers of slot \a p.
		 * @param p The element to update
		 */
		void dec_obs_count(PID_t p)
		{
			// Test if we need to start a new rollback point
			create_rollback_point();

			// Unlock only if it is not part of backtrackable elements
			if ((_first_rewind == END_LIST) || (p > _first_rewind))
			{
				assert(_data[p]._obs_count > 0);
				--_data[p]._obs_count;
			}
		}

		/**
		 * Create a rollback point. It saves the values to restore
		 * when we will rewind to this point. 
		 */
		void create_rollback_point();
	};

	/**
	 * @brief Iterator on a backtrackable list
	 */
	template< typename Bt_list_t >
	class Bt_list_iterator {
		friend class Bt_list< typename Bt_list_t::Value_t, Bt_list_t::_ENABLE_BACKTRACK, Bt_list_t::_SAFE_DELETE, Bt_list_t::_STATISTICS_T, typename Bt_list_t::_Allocator_t >;
	public:
		/**
		 * Copy constructor set default.
		 */
		constexpr Bt_list_iterator(const Bt_list_iterator& o_it) = default;

		/**
		 * Return the current element pointed by the iterator.
		 * @return A const reference to the element pointed
		 */
		Bt_list_iterator& operator=(const Bt_list_iterator& o_it)
		{
			assert(&_list == &o_it._list);
			_current = o_it._current;
			return *this;
		}

		/**
		 * Return the current element pointed by the iterator.
		 * @return A const reference to the element pointed
		 */
		const typename Bt_list_t::Value_t& operator*() const
		{
			assert(_current != Bt_list_t::END_LIST);
			return _list._data[_current]._e;
		}

		/**
		 * Return the current PID of the iterator.
		 * @return The PID value
		 */
		typename Bt_list_t::PID_t pid() const
		{
			return _current;
		}

		/**
		 * Move the iterator to the next element.
		 * @return A reference to the current iterator
		 */
		Bt_list_iterator& operator++()
		{
			assert(_current != Bt_list_t::END_LIST);
			_current = _list._data[_current]._next;
			return *this;
		}

		/**
		 * Tell if the iterator is at the end of the list or not.
		 * @return True if there is still element to see, false otherwise
		 */
		bool at_end() const
		{
			return (_current == Bt_list_t::END_LIST);
		}

		/**
		 * Lock the underlying node. As a result, if the underlying node
		 * must be removed, it will not be freed immediately but still remains
		 * available for the use of the ++ operator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void lock()
		{
			assert(_current != Bt_list_t::END_LIST);
			_list.inc_obs_count(_current);
		}

		/**
		 * Release a locked node and move it to free list if needed.
		 */
		void unlock()
		{
			assert(_current != Bt_list_t::END_LIST);
			_list.dec_obs_count(_current);
			if (_list._data[_current]._status == Bt_list_t::REMOVE)
				_list.free(_current);
		}

		/**
		 * Release a locked node and move it to free list if needed.
		 * Furthermore, the current iterator is moved to the next
		 * ALIVE element.
		 */
		void next_and_unlock()
		{
			assert(_current != Bt_list_t::END_LIST);
			auto p =_current;
			next_alive();
			_list.dec_obs_count(p);
			if (_list._data[p]._status == Bt_list_t::REMOVE)
				_list.free(p);
		}

		/**
		 * Refresh an iterator by going to the next alive element.
		 */
		void next_alive()
		{
			assert(_current != Bt_list_t::END_LIST);
			if (_list._data[_current]._status == Bt_list_t::ALIVE)
			{
				// Go to next element
				_current = _list._data[_current]._next;
			} else {
				// If the underlying element is not alive, we go to the next living one.
				while ( (_current != Bt_list_t::END_LIST)
						&& (_list._data[ _current ]._status != Bt_list_t::ALIVE))
				{
					assert((_list._data[_current]._status == Bt_list_t::REMOVE) || _list._data[_current]._status == Bt_list_t::BACKTRACK);
					_current = _list._data[_current]._next;
				}
			}
		}

		/**
		 * Test if two iterators are different.
		 * @param o The other iterator to compare with ourself
		 * @return True is the two iterators are different, false otherwise
		 */
		bool operator!=(const Bt_list_iterator& o) const
		{
			assert(&_list == &o._list);
			return _current != o._current;
		}

		/**
		 * Test if two iterators are equal.
		 * @param o The other iterator to compare with ourself
		 * @return True is the two iterators are equal, false otherwise
		 */
		bool operator==(const Bt_list_iterator& o) const
		{
			assert(&_list == &o._list);
			return _current == o._current;
		}

		/**
		 * Test if the iterator is still valid. The iterator is not valid if it sets
		 * to be removed.
		 * @return True is the iterator is valid, false otherwise
		 */
		bool valid() const
		{
			assert(_current != Bt_list_t::END_LIST);
			return _list._data[_current]._status == Bt_list_t::ALIVE;
		}

		/**
		 * Returns the status of the underlying slot refered by the iterator
		 * @return The status of the underlying slot
		 */
		unsigned int status() const
		{
			assert(_current != Bt_list_t::END_LIST);
			return _list._data[_current]._status;
		}

		/**
		 * Remove the element pointed by the current iterator.
		 * The iterator is no longer valid (can't be safely dereferenced).
		 */
		void remove()
		{
			assert(_current != Bt_list_t::END_LIST);
			_list.remove(_current);
		}

	private:
		Bt_list_t& _list;					///< Current list
		typename Bt_list_t::PID_t _current;	///< Current node

		/**
		 * Initialize.
		 * @param list The list to browse
		 */
		Bt_list_iterator(Bt_list_t& list)
			: _list(list), _current(list._first)
		{ }

		/**
		 * Initialize to a specific node.
		 * @param list The list to browse
		 * @param current The node to start with
		 */
		Bt_list_iterator(Bt_list_t& list, typename Bt_list_t::PID_t current)
			: _list(list), _current(current)
		{ }
	};
}

#include <bt_list.hpp>

#endif /* RUNTIME_BT_LIST_HH_ */
