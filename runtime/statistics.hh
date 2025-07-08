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

#ifndef RUNTIME_STATISTICS_HH_
#define RUNTIME_STATISTICS_HH_

#include <chrono>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <string>

#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS) && defined(__unix__)
#include <sys/resource.h>
#elif defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS) && defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

namespace chr {

#ifdef ENABLE_STATISTICS
	#define STATS(X) X
#else
	#define STATS(X)
#endif


	/**
	 * \defgroup Trace_statistics Statistics and traces over runtime program execution
	 *
	 * Statistics are gathered during prorgam execution in order to be printed at the
	 * end. Trace statements are set by user to see the CHR events while program is run.
	 */

	/**
	 * Convert a size \a s given in octet to Ko, Mo or Go, depending
	 * on the second parameter \a unit
	 * @param s The size to convert
	 * @param unit The unit to use (ko, mo, go), if empty, it convert to the more relevant size
	 * @return The string corresponding to the size conversion (with unit at the end)
	 * \ingroup Trace_statistics
	 */
	inline std::string size_to_unit(size_t s, std::string unit = std::string())
	{
		unsigned int idx = 0;

		if (unit.empty())
		{
			size_t s2 = s;
			unit = "o";
			while( (idx < 3) && ((s2 /= 1024) > 0) )
			{
				switch (idx)
				{
				case 0:
					unit = "ko";
					break;
				case 1:
					unit = "mo";
					break;
				case 2:
					unit = "go";
					break;
				}
				++idx;
			}
		}
		else if (unit == "o")
			idx = 0;
		else if (unit == "ko")
			idx = 1;
		else if (unit == "mo")
			idx = 2;
		else if (unit == "go")
			idx = 3;

		while (idx > 0)
		{
			s /= 1024;
			--idx;
		}
		return std::to_string(s) + " " + unit;
	}

	/**
	 * @brief Statistics
	 *
	 * Collect data over runtime program execution. For example, elapsed time,
	 * maximum memory amount, number of choices, etc.
	 * The template parameter is only here to allow static initialization
	 * in a .hh file (useful tip).
	 * \ingroup Trace_statistics
	 */
	template < typename T >
	class Statistics_t {
	public:
		/**
		 * List of different categories for memory statistics
		 */
		enum : unsigned int
		{
			VARIABLE			= 0,	///< Consider memory allocated for logical variables
			CONSTRAINT_STORE	= 1,	///< Consider memory allocated for constraint stores
			HISTORY			 	= 2,	///< Consider memory allocated for constraint History(s)
			OTHER			 	= 3		///< Consider memory allocated for other bt_list (like Intervals)
		};

		/**
		 * Clear all statistics gather until now
		 */
		static void clear()
		{
			runtime = std::chrono::duration_values<std::chrono::milliseconds>::zero();
			peak_depth = 0;
			nb_failures = 0;
			nb_choices = 0;
			nb_rules = 0;
			peak_variables_memory = 0;
			total_variables_memory = 0;
			peak_store_memory = 0;
			peak_history_memory = 0;
			peak_other_memory = 0;
			cur_variables_memory = 0;
			cur_store_memory = 0;
			cur_history_memory = 0;
			cur_other_memory = 0;
			top_of_call_stack = nullptr;
			peak_call_stack = 0;
			system_call_stack_size = 0;
		}

		/**
		 * Start a fresh new clock for computing execution time.
		 */
		static void start_clock()
		{
#ifdef ENABLE_STATISTICS
			runtime = std::chrono::duration_values<std::chrono::milliseconds>::zero();
			start = std::chrono::steady_clock::now();
#endif
		}

		/**
		 * Stop the clock for computing execution time and add elapsed time to runtime.
		 */
		static void stop_clock()
		{
#ifdef ENABLE_STATISTICS
			end = std::chrono::steady_clock::now();
			runtime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
#endif
		}

		/**
		 * Continue the previous clock for computing execution time.
		 */
		static void resume_clock()
		{
#ifdef ENABLE_STATISTICS
			start = std::chrono::steady_clock::now();
#endif
		}

#ifdef ENABLE_STATISTICS
		/**
		 * Update the peak search depth according to \a d
		 * @param d The current depth
		 */
		static void update_peak_depth(unsigned int d)
		{
			if (d > peak_depth)
				peak_depth = d;
		}

		/**
		 * Increase the number of rules by \a n.
		 * @param n The number of rules to add
		 */
		static void inc_nb_rules(unsigned int n = 1)
		{
			nb_rules += n;
		}

		/**
		 * Increase the number of failures by \a n.
		 * @param n The number of failures to add
		 */
		static void inc_nb_failures(unsigned int n = 1)
		{
			nb_failures += n;
		}

		/**
         * Open a new choice (subtree) in the search tree.
		 */
        static void open_choice()
		{
            ++nb_choices;
		}

		/**
         * Close a new choice (subtree) in the search tree.
		 */
        static void close_choice()
		{ }
#else
		/**
		 * Update the peak search depth.
		 */
		static void update_peak_depth(unsigned int)
		{ }

		/**
		 * Increase the number of rules.
		 */
		static void inc_nb_rules(unsigned int = 1)
		{ }

		/**
		 * Increase the number of failures.
		 */
		static void inc_nb_failures(unsigned int = 1)
		{ }

		/**
         * Open a new choice (subtree) in the search tree.
		 */
        static void open_choice()
		{ }

		/**
         * Close a new choice (subtree) in the search tree.
		 */
        static void close_choice()
		{ }
#endif

		/**
		 * Increase the size of the memory used by either by the variables,
		 * either the constraint store, either the history.
		 */
		template < const unsigned int TYPE >
		static void inc_memory(size_t) { }

		/**
		 * Decrease the size of the memory used by either by the variables,
		 * either the constraint store, either the history.
		 */
		template < const unsigned int TYPE >
		static void dec_memory(size_t) { }

#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
		/**
		 * Update the size of the call stack.
		 */
		static void update_call_stack()
		{
			uint8_t x = 0;
			if (top_of_call_stack != nullptr)
				peak_call_stack = std::max((size_t)(top_of_call_stack - &x), peak_call_stack);
			if ((system_call_stack_size > 0) && ((system_call_stack_size - peak_call_stack) < 100000 ))
			{
				std::cerr << "[CHR] Abort, remaining call stack size is less than 100ko." << std::endl;
				abort();
			}
		}
#else
		/**
		 * Update the size of the call stack.
		 */
		static void update_call_stack()
		{ }
#endif

		/**
		 * Return the elapsed running time from the first start of the chrono
		 * @return The current runtime
		 */
		static std::chrono::milliseconds get_runtime_from_start()
		{
#ifdef ENABLE_STATISTICS
			return std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - start);
#else
			return std::chrono::duration_values< std::chrono::milliseconds >::zero();
#endif
		}

		/**
		 * Return a string that contains all attributes in
		 * a human reading form.
		 * @return A string representation of statistics
         */
		static std::string to_string()
		{
			std::string str;
#ifdef ENABLE_STATISTICS
			str += "(runtime," + std::to_string(runtime.count()) + ")";
			str += ",(failures," + std::to_string(nb_failures) + ")";
			str += ",(nb_choices," + std::to_string(nb_choices) + ")";
			str += ",(peak_depth," + std::to_string(peak_depth) + ")";
			str += ",(nb_rules," + std::to_string(nb_rules) + ")";
#endif
#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			str += ",(peak_variables_memory," + std::to_string(peak_variables_memory) + ")";
			str += ",(total_variables_memory," + std::to_string(total_variables_memory) + ")";
			str += ",(peak_store_memory," + std::to_string(peak_store_memory) + ")";
			str += ",(peak_history_memory," + std::to_string(peak_history_memory) + ")";
			str += ",(peak_other_memory," + std::to_string(peak_other_memory) + ")";
			str += ",(peak_system_call_stack," + std::to_string(peak_call_stack) + ")";
#endif
			return str;
		}


#ifdef ENABLE_STATISTICS
		/**
		 * Print the statistics to an output stream.
		 * @param out The output stream
		 */
		static void print(std::ostream& out)
		{
			const unsigned int f1 = 20, f2 = 10;
			out << std::endl;
			out << "Summary" << std::endl;
			out << std::setw(f1) << std::left << "  runtime:";
			std::chrono::duration<double> runtime_s = runtime;
			out << std::setw(f2) << std::right << runtime_s.count();
			out << " (" << runtime.count() << " ms)" << std::endl;
			out << std::setw(f1) << std::left << "  failures:";
			out << std::setw(f2) << std::right << nb_failures << std::endl;
			out << std::setw(f1) << std::left << "  nodes:";
			out << std::setw(f2) << std::right << nb_choices << std::endl;
			out << std::setw(f1) << std::left << "  peak depth:";
			out << std::setw(f2) << std::right << peak_depth << std::endl;
			out << std::setw(f1) << std::left << "  triggered rules:";
			out << std::setw(f2) << std::right << nb_rules << std::endl;
#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			const unsigned int f3 = 40;
			out << std::setw(f3) << std::left << "  peak variables memory:";
			out << std::setw(f2) << std::right << size_to_unit(peak_variables_memory) << std::endl;
			out << std::setw(f3) << std::left << "  total allocated for variables memory:";
			out << std::setw(f2) << std::right << size_to_unit(total_variables_memory) << std::endl;
			out << std::setw(f3) << std::left << "  peak constraint stores memory:";
			out << std::setw(f2) << std::right << size_to_unit(peak_store_memory) << std::endl;
			out << std::setw(f3) << std::left << "  peak history memory:";
			out << std::setw(f2) << std::right << size_to_unit(peak_history_memory) << std::endl;
			out << std::setw(f3) << std::left << "  peak other data memory:";
			out << std::setw(f2) << std::right << size_to_unit(peak_other_memory) << std::endl;
			out << std::setw(f3) << std::left << "  peak system call stack:";
			out << std::setw(f2) << std::right << size_to_unit(peak_call_stack) << std::endl;
#endif
		}
#else
		/**
		 * Print the statistics to an output stream.
		 */
		static void print(std::ostream&)
		{ }
#endif

	private:
		static std::chrono::milliseconds runtime;		///< Runtime in milliseconds
		static unsigned long int nb_choices;			///< Total number choices
		static unsigned long int peak_depth;			///< Maximum number of choices among all CHR branches
		static unsigned long int nb_failures;			///< Number of failures
		static size_t nb_rules;							///< Number of applied rules
		static size_t peak_variables_memory;			///< Maximum memory used by variables
		static size_t total_variables_memory;			///< Total memory allocated for variables
		static size_t peak_store_memory;				///< Maximum memory used by constraint stores
		static size_t peak_history_memory;				///< Maximum memory used by history
		static size_t peak_other_memory;				///< Maximum memory used by other data structures (as Interval)

		static std::chrono::steady_clock::time_point start;	///< Start of the clock
		static std::chrono::steady_clock::time_point end;	///< End of the clock


		static size_t cur_variables_memory;				///< Current memory used by variables
		static size_t cur_store_memory;					///< Current memory used by constraint stores
		static size_t cur_history_memory;				///< Current memory used by history
		static size_t cur_other_memory;					///< Current memory used by other data 

		static size_t peak_call_stack;					///< Maximun computed call stack size

	public:
		static uint8_t* top_of_call_stack;				///< Adress of the top of the call stack
		static size_t system_call_stack_size;			///< Size of system call stack
	};

	// Initialization of static members
	template< typename T >
	std::chrono::steady_clock::time_point chr::Statistics_t<T>::start;
	template< typename T >
	std::chrono::steady_clock::time_point chr::Statistics_t<T>::end;
	template< typename T >
	std::chrono::milliseconds chr::Statistics_t<T>::runtime = std::chrono::duration_values<std::chrono::milliseconds>::zero();
	template< typename T >
	unsigned long int chr::Statistics_t<T>::peak_depth = 0;
	template< typename T >
	unsigned long int chr::Statistics_t<T>::nb_failures = 0;
	template< typename T >
	unsigned long int chr::Statistics_t<T>::nb_choices = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::nb_rules = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::peak_variables_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::total_variables_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::peak_store_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::peak_history_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::peak_other_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::cur_variables_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::cur_store_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::cur_history_memory = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::cur_other_memory = 0;
	template< typename T >
	uint8_t* chr::Statistics_t<T>::top_of_call_stack = nullptr;
	template< typename T >
	size_t chr::Statistics_t<T>::peak_call_stack = 0;
	template< typename T >
	size_t chr::Statistics_t<T>::system_call_stack_size = 0;

	// Dummy class to get rid off the template parameter
	struct Dummy_statistics { };

#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::inc_memory< chr::Statistics_t<Dummy_statistics>::VARIABLE >(size_t s)
	{
		cur_variables_memory += s;
		total_variables_memory += s;
		if (cur_variables_memory > peak_variables_memory)
			peak_variables_memory = cur_variables_memory;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::dec_memory< chr::Statistics_t<Dummy_statistics>::VARIABLE >(size_t s)
	{
		cur_variables_memory -= s;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::inc_memory< chr::Statistics_t<Dummy_statistics>::CONSTRAINT_STORE >(size_t s)
	{
		cur_store_memory += s;
		if (cur_store_memory > peak_store_memory)
			peak_store_memory = cur_store_memory;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::dec_memory< chr::Statistics_t<Dummy_statistics>::CONSTRAINT_STORE >(size_t s)
	{
		cur_store_memory -= s;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::inc_memory< chr::Statistics_t<Dummy_statistics>::HISTORY >(size_t s)
	{
		cur_history_memory += s;
		if (cur_history_memory > peak_history_memory)
			peak_history_memory = cur_history_memory;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::dec_memory< chr::Statistics_t<Dummy_statistics>::HISTORY >(size_t s)
	{
		cur_history_memory -= s;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::dec_memory< chr::Statistics_t<Dummy_statistics>::OTHER >(size_t s)
	{
		cur_other_memory -= s;
	}

	template<>
	template<>
	inline void chr::Statistics_t<Dummy_statistics>::inc_memory< chr::Statistics_t<Dummy_statistics>::OTHER >(size_t s)
	{
		cur_other_memory += s;
		if (cur_other_memory > peak_other_memory)
			peak_other_memory = cur_other_memory;
	}
#endif

	// Alias to get rid off the template parameter when calling Statistics
	// class.
	using Statistics = Statistics_t< Dummy_statistics >;
}


#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS) && defined(__unix__)
#define CHR_RUN( X ) \
		{ \
			chr::Statistics::resume_clock(); \
			auto lambda_chr_run = [&]() \
			{ \
				struct rlimit limit; \
				getrlimit (RLIMIT_STACK, &limit); \
				chr::Statistics::system_call_stack_size = limit.rlim_cur; \
				std::cout << "[CHR] Maximum stack size = " << chr::size_to_unit(chr::Statistics::system_call_stack_size) << std::endl; \
				uint8_t x = 0; \
				chr::Statistics::top_of_call_stack = &x; \
				X ; \
			}; \
			lambda_chr_run(); \
			chr::Statistics::stop_clock(); \
		}
#elif defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS) && defined(_WIN32)
#define CHR_RUN( X ) \
		{ \
			chr::Statistics::resume_clock(); \
			auto lambda_chr_run = [&]() \
			{ \
				HMODULE hModule = GetModuleHandle(NULL); \
				if(hModule != NULL) \
				{ \
					IMAGE_DOS_HEADER* pDOSHeader = (IMAGE_DOS_HEADER*)hModule; \
					IMAGE_NT_HEADERS* pNTHeaders =(IMAGE_NT_HEADERS*)((BYTE*)pDOSHeader + pDOSHeader->e_lfanew); \
					chr::Statistics::system_call_stack_size = pNTHeaders->OptionalHeader.SizeOfStackReserve; \
					std::cout << "[CHR] Maximum stack size = " << chr::size_to_unit(chr::Statistics::system_call_stack_size) << std::endl; \
				} \
				uint8_t x = 0; \
				chr::Statistics::top_of_call_stack = &x; \
				X ; \
			}; \
			lambda_chr_run(); \
			chr::Statistics::stop_clock(); \
		}
#elif defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
#define CHR_RUN( X ) \
		{ \
			chr::Statistics::resume_clock(); \
			auto lambda_chr_run = [&]() \
			{ \
				uint8_t x = 0; \
				chr::Statistics::top_of_call_stack = &x; \
				X ; \
			}; \
			lambda_chr_run(); \
			chr::Statistics::stop_clock(); \
		}
#else
#define CHR_RUN( X ) \
		{ \
			chr::Statistics::resume_clock(); \
			X ; \
			chr::Statistics::stop_clock(); \
		}
#endif

#endif /* RUNTIME_STATISTICS_HH_ */
