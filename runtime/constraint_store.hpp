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
	template< typename T, bool ENABLE_BACKTRACK >
	typename Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator Constraint_store_simple< T, ENABLE_BACKTRACK >::begin()
	{
		return Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator(*this,_store.begin());
	}

	template< typename T, bool ENABLE_BACKTRACK >
	typename Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator Constraint_store_simple< T, ENABLE_BACKTRACK >::end()
	{
		return Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator(*this,_store.end());
	}

	template< typename T, bool ENABLE_BACKTRACK >
	typename Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator Constraint_store_simple< T, ENABLE_BACKTRACK >::add(T e)
	{
		if constexpr (!ENABLE_BACKTRACK)
		{
			auto ret = Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator(*this,_store.insert( std::move(e) ));
			return ret;
		} else {
			bool has_rb = _store.has_rollback_point();
			auto ret = Constraint_store_simple< T, ENABLE_BACKTRACK >::iterator(*this,_store.insert( std::move(e) ));
			if (has_rb != _store.has_rollback_point())
			{
				assert(!has_rb);
				Backtrack::schedule(this);
			}
			return ret;
		}
	}

	template< typename T, bool ENABLE_BACKTRACK >
	void Constraint_store_simple< T, ENABLE_BACKTRACK >::remove(PID_t pid)
	{
		if constexpr (!ENABLE_BACKTRACK)
		{
			_store.remove(pid);
		} else {
			bool has_rb = _store.has_rollback_point();
			_store.remove(pid);
			if (has_rb != _store.has_rollback_point())
			{
				assert(!has_rb);
				Backtrack::schedule(this);
			}
		}
	}

	template< typename T, bool ENABLE_BACKTRACK >
	bool Constraint_store_simple< T, ENABLE_BACKTRACK >::rewind(Depth_t previous_depth, Depth_t new_depth)
	{
		if constexpr (!ENABLE_BACKTRACK)
		{
			(void) previous_depth; // Avoid compilation warnings
			(void) new_depth;      // Avoid compilation warnings
			assert(false);
			return false;
		} else {
			return _store.rewind(previous_depth, new_depth);
		}
	}
}
