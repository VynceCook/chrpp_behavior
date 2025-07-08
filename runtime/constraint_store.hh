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

#ifndef RUNTIME_CONSTRAINT_STORE_HH_
#define RUNTIME_CONSTRAINT_STORE_HH_

#include <memory>

#include <statistics.hh>
#include <chrpp.hh>
#include <bt_list.hh>

/**
 * \defgroup Constraints Constraints management
 * 
 * Constraints are a a key component (like logical variables) of a \e CHR++ program.
 * They are managed thanks to constraint stores.
 */
namespace chr {

	// Forward declaration
	template< typename T > class Constraint_store_simple_iterator;

	/**
	 * @brief Constraint store management
	 *
	 * A constraint store deals with a specific constraint type T.
	 * The store is a list of constraints (order is ensured).
	 * The main purpose is to efficiently manage the add and removal
	 * of constraints and to provide a efficient iterator on the set.
	 * \ingroup Constraints
	 */
	template< typename T, bool ENABLE_BACKTRACK = true >
	class Constraint_store_simple : public Backtrack_observer
	{
		friend class Constraint_store_simple_iterator< Constraint_store_simple< T, ENABLE_BACKTRACK > >;
	public:
		typedef T Constraint_t;
		typedef class Constraint_store_simple_iterator< Constraint_store_simple< T, ENABLE_BACKTRACK > > iterator;
		typedef typename Bt_list< T, ENABLE_BACKTRACK, false, chr::Statistics::CONSTRAINT_STORE >::PID_t PID_t;

		static constexpr bool _ENABLE_BACKTRACK = ENABLE_BACKTRACK;

		/**
		 * Initialize.
		 */
		Constraint_store_simple(std::string_view label = "")
			: Backtrack_observer(), _label(label)
		{ }

		/**
		 * Copy constructor: deleted.
		 * @param o The source Constraint_store
		 */
		Constraint_store_simple(const Constraint_store_simple& o) =delete;

		/**
		  * Virtual destructor
		  */
		virtual ~Constraint_store_simple() { }

		/**
		 * Assignment operator: deleted.
		 * @param o The source Constraint_store
		 * @return A reference to this
		 */
		Constraint_store_simple& operator=(const Constraint_store_simple& o) =delete;

		/**
		 * Returns the label of this constraint store.
		 * @return A string corresponding to the label.
		 */
		std::string_view label() const { return _label; }

		/**
		 * Check is the store is empty.
		 * @return True if the store is empty, false otherwise
		 */
		bool empty() const { return _store.empty(); }

		/**
		 * Return the number of constraints stores in this store
		 * @return The size of this store of constraints
		 */
		std::size_t size() const { return _store.size(); }

		/**
		 * Return the current constraint store backtrack depth.
		 * @return The current constraint store backtrack depth
		 */
		Depth_t depth() const { return _store.depth(); }

		/**
		 * Build and return an iterator on the first constraint
		 * of the store.
		 * @return An iterator on first element
		 */
		iterator begin();

		/**
		 * Build and return an iterator on the last constraint
		 * of the store. Usefull for the C++ for range loop!
		 * @return An iterator on first element
		 */
		iterator end();

		/**
		 * Add a constraint into the store.
		 * @param e The constraint to add
		 * @return An iterator on the new element
		 */
		iterator add(T e);

	private:
		Bt_list< T, ENABLE_BACKTRACK, false, chr::Statistics::CONSTRAINT_STORE > _store;	///< The store of constraints
		std::string _label; ///< Label of this constraint store (to print before each constraint)

		/**
		 * Check is the store has a rollback point.
		 * @return True if there is a rollback point, false otherwise
		 */
		bool has_rollback_point() const { return _store.has_rollback_point(); }

		/**
		 * Remove a constraint from the store.
		 * @param pid The pid of the constraint in the tore
		 */
		void remove(PID_t pid);

		/**
		 * Rewind to the snapshot of the list stored at \a depth.
		 * All snapshots encountered along the path will be forgotten.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		bool rewind(Depth_t previous_depth, Depth_t new_depth) override;
	};

	template< typename Constraint_store_t >
	class Constraint_store_simple_iterator {
		friend class Constraint_store_simple< typename Constraint_store_t::Constraint_t, Constraint_store_t::_ENABLE_BACKTRACK >;
	public:
		/**
		 * Return the current element pointed by the iterator.
		 * @return A const reference to the element pointed
		 */
		const typename Constraint_store_t::Constraint_t& operator*() const {
			return *_it;
		}

		/**
		 * Return the string conversion of the underlying constraint of this iterator
		 * @return The string
		 */
		std::string to_string() const {
			return std::string(_store->label()) + chr::TIW::constraint_to_string(*_it);
		}

		/**
		 * Move the iterator to the next living element.
		 * @return A reference to the current iterator
		 */
		Constraint_store_simple_iterator& operator++()
		{
			++_it;
			return *this;
		}

		/**
		 * Tell if the iterator is at the end of the constraint store or not.
		 * Remember, that the "end" corresponds to the end() marked when
		 * this iterator was created.
		 * @return True if there is still element to see, false otherwise
		 */
		bool at_end() const
		{
			return _it.at_end();
		}

		/**
		 * Lock the current underlying iterator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void lock()
		{
			if constexpr (!Constraint_store_t::_ENABLE_BACKTRACK)
			{
				_it.lock();
			} else {
				bool has_rb = _store->has_rollback_point();
				_it.lock();
				if (has_rb != _store->has_rollback_point())
				{
					assert(!has_rb);
					Backtrack::schedule(_store);
				}
			}
		}

		/**
		 * Unlock and move the iterator to the next living element.
		 */
		void next_and_unlock()
		{
			if constexpr (!Constraint_store_t::_ENABLE_BACKTRACK)
			{
				_it.next_and_unlock();
			} else {
				bool has_rb = _store->has_rollback_point();
				_it.next_and_unlock();
				if (has_rb != _store->has_rollback_point())
				{
					assert(!has_rb);
					Backtrack::schedule(_store);
				}
			}
		}

		/**
		 * Unlock the current underlying iterator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void unlock()
		{
			if constexpr (!Constraint_store_t::_ENABLE_BACKTRACK)
			{
				_it.unlock();
			} else {
				bool has_rb = _store->has_rollback_point();
				_it.unlock();
				if (has_rb != _store->has_rollback_point())
				{
					assert(!has_rb);
					Backtrack::schedule(_store);
				}
			}
		}

		/**
		 * Unlock and refresh the frozen underlying iterator.
		 * Release must be called to refresh iterators after
		 * a constraint remove because the underlying iterator
		 * is not valid anymore.
		 */
		void release()
		{
			if constexpr (!Constraint_store_t::_ENABLE_BACKTRACK)
			{
				_it.release();
			} else {
				bool has_rb = _store->has_rollback_point();
				_it.release();
				if (has_rb != _store->has_rollback_point())
				{
					assert(!has_rb);
					Backtrack::schedule(_store);
				}
			}
		}

		/**
		 * Tell if the element pointed by the iterator is alive or not.
		 * @return True if the underlying constraint is alive, false otherwise
		 */
		bool alive() const
		{
			return _it.valid();
		}

		/**
		 * Kill the current iterator.
		 * It only mark for kill, the real removing of element will
		 * be done when no iterator will point at it.
		 */
		void kill()
		{
			_store->remove(_it.pid());
		}

		/**
		 * Check if two iterators are equals.
		 * @return True if the two iterators are equals, false otherwise
		 */
		bool operator==(const Constraint_store_simple_iterator& o) const
		{
			return (_it == o._it);
		}

		/**
		 * Check if two iterators are not equals.
		 * @return True if the two iterators are not equals, false otherwise
		 */
		bool operator!=(const Constraint_store_simple_iterator& o) const
		{
			return (!(*this == o));
		}

	private:
		Constraint_store_t* _store;	///< Store of constraint
		typename Bt_list< typename Constraint_store_t::Constraint_t, Constraint_store_t::_ENABLE_BACKTRACK, false, chr::Statistics::CONSTRAINT_STORE >::iterator _it;	///< The current iterator on the list of constraints

		/**
		 * Initialize.
		 * @param store The constraint store to browse
		 * @param it The initial pos of the iterator
		 */
		Constraint_store_simple_iterator(Constraint_store_t& store, const typename Bt_list< typename Constraint_store_t::Constraint_t, Constraint_store_t::_ENABLE_BACKTRACK, false, chr::Statistics::CONSTRAINT_STORE >::iterator it) : _store(&store), _it(it) { }
	};

}

#include <constraint_store.hpp>

#endif /* RUNTIME_CONSTRAINT_STORE_HH_ */
