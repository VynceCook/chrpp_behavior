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

#ifndef RUNTIME_BACKTRACK_HH_
#define RUNTIME_BACKTRACK_HH_

#include <statistics.hh>
#include <list>
#include <cassert>
#include <memory>

#include <trace.hh>
#include <shared_obj.hh>

/**
 * \defgroup Backtrack Backtrack management
 * 
 * To implement the \e CHR++ disjunction, the program must be able to backtrack
 * to a previous state. Each element must be able to restore one of its previous values.
 */

namespace chr {
	using Depth_t = unsigned int;	///< Alias for type of backtrack depth value

	// Forward declaration
	template < typename T >	class Backtrack_t;

	/**
	 * @brief Execution Status of a CHR constraint call
	 */
	enum ES_CHR : bool {
		FAILURE = false,	//!< A failure is raised
		SUCCESS = true		//!< No failure, so success
	};

	/**
	 * @brief Backtrack observer (root class)
	 *
	 * This class is a base class which must be inherited for
	 * each observer of a Backtrack.
	 * Observer elements are stored in wake up list in the Backtrack class
	 * and wake up all objects when a Backtrack event is triggered.
	 * \ingroup Backtrack
	 */
	class Backtrack_observer
	{
	public:
		template < typename T >	friend class Backtrack_t;
		// Two member hereafter deal with shared and weak references of a Backtrack_observer
		unsigned int _ref_use_count;		///< Count of shared references
		unsigned int _ref_weak_count;		///< Count of weak references + (#shared != 0)

		// Friend class for managing the shared reference counters
		template< typename U > friend bool chr_expired(U*);

		/**
		 * Initialize.
		 */
		Backtrack_observer() : _ref_use_count(0), _ref_weak_count(0) { }

		/**
		 * Run the following code when the an object needs to be updated after a Backtrack event.
		 * It is called in order to rewind the involved data structure.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		virtual bool rewind(Depth_t previous_depth, Depth_t new_depth) = 0;
	};

	/**
	 * @brief Backtrack management
	 *
	 * Class to manage Backtrack. It records each downward step and wakes up
	 * every object which need to be restored when program come back to a previous
	 * choice.
	 * The template parameter is only here to allow static initialization
	 * in a .hh file (useful tip).
	 * \ingroup Backtrack
	 */
	template < typename T >
	class Backtrack_t
	{
	public:
		/**
		 * Initialize: disabled.
		 */
		Backtrack_t() =delete;

		/**
		 * Copy constructor: disabled.
		 */
		Backtrack_t(const Backtrack_t&) =delete;

		/**
		 * Assignment operator: disabled.
		 */
		Backtrack_t& operator=(const Backtrack_t&) =delete;

		/**
		 * Return the current backtrack depth.
		 * @return The current backtrack depth
		 */
		static Depth_t depth()
		{
			return _backtrack_depth;
		}

		/**
		 * Increasing the current backtrack depth.
		 * @return The new backtrack depth
		 */
		static Depth_t inc_backtrack_depth()
		{
			chr::Statistics::update_peak_depth(_backtrack_depth);
			return ++_backtrack_depth;
		}

		/**
		 * Return the current execution status of the CHR program
		 * @return The current execution status
		 */
		static ES_CHR es_state()
		{
			return _es_state;
		}

		/**
		 * Set the execution status to FAILURE.
		 */
		static void failure()
		{
			_es_state = ES_CHR::FAILURE;
		}

		/**
		 * Set the execution status to SUCCESS.
		 */
		static void reset()
		{
			_es_state = ES_CHR::SUCCESS;
		}

		/**
		 * Add a new backtrack observer object to the list of observers to wake up when a backtrack
		 * event is triggered.
		 * @param callback The *shared* reference on the callback object to run
		 */
		static void schedule(Backtrack_observer* callback)
		{
			_wake_up.push_back( chr::Weak_obj<Backtrack_observer>(callback) );
		}

		/**
		 * Return the number of callback objects scheduled.
		 * @return The number of callbacks objects
		 */
		static std::size_t callback_count()
		{
			return _wake_up.size();
		}

		/**
		 * Backtrack to a given depth of the CHR program tree.
		 * It wakes up all object which have been registered to backtrack events.
		 * @return The new backtrack depth
		 */
		static Depth_t back_to(Depth_t new_depth)
		{
			assert( new_depth < _backtrack_depth );

			auto it = _wake_up.begin();
			auto it_end = _wake_up.end();
			while( it != it_end )
			{
				assert( (*it)._ptr != nullptr );
				if ((*it).expired() || !(*it)->rewind(_backtrack_depth,new_depth))
				{
					it = _wake_up.erase(it);
				} else
					++it;
			}
			_backtrack_depth = new_depth;
			return _backtrack_depth;
		}

	private:
		static Depth_t _backtrack_depth;								///< Current depth in CHR program runtime tree
		static chr::ES_CHR _es_state;									///< Current depth in CHR program runtime tree
		static std::list< chr::Weak_obj<Backtrack_observer> > _wake_up;	///< List of callback to wake up after a Backtrack event
	};

	// Initialization of static members
	template< typename T >
	Depth_t chr::Backtrack_t<T>::_backtrack_depth = 0;
	template< typename T >
	chr::ES_CHR chr::Backtrack_t<T>::_es_state = chr::ES_CHR::SUCCESS;
	template< typename T >
	std::list< chr::Weak_obj<Backtrack_observer> > chr::Backtrack_t<T>::_wake_up;

	// Dummy class to get rid off the template parameter
	struct Dummy_backtrack { };

	// Alias to get rid off the template parameter when calling Backtrack
	// class.
	using Backtrack = Backtrack_t< Dummy_backtrack >;

	/**
	 * Raise a CHR failure.
	 * Set the global execution status to FAILURE and increases the number
	 * of failures.
	 * @return ES_CHR::FAILURE
	 */
	inline chr::ES_CHR failure()
	{
		TRACE( chr::Log::trace_constraint(chr::Log::FAIL, "", "CORE_ENGINE", -1, std::make_tuple("Failure raised")); )
		chr::Statistics::inc_nb_failures();
		chr::Backtrack::failure();
		return chr::ES_CHR::FAILURE;
	}

	/**
	 * Return the ES_CHR::SUCCESS value. Convenient function
	 * used to design CHR rules or C++ user functions. 
	 * @return ES_CHR::SUCCESS
	 */
	inline chr::ES_CHR success() {
		return chr::ES_CHR::SUCCESS;
	}

	/**
	 * Check if the current CHR program status is failed.
	 * @return True if the execution status is FAILURE, false otherwise
	 */
	inline bool failed()
	{
		return (chr::Backtrack::es_state() == ES_CHR::FAILURE);
	}

	/**
	 * Reset the execution status to SUCCESS.
	 * Usefull to continue executing rules.
	 */
	inline void reset()
	{
		chr::Backtrack::reset();
	}

}

#endif /* RUNTIME_BACKTRACK_HH_ */
