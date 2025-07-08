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

#ifndef RUNTIME_CONSTRAINT_STORES_ITERATOR_HH_
#define RUNTIME_CONSTRAINT_STORES_ITERATOR_HH_

#include <tuple>
#include <vector>
#include <string>

namespace chr {

	/**
	 * @brief Constraint stores iterator
	 *
	 * A constraint stores iterator is able to browse a list of constraint stores.
	 * A Constraint_stores_iterator is built with a list of stores of type Ts...
	 * \ingroup Constraints
	 */
	template < typename... Ts >
	class Constraint_stores_iterator
	{
	public:
		using Stores_it_t = std::tuple<typename Ts::type::iterator...>;	///< Type mapping for a tuple of stores iterators
		static constexpr std::size_t NB_STORES = sizeof...(Ts);			///< Number of stores

		/**
		 * Build a Constraint_stores_iterator from a list of constraint stores.
		 * @param stores The stores constraint
		 */
		Constraint_stores_iterator(Ts... stores)
			: _stores_it(std::forward_as_tuple(stores->begin()...)), _i_store(0)
		{
	        (_store_labels.push_back(std::string(stores->label())), ...);
			valid_it();
		}

		/**
		 * Move the iterator to the next constraint. It can move across all the stores
		 * recorded in the iterator class.
		 * @return A reference to this iterator
		 */
		Constraint_stores_iterator& operator++()
		{
			loop_incr(std::make_index_sequence<NB_STORES>());
			valid_it();
			return *this;
		}

		/**
		 * Return true if the iterator is at the end of all stores of constraints.
		 * @return True if iterator is at the end, false otherwise
		 */
		bool at_end() const { return (_i_store == NB_STORES); }

		/**
		 * Return the string conversion of the underlying constraint of this iterator
		 * @return The string
		 */
		std::string to_string()
		{
			return loop_to_string(std::make_index_sequence<NB_STORES>());
		}

	private:
		Stores_it_t _stores_it;					///< The store iterators to use
		std::vector<std::string> _store_labels;	///< The store labels
		unsigned int _i_store;					///< The number of the current store iterator

		/**
		 * Local loop of the operator++. It uses the store refered by _i_store
		 */
		template< size_t... I >
		void loop_incr(std::index_sequence<I...>)
		{
			([&]{
				if (I == _i_store) ++std::get<I>(_stores_it);
			}(), ...);
		}

		/**
		 * Local loop of the to_string function. It uses the store refered by _i_store
		 * @return Return the string conversion of the underlying constraint
		 */
		template< size_t... I >
		std::string loop_to_string(std::index_sequence<I...>)
		{
			std::string res;
			([&]{
				if (I == _i_store) res = _store_labels[I] + chr::TIW::constraint_to_string(*std::get<I>(_stores_it));
			}(), ...);
			return res;
		}

		/**
		 * Local loop of the valid_it function.
		 * @return True if the a not empty store has been found.
		 */
		template< size_t... I >
		bool loop_valid_it(std::index_sequence<I...>)
		{
			bool ret = false;
			([&]{
				if (I == _i_store)
				{
					if (!std::get<I>(_stores_it).at_end()) ret = true;
					else ++_i_store;
				}
			}(), ...);
			return ret;
		}

		/**
		 * Move the iterator to the next store if the current underlying store is
		 * at the end. When multiple consecutive stores are at the end, the function
		 * move to the first not empty store.
		 */
		void valid_it()
		{
			while(_i_store < NB_STORES)
				if (loop_valid_it(std::make_index_sequence<NB_STORES>())) return;
		}
	};
}

#endif /* RUNTIME_CONSTRAINT_STORES_ITERATOR_HH_ */
