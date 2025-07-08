/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Apache License, Version 2.0.
 *
 *  Copyright:
 *     2022, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
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

#pragma once

#include <array>

namespace chr::compiler
{
	/**
	 * @brief CHR constraint argument mode
	 */
	enum class Arg_mode : unsigned int
	{
		grounded = 0,	//!< Grounded
		mutable_,		//!< Mutable
		variant			//!< Not yet decided (can be grounded or not grounded or not var_mutable)
	};

	/**
	 * @brief Pragma values
	 */
	enum class Pragma : int
	{
		passive = 0,	//!< Pragma passive
		no_history,		//!< Pragma no_history
		no_reactivate,	//!< Pragma no_reactivate
		bang,			//!< Pragma bang
		persistent,		//!< Pragma persistent
		catch_failure,	//!< Pragma catch_failure
	};

	/**
	 * @brief String representation of pragma values
	 */
	const std::array< const char*, 6 > StrPragma {
		"passive",
		"no_history",
		"no_reactivate",
		"bang",	
		"persistent",
		"catch_failure"
	};

	/**
	 * Global options for the compiler
	 */
	struct Compiler_options
	{
		static bool TRACE;						///< Enable trace output of CHR actions
		static bool STDIN;						///< Listen Stdin for incomming CHR statements
		static bool STDOUT;						///< Ouput all CHR generated code to standard output
		static bool WARNING_UNUSED_RULE;		///< Print a warning message if an unused rule is detected
		static bool NEVER_STORED;				///< Enable never stored optimization
		static bool HEAD_REORDER;				///< Enable head reorder optimization
		static bool GUARD_REORDER;				///< Enable guard reorder optimization
		static bool OCCURRENCES_REORDER;		///< Enable occurrences reorder optimization
		static bool CONSTRAINT_STORE_INDEX;		///< Enable the use of an indexing data structure for managing constraint store
		static bool LINE_ERROR;					///< Write friendly line errors in chrpp source file
		static std::string OUTPUT_DIR;			///< Ouput directory for all CHR generated file (only if no -stdout set)
		static int CHRPPC_MAJOR;				///< Major version of chrppc
		static int CHRPPC_MINOR;				///< Minor version of chrppc
	};

} // namespace chr::compiler

