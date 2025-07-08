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

#ifndef RUNTIME_TRACE_HH_
#define RUNTIME_TRACE_HH_

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <string>

#include <utils.hpp>

#ifndef ENABLE_TRACE

#define TRACE( X )

#else // ENABLE_TRACE

#define TRACE( EXP ) EXP

namespace chr
{
	/**
	 * @brief Class centralizing logging/tracing facilities
	 *
	 * This class is used in CHR program to trace activity of
	 * program. Flags can be activated or disabled dynamically.
	 * The template parameter is only here to allow static initialization
	 * in a .hh file (useful tip).
	 * \ingroup Trace_statistics
	 */
	template < typename T >
	class Log_t
	{
	public:
		/**
		 * List of properties that can be activating for tracing.
		 * The first one are similar to those used in SWI prolog for
		 * tracing CHR program.
		 */
		enum : unsigned int
		{
			NONE 		= 0,	///< No flag register

			EXIT 		= 1,	///< An active constraint exits: it has either been inserted in the store after trying all rules or has been removed from the constraint store
			FAIL	 	= 2,	///< An active constraint fails
			GOAL	 	= 4,	///< A new goal constraint is called and becomes active (called from outside the CHR space)
			CALL	 	= 8,	///< A new constraint is called and becomes active
			WAKE	 	= 16,	///< A suspended constraint is woken and becomes active
			INSERT 		= 32,	///< An active constraint is inserted in the constraint store
			REMOVE	 	= 64,	///< An active or passive constraint is removed from the constraint store
			TRY 		= 128,	///< An active constraint tries a rule with possibly some passive constraints.
			PARTNER		= 256,	///< A partner of the active constraint has been found.
			COMMIT	 	= 512,	///< The head and the guard of a rule matched.

			BACKTRACK	= 1024,	///< An active constraint receives a failure event and executes another alternative.

			HISTORY		= 2048,	///< The history of a rule has been checked

			ALL			= 65535	///< All flags are registered
		};

		/**
		 * Default constructor: disabled.
		 */
		Log_t() =delete;

		/**
		 * Copy constructor: disabled.
		 */
		Log_t(const Log_t&) =delete;

		/**
		 * Assignment operator: disabled.
		 */
		Log_t& operator=(const Log_t&) =delete;

		/**
		 * Return the instance of the log and create it if needed.
		 * @return A Log object
		 */
		static Log_t& get_instance()
		{
			assert( (_instance) );
			if (!(_instance))
				_instance = std::unique_ptr< Log_t<T> >( new Log_t<T>(Log_t<T>::NONE,std::cerr) );
			return *_instance;
		}

		/**
		 * Change the output of the logs.
		 * @param out The new output
		 */
		static void set_output(std::ostream& out)
		{
			_instance = std::unique_ptr< Log_t<T> >( new Log_t<T>(_instance->_active_flags,out) );
		}

		/**
		 * Add flags to the active flags list.
		 * @param flags The new flags to register
		 */
		static void register_flags(unsigned int flags)
		{
			get_instance()._active_flags |= flags;
		}

		/**
		 * Remove flags from the active flags list.
		 * @param flags The flags to unregister
		 */
		static void unregister_flags(unsigned int flags)
		{
			get_instance()._active_flags &= ~flags;
		}

		/**
		 * Add a constraint to the active constraint list.
		 * @param name The name of the constraint to add
		 */
		static void register_constraint(const std::string& name)
		{
			get_instance()._active_constraints.insert(name);
		}

		/**
		 * Remove the constraint from the active constraint list.
		 * @param name The name of the constraint to remove
		 */
		static void unregister_constraint(const std::string& name)
		{
			get_instance()._active_constraints.erase(name);
		}

		/**
		 * Add a rule to the active rule list.
		 * @param name The name of the rule to add
		 */
		static void register_rule(const std::string& name)
		{
			get_instance()._active_rules.insert(name);
		}

		/**
		 * Remove the rule from the active rule list.
		 * @param name The name of the rule to remove
		 */
		static void unregister_rule(const std::string& name)
		{
			get_instance()._active_rules.erase(name);
		}

		/**
		 * Main function to output a debug message. If a flag of \a flags
		 * is in the active flags set, the message is written.
		 * @param flags List of properties for this message
		 * @param rule_name The name of the rule that send the debug
		 * @param constraint_name The name of the rule that send the debug
		 * @param msg The debug message
		 */
		static void debug(unsigned int flags, std::string rule_name, std::string constraint_name, const std::string& msg)
		{
			if ( (flags & get_instance()._active_flags)
					&& (get_instance()._active_rules.empty()
							|| (rule_name == "")
							|| (get_instance()._active_rules.find(rule_name) != get_instance()._active_rules.end())
							)
					&& (get_instance()._active_constraints.empty()
							|| (constraint_name == "")
							|| (get_instance()._active_constraints.find(constraint_name) != get_instance()._active_constraints.end())
							)
					)
				get_instance()._out << msg << std::endl;
		}

		/**
		 * Function to output a debug message based on the properties of \a flag. If a flag of \a flags
		 * is in the active flags set, the message is written.
		 * @param flags List of properties for this message
		 * @param rule_name The name of the rule that send the debug
		 * @param constraint_name The name of the rule that send the debug
		 * @param num_occ The number of occurrence of the constraint
		 * @param msg_parts The debug message
		 */
		template < typename... TT >
		static void trace_constraint(unsigned int flags, std::string rule_name, const std::string& constraint_name, int num_occ, std::tuple< TT... > msg_parts)
		{
			static const std::size_t tab_width = 50;
			auto& instance = get_instance();
			if ( (flags & instance._active_flags)
					&& (instance._active_constraints.empty()
							|| (instance._active_constraints.find(constraint_name) != instance._active_constraints.end())
					)
					&& (instance._active_rules.empty()
							|| (rule_name == "")
							|| (instance._active_rules.find(rule_name) != instance._active_rules.end())
					)
			)
				{
					std::string str_flag;
					switch (flags)
					{
					case EXIT:
						str_flag = "EXIT";
						flags &= ~EXIT;
						break;
					case FAIL:
						str_flag = "FAIL";
						flags &= ~FAIL;
						break;
					case GOAL:
						str_flag = "GOAL";
						flags &= ~GOAL;
						break;
					case CALL:
						str_flag = "CALL";
						flags &= ~CALL;
						break;
					case WAKE:
						str_flag = "WAKE";
						flags &= ~WAKE;
						break;
					case INSERT:
						str_flag = "INSERT";
						flags &= ~INSERT;
						break;
					case REMOVE:
						str_flag = "REMOVE";
						flags &= ~REMOVE;
						break;
					case TRY:
						str_flag = "TRY";
						flags &= ~TRY;
						break;
					case PARTNER:
						str_flag = "PARTNER";
						flags &= ~PARTNER;
						break;
					case COMMIT:
						str_flag = "COMMIT";
						flags &= ~COMMIT;
						break;
					case BACKTRACK:
						str_flag = "BACKTRACK";
						flags &= ~BACKTRACK;
						break;
					case HISTORY:
						str_flag = "HISTORY";
						flags &= ~HISTORY;
						break;
					default:
						assert(false);
					}
					std::string str_out;
					str_out += "TRACE [" + str_flag + "][" + constraint_name + "]";
					if (num_occ > -1)
						str_out += "[" + std::to_string(num_occ+1) + "]";
					if (!rule_name.empty())
						str_out += "[" + rule_name + "]";
					std::size_t space_size = (tab_width <= str_out.size())?0:(tab_width - str_out.size());
					instance._out << str_out << std::string(space_size, ' ') << chr::TIW::trace_to_string(msg_parts) << std::endl;
				}
		}

		/**
		 * Function to output a debug message and stop program here by
		 * exiting with abort(). If a flag of \a flags is in the active
		 * flags set, the message is written and abort is done.
		 * @param flags List of properties for this message
		 * @param rule_name The name of the rule that send the debug
		 * @param constraint_name The name of the rule that send the debug
		 * @param msg The debug message
		 */
		static void fatal(unsigned int flags, std::string rule_name, std::string constraint_name, const std::string& msg)
		{
			if (flags & get_instance()._active_flags)
			{
				get_instance().debug(flags,rule_name,constraint_name,msg);
				abort();
			}
		}

	private:
		static std::unique_ptr< Log_t<T> > _instance;	///< Unique instance of Log class
		unsigned int _active_flags;						///< Set of active flags
		std::ostream& _out;								///< Default output for logs
		std::set< std::string > _active_rules;			///< Set of active rules (if empty, trace all rules else trace only the rules of the set)
		std::set< std::string > _active_constraints;	///< Set of active constraints (if empty, trace all constraints else trace only the constraints of the set)

		/**
		 * Initialize.
		 */
		Log_t(unsigned int flags, std::ostream& out) : _active_flags(flags), _out(out) {}
	};

	// Initialization of static instance
	template< typename T >
	std::unique_ptr< chr::Log_t<T> > chr::Log_t<T>::_instance = std::unique_ptr< chr::Log_t<T> >( new chr::Log_t<T>(chr::Log_t<T>::NONE,std::cerr) );

	// Dummy class to get rid off the template parameter
	struct Dummy_log { };

	// Alias to get rid off the template parameter when calling Log
	// class.
	using Log = Log_t< Dummy_log >;
}

#endif // ENABLE_TRACE
#endif // RUNTIME_TRACE_HH_
