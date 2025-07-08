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

#include <string>
#include <stdexcept>
#include <tao/pegtl.hpp>

namespace chr::compiler {
	/**
	 * @brief Info about position in source file
	 *
	 * Store position in parsed source file (file name, number line, etc.)
	 * of a specific parsed element. Useful to print debug messages and
	 * throwing exceptions.
	 */
	using PositionInfo = TAO_PEGTL_NAMESPACE::position;

	/**
	 * @brief Error raised while parsing input CHR statements
	 */
	struct ParseError : public std::runtime_error  {
		/**
		 * Initialize an error.
		 * @param msg The message to print
		 * @param pos The position of error in input source
		 */
		ParseError(const char* msg, PositionInfo pos)
			: std::runtime_error( msg ), _msg(msg), _pos({pos})
		{
			const auto prefix = to_string( _pos.front() );
			_msg = prefix + ": " + _msg;
			_prefix += prefix.size() + 2;
		}

		/**
		 * Initialize an nested error with another ParseError.
		 * @param msg The message to print
		 * @param pos The position of error in input source
		 * @param nested_e The nested_e
		 * @param in_line The line where the error occured
		 */
		ParseError(const char* msg, PositionInfo pos, const ParseError& nested_e, std::string_view in_line)
			: std::runtime_error( nested_e._msg ), _msg(nested_e._msg), _in_line(in_line), _pos(nested_e.positions())
		{
			_pos.emplace_back(pos);
			const auto prefix = to_string( _pos.at(1) ) + ": " + msg + "\n" + to_string( _pos.front() );
			_msg = prefix + ": " + std::string(nested_e.message());
			_prefix += prefix.size() + 2;
		}

		[[nodiscard]] const char* what() const noexcept override
		{
			return _msg.c_str();
		}

		[[nodiscard]] std::string_view in_line() const noexcept
		{
			return _in_line;
		}

		[[nodiscard]] std::string_view message() const noexcept
		{
			return { _msg.data() + _prefix, _msg.size() - _prefix };
		}

		[[nodiscard]] const std::vector<PositionInfo>& positions() const noexcept
		{
			return _pos;
		}

	private:
		std::string _msg;					///< The message to print
		std::string _in_line;				///< The line where the error occured
		std::size_t _prefix = 0;			///< Prefix to print at line begining
		std::vector<PositionInfo> _pos;		///< The position of the error in input source
	};

};

