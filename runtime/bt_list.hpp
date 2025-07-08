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

#include <memory>
namespace chr
{
	/*
	 * Bt_list
	 */
	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::~Bt_list()
	{
		if constexpr (NEED_DESTROY)
		{
			for (Node* p = _data; p != (_data + _first_unused_pid); ++p)
			{
				if (p->_status < REMOVE)
					std::destroy_at(std::addressof(p->_e)); // Call the destructor of _e
			}
		}
		typename Node::Allocator na;
		na.deallocate(_data,_capacity);
		Statistics::dec_memory<STATISTICS_T>(sizeof(Node)*_capacity);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t > Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::clone() const
	{
		using _Alloc_traits = std::allocator_traits< typename Node::Allocator >;
		typename Node::Allocator na;

		Bt_list bl;
		bl._first = _first;
		bl._last = _last;
		bl._first_unused_pid = _first_unused_pid;
		bl._first_free = _first_free;
		bl._first_rewind = _first_rewind;
		bl._size = _size;
		bl._backtrack_depth = _backtrack_depth;
		bl._capacity = _capacity;
	
		bl._data = na.allocate(_capacity);
		assert( bl._data );
		if (_data != nullptr)
		{
			assert(_capacity > 0);
			Node* p = _data;
			Node* p_last = _data + _first_unused_pid;
			Node* p_dest = bl._data;
			for (; p != p_last; ++p, (void)++p_dest)
			{
				if (p->_status > BACKTRACK)
					std::memcpy((char*)p_dest, (char*)p, sizeof(Node));
				else
					_Alloc_traits::construct(na, p_dest, std::move(*p));
			}
		}
		return bl; // bl is implicity moved, no copy constructor exists
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	typename Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::begin()
	{
		return Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator(*this);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	typename Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::end()
	{
		return Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator(*this,END_LIST);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	void Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::reallocate_list()
	{
		if constexpr (NB_SPECIAL_SLOTS == 1)
			assert((_first_unused_pid == _capacity) || ((_first_unused_pid + 1) == _capacity));
		else 
			static_assert(true,"NB_SPECIAL_SLOTS > 1, if statement must be updated");

		using _Alloc_traits = std::allocator_traits< typename Node::Allocator >;
		typename Node::Allocator na;
		// Capacity is not enough, reallocate
		std::size_t max_capacity = _Alloc_traits::max_size(na);
		const std::size_t foreseen_capacity = (_capacity == 0)?4:2 * _capacity; // We try to double size
		const std::size_t new_capacity = (foreseen_capacity > max_capacity) ? max_capacity : foreseen_capacity;

		Node *new_data = na.allocate(new_capacity);
		assert( new_data );
		if (_data != nullptr)
		{
			assert(_capacity > 0);
			Node* p = _data;
			Node* p_last = _data + _first_unused_pid;
			Node* p_dest = new_data;
			for (; p != p_last; ++p, (void)++p_dest)
			{
				if (p->_status > BACKTRACK)
					std::memcpy((char*)p_dest, (char*)p, sizeof(Node));
				else
					_Alloc_traits::construct(na, p_dest, std::move(*p));
			}
			na.deallocate(_data,_capacity);
		}
		Statistics::inc_memory<STATISTICS_T>(sizeof(Node)*(new_capacity - _capacity));
		_data = new_data;
		_capacity = new_capacity;
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	typename Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::insert(T e)
	{
		// Test if we need to start a new rollback point
		create_rollback_point();

		if ((_first_unused_pid == _capacity) && (_first_free == END_LIST))
			reallocate_list();

		// Increases the number of elements
		++_size;

		PID_t p = _first_unused_pid;
		if (_first_free != END_LIST)
		{
			p = _first_free;
			_first_free = _data[_first_free]._prev;
			if constexpr (SAFE_DELETE)
			{
				// We manage number of free only if there is no previous backtrack state
				if (_first_rewind == END_LIST)
					--extra._nb_free;
			}
		} else {
			// Set the next free pid usable
			++_first_unused_pid;
		}
		Allocator_t a;
		std::allocator_traits< Allocator_t >::construct(a, &_data[p]._e, std::move(e) );
		_data[p]._prev = END_LIST;
		_data[p]._next = _first;
		_data[p]._status = ALIVE;
		_data[p]._obs_count = 0;
		if (_first != END_LIST)
			_data[_first]._prev = p;
		else
			_last = p;
		_first = p;
		return Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator(*this, _first);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	void Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::free(Bt_list::PID_t p)
	{
		assert(p != END_LIST);
		assert(_data[p]._status == REMOVE);

		if (_data[p]._obs_count == 0)
		{
			// Add node to list of free nodes
			PID_t p_next = _data[p]._next;
			#ifndef NDEBUG
				// Normally useless only needed to make things cleaner
				_data[p]._next = END_LIST;
			#endif
			if constexpr (SAFE_DELETE)
			{
				// We manage number of free only if there is no previous backtrack state
				if (_first_rewind == END_LIST)
					++extra._nb_free;
			}
			_data[p]._prev = _first_free;
			_first_free = p;
			p = p_next;
			while (p != END_LIST)
			{
				if ((_data[p]._status != REMOVE) || (_data[p]._obs_count > 1))
				{
					// If status != REMOVE and obs_count is 0, don't decrement obs_count
					if (_data[p]._obs_count > 0) --_data[ p ]._obs_count;
					break;
				}
				--_data[p]._obs_count;
				if constexpr (SAFE_DELETE)
				{
					// We manage number of free only if there is no previous backtrack state
					if (_first_rewind == END_LIST)
						++extra._nb_free;
				}
				// Add node to list of free nodes
				_data[p]._prev = _first_free;
				_first_free = p;
				p = _data[p]._next;
				#ifndef NDEBUG
					// Normally useless only needed to make things cleaner
					_data[_first_free]._next = END_LIST;
				#endif
			}
		}
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	typename Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::remove(const Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator& it)
	{
		assert(it._current != END_LIST);
		auto n_it = it;
		++n_it;
		remove(it._current);
		return n_it;
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	void Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::remove(Bt_list::PID_t p)
	{
		assert(p != END_LIST);
		assert(_first != END_LIST);

		// Test if we need to start a new rollback point
		create_rollback_point();

		// Decreases the number of elements
		assert(_size > 0);
		--_size;

		// Unlink node from current list
		if (p == _first) // Unlink first node
		{
			if (_data[p]._next == END_LIST)
			{
				_first = END_LIST;
				_last = END_LIST;
			} else {
				_first = (_data[_first])._next;
				_data[_first]._prev = END_LIST;
				// Lock next node
				do_inc_obs_count(_data[p]._next);
			}
		} else if (p == _last) // Unlink last node
		{
			assert(_data[p]._next == END_LIST);
			_last = _data[p]._prev;
			_data[ _last ]._next = END_LIST;
		} else {
			_data[ _data[p]._prev ]._next = _data[p]._next;
			if (_data[p]._next != END_LIST)
			{
				_data[ _data[p]._next ]._prev = _data[p]._prev;
				// Lock next node
				do_inc_obs_count(_data[p]._next);
			}
		}

		if ((_first_rewind == END_LIST) || (p > _first_rewind))
		{
			_data[p]._status = REMOVE;
			// Remove and free element, we won't need to backtrack this one at
			// its PID is after the first rewind one.
			// Call the destructor of _e
			if constexpr (NEED_DESTROY)
				std::destroy_at(std::addressof(_data[p]._e));
			free(p);
		} else {
			// Link node to the list of rewind ones (we may backtrack this later)
			assert(p < _first_rewind);
			_data[p]._status = BACKTRACK;
			// Update before_pid (_next field) if the _next is after the last backtrack point
            // and add a closure node to link the nodes
			if ((_first_rewind != END_LIST) && (_data[p]._next > _first_rewind))
			{
                if ((_first_unused_pid == _capacity) && (_first_free == END_LIST))
                    reallocate_list();

                // Add a closure to link relink with previous
                // Get a new slot
                PID_t p_new = _first_unused_pid;
                if (_first_free != END_LIST)
                {
                    p_new = _first_free;
                    _first_free = _data[_first_free]._prev;
                } else {
                    // Set the next free pid usable
                    ++_first_unused_pid;
                }
                // Link node to the list of rewind ones (we may backtrack this later)
                _data[p_new]._status = BACKTRACK_CLOSURE;
                _data[p_new]._closure_prev = _data[p]._prev;
                _data[p_new]._closure_next = p;
                _data[p_new]._prev = _data[_first_rewind]._first_removed_to_restore;
                _data[_first_rewind]._first_removed_to_restore = p_new;

                // Update before_pid (_next field) if the _next is after the last backtrack point
				Bt_list::PID_t p_next = _data[p]._next;
				while ( (p_next != END_LIST) && (p_next > _first_rewind))
				{
					assert(_data[p_next]._status == ALIVE);
					p_next = _data[p_next]._next;
				}
				_data[p]._next = p_next;
            }
            _data[p]._prev = _data[_first_rewind]._first_removed_to_restore;
            _data[_first_rewind]._first_removed_to_restore = p;
		}
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	typename Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::insert_before(const Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator& before_it, T e)
	{
		assert(before_it._current != END_LIST);
		insert_before(before_it._current, std::move(e));
		return Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator(*this, _data[before_it._current]._prev);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	void Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::insert_before(Bt_list::PID_t before_pid, T e)
	{
		// Test if we need to start a new rollback point
		create_rollback_point();

		// Add closure slot if needed
		// A closure slot is required if the before_pid is before the last backtrack point
		if ((_first_rewind != END_LIST) && (before_pid < _first_rewind))
		{
			if ((_first_unused_pid == _capacity) && (_first_free == END_LIST))
				reallocate_list();

			// Get a new slot
			PID_t p = _first_unused_pid;
			if (_first_free != END_LIST)
			{
				p = _first_free;
				_first_free = _data[_first_free]._prev;
			} else {
				// Set the next free pid usable
				++_first_unused_pid;
			}
			// Link node to the list of rewind ones (we may backtrack this later)
			_data[p]._status = BACKTRACK_CLOSURE;
			_data[p]._closure_prev = _data[ before_pid ]._prev ;
			_data[p]._closure_next = before_pid;
			// Update closure_next if the _closure_next is after the last backtrack point
			if ((_first_rewind != END_LIST) && (_data[p]._closure_next > _first_rewind))
			{
				Bt_list::PID_t p_next = _data[p]._closure_next;
				while ( (p_next != END_LIST) && (p_next > _first_rewind))
				{
					assert(_data[p_next]._status == ALIVE);
					p_next = _data[p_next]._next;
				}
				_data[p]._closure_next = p_next;
			}

			_data[p]._prev = _data[_first_rewind]._first_removed_to_restore;
			_data[_first_rewind]._first_removed_to_restore = p;
		}

		// Add the new slot
		// Increases the number of elements
		++_size;

		if ((_first_unused_pid == _capacity) && (_first_free == END_LIST))
			reallocate_list();

		PID_t p = _first_unused_pid;
		if (_first_free != END_LIST)
		{
			p = _first_free;
			_first_free = _data[_first_free]._prev;
			if constexpr (SAFE_DELETE)
			{
				// We manage number of free only if there is no previous backtrack state
				if (_first_rewind == END_LIST)
					--extra._nb_free;
			}
		} else {
			// Set the next free pid usable
			++_first_unused_pid;
		}
		_Allocator_t a;
		std::allocator_traits< _Allocator_t >::construct(a, &_data[p]._e, std::move(e) );
		_data[p]._status = ALIVE;
		_data[p]._obs_count = 0;
		_data[p]._next = before_pid;

		if (before_pid == END_LIST)
		{
			if (_first == END_LIST) // Empty list
			{
				_first = p;
				_data[p]._prev = END_LIST;
			} else {
				_data[p]._prev = _last;
				_data[_last]._next = p;
			}
			_last = p;
		} else if (before_pid == _first)
		{
			_data[p]._prev = END_LIST;
			_data[_first]._prev = p;
			_first = p;
		} else {
			_data[p]._prev = _data[before_pid]._prev;
			_data[before_pid]._prev = p;
			_data[ _data[p]._prev ]._next = p;
		}
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	typename Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::replace(const Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::iterator& it, T e)
	{
		assert(it._current != END_LIST);
		auto n_it = it;
		++n_it;
		replace(it._current, std::move(e));
		return n_it;
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	void Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::replace(Bt_list::PID_t p, T e)
	{
		// If we have no backtrack (i.e. ENABLE_BACKTRACK is false or p is
		// after the last rollback point), we replace directly by the new element
		if constexpr (!ENABLE_BACKTRACK)
		{
			// Call the destructor of _e
			if constexpr (NEED_DESTROY)
				std::destroy_at(std::addressof(_data[p]._e));
			_Allocator_t a;
			std::allocator_traits< _Allocator_t >::construct(a, &_data[p]._e, std::move(e) );
			return;
		} else {
			// Test if we need to start a new rollback point
			create_rollback_point();
			// No backtrack point or p is after last backtrack point (no need to rewind it)
			if ((_first_rewind == END_LIST) || (p > _first_rewind))
			{
				// Call the destructor of _e
				if constexpr (NEED_DESTROY)
					std::destroy_at(std::addressof(_data[p]._e));
				_Allocator_t a;
				std::allocator_traits< _Allocator_t >::construct(a, &_data[p]._e, std::move(e) );
				return;
			}
		}
		insert_before(p, std::move(e));
		remove(p);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	void Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::create_rollback_point()
	{
		if constexpr (ENABLE_BACKTRACK)
		{
			assert(_backtrack_depth <= Backtrack::depth());
			// Check is we need to create a rollback point
			if (_backtrack_depth == Backtrack::depth()) return;

			if ((_first_unused_pid + NB_SPECIAL_SLOTS) >= _capacity)
				reallocate_list();
			assert((_first_unused_pid + NB_SPECIAL_SLOTS) < _capacity);

			// Use slot at _first_unused_pid and link it
			// to other element to rewind.
			_data[_first_unused_pid]._status = SPECIAL;
			_data[_first_unused_pid]._obs_count = 0;
			_data[_first_unused_pid]._next_rewind = _first_rewind;
			_data[_first_unused_pid]._first_removed_to_restore = END_LIST; // Start of removed elements to restore on rewind
			_data[_first_unused_pid]._rem_backtrack_depth = _backtrack_depth;
			_first_rewind = _first_unused_pid;
			++_first_unused_pid;

			// We use the slot close to previous one to store other data
			_data[_first_unused_pid]._status = SPECIAL;
			_data[_first_unused_pid]._obs_count = 0;
			_data[_first_unused_pid]._rem_first_free = _first_free; // Backup first free of the list
			_data[_first_unused_pid]._rem_first = _first;
			_data[_first_unused_pid]._rem_size = _size;
			_first_free = END_LIST;
			_backtrack_depth = Backtrack::depth();
			++_first_unused_pid;
		}
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	template< bool EB >
	std::enable_if_t<EB, bool> Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::rewind(Depth_t, Depth_t new_depth)
	{
		assert(ENABLE_BACKTRACK); // Can't occur because of SFINAE
		if (_backtrack_depth <= new_depth) return true;

		// Search for the relevant backtrack depth
		assert(_first_rewind != END_LIST); // If callback called, we must have previous elements to restore
		do
		{
			// Destroy element not relevant anymore
			if constexpr (NEED_DESTROY)
			{
				for (Node* p = _data + _first_rewind + 2; p != (_data + _first_unused_pid); ++p)
				{
					if (p->_status < REMOVE)
						std::destroy_at(std::addressof(p->_e)); // Call the destructor of _e
				}
			}

			// Restore removed and closure elements
			PID_t p = _data[ _first_rewind ]._first_removed_to_restore;
			while (p != END_LIST)
			{
				PID_t bak_p_next = _data[p]._prev;
				assert((_data[p]._status == BACKTRACK) || (_data[p]._status == BACKTRACK_CLOSURE));

				if (_data[p]._status == BACKTRACK_CLOSURE)
				{
					// A closure node has to be inserted after the backtrack point
					assert(p > _first_rewind);
					PID_t p_next = _data[p]._closure_next; // p_next is the next alive element of this closure (inserted after first_rewind)
					PID_t p_prev = _data[p]._closure_prev; // p_prev is the previous alive element of this closure (inserted after first_rewind)
					if (p_prev == END_LIST)
						_first = p;
					else
						_data[ p_prev ]._next = p_next;
					if (p_next == END_LIST)
						_last = p;
					else	
						_data[ p_next ]._prev = p_prev;
					// The closure slot will be freed together with the other elements after the _first_rewind slot
				} else {
					// We restore a remove node
					assert(p < _first_rewind);
					_data[p]._status = ALIVE;

					PID_t p_next = _data[p]._next; // p_next is the next alive element of this removed one
					if (p_next == END_LIST)
					{
						if (_first == END_LIST) // Empty list
						{
							_data[p]._prev = END_LIST;
							_data[p]._next = END_LIST;
							_first = p;
							_last = p;
						} else {
                            PID_t p_prev = _last; // p_prev is the previous alive element of this removed one
                            if (p_prev > _first_rewind)
                            {
                                // If the p_prev element is after the first_rewind, we go further.
                                while ( (p_prev != END_LIST) && (p_prev > _first_rewind))
                                {
                                    assert(_data[p_prev]._status == ALIVE);
                                    p_prev = _data[p_prev]._prev;
                                }
                            }
                            if (p_prev == END_LIST)
                            {
                                _data[p]._prev = END_LIST;
                                _data[p]._next = END_LIST;
                                _first = p;
                                _last = p;
                            } else {
                                _data[p]._prev = p_prev;
                                _data[p_prev]._next = p;
                                _last = p;
                            }
						}
					} else {
						assert(_data[p_next]._status == ALIVE);
						_data[p]._prev = _data[p_next]._prev;
						_data[p_next]._prev = p;
						if (_data[p]._prev == END_LIST)
							_first = p;
						else
							_data[ _data[p]._prev ]._next = p;
					}
				}
				p = bak_p_next;
			}

			// Restore list members
			_first = _data[ _first_rewind + 1 ]._rem_first;
			if (_first != END_LIST)
				_data[_first]._prev = END_LIST;
			else
				_last = END_LIST; // If list not empty, then _last MUST be an ALIVE node (because we always insert in front)
			if (_last != END_LIST)
				_data[_last]._next = END_LIST;
			_size = _data[ _first_rewind + 1 ]._rem_size;
			_first_free = _data[ _first_rewind + 1 ]._rem_first_free;
			_backtrack_depth = _data[ _first_rewind ]._rem_backtrack_depth;
			_first_unused_pid = _first_rewind;
			_first_rewind = _data[ _first_rewind ]._next_rewind;

		} while ((_first_rewind != END_LIST) && (_backtrack_depth > new_depth));

		return (_first_rewind != END_LIST);
	}

	template< typename T, bool ENABLE_BACKTRACK, bool SAFE_DELETE, unsigned int STATISTICS_T, typename Allocator_t >
	std::string Bt_list< T, ENABLE_BACKTRACK, SAFE_DELETE, STATISTICS_T, Allocator_t >::to_string(unsigned int debug) const
	{
		std::string str;
		if (debug > 0)
		{
			str += "<s=" + std::to_string(_size);
			if (debug == 2)
				str += ",c=" + std::to_string(_capacity);
			str += ",f=" + ((_first == END_LIST)?"END":std::to_string(_first)) + ",l=" + ((_last == END_LIST)?"END":std::to_string(_last)) + ",ff=" + ((_first_free == END_LIST)?"END":std::to_string(_first_free)) + ",bd=" + std::to_string(_backtrack_depth) + ",frw=" + ((_first_rewind == END_LIST)?"END":std::to_string(_first_rewind)) + ">{";
			for (unsigned int i=0; i < _first_unused_pid; ++i)
			{
				if (i > 0) str += ", ";
				str += "[" + std::to_string(i) + "](";
				if (_data[i]._status == ALIVE)
					str += chr::TIW::to_string(_data[i]._e);
				else if (_data[i]._status == BACKTRACK)
					str += "_!" + chr::TIW::to_string(_data[i]._e) + "!_";
				else if (_data[i]._status == BACKTRACK_CLOSURE)
					str += "_!!_";
				else if (_data[i]._status == REMOVE)
					str += "_--_";
				else
					str += "_##_";
				if (_data[i]._status == SPECIAL)
				{
					str += ",bd=" + std::to_string(_data[i]._rem_backtrack_depth) + ",nr=" + ((_data[i]._next_rewind == END_LIST)?"END":std::to_string(_data[i]._next_rewind)) + ",fr=" + ((_data[i]._first_removed_to_restore == END_LIST)?"END":std::to_string(_data[i]._first_removed_to_restore));
					++i;
					str += ",ff=" + ((_data[i]._rem_first_free == END_LIST)?"END":std::to_string(_data[i]._rem_first_free)) + ",f=" + ((_data[i]._rem_first== END_LIST)?"END":std::to_string(_data[i]._rem_first)) + ",s=" + std::to_string(_data[i]._rem_size);
				} else if (_data[i]._status == BACKTRACK)
				{
					str += ",obs=" + std::to_string(_data[i]._obs_count) + ",before=" + ((_data[i]._next == END_LIST)?"END":std::to_string(_data[i]._next)) + "," + "-->" + ((_data[i]._prev == END_LIST)?"END":std::to_string(_data[i]._prev));
				} else if (_data[i]._status == BACKTRACK_CLOSURE)
				{
					str += ",obs=" + std::to_string(_data[i]._obs_count) + "," + ((_data[i]._closure_prev == END_LIST)?"END":std::to_string(_data[i]._closure_prev)) + "/-\\" + ((_data[i]._closure_next == END_LIST)?"END":std::to_string(_data[i]._closure_next)) + "," + "-->" + ((_data[i]._prev == END_LIST)?"END":std::to_string(_data[i]._prev));
				} else
					str += ",obs=" + std::to_string(_data[i]._obs_count) + "," + ((_data[i]._prev == END_LIST)?"END":std::to_string(_data[i]._prev)) + "<--|-->" + ((_data[i]._next == END_LIST)?"END":std::to_string(_data[i]._next));
				str += ")";
			}
			str += "}";
		} else {
			str += "{";
			PID_t  p = _first;
			while (p != END_LIST)
			{
				str += chr::TIW::to_string(_data[p]._e);
				p = _data[p]._next;
				if (p != END_LIST) str += ", ";
			}
			str += "}";
		}
		return str;
	}
}
