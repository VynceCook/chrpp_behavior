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
#include "bt_interval.hh"
#include <utility>

namespace chr
{
	/*
	 * Interval Range, no holes !
	 */
	template< typename T >
	Interval< T, true >::Interval()
		: _min(chr::Numerics<T>::next(0)), _max(0)
	{ }

	template< typename T >
	Interval< T, true >::Interval(T min, T max)
		: _min(min), _max(max)
	{
		assert(min <= max);
	}

	template< typename T >
	Interval< T, true >::Interval(const Interval& o, T offset, bool opposite)
	{
		if (opposite)
		{
			_min = -o._max+offset;
			_max = -o._min+offset;
		} else {
			_min = o._min+offset;
			_max = o._max+offset;
		}
	}

	template< typename T >
	bool Interval< T, true >::operator==(const Interval& o) const
	{
		return ((_min == o._min) && (_max == o._max));
	}

	template< typename T >
	std::string Interval< T, true >::to_string() const
	{
		if (_max < _min) // Empty interval
			return "[/]";
		else
			return "[" + chr::TIW::to_string(_min) + ";" + chr::TIW::to_string(_max) + "]";
	}

	template< typename T >
	T Interval< T, true >::min() const
	{
		return _min;
	}

	template< typename T >
	T Interval< T, true >::max() const
	{
		return _max;
	}

	template< typename T >
	T Interval< T, true >::val() const
	{
		assert(_min == _max);
		return _max;
	}

	template< typename T >
	size_t Interval< T, true >::width() const
	{
		assert(!empty());
		return static_cast<size_t>(chr::Numerics<T>::next(_max) - _min);
	}

	template< typename T >
	size_t Interval< T, true >::count() const
	{
		assert(!empty());
		if constexpr (chr::Numerics<T>::countable)
			return width();
		else {
			if (singleton()) return 1;
			else return std::numeric_limits<size_t>::infinity();
		}
	}

	template< typename T >
	bool Interval< T, true >::empty() const
	{
		return (_min > _max);
	}

	template< typename T >
	bool Interval< T, true >::range() const
	{
		return true;
	}

	template< typename T >
	bool Interval< T, true >::singleton() const
	{
		return (_min == _max);
	}

	template< typename T >
	bool Interval< T, true >::in(T n) const
	{
		return !((n < _min) || (n > _max));
	}

	template< typename T >
	bool Interval< T, true >::eq(T n)
	{
		if ((n == _min) && (n == _max)) return false;
		if (n < _min)
		{
			_max = n;
			return true;
		} else if (n > _max)
		{
			_min = n;
			return true;
		} else {
			_min = _max = n;
			return true;
		}
	}

	template< typename T >
	bool Interval< T, true >::nq(T n)
	{
		if (n == _min)
		{
			_min = chr::Numerics<T>::next(_min);
			return true;
		} else if (n == _max)
		{
			_max = chr::Numerics<T>::previous(_max);
			return true;
		} else {
			return false;
		}
	}

	template< typename T >
	bool Interval< T, true >::lq(T n)
	{
		if (n >= _max) return false;
		_max = n;
		return true;
	}

	template< typename T >
	bool Interval< T, true >::gq(T n)
	{
		if (n <= _min) return false;
		_min = n;
		return true;
	}

	template< typename T >
	bool Interval< T, true >::narrow(const chr::Interval<T, true>& iv)
	{
		if (iv.empty() || (_max < iv._min) || (iv._max < _min))
		{
			_min = chr::Numerics<T>::next(_max);
			return true;
		} else if ((iv._min <= _min) && (_max <= iv._max)) {
			return false;
		} else {
			_min = std::max(_min, iv._min);
			_max = std::min(_max, iv._max);
			return true;
		}
	}

	template< typename T >
	bool Interval< T, true >::minus(const chr::Interval<T, true>& iv)
	{
		if (iv.empty() || (_max < iv._min) || (iv._max < _min))
		{
			return false;
		} else if ((iv._min <= _min) && (_max <= iv._max)) {
			_max = chr::Numerics<T>::previous(_min);
			return true;
		} else if ((_min < iv._min) && (iv._max < _max)) {
			return false;
		} else if (_min <= iv._min) {
			assert(_max <= iv._max);
			_max = chr::Numerics<T>::previous(iv._min);
			return true;
		} else {
			assert((iv._min <= _min) && (iv._max <= _max));
			_min = chr::Numerics<T>::next(iv._max);
			return true;
		}
	}

	/*
	 * Interval Not range ::Range
	 */
	template< typename T >
	std::string Interval< T, false >::Range::to_string() const
	{
		std::string str;
		str += "[" + chr::TIW::to_string(_min) + ";" + chr::TIW::to_string(_max) + "]";
		return str;
	}

	/*
	 * Interval Not range
	 */
	template< typename T >
	Interval< T, false >::Interval()
	{
		_range_list.insert(Range(chr::Numerics<T>::next(0),0));
	}

	template< typename T >
	Interval< T, false >::Interval(T min, T max)
	{
		assert(min <= max);
		_range_list.insert(Range(min,max));
	}

	template< typename T >
	Interval< T, false >::Interval(const Interval& o, T offset, bool opposite)
	{
		if (opposite)
		{
			PID_t p_o = o._range_list._first;
			while (p_o != END_LIST)
			{
				Range r(-o._range_list.get(p_o)._max+offset,-o._range_list.get(p_o)._min+offset);
				_range_list.insert(r);
				p_o = o._range_list.data(p_o)._next;
			}
		} else {
			PID_t p_o = o._range_list._last;
			while (p_o != END_LIST)
			{
				Range r(o._range_list.get(p_o)._min+offset,o._range_list.get(p_o)._max+offset);
				_range_list.insert(r);
				p_o = o._range_list.data(p_o)._prev;
			}
		}
	}

	template< typename T >
	void Interval< T, false >::append_range(T r_min, T r_max)
	{
		assert(r_min <= r_max);
		assert(empty() || (max() <= r_min));
		if (empty()) {
			// Clear list (still backtrackable)
			PID_t p_prev, p = _range_list._first;
			while (p != END_LIST)
			{
				p_prev = p;
				p = _range_list.data(p)._next;
				_range_list.remove(p_prev);
			}
			_range_list.insert(Range(r_min,r_max));
		} else if (max() == r_min)
			_range_list.replace(_range_list._last,
								Range(_range_list.get(_range_list._last)._min,r_max));
		else
			_range_list.insert_before(chr::Bt_list<Range>::END_LIST, Range(r_min,r_max));
	}

	template< typename T >
	bool Interval< T, false >::rewind(Depth_t previous_depth, Depth_t new_depth)
	{
		return _range_list.rewind(previous_depth, new_depth);
	}

	template< typename T >
	bool Interval< T, false >::operator==(const Interval& o) const
	{
		auto p = const_cast< chr::Bt_list<Range>* >(&_range_list)->_first;
		auto po = const_cast< chr::Bt_list<Range>* >(&o._range_list)->_first;
		if (empty() && o.empty()) return true;
	
		while ( (p != END_LIST) &&
				(po != END_LIST) )
		{
			if (!(_range_list.get(p) == o._range_list.get(po))) return false; 
			p = _range_list.data(p)._next;
			po = o._range_list.data(po)._next;
		}
		return ((p == END_LIST) && (po == END_LIST));
	}

	template< typename T >
	std::string Interval< T, false >::to_string() const
	{
		auto p = const_cast< chr::Bt_list<Range>* >(&_range_list)->_first;
		if (empty()) // Empty interval
		{
			return "[/]";
		} else if (_range_list.size() == 1) // no holes
		{
			return _range_list.get(p).to_string();
		} else {
			std::string str;
			bool first = true;
			#ifndef NDEBUG
			auto pprev = p;
			#endif
			str += "{";
			assert((max() < min()) || (min() == _range_list.get(p)._min)); // Empty or well-formed
			while (p != END_LIST)
			{
				if (!first) str += ",";
				str +=  _range_list.get(p).to_string();
				first = false;
				#ifndef NDEBUG
				pprev = p;
				#endif
				p = _range_list.data(p)._next;
			}
			#ifndef NDEBUG
			assert((max() < min()) || (max() == _range_list.get(pprev)._max)); // Empty or well-formed
			#endif
			str += "}";
			return str;
		}
	}

	template< typename T >
	typename Interval< T, false >::const_iterator Interval< T, false >::begin() const
	{
		if (empty())
			return Interval< T, false >::const_iterator(_range_list, chr::Numerics<T>::next(_range_list.get(_range_list._last)._max), END_LIST);
		else
			return Interval< T, false >::const_iterator(_range_list, min(), _range_list._first);
	}

	template< typename T >
	typename Interval< T, false >::const_iterator Interval< T, false >::end() const
	{
		return Interval< T, false >::const_iterator(_range_list, chr::Numerics<T>::next(_range_list.get(_range_list._last)._max), END_LIST);
	}

	template< typename T >
	typename Interval< T, false >::const_range_iterator Interval< T, false >::range_begin() const
	{
		if (empty())
			return Interval< T, false >::const_range_iterator(_range_list, END_LIST);
		else
			return Interval< T, false >::const_range_iterator(_range_list,  _range_list._first);
	}

	template< typename T >
	typename Interval< T, false >::const_range_iterator Interval< T, false >::range_end() const
	{
		return Interval< T, false >::const_range_iterator(_range_list, END_LIST);
	}

	template< typename T >
	T Interval< T, false >::min() const
	{
		assert(!empty());
		return _range_list.get(_range_list._first)._min;
	}

	template< typename T >
	T Interval< T, false >::max() const
	{
		assert(!empty());
		return _range_list.get(_range_list._last)._max;
	}

	template< typename T >
	bool Interval< T, false >::closer_min(T n) const
	{
		T l = n - min();
		T r = max() - n;
		return l < r;
	}

	template< typename T >
	T Interval< T, false >::val() const
	{
		assert(min() == max());
		return _range_list.get(_range_list._first)._max;
	}

	template< typename T >
	size_t Interval< T, false >::width() const
	{
		assert(!empty());
		return static_cast<size_t>(chr::Numerics<T>::next(max()) - min());
	}

	template< typename T >
	size_t Interval< T, false >::count() const
	{
		assert(!empty());
		if constexpr (chr::Numerics<T>::countable)
		{
			size_t nb = 0;
			auto p = const_cast< chr::Bt_list<Range>* >(&_range_list)->_first;
			while (p != END_LIST)
			{
				nb += static_cast<size_t>(chr::Numerics<T>::next(_range_list.get(p)._max) - _range_list.get(p)._min);
				p = _range_list.data(p)._next;
			}
			return nb;
		} else {
			if (singleton()) return 1;
			else return std::numeric_limits<size_t>::infinity();
		}
	}

	template< typename T >
	bool Interval< T, false >::empty() const
	{
		// If empty, we have a special reverse range at first
		return (_range_list.get(_range_list._first)._max < _range_list.get(_range_list._first)._min);
	}

	template< typename T >
	bool Interval< T, false >::range() const
	{
		assert(!empty());
		return (_range_list._first == _range_list._last);
	}

	template< typename T >
	bool Interval< T, false >::singleton() const
	{
		return (min() == max());
	}

	template< typename T >
	bool Interval< T, false >::in(T n) const
	{
		assert(!empty());
		if ((n < min()) || (n > max()))
			return false;
		else if (range())
			return true;
		else if (closer_min(n))
		{
			PID_t p = _range_list._first;
			while (n > _range_list.get(p)._max)
				p = _range_list.data(p)._next;
			return (n >= _range_list.get(p)._min);
		} else {
			PID_t p = _range_list._last;
			while (n < _range_list.get(p)._min)
				p = _range_list.data(p)._prev;
			return (n <= _range_list.get(p)._max);
		}
	}

	template< typename T >
	bool Interval< T, false >::eq(T n)
	{
		assert(!empty());
		if ((n == min()) && (n == max())) return false;
		if ((n < min()) || (n > max()))
		{
			_range_list.insert(Range(chr::Numerics<T>::next(n),n));
			return true;
		}
		if (range())
		{
			_range_list.replace(_range_list._first, Range(n, n)); 
			return true;
		}
		// Not range
		while (n > _range_list.get(_range_list._first)._max)
			_range_list.remove(_range_list._first);

		if (n < _range_list.get(_range_list._first)._min)
		{
			// Value not in interval
			_range_list.insert(Range(chr::Numerics<T>::next(n),n));
		} else {
			while (_range_list._first != END_LIST)
				_range_list.remove(_range_list._first);
			_range_list.insert(Range(n, n)); 
		}
		return true;
	}

	template< typename T >
	bool Interval< T, false >::nq(T n)
	{
		if ((n < min()) || (n > max())) return false;
		if (range())
		{
			if ((n == min()) && (n == max()))
				_range_list.insert(Range(chr::Numerics<T>::next(n),n));
			else if (n == min())
				_range_list.replace(_range_list._first, Range(chr::Numerics<T>::next(n), max())); 
			else if (n == max())
				_range_list.replace(_range_list._first, Range(min(), chr::Numerics<T>::previous(n))); 
			else {
				auto o_min = min();
				auto o_max = max();
				_range_list.remove(_range_list._first);
				_range_list.insert(Range(chr::Numerics<T>::next(n), o_max)); 
				_range_list.insert(Range(o_min, chr::Numerics<T>::previous(n))); 
			}
			return true;
		} else {
			// Not range, search for the concerned range in the list of ranges
			auto p = END_LIST;
			if (closer_min(n))
			{
				p = _range_list._first;
				while (n > _range_list.get(p)._max)
					p = _range_list.data(p)._next;

				if (n < _range_list.get(p)._min)
				{
					// Value not in interval, already different from n
					return false;
				} else {
					if (n == _range_list.get(p)._min)
					{
						if (n == _range_list.get(p)._max)
						{
							// Remove range as it contained only one value (min == max == n)
							_range_list.remove(p); 
							if (_range_list.empty())
								_range_list.insert(Range(chr::Numerics<T>::next(n),n));
						} else {
							_range_list.replace(p, Range(chr::Numerics<T>::next(n), _range_list.get(p)._max)); 
						}
					} else if (n == _range_list.get(p)._max)
						_range_list.replace(p, Range(_range_list.get(p)._min, chr::Numerics<T>::previous(n))); 
					else {
						_range_list.insert_before(p, Range(_range_list.get(p)._min, chr::Numerics<T>::previous(n)));
						_range_list.replace(p, Range(chr::Numerics<T>::next(n), _range_list.get(p)._max));
					}

					return true;
				}
			} else {
				p = _range_list._last;
				while (n < _range_list.get(p)._min)
					p = _range_list.data(p)._prev;

				if (n > _range_list.get(p)._max)
				{
					// Value not in interval, already different from n
					return false;
				} else {
					if (n == _range_list.get(p)._max)
					{
						if (n == _range_list.get(p)._min)
						{
							_range_list.remove(p); 
							if (_range_list.empty())
								_range_list.insert(Range(chr::Numerics<T>::next(n),n));
						} else {
							_range_list.replace(p, Range(_range_list.get(p)._min, chr::Numerics<T>::previous(n))); 
						}
					} else if (n == _range_list.get(p)._min)
						_range_list.replace(p, Range(chr::Numerics<T>::next(n), _range_list.get(p)._max)); 
					else {
						_range_list.insert_before(p, Range(_range_list.get(p)._min, chr::Numerics<T>::previous(n)));
						_range_list.replace(p, Range(chr::Numerics<T>::next(n), _range_list.get(p)._max));
					}

					return true;
				}
			}
		}
	}

	template< typename T >
	bool Interval< T, false >::nq(typename chr::Interval<T, false>::const_range_iterator& it, T n)
	{
		assert(&it._range_list == &_range_list);
		const Range& r = *it;
		if ((n < r._min) || (n > r._max)) return false;
		PID_t p_next = _range_list.data(it._range_pid)._next;

		if ((n == r._min) && (n == r._max)) {
			_range_list.remove(it._range_pid);
			if (_range_list._first == END_LIST)
				_range_list.insert(Range(n, chr::Numerics<T>::previous(n)));
			it._range_pid = p_next;
			return true;
		} else if (n == r._min)
			_range_list.replace(it._range_pid,Range(chr::Numerics<T>::next(r._min), r._max));
		else if (n == r._max) {
			_range_list.replace(it._range_pid,Range(r._min, chr::Numerics<T>::previous(r._max)));
			it._range_pid = p_next;
			return true;
		} else {
			_range_list.insert_before(it._range_pid,Range(r._min, chr::Numerics<T>::previous(n)));
			_range_list.replace(it._range_pid,Range(chr::Numerics<T>::next(n), r._max));
		}

		if (p_next == END_LIST)
			it._range_pid = _range_list._last;
		else
			it._range_pid = _range_list.data(p_next)._prev;

		return true;
	}

	template< typename T >
	bool Interval< T, false >::lq(T n)
	{
		if (n >= max()) return false;
		else if (n < min())
		{
			_range_list.insert(Range(chr::Numerics<T>::next(n),n));
			return true;
		}
		else if (range())
		{
			_range_list.replace(_range_list._first, Range(min(), n)); 
			return true;
		}
	
		// Remove elements after n
		while (n < _range_list.get(_range_list._last)._min)
			_range_list.remove(_range_list._last);

		if (n < _range_list.get(_range_list._last)._max)
			_range_list.replace(_range_list._last, Range(_range_list.get(_range_list._last)._min, n));

		return true;
	}

	template< typename T >
	bool Interval< T, false >::gq(T n)
	{
		assert(!empty());
		if (n <= min()) return false;
		else if (n > max())
		{
			_range_list.insert(Range(chr::Numerics<T>::next(n),n));
			return true;
		}
		else if (range())
		{
			_range_list.replace(_range_list._first, Range(n, max())); 
			return true;
		}
	
		// Remove elements before n
		while (n > _range_list.get(_range_list._first)._max)
			_range_list.remove(_range_list._first);

		if (n > _range_list.get(_range_list._first)._min)
			_range_list.replace(_range_list._first, Range(n, _range_list.get(_range_list._first)._max));

		return true;
	}

	template< typename T >
	bool Interval< T, false >::remove(typename chr::Interval<T, false>::const_range_iterator& it)
	{
		assert(!empty());
		assert(&it._range_list == &_range_list);
		assert(it._range_pid != END_LIST);
		PID_t p_next = _range_list.data(it._range_pid)._next;
		_range_list.remove(it._range_pid);
		if (_range_list._first == END_LIST)
			_range_list.insert(Range(chr::Numerics<T>::next(0),0));
		it._range_pid = p_next;
		return true;
	}

	template< typename T >
	bool Interval< T, false >::narrow(const chr::Interval<T, false>& iv)
	{
		assert(!empty());
		if (iv.empty() || (max() < iv.min()) || (iv.max() < min()))
		{
			_range_list.insert(Range(chr::Numerics<T>::next(max()),max()));
			return true;
		} else if (range() && iv.range())
		{
			if ((iv.min() <= min()) && (max() <= iv.max()))
				return false;
			else {
				_range_list.insert(Range(std::max(min(), iv.min()), std::min(max(), iv.max()))); 
				_range_list.remove(_range_list._last);
				return true;
			}
		} else {
			assert(!iv.empty());
			bool modified = false;

			// Do gq (iv.min() >= this->min()) 
			auto iv_min = iv.min();
			while (iv_min > _range_list.get(_range_list._first)._max)
			{
				modified = true;
				_range_list.remove(_range_list._first);
			}

			// Do lq (iv.max() <= this->max()) 
			auto iv_max = iv.max();
			while ((_range_list._last != END_LIST) && (iv_max < _range_list.get(_range_list._last)._min))
			{
				modified = true;
				_range_list.remove(_range_list._last);
			}

			auto p_this = _range_list._first;
			auto p_iv = iv._range_list._first;

			while (p_this != END_LIST)
			{
				assert(p_iv != END_LIST);
				auto p_this_min = _range_list.get(p_this)._min;
				auto p_this_max = _range_list.get(p_this)._max;

				// Search for first range which is >= p_this->_min
				while (p_this_min > iv._range_list.get(p_iv)._max)
					p_iv = iv._range_list.data(p_iv)._next;

				// Do the intersection with p_this
				while ((p_iv != END_LIST) && (p_this_max >= iv._range_list.get(p_iv)._max))
				{
					_range_list.insert_before(p_this, Range(std::max(p_this_min, iv._range_list.get(p_iv)._min), iv._range_list.get(p_iv)._max)); 
					p_iv = iv._range_list.data(p_iv)._next;
				}

				if (p_iv != END_LIST)
				{
					assert(p_this_max < iv_max);
					if (_range_list.get(p_this)._max >= iv._range_list.get(p_iv)._min)
						_range_list.insert_before(p_this, Range(std::max(p_this_min, iv._range_list.get(p_iv)._min), std::min(iv._range_list.get(p_iv)._max,p_this_max))); 
				}

				// We go for next range of this
				auto p_backup = p_this;
				p_this = _range_list.data(p_this)._next;
				modified |= ((_range_list.data(p_backup)._prev == END_LIST) || !(_range_list.get(p_backup) == _range_list.get( _range_list.data(p_backup)._prev )));
				_range_list.remove(p_backup);
			}

			// Empty interval
			if (_range_list._first == END_LIST)
			{
				assert(modified);
				_range_list.insert(Range(chr::Numerics<T>::next(iv_max),iv_max));
			}

			return modified;
		}
	}

	template< typename T >
	bool Interval< T, false >::narrow(typename chr::Interval<T, false>::const_range_iterator& it, T min, T max)
	{
		assert(!empty());
		assert(&it._range_list == &_range_list);
		assert(min <= max);
		const Range& r = *it;
		if ((min > r._min) || (max < r._max))
		{
			PID_t p_next = _range_list.data(it._range_pid)._next;
			_range_list.replace(it._range_pid,Range(std::max(min,r._min),std::min(max,r._max)));
			if (p_next == END_LIST)
				it._range_pid = _range_list._last;
			else
				it._range_pid = _range_list.data(p_next)._prev;
			return true;
		}
		return false;
	}

	template< typename T >
	bool Interval< T, false >::minus(const chr::Interval<T, false>& iv)
	{
		assert(!empty());
		if (iv.empty() || (max() < iv.min()) || (iv.max() < min()))
		{
			return false;
		} else if (range() && iv.range())
		{
			if ((min() < iv.min()) && (iv.max() < max()))
			{
				auto tmp_min = min();
				auto tmp_max = max();
				_range_list.insert(Range(chr::Numerics<T>::next(iv.max()), tmp_max)); 
				_range_list.insert(Range(tmp_min, chr::Numerics<T>::previous(iv.min()))); 
				_range_list.remove(_range_list._last);
			} else if (max() <= iv.max()) {
				if (iv.min() <= min()) {// Empty
					_range_list.insert(Range(chr::Numerics<T>::next(iv.max()),iv.max()));
					_range_list.remove(_range_list._last);
				} else {
					_range_list.insert(Range(min(),chr::Numerics<T>::previous(iv.min())));
					_range_list.remove(_range_list._last);
				}
			} else {
				_range_list.insert(Range(chr::Numerics<T>::next(iv.max()),max()));
				_range_list.remove(_range_list._last);
			}
			return true;
		} else {
			assert(!iv.empty());
			bool modified = false;

			auto p_this = _range_list._first;
			auto p_iv = iv._range_list._first;

			while ((p_this != END_LIST) && (p_iv != END_LIST))
			{
				// Search for first range which is >= p_this->_min
				auto p_this_min = _range_list.get(p_this)._min;
				auto p_this_max = _range_list.get(p_this)._max;
				while ((p_iv != END_LIST) && (p_this_min > iv._range_list.get(p_iv)._max))
					p_iv = iv._range_list.data(p_iv)._next;
				if (p_iv == END_LIST) continue;

				// Remove values from p_this
				auto pending_min = p_this_min;
				while ((p_iv != END_LIST) && (p_this_max >= iv._range_list.get(p_iv)._min))
				{
					if (pending_min < iv._range_list.get(p_iv)._min)
					{
						modified = true;
						_range_list.insert_before(p_this,Range(pending_min,std::min(chr::Numerics<T>::previous(iv._range_list.get(p_iv)._min),p_this_max)));
					}

					pending_min = chr::Numerics<T>::next(iv._range_list.get(p_iv)._max);
					if (iv._range_list.get(p_iv)._max > p_this_max)
						break;

					p_iv = iv._range_list.data(p_iv)._next;
				}

				if ((pending_min != p_this_min) && (pending_min <= p_this_max))
				{
					modified = true;
					_range_list.insert_before(p_this, Range(pending_min, p_this_max));
				}

				// We go for next range of this
				auto p_backup = p_this;
				p_this = _range_list.data(p_this)._next;
				if (pending_min != p_this_min)
				{
					modified = true;
					_range_list.remove(p_backup);
				}
			}

			// Empty interval
			if (_range_list._first == END_LIST)
			{
				assert(modified);
				_range_list.insert(Range(chr::Numerics<T>::next(iv.max()),iv.max()));
			}

			return modified;
		}
	}
}
