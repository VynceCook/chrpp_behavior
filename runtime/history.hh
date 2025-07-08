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

#ifndef RUNTIME_HISTORY_HH_
#define RUNTIME_HISTORY_HH_

#include <algorithm>
#include <list>
#include <array>

#include <utils.hpp>
#include <third_party/robin_set.h>
#include <constraint_store.hh>
#include <statistics.hh>

/**
 * \defgroup History management
 * 
 * As each propagation rule can be triggered only once for each permutation
 * of same constraints (unique ids), an history must be managed to remember
 * of previous propagation rule executions.
 */

namespace chr
{	
	/**
	 * @brief History class
	 *
	 * Class to manage the history of one rule, it is based on
	 * an unordered set and the main operation is to check and
	 * add if it is not exist. The history will grow up, no removal
	 * of added value is done.
	 * Each element of the history is an array of unsigned long int.
	 * The array is of size N (number of unsigned long int).
	 * \ingroup History
	 */
	template < size_t N >
	class History
	{
	public:
		using Data_type = std::array< unsigned long int, N >;
		/*
		 * Computes hash for Data_type
		 */
		struct Hash
		{
			chr::CHR_XXHASH_hash_t operator()(const Data_type& a) const
			{
				return CHR_XXHash(a);
			}
		};

		/**
		 * Default constructor.
		 */
		History()
		{ }

		/**
		 * Copy constructor (deleted).
		 */
		History(const History&) =delete;

		/**
		 * Destructor.
		 */
		~History() {
			Statistics::dec_memory< Statistics::HISTORY >(sizeof(tsl::detail_robin_hash::bucket_entry<Data_type, false>) * _p_h.size());
		}

		/**
		 * Check if an element is in the history. It the element doesn't
		 * already exist, it inserts it in the history.
		 * The given array will be modified (sorted)
		 * @param e The element to check
		 * @return True if the element doesn't already exist, false otherwise
		 */
		bool check(Data_type&& e)
		{
			if constexpr (N > 1)
				std::sort(e.begin(), e.end());
			auto res = _p_h.emplace( e );
#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			if (res.second)
				Statistics::inc_memory< Statistics::HISTORY >(sizeof(tsl::detail_robin_hash::bucket_entry<Data_type, false>));
#endif
			return res.second;
		}

		/**
		 * Check if an element is in the history. It the element doesn't
		 * already exist, it inserts it in the history.
		 * The given array will be modified (sorted)
		 * @param e The element to check
		 * @return True if the element doesn't already exist, false otherwise
		 */
		bool check(Data_type& e)
		{
			if constexpr (N > 1)
				std::sort(e.begin(), e.end());
			auto res = _p_h.insert( e );
#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			if (res.second)
				Statistics::inc_memory< Statistics::HISTORY >(sizeof(tsl::detail_robin_hash::bucket_entry<Data_type, false>));
#endif
			return res.second;
		}

		/**
		 * Return the size of the history.
		 * @return The size of the history
		 */
		size_t size() const
		{
			return _p_h.size();
		}

		/**
		 * Convert the history to a string representation.
		 * Only for testing purposes as big history will make
		 * huge strings!
		 * @return A string with all history elements
		 */
		std::string to_string() const
		{
			std::string res;
			for (auto& a : _p_h)
			{
				res += "(";
				for (auto& e : a)
					res += std::to_string(e) + ",";
				res.resize(res.size() - 1);
				res += ")";
			}
			return res;
		}

	private:
		tsl::robin_set< Data_type, Hash > _p_h; ///< History unordered set
	};

	/**
	 * @brief History_dyn class
	 *
	 * Class to manage the history of one rule, it is based on
	 * a set and the main operation is to check and
	 * add if it is not exist. The history will grow up, but when
	 * backtracking, it removes the values which are not relevant anymore
	 * because they are belong to forget space depths.
	 * Each element of the history is an array of unsigned long int.
	 * The array is of size N (number of unsigned long int).
	 * \ingroup History
	 */
	template< size_t N >
	class History_dyn : public Backtrack_observer
	{
	public:
		using Data_type = std::array< unsigned long int, N >;
		/*
		 * Computes hash for Data_type
		 */
		struct Hash
		{
			chr::CHR_XXHASH_hash_t operator()(const Data_type& a) const
			{
				return CHR_XXHash(a);
			}
		};
		using Set_t = tsl::robin_set< Data_type, Hash >;

		/**
		 * Default constructor.
		 */
		History_dyn() :
			_backtrack_depth(Backtrack::depth())
		{}

		/**
		 * Copy constructor (deleted).
		 */
		History_dyn(const History_dyn&) =delete;

		/**
		 * Virtual destructor.
		 */
		virtual ~History_dyn() {
			Statistics::dec_memory< Statistics::HISTORY >(sizeof(tsl::detail_robin_hash::bucket_entry<Data_type, false>) * _p_h.size());
		}

		/**
		 * Check if an element is in the history. It the element doesn't
		 * already exist, it inserts it in the history.
		 * The given array will be modified (sorted)
		 * @param e The element to check
		 * @return True if the element doesn't already exist, false otherwise
		 */
		bool check(Data_type&& e)
		{
			// Test if we need to start a new snapshot point
			if (_backtrack_depth < Backtrack::depth())
				create_snapshot();
			if constexpr (N > 1)
				std::sort(e.begin(), e.end());
			auto res = _p_h.emplace( e );
			if (res.second && _previous_snapshot)
			{
				_previous_snapshot->_values_to_remove.push_front( *res.first );
				Statistics::inc_memory< Statistics::HISTORY >(sizeof(Data_type));
			}
			#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			if (res.second)
				Statistics::inc_memory< Statistics::HISTORY >(sizeof(tsl::detail_robin_hash::bucket_entry<Data_type, false>));
			#endif
			return res.second;
		}

		/**
		 * Check if an element is in the history. It the element doesn't
		 * already exist, it inserts it in the history.
		 * The given array will be modified (sorted)
		 * @param e The element to check
		 * @return True if the element doesn't already exist, false otherwise
		 */
		bool check(Data_type& e)
		{
			// Test if we need to start a new snapshot point
			if (_backtrack_depth < Backtrack::depth())
				create_snapshot();
			if constexpr (N > 1)
				std::sort(e.begin(), e.end());
			auto res = _p_h.insert( e );
			if (res.second && _previous_snapshot)
			{
				_previous_snapshot->_values_to_remove.push_front( *res.first );
				Statistics::inc_memory< Statistics::HISTORY >(sizeof(Data_type));
			}
			#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			if (res.second)
				Statistics::inc_memory< Statistics::HISTORY >(sizeof(tsl::detail_robin_hash::bucket_entry<Data_type, false>));
			#endif
			return res.second;
		}

		/**
		 * Return the size of the history.
		 * @return The size of the history
		 */
		size_t size() const
		{
			return _p_h.size();
		}

		/**
		 * Convert the history to a string representation.
		 * Only for testing purposes as big history will make
		 * huge strings!
		 * @return A string with all history elements
		 */
		std::string to_string() const
		{
			std::string res;
			for (auto& a : _p_h)
			{
				res += "(";
				for (auto& e : a)
					res += std::to_string(e) + ",";
				res.resize(res.size() - 1);
				res += ")";
			}
			return res;
		}

	private:
		/**
		 * Data structure which represents a snapshot of a History_dyn.
		 * It stores elements that must be removed on rewind.
		 */
		struct Linked_snapshot
		{
			Depth_t _backtrack_depth;								///< The backtrack depth used to create this snapshot
			std::list< Data_type > _values_to_remove;				///< List of values to remove
			std::unique_ptr< Linked_snapshot > _previous_snapshot;	///< Previous snapshot

			#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			~Linked_snapshot()
			{
				Statistics::dec_memory< Statistics::HISTORY >(sizeof(Linked_snapshot));
				Statistics::dec_memory< Statistics::HISTORY >(sizeof(Data_type) * _values_to_remove.size());
			}
			#endif
		};

		Set_t _p_h;												///< History unordered set
		Depth_t _backtrack_depth;								///< The current backtrack depth for this snapshot
		std::unique_ptr< Linked_snapshot > _previous_snapshot;	///< The previous snapshot

		/**
		 * Create a snapshot of the values to remove when rewind.
		 */
		void create_snapshot()
		{
			if (!_previous_snapshot)
				Backtrack::schedule(this); // Schedule history observer

			auto ps = std::make_unique< Linked_snapshot >();
			Statistics::inc_memory< Statistics::HISTORY >(sizeof(Linked_snapshot));
			ps->_backtrack_depth = _backtrack_depth;
			ps->_previous_snapshot = std::move( _previous_snapshot );

			_previous_snapshot = std::move( ps );
			_backtrack_depth = Backtrack::depth();
		}

		/**
		 * Rewind the current node in the backtrackable tree to the node of depth
		 * \a new_depth of the current branch.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		bool rewind(chr::Depth_t, chr::Depth_t new_depth) override
		{
			if (_backtrack_depth <= new_depth) return true;

			assert((bool) _previous_snapshot); // If callback called, we must have previous snapshot
			while (_previous_snapshot->_backtrack_depth > new_depth)
			{
				// Remove unknown variables indexes
				for (auto& e : _previous_snapshot->_values_to_remove)
					_p_h.erase( e );
				_previous_snapshot = std::move(_previous_snapshot->_previous_snapshot);
			}
			assert((bool) _previous_snapshot); // If callback called, we must have something to rewind
			assert(_previous_snapshot->_backtrack_depth <= new_depth);
			// Remove unknown variables indexes
			for (auto& e : _previous_snapshot->_values_to_remove)
				_p_h.erase( e );
			_backtrack_depth = _previous_snapshot->_backtrack_depth;
			_previous_snapshot = std::move(_previous_snapshot->_previous_snapshot);
			return ((bool) _previous_snapshot);
		}
	};
}

#endif /* RUNTIME_HISTORY_HH_ */
