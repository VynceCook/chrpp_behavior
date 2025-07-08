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

#include <chrpp.hh>

namespace chr
{
	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::~Constraint_store_index()
	{
		chr::Statistics::dec_memory< chr::Statistics::CONSTRAINT_STORE >(sizeof(Sublist_t*) * _pending_sl.size());
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	typename Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::begin()
	{
		return Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator(*this);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	typename Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::end()
	{
		return Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator(*this,_store.end());
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template < unsigned int N_INDEX, typename ...Args >
	std::size_t Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::size(Args&& ... args) const
	{
		static_assert(N_INDEX < INDEX_COUNT, "The index number exceeds the maximum number of indexes");
		using Key_t = typename std::tuple_element<N_INDEX,TupleKey_t>::type;
		if (!Key_t::check_validity(args...))
			// Ill form args sequence (trouble with not grounded logical variables)
			return 0;

		auto k = Key_t::build_from_vars(std::forward_as_tuple(args...));
		auto it = std::get<N_INDEX>(_indexes).find( k );
		if (it == std::get<N_INDEX>(_indexes).end())
			return 0; // Iterator already at end
		else
			return (*it).second._sublist->size();
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template < unsigned int N_INDEX, typename ...Args >
	typename Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator_hash Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::begin(Args&& ... args)
	{
		static_assert(N_INDEX < INDEX_COUNT, "The index number exceeds the maximum number of indexes");
		using Key_t = typename std::tuple_element<N_INDEX,TupleKey_t>::type;
		if (!Key_t::check_validity(args...))
			// Ill form args sequence (trouble with not grounded logical variables)
			return Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator_hash(*this, Sublist_t().end()); // Iterator already at end

		auto k = Key_t::build_from_vars(std::forward_as_tuple(args...));
		auto it = std::get<N_INDEX>(_indexes).find( k );
		if (it == std::get<N_INDEX>(_indexes).end())
			return Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator_hash(*this, Sublist_t().end()); // Iterator already at end
		else
			return Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator_hash(*this, it->second._sublist->begin());
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template< unsigned int N_INDEX, size_t... I >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::schedule_index_callback(const T& e, std::index_sequence<I...>)
	{
		auto wo_cs = chr::Weak_obj< Constraint_store_index< T, TupleIndexes, ENABLE_BACKTRACK > >(this); // call private constructor
		([&]{
			using Index_t = typename std::tuple_element<N_INDEX,TupleIndexes>::type;
			// We shift with +1 because first element is the unique constraint id,
			// the first parameter starts just after.
			constexpr unsigned int N = Index_t::template get<I>::value + 1;
			if constexpr (!std::tuple_element<N,T>::type::LV_GROUND)
			{
				auto& var = const_cast<typename std::tuple_element<N,T>::type&>(std::get<N>(e)); // Get not const ref on involved variable
				if (var.status() != chr::Status::GROUND)
					var.schedule( new chr::LNS::IndexCallback<Constraint_store_index,N_INDEX,I>(e,wo_cs) );
			}
		}(), ...);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template< size_t... I >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::loop_add(const T& e, PID_t pid, std::index_sequence<I...>)
	{
		([&]{
			using Key_t = typename std::tuple_element<I,TupleKey_t>::type;
			using Index_t = typename std::tuple_element<I,TupleIndexes0>::type;
			Key_t k( Key_t::template build_from_constraint<T,Index_t>(e) );
			Index_element_t ie(Backtrack::depth());

			auto res = std::get<I>(_indexes).insert( { std::move( k ), std::move( ie ) } );
			if (res.second)
			{
				auto sl = std::make_unique<Sublist_t>();
				sl->insert( pid );
				(res.first.value())._sublist = std::move( sl );
			} else {
				auto& sl = (*res.first).second._sublist;
				sl->insert( pid );
			}
			// Schedule index callback
			schedule_index_callback<I>(e,std::make_index_sequence<Index_t::size>());
		}(), ...);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	typename Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::add(T e)
	{
		prepare_rollback_point();
		auto it = _store.insert( std::move(e) );
		// Build all indexes
		loop_add(*it,it.pid(),std::make_index_sequence<INDEX_COUNT>());
		return Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::iterator(*this,it);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template< size_t... I >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::loop_remove(const T& e, PID_t pid_e, std::index_sequence<I...>)
	{
		([&]{
			// Retrieve element at key from_key
			using Index_t = typename std::tuple_element<I,TupleIndexes>::type;
			using Key_t = typename std::tuple_element<I,TupleKey_t>::type;
			using Hash_t = typename std::tuple_element<I,TupleKey_t>::type::Hash;
			Key_t k = Key_t::template build_from_constraint<T,Index_t>(e);
			auto hash_value = Hash_t()(k);
			auto res = std::get<I>(_indexes).find( k, hash_value );
			assert(res != std::get<I>(_indexes).end());

			std::unique_ptr<Sublist_t>& p_sl = res.value()._sublist; // Modifiable reference to from_it
			auto it = p_sl->begin();
			#ifndef NDEBUG
				auto it_end = p_sl->end();
			#endif
			while (true)
			{
				if ((*it) == pid_e)
				{
					p_sl->remove(it);
					break;
				}
				++it;
			}
			assert( it != it_end );

			// Remove hash tag if list is empty and no need to backtrack this removal
			if (!p_sl->has_rollback_point() && p_sl->empty())
			{
				if (p_sl->safe_delete())
					p_sl.reset(nullptr);
				else
					add_to_pending_list( std::move(p_sl) );
				std::get<I>(_indexes).erase( std::move( k ), hash_value );
			}
		}(), ...);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::remove(PID_t pid)
	{
		assert(_store.alive(pid));
		prepare_rollback_point();

		const T& e = _store.get(pid);
		// Remove all index elements that involve this constraint
		loop_remove(e,pid,std::make_index_sequence<INDEX_COUNT>());
		// Remove the constraint from the main store
		_store.remove(pid);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template < unsigned int N_INDEX  >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::update_index(typename std::tuple_element<N_INDEX,TupleKey_t>::type from_key, typename std::tuple_element<N_INDEX,TupleKey_t>::type to_key)
	{
		static_assert(N_INDEX < INDEX_COUNT, "The index number exceeds the maximum number of indexes");
		using Key_t = typename std::tuple_element<N_INDEX,TupleKey_t>::type;
		auto& index_map = std::get<N_INDEX>(_indexes);

		 // Retrieve element at key from_key
		 auto from_hash_value = typename Key_t::Hash()(from_key);
		 auto from_it = index_map.find(from_key, from_hash_value);
		 if (from_it == index_map.end())
			 // We don't find the key, because the constraint has been previously moved because
			 // they are in another sublist
			 return;

		 assert(!(from_key == to_key));
		 prepare_rollback_point();

		// Insert or retrieve element at key to_key
		Index_element_t ie(Backtrack::depth());
		auto to_pair_it = index_map.insert( { to_key, std::move(ie) } );
		Sublist_t* to_sublist = nullptr;
		if (to_pair_it.second)
		{
			(to_pair_it.first.value())._sublist = std::make_unique<Sublist_t>();
			to_sublist = (to_pair_it.first.value())._sublist.get();
			// Keep index update callback
			from_it = index_map.find(from_key); // refresh from_it because of new insert
			assert(from_it != index_map.end());
		} else
			to_sublist = (*to_pair_it.first).second._sublist.get();

		// Transfer elements from from_list to to_list
		auto& from_sublist = from_it.value()._sublist; // Modifiable reference to from_it
		auto it = from_sublist->begin();
		auto it_end = from_sublist->end();
		while (it != it_end)
		{
			to_sublist->insert(*it);
			it = from_sublist->remove(it);
		}

		// Remove hash tag if list is empty and no need to backtrack this removal
		if (!from_sublist->has_rollback_point() && from_sublist->empty())
		{
			if (from_sublist->safe_delete())
				from_sublist.reset(nullptr);
			else
				add_to_pending_list( std::move(from_sublist) );
			index_map.erase( std::move( from_key ), from_hash_value );
		}
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::add_to_pending_list(std::unique_ptr<Constraint_store_index::Sublist_t>&& sl)
	{
		auto it = _pending_sl.begin();
		auto it_end = _pending_sl.end();
		while (it != it_end)
		{
			if ((*it)->safe_delete())
			{
				// If we found a new safe delete list, we delete it
				// and put sl instead
				sl.swap(*it);
				sl.reset(nullptr);
				return;
			}
			++it;
		}
		// No safe delete list found in the pending list, add sl to the end
		_pending_sl.push_back(std::move(sl));
		chr::Statistics::inc_memory< chr::Statistics::CONSTRAINT_STORE >(sizeof(Sublist_t*));
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::prepare_rollback_point()
	{
		// Schedule
		if constexpr (ENABLE_BACKTRACK)
		{
			assert(_backtrack_depth <= Backtrack::depth());
			// Update backtrack depth and test if we need to registrer for backtrack
			if ((_backtrack_depth < Backtrack::depth()))
			{
				if (!_backtrack_scheduled)
				{
					_backtrack_scheduled = true;
					Backtrack::schedule(this);
				}
				_backtrack_depth = Backtrack::depth();
			}
		}
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template< size_t... I >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::loop_rewind(Depth_t new_depth, std::index_sequence<I...>)
	{
		([&]{
			auto& map = std::get<I>(_indexes);
			auto it = map.begin();
			auto it_end = map.end();
			while (it != it_end)
			{
				if ((*it).second._backtrack_depth > new_depth)
				{
					it = map.erase(it);
				} else  {
					(*it).second._sublist->rewind(0,new_depth);
					 // _backtrack_depth is the maximum depth of all list and sublist of the constraint store
					_backtrack_depth = std::max(_backtrack_depth, (*it).second._sublist->_backtrack_depth);
					// _backtrack_scheduled is true if any store needs to rewind
					_backtrack_scheduled = _backtrack_scheduled || (*it).second._sublist->has_rollback_point();
					++it;
				}
			}
		}(), ...);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	bool Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::rewind(Depth_t previous_depth, Depth_t new_depth)
	{
		if constexpr (!ENABLE_BACKTRACK)
		{
			(void) previous_depth; // Avoid compilation warnings
			(void) new_depth;      // Avoid compilation warnings
			assert(false);
			return false;
		} else {
			if (_backtrack_depth <= new_depth) return true;

			assert(_backtrack_scheduled);
			// Backtrack main store
			_store.rewind(previous_depth, new_depth);
			_backtrack_depth = _store._backtrack_depth; // _backtrack_depth is the maximum depth of all list and sublist of the constraint store
			_backtrack_scheduled = _store.has_rollback_point(); // _backtrack_scheduled is true if any store needs to be scheduled

			// Rewind all indexes
			loop_rewind(new_depth,std::make_index_sequence<INDEX_COUNT>());
			return _backtrack_scheduled;
		}
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template< size_t... I >
	void Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::loop_statistics(Statistics& stats, std::index_sequence<I...>) const
	{
		([&]{
			 auto& map = std::get<I>(_indexes);
			 for(auto it = map.begin(); it != map.end(); ++it)
			 {
				std::vector< unsigned long int > c_ids;
				auto it_sl = (*it).second._sublist->begin();
				while (!it_sl.at_end())
				{
					c_ids.push_back(std::get<0>(_store.get(*it_sl)));
					++it_sl;
				}
				using Hash_t = typename std::tuple_element<I,TupleKey_t>::type::Hash;
				auto hash_value = Hash_t()((*it).first);
				auto res = stats.indexes[I].insert( { hash_value, c_ids } );
				if (!res.second)
				{
					for (auto x : c_ids)
						(*res.first).second.push_back( x); // We found another key with same hash, unlikely, but still possible
				}
			}
		}(), ...);
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	typename Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::Statistics Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::index_statistics() const
	{
		Statistics stats;
		stats.size = _store.size();
		stats.nb_pending = _pending_sl.size();
		auto it = const_cast< Bt_list<T,ENABLE_BACKTRACK,false,chr::Statistics::CONSTRAINT_STORE>* >(&_store)->begin();
		while (!it.at_end())
		{
			stats.constraints.push_back(std::get<0>(*it));
			++it;
		}
		// Computes statistics about all indexes
		loop_statistics(stats, std::make_index_sequence<INDEX_COUNT>());
		return stats;
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	std::string Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::str_index_statistics() const
	{
		std::string res;
		auto stats = index_statistics();
		res += "Constraint_t: " + type_name< Constraint_t >() + "\n";
		res += "TupleIndexes: " + type_name< TupleIndexes >() + "\n";
		res += "TupleIndexes_t: " + type_name< TupleIndexes_t >() + "\n";
		res += "TupleKey_t: " + type_name< TupleKey_t >() + "\n";
		res += "Constraints(" + std::to_string(stats.size) + "):";
		auto it = const_cast<Constraint_store_index< T, TupleIndexes, ENABLE_BACKTRACK >*>(this)->begin();
		while (!it.at_end())
		{
			res += " " + _label + chr::TIW::constraint_to_string(*it);
			++it;
		}
		res += "\n";

		res += "Indexes (pending=" + std::to_string(stats.nb_pending) + "):\n";
		unsigned int n_idx = 0;
		for (auto idx_m : stats.indexes)
		{
			res += "  Idx " + std::to_string(n_idx) + "\n";
			for (auto e : idx_m)
			{
				res += "    [" + chr::TIW::to_string(e.first) + "]:";
				for (unsigned int j = 0; j < e.second.size(); ++j)
					res += + " " + chr::TIW::to_string(e.second[j]);
				res += "\n";
			}
			++n_idx;
		}
		return res;
	}

	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 >
	template <class TT>
	std::string Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 >::type_name() const
	{
	    typedef typename std::remove_reference<TT>::type TR;
	    std::unique_ptr<char, void(*)(void*)> own
	           (
			#ifndef _MSC_VER
	                abi::__cxa_demangle(typeid(TR).name(), nullptr,
			                                           nullptr, nullptr),
			#else
	                nullptr,
			#endif
	                std::free
	           );
	    std::string r = own != nullptr ? own.get() : typeid(TR).name();
	    if (std::is_const<TR>::value)
	        r += " const";
	    if (std::is_volatile<TR>::value)
	        r += " volatile";
	    if (std::is_lvalue_reference<T>::value)
	        r += "&";
	    else if (std::is_rvalue_reference<T>::value)
	        r += "&&";
	    return r;
	}

}
