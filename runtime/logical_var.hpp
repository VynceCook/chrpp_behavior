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

#include "backtrack.hh"
#include "utils.hpp"
#include <chrpp.hh>

namespace chr
{
	template< typename T >
	Logical_var_imp<T>::~Logical_var_imp()
	{
		// Not yet deallocate, but still relevant for statistics
		Statistics::dec_memory< Statistics::VARIABLE >(sizeof *this);
	}

	template< typename T >
	const void* Logical_var_imp<T>::address() const
	{
		auto& this_ra = ra();
		return reinterpret_cast<void const *>(&this_ra);
	}

	template< typename T >
	std::string Logical_var_imp<T>::to_string() const
	{
		auto& this_ra = ra();
		if (this_ra._status != Status::NOT_GROUND)
			return TIW::to_string(this_ra._e);
		else
		{
			std::ostringstream address;
			address << reinterpret_cast<void const *>(&this_ra);
			std::string str_address = std::string("?") + address.str();
			return str_address;
		}
	}

	template< typename T >
	void Logical_var_imp<T>::ra_schedule(const chr::Shared_x_obj< Logical_var_imp_observer_index >& callback)
	{
		create_rollback_point();
		_wake_up_index_updates.insert( callback );
	}

	template< typename T >
	void Logical_var_imp<T>::ra_schedule(chr::Shared_x_obj< Logical_var_imp_observer_index >&& callback)
	{
		create_rollback_point();
		_wake_up_index_updates.insert( std::move(callback) );
	}


	template< typename T >
	void Logical_var_imp<T>::ra_schedule(const chr::Shared_x_obj< Logical_var_imp_observer_constraint >& callback)
	{
		create_rollback_point();
		_wake_up_constraints.insert( callback );
	}

	template< typename T >
	void Logical_var_imp<T>::ra_schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >&& callback)
	{
		create_rollback_point();
		_wake_up_constraints.insert( std::move(callback) );
	}

	template< typename T >
	void Logical_var_imp<T>::wake_up_index_update_callbacks(const void* old_key)
	{
		create_rollback_point();
		auto status = this->status();

		auto it = _wake_up_index_updates.begin();
		auto it_end = _wake_up_index_updates.end();
		while( it != it_end )
		{
			if ( const_cast< Logical_var_imp_observer_index* >((*it).get())->update_index(old_key) && (status != Status::GROUND) )
				++it;
			else
				it = _wake_up_index_updates.remove(it);
		}
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::wake_up_constraint_callbacks()
	{
		auto it = _wake_up_constraints.begin();
		auto it_end = _wake_up_constraints.end();
		while( it != it_end )
		{
			it.lock();
			switch ( const_cast< chr::Logical_var_imp_observer_constraint* >((*it).get())->run() )
			{
				case 0:
					if (it.valid()) {
						it.unlock();
						it = _wake_up_constraints.remove(it);
					} else
						it.next_and_unlock();
					break;
				case 1:
					it.next_and_unlock();
					break;
				case 2:
					// A failure has been raised in callback
					it.unlock();
					return chr::ES_CHR::FAILURE;
				default:
					assert(false);
					break;
			}
		}
		return chr::ES_CHR::SUCCESS;
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::wake_up_constraint_callbacks(Logical_var_imp<T>& o_ra)
	{
		auto it = _wake_up_constraints.begin();
		auto it_end = _wake_up_constraints.end();
		while( it != it_end )
		{
			it.lock();
			switch ( const_cast< chr::Logical_var_imp_observer_constraint* >((*it).get())->run() )
			{
				case 0:
					if (it.valid()) {
						it.unlock();
						it = _wake_up_constraints.remove(it);
					} else
						it.next_and_unlock();
					break;
				case 1:
					it.next_and_unlock();
					break;
				case 2:
					// A failure has been raised in callback
					it.unlock();
					return chr::ES_CHR::FAILURE;
				default:
					assert(false);
					break;
			}
		}
		auto it_o_ra = o_ra._wake_up_constraints.begin();
		auto it_o_ra_end = o_ra._wake_up_constraints.end();
		while( it_o_ra != it_o_ra_end )
		{
			it_o_ra.lock();
			switch ( const_cast< chr::Logical_var_imp_observer_constraint* >((*it_o_ra).get())->run() )
			{
				case 0:
					if (it_o_ra.valid()) {
						it_o_ra.unlock();
						it_o_ra = o_ra._wake_up_constraints.remove(it_o_ra);
					} else
						it_o_ra.next_and_unlock();
					break;
				case 1:
					if (it_o_ra.valid()) {
						it_o_ra.unlock();
						ra_schedule( *it_o_ra );
						it_o_ra = o_ra._wake_up_constraints.remove(it_o_ra);
					} else
						it_o_ra.next_and_unlock();
					break;
				case 2:
					// A failure has been raised in callback
					it_o_ra.unlock();
					return chr::ES_CHR::FAILURE;
				default:
					assert(false);
					break;
			}
		}
		return chr::ES_CHR::SUCCESS;
	}

	template< typename T >
	const Logical_var_imp< T >& Logical_var_imp<T>::ra() const
	{
		assert( _backtrack_depth <= Backtrack::depth() ); // Ensure that variable exists in this backtrack depth
		const Logical_var_imp< T >* ra = this;
		while (ra->_ancestor.get())
			ra = ra->_ancestor.get();
		return *ra;
	}

	template< typename T >
	Logical_var_imp< T >& Logical_var_imp<T>::ra()
	{
		assert( _backtrack_depth <= Backtrack::depth() ); // Ensure that variable exists in this backtrack depth
		Logical_var_imp< T >* ra = this;
		while (ra->_ancestor.get())
			ra = ra->_ancestor.get();
		return *ra;
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::operator%=(const T& value)
	{
		auto& this_ra = ra();
		Shared_obj< Logical_var_imp<T> > var_lock(&this_ra); // To prevent the current var to be destroy during callbacks wake up
		if (this_ra._status == Status::GROUND)
			return (this_ra._e == value)?chr::success():chr::failure();
		assert(this_ra._status == Status::NOT_GROUND);

		// Check if we need to start a new snapshot point
		this_ra.create_rollback_point();

		// Lazy update of ancestor
		if ((&this_ra != this) && (&this_ra != _ancestor.get()))
		{
			// Check if we need to start a new snapshot point
			create_rollback_point();
			_ancestor = const_cast< Logical_var_imp< T >* >(&this_ra); /* special operator=() overload which assumes that right part is a *naked* Shared_obj */
		}

		// Copy value
		this_ra._status = Status::GROUND;
		this_ra._e = value;

		// Wake up whole queue
		this_ra.wake_up_index_update_callbacks( reinterpret_cast<void const *>(&this_ra) );
		assert(this_ra._wake_up_index_updates.empty());
		return this_ra.wake_up_constraint_callbacks();
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::operator%=(Logical_var_imp< T >& o)
	{
		auto& this_ra = ra();
		auto& o_ra = o.ra();
		Shared_obj< Logical_var_imp<T> > this_var_lock(&this_ra); // To prevent the current var to be destroy during callbacks wake up
		Shared_obj< Logical_var_imp<T> > o_var_lock(&o_ra); // To prevent the o var to be destroy during callbacks wake up

		if (&this_ra == &o_ra) return chr::success();
		// A grounded variable cannot be unified with a mutable variable (use grounded value instead)
		if ( ((this_ra._status == Status::GROUND) && (o_ra._status == Status::MUTABLE)) ||
			 ((this_ra._status == Status::MUTABLE) && (o_ra._status == Status::GROUND)) ) return chr::failure();
		// If the two variables are grounded, the two values must be equal
		if ((this_ra._status == Status::GROUND) && (o_ra._status == Status::GROUND))
		   return (this_ra._e == o_ra._e)?chr::success():chr::failure();

		// Check if we need to start some new snapshot points
		this_ra.create_rollback_point();
		o_ra.create_rollback_point();

		// Lazy update of ancestor
		if ((&this_ra != this) && (&this_ra != _ancestor.get()))
		{
			// Check if we need to start a new snapshot point
			create_rollback_point();
			_ancestor = const_cast< Logical_var_imp< T >* >(&this_ra); /* special operator=() overload which assumes that right part is a *naked* Shared_obj */
		}
		// Lazy update of o ancestor
		if ((&o_ra != &o) && (&o_ra != o._ancestor.get()))
		{
			// Check if we need to start a new snapshot point
			o.create_rollback_point();
			o._ancestor = const_cast< Logical_var_imp< T >* >(&o_ra); /* special operator=() overload which assumes that right part is a *naked* Shared_obj */
		}

		assert(!((this_ra._status == Status::GROUND) && (o_ra._status == Status::GROUND)));
		assert(!((this_ra._status == Status::GROUND) && (o_ra._status == Status::MUTABLE)));
		assert(!((this_ra._status == Status::MUTABLE) && (o_ra._status == Status::GROUND)));
		// If this_ra is mutable or ground, then this_ra MUST be the root ancestor.
		// Be carefull, %= operator is not commutative with mutable variables.
		if ((this_ra._status != Status::NOT_GROUND)
			|| ((o_ra._status == Status::NOT_GROUND) && (o_ra._equiv_class_size <= this_ra._equiv_class_size)))
		{
			// --- Root ancestor is this_ra
			o_ra._ancestor = &this_ra; /* special operator=() overload which assumes that right part is a *naked* Shared_obj */
			this_ra._equiv_class_size += o_ra._equiv_class_size;

			// Wake up
			o_ra.wake_up_index_update_callbacks( reinterpret_cast<void const *>(&o_ra) );
			// Merge o_ra index update queues to this
			auto it_idxup = o_ra._wake_up_index_updates.begin();
			auto it_idxup_end = o_ra._wake_up_index_updates.end();
			assert((it_idxup == it_idxup_end) || (o_ra._status != Status::GROUND)); // If no callback then we have a Ground variable
			while (it_idxup != it_idxup_end)
			{
				this_ra.ra_schedule( *it_idxup );
				it_idxup = o_ra._wake_up_index_updates.remove( it_idxup );
			}

			return this_ra.wake_up_constraint_callbacks(o_ra);
		}
		else
		{
			assert((o_ra._status != Status::NOT_GROUND) || ((this_ra._status == Status::NOT_GROUND) && (o_ra._equiv_class_size > this_ra._equiv_class_size)));

			// --- Root ancestor is o_ra
			this_ra._ancestor = &o_ra; /* special operator=() overload which assumes that right part is a *naked* Shared_obj */
			o_ra._equiv_class_size += this_ra._equiv_class_size;

			// Wake up
			this_ra.wake_up_index_update_callbacks(  reinterpret_cast<const void*>(&this_ra) );
			// Merge o_ra index update queues to this
			auto it_idxup = this_ra._wake_up_index_updates.begin();
			auto it_idxup_end = this_ra._wake_up_index_updates.end();
			assert((it_idxup == it_idxup_end) || (this_ra._status != Status::GROUND)); // If no callback then we have a Ground variable
			while (it_idxup != it_idxup_end)
			{
				o_ra.ra_schedule( *it_idxup );
				it_idxup = this_ra._wake_up_index_updates.remove( it_idxup );
			}

			return o_ra.wake_up_constraint_callbacks(this_ra);
		}
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::update_mutable()
	{
		auto& this_ra = ra();
		assert((this_ra._status == Status::NOT_GROUND) || (this_ra._status == Status::MUTABLE));
		// Wake up
		create_rollback_point();
		return this_ra.wake_up_constraint_callbacks();
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::update_mutable(const T& value)
	{
		auto& this_ra = ra();
		assert((this_ra._status == Status::NOT_GROUND) || (this_ra._status == Status::MUTABLE));

		// Check if we need to start a new snapshot point
		this_ra.create_rollback_point();

		// Lazy update of ancestor
		if ((&this_ra != this) && (&this_ra != _ancestor.get()))
			_ancestor = const_cast< Logical_var_imp< T >* >(&this_ra); /* special operator=() overload which assumes that right part is a *naked* Shared_obj */

		this_ra._status = Status::MUTABLE;
		this_ra._e = value;
		// Wake up
		return this_ra.wake_up_constraint_callbacks();
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::update_mutable(std::function< void (T&) > f)
	{
		auto& this_ra = ra();
		assert((this_ra._status == Status::NOT_GROUND) || (this_ra._status == Status::MUTABLE));

		// Check if we need to start a new snapshot point
		this_ra.create_rollback_point();

		// Lazy update of ancestor
		if ((&this_ra != this) && (&this_ra != _ancestor.get()))
			_ancestor = const_cast< Logical_var_imp< T >* >(&this_ra); /* special operator=() overload which assumes that right part is a *naked* Shared_obj */

		this_ra._status = Status::MUTABLE;
		f(this_ra._e);
		// Wake up
		return this_ra.wake_up_constraint_callbacks();
	}

	template< typename T >
	chr::ES_CHR Logical_var_imp<T>::update_mutable(const bool& flag, std::function< void (T&) > f)
	{
		auto& this_ra = ra();
		assert((this_ra._status == Status::NOT_GROUND) || (this_ra._status == Status::MUTABLE));

		// Check if we need to start a new snapshot point
		this_ra.create_rollback_point();

		// Lazy update of ancestor
		if ((&this_ra != this) && (&this_ra != _ancestor.get()))
			_ancestor = const_cast< Logical_var_imp< T >* >(&this_ra); /* special operator=() overload which assumes that right part is a *naked* Shared_obj */

		this_ra._status = Status::MUTABLE;
		f(this_ra._e);
		if (flag)
			// Wake up
			return this_ra.wake_up_constraint_callbacks();
		else
			return chr::ES_CHR::SUCCESS;
	}

	template< typename T >
	Logical_var_imp<T>::Snapshot_t::Snapshot_t(Logical_var_imp< T >& v)
		: _e(v._e), _status(v._status),
		  _backtrack_depth(v._backtrack_depth),
		  _ancestor(v._ancestor), _equiv_class_size(v._equiv_class_size),
		  _previous_snapshot( std::move(v._previous_snapshot) )
	{
		Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
	}

	template< typename T >
	Logical_var_imp<T>::Snapshot_t::Snapshot_t(Logical_var_imp< T >& v, bool)
		: _status(v._status),
		  _backtrack_depth(v._backtrack_depth),
		  _ancestor(v._ancestor), _equiv_class_size(v._equiv_class_size),
		  _previous_snapshot( std::move(v._previous_snapshot) )
	{
		Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
	}

	template< typename T >
	void Logical_var_imp<T>::create_rollback_point()
	{
		assert(_backtrack_depth <= Backtrack::depth());
		// Test if we need to create a snapshot point
		if (_backtrack_depth == Backtrack::depth()) return;

		// Test if we need to registrer for backtrack
		if (!_backtrack_scheduled)
		{
			_backtrack_scheduled = true;
			Backtrack::schedule(this);/* special Weak_obj(T* o) which assumes that o is a valid Shared_obj */
		}
		// If the object manages itself the backtrack management, we call the secondary Snapshot_t constructor
		if constexpr(chr::Backtrack_management<T>::self_managed)
			_previous_snapshot = std::move( std::unique_ptr< Snapshot_t >( new Snapshot_t(*this,true) ) );
		else
			_previous_snapshot = std::move( std::unique_ptr< Snapshot_t >( new Snapshot_t(*this) ) );
		_backtrack_depth = Backtrack::depth();
	}

	template< typename T >
	bool Logical_var_imp<T>::rewind(Depth_t old_depth, Depth_t new_depth)
	{
		if (_backtrack_depth <= new_depth) return true;

		if (_wake_up_index_updates.has_rollback_point())
			_wake_up_index_updates.rewind(old_depth,new_depth);
		if (_wake_up_constraints.has_rollback_point())
			_wake_up_constraints.rewind(old_depth,new_depth);
		_backtrack_scheduled = _wake_up_index_updates.has_rollback_point() || _wake_up_constraints.has_rollback_point();
		_backtrack_depth = std::max(_wake_up_index_updates._backtrack_depth, _wake_up_constraints._backtrack_depth);

		if constexpr(chr::Backtrack_management<T>::self_managed)
			_e.rewind(old_depth,new_depth);

		if (static_cast<bool>(_previous_snapshot))
		{
			while (_previous_snapshot->_previous_snapshot && _previous_snapshot->_backtrack_depth > new_depth)
				_previous_snapshot = std::move(_previous_snapshot->_previous_snapshot);

			assert(static_cast<bool>(_previous_snapshot));
			// If the object manages itself the backtrack management, we call its rewind member function
			// otherwise, we restore the previous copy of _e
			if constexpr(!chr::Backtrack_management<T>::self_managed)
				_e = _previous_snapshot->_e;
			_status = _previous_snapshot->_status;
			_backtrack_depth = std::max(_backtrack_depth, _previous_snapshot->_backtrack_depth);
			_ancestor = _previous_snapshot->_ancestor;
			_previous_snapshot->_ancestor = nullptr;
			_equiv_class_size = _previous_snapshot->_equiv_class_size;
			_previous_snapshot = std::move(_previous_snapshot->_previous_snapshot);

			_backtrack_scheduled = _backtrack_scheduled || static_cast<bool>(_previous_snapshot);
		}
		return _backtrack_scheduled;
	}

	template < typename T >
	std::ostream & operator<<(std::ostream & out, const Logical_var_mutable< T >& var)
	{
		return out << var.to_string();
	}

	template < typename T >
	std::ostream & operator<<(std::ostream & out, const Logical_var< T >& var)
	{
		return out << var.to_string();
	}

	/**
	 * Class wrapper to schedule callbacks. This class can be specialized
	 * by user to adapt a self made data structure to schedule differently.
	 * For example, it can be used to create containers.
	 * \ingroup Logical_variables
	 */
	template< typename T >
    struct Schedule
    {
        /**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a variable.
         * @param variable The logical mutable variable to schedule
         * @param ccb The constraint callback to add
         */
		static void schedule(chr::Logical_var_mutable< T >& variable, Constraint_callback& ccb)
		{
			variable.schedule(ccb);
        }
        /**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a variable.
         * @param variable The logical variable to schedule
         * @param ccb The constraint callback to add
         */
		static void schedule(chr::Logical_var< T >& variable, Constraint_callback& ccb)
		{
			variable.schedule(ccb);
		}
	};

	/**
     * Specialisation of the Schedule templated class to schedule the two elements of a Logical_pair
	 * \ingroup Logical_variables
	 */
    template< typename U, typename V >
    struct Schedule< Logical_pair< U, V > >
	{
		/**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a logical_pair and its two sub-elements.
         * @param logical_pair The logical vector to use
		 * @param ccb The constraint callback to add
		 */
        static void schedule(chr::Logical_var_mutable< Logical_pair< U, V > >& logical_pair, chr::Constraint_callback& ccb)
		{
            logical_pair.schedule(ccb);
            auto nc_p = const_cast< std::pair< U, V >& >((*logical_pair).get());
            schedule_constraint_callback(nc_p.first,ccb);
            schedule_constraint_callback(nc_p.second,ccb);
        }
		/**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a logical_pair and its two sub-elements.
         * @param logical_pair The logical pair to use
		 * @param ccb The constraint callback to add
		 */
        static void schedule(chr::Logical_var< Logical_pair< U, V > >& logical_pair, chr::Constraint_callback& ccb)
		{
            logical_pair.schedule(ccb);
            auto nc_p = const_cast< std::pair< U, V >& >((*logical_pair).get());
            schedule_constraint_callback(nc_p.first,ccb);
            schedule_constraint_callback(nc_p.second,ccb);
		}
	};

    /**
     * Specialisation of the Schedule templated class to schedule recursively all logical
     * variables of a Logical_vector
     * \ingroup Logical_variables
     */
    template< typename T >
    struct Schedule< Logical_vector< T > >
    {
        /**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a logical_vector and all its elements.
         * @param logical_vector The logical vector to browse for logical variables
         * @param ccb The constraint callback to add
         */
        static void schedule(chr::Logical_var_mutable< Logical_vector< T > >& logical_vector, chr::Constraint_callback& ccb)
        {
            logical_vector.schedule(ccb);
            for(auto& i : const_cast< std::vector< T >& >((*logical_vector).get()))
                schedule_constraint_callback(i,ccb);
        }
        /**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a logical_vector and all its elements.
         * @param logical_vector The logical vector to browse for logical variables
         * @param ccb The constraint callback to add
         */
        static void schedule(chr::Logical_var< Logical_vector< T > >& logical_vector, chr::Constraint_callback& ccb)
        {
            logical_vector.schedule(ccb);
            for(auto& i : const_cast< std::vector< T >& >((*logical_vector).get()))
                schedule_constraint_callback(i,ccb);
        }
    };

	/**
	 * Specialisation of the Schedule templated class to schedule recursively all logical
	 * variables of a Logical_list
	 * \ingroup Logical_variables
	 */
	template< typename T >
	struct Schedule< Logical_list< T > >
	{
		/**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a logical_list and all its elements.
		 * @param logical_list The logical list to browse for logical variables
		 * @param ccb The constraint callback to add
		 */
		static void schedule(chr::Logical_var_mutable< Logical_list< T > >& logical_list, chr::Constraint_callback& ccb)
		{
            logical_list.schedule(ccb);
            for(auto& i : const_cast< std::list< T >& >((*logical_list).get()))
                schedule_constraint_callback(i,ccb);
		}
		/**
         * Schedule a constraint callback \a ccb to the list of callbacks of \a logical_list and all its elements.
		 * @param logical_list The logical list to browse for logical variables
		 * @param ccb The constraint callback to add
		 */
		static void schedule(chr::Logical_var< Logical_list< T > >& logical_list, chr::Constraint_callback& ccb)
		{
            logical_list.schedule(ccb);
            for(auto& i : const_cast< std::list< T >& >((*logical_list).get()))
                schedule_constraint_callback(i,ccb);
		}
	};

    /**
	 * Class wrapper to schedule callbacks only on
	 * not grounded or mutable variables
	 * \ingroup Logical_variables
	 */
	template< typename T >
	struct Wrapper_schedule_constraint_callback
    { };

	/**
	 * Specialization of schedule for chr::Logical_var_ground.
	 * Does nothing as nothing must be scheduled on grounded variables.
	 * \ingroup Logical_variables
	 */
	template< typename T >
	struct Wrapper_schedule_constraint_callback< chr::Logical_var_ground< T > >
	{
		/**
		 * Schedule a constraint callback \a ccb to the list of callbacks of \a variable.
		 * @param variable The logical variable to update
		 * @param ccb The constraint callback to add
		 */
		static void schedule(chr::Logical_var_ground< T >&, Constraint_callback&) { }
	};

	/**
	 * Specialization of schedule for chr::Logical_var_mutable.
	 * Schedule the mutable logical variable.
	 * \ingroup Logical_variables
	 */
	template< typename T >
	struct Wrapper_schedule_constraint_callback< chr::Logical_var_mutable< T > >
	{
		/**
		 * Schedule a constraint callback \a ccb to the list of callbacks of \a variable.
		 * @param variable The logical variable to update
		 * @param ccb The constraint callback to add
		 */
		static void schedule(chr::Logical_var_mutable< T >& variable, Constraint_callback& ccb)
		{
			Schedule< T >::schedule(variable,ccb);
		}
	};

	/**
	 * Specialization of schedule for chr::Logical_var
	 * Test the status of the logical variable and call schedule if needed.
	 * \ingroup Logical_variables
	 */
	template< typename T >
	struct Wrapper_schedule_constraint_callback< chr::Logical_var< T > >
	{
		/**
		 * Schedule a constraint callback \a ccb to the list of callbacks of \a variable.
		 * @param variable The logical variable to update
		 * @param ccb The constraint callback to add
		 */
		static void schedule(chr::Logical_var< T >& variable, Constraint_callback& ccb) {
			switch (variable.status())
			{
				case chr::Status::NOT_GROUND:
					[[fallthrough]];
				case chr::Status::MUTABLE:
					Schedule< T >::schedule(variable,ccb);
					return;
				case chr::Status::GROUND:
					return;
				default:
					assert(false);
			}
		}
	};

	template< typename Variable_type >
	void schedule_constraint_callback(Variable_type& variable, Constraint_callback& ccb)
	{
		Wrapper_schedule_constraint_callback< Variable_type >::schedule(variable,ccb);
	}
}
