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

#ifndef RUNTIME_INTERVAL_HH_
#define RUNTIME_INTERVAL_HH_

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

#include <utils.hpp>
#include <backtrack.hh>
#include <bt_list.hh>

// Forward declaration
namespace tests {
	namespace bt_interval {
		void basic_tests();
		void basic_tests_float();
		void backtrack_tests();
	}
}

namespace chr {
	/**
	 * Class used to get an abstraction of next() and previous() value
	 * of an arbitrary numeric value (int, float, ...)
	 */
	template< typename T >
	struct Numerics
	{ };

	/**
	 * Specialization for short int
	 */
	template< >
	struct Numerics< short int >
	{
		/**
		 * Is template type countable
		 */
		static constexpr bool countable = true;

		/**
		 * Returns the next (short) integer value of \a n toward infty
		 * @param n The input (short) integer value 
		 * @return The next (short) integer value of \a n
		 */
		static short int next(short int n) { return n+1; }

		/**
		 * Returns the previous (short) integer value of \a n toward -infty
		 * @param n The input (short) integer value 
		 * @return The previous (short) integer value of \a n
		 */
		static short int previous(short int n) { return n-1; }
	};

	/**
	 * Specialization for int
	 */
	template< >
	struct Numerics< int >
	{
		/**
		 * Is template type countable
		 */
		static constexpr bool countable = true;

		/**
		 * Returns the next integer value of \a n toward infty
		 * @param n The input integer value 
		 * @return The next integer value of \a n
		 */
		static int next(int n) { return n+1; }

		/**
		 * Returns the previous integer value of \a n toward -infty
		 * @param n The input integer value 
		 * @return The previous integer value of \a n
		 */
		static int previous(int n) { return n-1; }
	};

	/**
	 * Specialization for long int
	 */
	template< >
	struct Numerics< long int >
	{
		/**
		 * Is template type countable
		 */
		static constexpr bool countable = true;

		/**
		 * Returns the next (long) integer value of \a n toward infty
		 * @param n The input (long) integer value 
		 * @return The next (long) integer value of \a n
		 */
		static long int next(long int n) { return n+1; }

		/**
		 * Returns the previous (long) integer value of \a n toward -infty
		 * @param n The input (long) integer value 
		 * @return The previous (long) integer value of \a n
		 */
		static long int previous(long int n) { return n-1; }
	};

	/**
	 * Specialization for float
	 */
	template< >
	struct Numerics< float >
	{
		/**
		 * Is template type countable
		 */
		static constexpr bool countable = false;

		/**
		 * Returns the next float value of \a n toward infty
		 * @param n The input float value 
		 * @return The next float value of \a n
		 */
		static float next(float n) { return std::nextafter(n, std::numeric_limits<float>::infinity()); }

		/**
		 * Returns the previous integer value of \a n toward -infty
		 * @param n The input integer value 
		 * @return The previous integer value of \a n
		 */
		static float previous(float n) { return std::nextafter(n, -std::numeric_limits<float>::infinity()); }
	};

	/**
	 * Specialization for double
	 */
	template< >
	struct Numerics< double >
	{
		/**
		 * Is template type countable
		 */
		static constexpr bool countable = false;

		/**
		 * Returns the next double value of \a n toward infty
		 * @param n The input double value 
		 * @return The next double value of \a n
		 */
		static double next(double n) { return std::nextafter(n, std::numeric_limits<double>::infinity()); }

		/**
		 * Returns the previous integer value of \a n toward -infty
		 * @param n The input integer value 
		 * @return The previous integer value of \a n
		 */
		static double previous(double n) { return std::nextafter(n, -std::numeric_limits<double>::infinity()); }
	};

	/**
	 * @brief Backtrackable interval
	 *
	 * Data structure used to represent intervals.
	 * The first template parameter define the underlying type
	 * of the bounds and values. The second template parameter
	 * specifies if the interval can handle holes inside. It RANGE
	 * is true, then no hole is allowed.
	 */
	template< typename T = int, bool RANGE = false >
	class Interval
	{ };

	/**
	 * @brief Backtrackable interval (range)
	 *
	 * An interval type formed by a lower bound and
	 * an upper bound. No hole inside so the nq()
	 * operator is unavailable.
	 */
	template <class T >
	class Interval<T, true>
	{
	private:
		/// Lower bound
		T _min;
		/// Upper bound
		T _max;	

	public:
		/**
		 * Default empty constructor
		 */
		Interval();

		/**
		 * Default constructor
		 * @param min The lower bound of the interval
		 * @param max The upper bound of the interval
		 */
		Interval(T min, T max);

		/**
		 * Copy constructor
		 * @param o The interval to copy
		 * @param offset The offset to apply
		 * @param opposite True if we want to take the opposite of o, false otherwise
		 */
		Interval(const Interval& o, T offset, bool opposite = false);

		/**
		 * Check if two intervals are equals
		 * @param o The interval to check
		 * @return True if the intervals are not equal, false otherwise
		 */
		bool operator==(const Interval& o) const;

		/**
		 * Check if two intervals are equals
		 * @param o The interval to check
		 * @return True if the intervals are not equal, false otherwise
		 */
		bool operator!=(const Interval& o) const { return !(*this == o); }

		/// \name Value access
		//@{
		/**
		 * Return minimum of the interval
		 * @return The lower bound
		 */
		T min() const;

		/**
		 * Return maximum of the interval
		 * @return The upper bound
		 */
		T max() const;

		/**
		 * Return assigned value (only if assigned)
		 * @return The value (lower bound == upper bound)
		 */
		T val() const;

		/**
		 * Return width of interval (distance between maximum and minimum)
		 * @return The width of interval
		 */
		size_t width() const;

		/**
		 * Return the number of elements in interval (count all elements)
		 * @return The number of elements
		 */
		size_t count() const;
		//@}

		/// \name Tests
		//@{
		/**
		 * Test whether the interval is empty
		 * @return True if the interval is empty, false otherwise
		 */
		bool empty() const;

		/**
		 * Test whether the interval is a range (no hole inside)
		 * @return True if the interval is a range, false otherwise
		 */
		bool range() const;

		/**
		 * Test whether the interval is a singleton (i.e. lower bound equals to
		 * upper bound)
		 * @return True if the interval is a singleton, false otherwise
		 */
		bool singleton() const;

		/**
		 * Test whether \a n is containded in the interval
		 * @return True is \a n is contained in the interval, false otherwise
		 */
		bool in(T n) const;
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Restrict this interval to \a n
		 * @param n The value to be equal
		 * @return True if the interval has been updated, false otherwise
		 */
		bool eq(T n);

		/**
		 * Remove \a n from the interval.
		 * If \a n is not equal to a bound of this interval nothing is done.
		 * @param n The value to remove
		 * @return True if the interval has been updated, false otherwise
		 */
		bool nq(T n);

		/**
		 * Restrict this interval to be less or equal than \a n
		 * @param n The value to be less or equal
		 * @return True if the interval has been updated, false otherwise
		 */
		bool lq(T n);

		/**
		 * Restrict this interval to be greater or equal than \a n
		 * @param n The value to be greater or equal
		 * @return True if the interval has been updated, false otherwise
		 */
		bool gq(T n);

		/**
		 * Narrow this interval to the intersection of this and \a iv
		 * @param iv The other interval to narrow with
		 * @return True if the interval has been updated, false otherwise
		 */
		bool narrow(const chr::Interval<T, true>& iv);

		/**
		 * Remove from this interval all elements of \a iv.
		 * If \a iv is strictly included in this interval, nothing is done.
		 * @param iv The other interval to us
		 * @return True if the interval has been updated, false otherwise
		 */
		bool minus(const chr::Interval<T, true>& iv);
		//@}


		/// \name Iterators
		//@{

		/**
		 * @brief Iterator on a backtrackable (only range) interval
		 */
		class const_iterator {
			friend class Interval<T,true>;
		public:
			/**
			 * Return the current element pointed by the iterator.
			 * @return A const reference to the element pointed
			 */
			const T operator*() const
			{
				return  _value;
			}

			/**
			 * Move the iterator to the next element.
			 * @return A reference to the current iterator
			 */
			const_iterator& operator++()
			{
				_value = chr::Numerics<T>::next(_value);
				return *this;
			}

			/**
			 * Test if two iterators are not equal.
			 * @param o The other iterator to compare with ourself
			 * @return True is the two iterators are different, false otherwise
			 */
			bool operator!=(const const_iterator& o) const
			{
				return _value != o._value;
			}

		private:
			T _value;	/// Current value pointed by the iterator

			/**
			 * Initialize.
			 * @param value Initial value of the iterator
			 */
			const_iterator(const T& value) 
				: _value(value)
			{ }
		};

		/**
		 * Build and return an const iterator on the first element
		 * of the list.
		 * @return An iterator on first element
		 */
		const_iterator begin() const { return const_iterator(min()); }

		/**
		 * Build and return an const iterator on the end of the list
		 * of the list.
		 * @return An iterator on the end of the list
		 */
		const_iterator end() const { return const_iterator(chr::Numerics<T>::next(max())); }
		//@}

		/**
		 * Return a string representation of the interval.
		 * @return A string representation of the structure of the interval
		 */
		std::string to_string() const;
	};

	/**
	  * Hash data structure needed for Interval< T, true >
	  */
	template< typename T >
		struct XXHash< Interval< T, true > >
		{
			static void update(const Interval< T, true >& iv)
			{
				XXHash<T>::update(iv._min);
				XXHash<T>::update(iv._max);
			}
		};


	// Forward declaration
	template< typename T > struct XXHash< Interval< T, false > >;

	/**
	 * @brief Backtrackable interval (with list of ranges)
	 *
	 * Template spécialization for interval with list of ranges.
	 * A backtrackable interval type composed of a list of
	 * ranges. It can have holes, the list of subsets is managed
	 * thanks to a Bt_list.
	 */
	template <class T >
	class Interval<T, false>
	{
	public:
		// Friend
		friend struct XXHash< Interval< T, false > >;
		// Friend for Unit tests
		friend void tests::bt_interval::basic_tests();
		friend void tests::bt_interval::basic_tests_float();
		friend void tests::bt_interval::backtrack_tests();
		static constexpr bool BACKTRACK_SELF_MANAGED = true;
		class const_range_iterator;
	private:
		/**
		 * Interval is composed of a list of Range
		 */
		struct Range
		{
			/// Minimum of range
			T _min;
			/// Maximum of range
			T _max;	

			/**
			 * Default constructor
			 */
			Range() { }

			/**
			 * Initialize with minimum and maximum values.
			 * @param min Minimum value
			 * @param max Maximum value
			 */
			Range(T min, T max) : _min(min), _max(max) { }
	
			/**
			 * Return minimum of the range
			 * @return The lower bound
			 */
			T min() const { return _min; }

			/**
			 * Return maximum of the range
			 * @return The upper bound
			 */
			T max() const { return _max; }

			/**
			 * Check if two range are equals
			 * @param o The interval to check
			 * @return True if the ranges are equal, false otherwise
			 */
			bool operator==(const Range& o) const { return ((_min == o._min) && (_max == o._max)); }

			/**
			 * Return a string representation of a range.
			 * @return A string representation of a range 
			 */
			std::string to_string() const;
		};

		typedef typename Bt_list<Range>::PID_t PID_t;
		static constexpr PID_t END_LIST = Bt_list<Range>::END_LIST;
	
		/**
		 * Range list
		 * The first node is a special one. It contains the lower and
		 * upper bounds values of the whole range set.
		 * If a single element is contained in the range, then
		 * the range has no hole.
		 */
		chr::Bt_list<Range> _range_list;

		/**
		 * Check if \a n is closer to the lower bound
		 * @param n The value to check
		 * @return True if \a n is closer to the lower bound, false otherwise
		 */
		bool closer_min(T n) const;

		/**
		 * Returns the first pid of the list of ranges.
		 * Used in XXHASH<> specialization.
		 * @return The pid of first list node
		 */
		PID_t first_pid() const { return _range_list._first; }

		/**
		 * Returns the pid of the next range after \a p
		 * Used in XXHASH<> specialization.
		 * @param p The pid of the node
		 * @return The pid of next list node of \a p
		 */
		PID_t next_pid(PID_t p) const { return _range_list.data(p)._next; }

	public:
		/**
		 * Default empty constructor
		 */
		Interval();

		/**
		 * Default constructor
		 * @param min The lower bound of the interval
		 * @param max The upper bound of the interval
		 */
		Interval(T min, T max);

		/**
		 * Copy constructor
		 * @param o The interval to copy
		 * @param offset The offset to apply
		 * @param opposite True if we want to take the opposite of o, false otherwise
		 */
		Interval(const Interval& o, T offset, bool opposite = false);

		/**
		 * Move constructor.
		 * @param o The other interval
		 */
		Interval(Interval&& o) : _range_list( std::move(o._range_list) ) { }

		/**
		 * Copy constructor set deleted.
		 * @param o The other interval
		 */
		Interval(const Interval& )=delete;

		/**
		 * Append a the range [min, max] at the end of the current interval.
		 * The \a min value must be superior or equal to the actual upper bound of the interval.
		 * @param min The lower bound of the range
		 * @param max The upper bound of the range
		 */
		void append_range(T min, T max);

		/**
		 * Move assignment operator.
		 * @param o The other interval
		 */
		Interval& operator=(Interval&& o) { assert(_range_list.empty()); _range_list = std::move(o._range_list); return *this; } // If _range_list not empty, we may have memory leak

		/**
		 * Check if two intervals are equals
		 * @param o The interval to check
		 * @return True if the intervals are not equal, false otherwise
		 */
		bool operator==(const Interval& o) const;

		/**
		 * Check if two intervals are equals
		 * @param o The interval to check
		 * @return True if the intervals are not equal, false otherwise
		 */
		bool operator!=(const Interval& o) const { return !(*this == o); }

		/// \name Value access
		//@{
		/**
		 * Return minimum of the interval
		 * @return The lower bound
		 */
		T min() const;

		/**
		 * Return maximum of the interval
		 * @return The upper bound
		 */
		T max() const;

		/**
		 * Return assigned value (only if assigned)
		 * @return The value (lower bound == upper bound)
		 */
		T val() const;

		/**
		 * Return width of interval (distance between maximum and minimum)
		 * @return The width of interval
		 */
		size_t width() const;

		/**
		 * Return the number of elements in interval (count all elements)
		 * @return The number of elements
		 */
		size_t count() const;
		//@}

		/// \name Tests
		//@{
		/**
		 * Test whether the interval is empty
		 * @return True if the interval is empty, false otherwise
		 */
		bool empty() const;

		/**
		 * Test whether the interval is a range (no hole inside)
		 * @return True if the interval is a range, false otherwise
		 */
		bool range() const;

		/**
		 * Test whether the interval is a singleton (i.e. lower bound equals to
		 * upper bound)
		 * @return True if the interval is a singleton, false otherwise
		 */
		bool singleton() const;

		/**
		 * Test whether \a n is containded in the interval
		 * @return True is \a n is contained in the interval, false otherwise
		 */
		bool in(T n) const;
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Restrict interval to \a n.
		 * Be careful, eq() invalidates all iterators which browses the interval.
		 * @param n The value to be equal
		 * @return True if the interval has been updated, false otherwise
		 */
		bool eq(T n);

		/**
		 * Remove \a n from the interval.
		 * Be careful, nq() invalidates all iterators which browses the interval.
		 * @param n The value to remove
		 * @return True if the interval has been updated, false otherwise
		 */
		bool nq(T n);

		/**
		 * Remove \a n from the range givent by the range iterator \a it.
		 * Be careful, nq() invalidates all iterators which browses the interval .
		 * The iterator \a it is moved to the last part of the split that occured
		 * because of nq(). For example, nq([a,b],c) will split in [a,c-1],[c+1,b] and
		 * the iterator \a it is moved to the second range [c+1,b].
		 * @param it The iterator to use
		 * @param n The value to remove
		 * @return True if the interval has been updated, false otherwise
		 */
		bool nq(const_range_iterator& it, T n);

		/**
		 * Restrict interval to be less or equal than \a n.
		 * Be careful, lq() invalidates all iterators which browses the interval.
		 * @param n The value to be less or equal
		 * @return True if the interval has been updated, false otherwise
		 */
		bool lq(T n);

		/**
		 * Restrict interval to be greater or equal than \a n.
		 * Be careful, gq() invalidates all iterators which browses the interval.
		 * @param n The value to be greater or equal
		 * @return True if the interval has been updated, false otherwise
		 */
		bool gq(T n);

		/**
		 * Narrow this interval to the intersection of this and \a iv.
		 * Be careful, narrow() invalidates all iterators which browses the interval.
		 * @param iv The other interval to narrow with
		 * @return True if the interval has been updated, false otherwise
		 */
		bool narrow(const chr::Interval<T, false>& iv);

		/**
		 * Narrow a single range \a it of this interval to the intersection of \a it and
		 * the range given by [ \a min , \a max ].
		 * The \a it range iterator is moved to the next range because \a it may become
		 * undefined if the underlying range is updated.
		 * @param it The range to narrow (moved to the next range during the call)
		 * @param min The lower bound used to narrow
		 * @param max The upper bound used to narrow
		 * @return True if the interval has been updated, false otherwise
		 */
		bool narrow(const_range_iterator& it, T min, T max);

		/**
		 * Remove from this interval all elements of \a iv.
		 * Be careful, minus() invalidates all iterators which browses the interval.
		 * @param iv The other interval to us
		 * @return True if the interval has been updated, false otherwise
		 */
		bool minus(const chr::Interval<T, false>& iv);

		/**
		 * Remove this range (given by \a it) from this interval.
		 * @param it The range to remove (moved to the next range during the call)
		 * @return True if the interval has been updated, false otherwise (should be always true)
		 */
		bool remove(const_range_iterator& it);
		//@}

		/// \name Iterators
		//@{

		/**
		 * @brief Iterator on a backtrackable interval
		 *
		 * Be careful!!
		 * Don't use nq() on an interval which is browsed. The
		 * corresponding iterator will becom undefined!
		 */
		class const_iterator {
			friend class Interval<T,false>;
		public:
			/**
			 * Return the current element pointed by the iterator.
			 * @return A const reference to the element pointed
			 */
			const T operator*() const
			{
				return  _value;
			}

			/**
			 * Move the iterator to the next element.
			 * @return A reference to the current iterator
			 */
			const_iterator& operator++()
			{
				if (_value == _range_list.data(_range_pid)._e._max)
				{
					_range_pid = _range_list.data(_range_pid)._next;
					if (_range_pid == END_LIST)
						_value = chr::Numerics<T>::next(_value);
					else
						_value = _range_list.data(_range_pid)._e._min;
				} else 
					_value = chr::Numerics<T>::next(_value);
				return *this;
			}

			/**
			 * Test if two iterators are equal.
			 * @param o The other iterator to compare with ourself
			 * @return True is the two iterators are equal, false otherwise
			 */
			bool operator==(const const_iterator& o) const
			{
				assert(&_range_list == &o._range_list);
				return (_range_pid == o._range_pid) && (_value == o._value);
			}

			/**
			 * Test if two iterators are not equal.
			 * @param o The other iterator to compare with ourself
			 * @return True is the two iterators are different, false otherwise
			 */
			bool operator!=(const const_iterator& o) const
			{
				return not(*this == o);
			}

		private:
			const typename chr::Bt_list<Range>& _range_list;/// Range list to browse with this iterator
			T _value;										/// Current value pointed by the iterator
			PID_t _range_pid;								/// Current range pid pointed by the iterator

			/**
			 * Initialize.
			 * @param ranges List of ranges to browse
			 * @param value Initial value of the iterator
			 * @param pid Pid of the first range to browse
			 */
			const_iterator(const chr::Bt_list<Range>& ranges, const T& value, PID_t pid) 
				: _range_list(ranges), _value(value), _range_pid(pid)
			{ }
		};

		/**
		 * Build and return an const iterator on the first element
		 * of the list.
		 * @return An iterator on first element
		 */
		const_iterator begin() const;

		/**
		 * Build and return an const iterator on the end of the list
		 * of the list.
		 * @return An iterator on the end of the list
		 */
		const_iterator end() const;

		/**
		 * @brief Range iterator on a backtrackable interval
		 *
		 * Be careful!!
		 * Don't use nq() on an interval which is browsed. The
		 * corresponding iterator will becom undefined!
		 */
		class const_range_iterator {
			friend class Interval<T,false>;
		public:
			/**
			 * Return the current element pointed by the iterator.
			 * @return A const reference to the element pointed
			 */
			const Interval<T,false>::Range operator*() const
			{
				return  _range_list.get(_range_pid);
			}

			/**
			 * Move the iterator to the next element.
			 * @return A reference to the current iterator
			 */
			const_range_iterator& operator++()
			{
				_range_pid = _range_list.data(_range_pid)._next;
				return *this;
			}

			/**
			 * Test if two iterators are equal.
			 * @param o The other iterator to compare with ourself
			 * @return True is the two iterators are equal, false otherwise
			 */
			bool operator==(const const_range_iterator& o) const
			{
				assert(&_range_list == &o._range_list);
				return (_range_pid == o._range_pid);
			}

			/**
			 * Test if two iterators are not equal.
			 * @param o The other iterator to compare with ourself
			 * @return True is the two iterators are different, false otherwise
			 */
			bool operator!=(const const_range_iterator& o) const
			{
				return not(*this == o);
			}

		private:
			const typename chr::Bt_list<Range>& _range_list;/// Range list to browse with this iterator
			PID_t _range_pid;								/// Current range pid pointed by the iterator

			/**
			 * Initialize.
			 * @param ranges List of ranges to browse
			 * @param pid Pid of the first range to browse
			 */
			const_range_iterator(const chr::Bt_list<Range>& ranges, PID_t pid) 
				: _range_list(ranges), _range_pid(pid)
			{ }
		};

		/**
		 * Build and return an const range iterator on the first element
		 * of the list.
		 * @return An iterator on first range
		 */
		const_range_iterator range_begin() const;

		/**
		 * Build and return an const range iterator on the end of the list
		 * of the list.
		 * @return An iterator on the end of the list
		 */
		const_range_iterator range_end() const;
		//@}

		/**
		 * Return a string representation of the interval.
		 * @return A string representation of the structure of the interval
		 */
		std::string to_string() const;

		/**
		 * Rewind to the interval state of depth \a depth.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		bool rewind(Depth_t previous_depth, Depth_t new_depth);
	};
}

#include <bt_interval.hpp>

#endif /* RUNTIME_INTERVAL_HH_ */
