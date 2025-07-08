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

#ifndef EXAMPLES_OPTIONS_HPP_
#define EXAMPLES_OPTIONS_HPP_

#include <iostream>
#include <vector>
#include <cassert>
#include <string>

/**
 * Different possible type for an option value
 */
class Option_value
{
private:
	std::string _str; //> The string representation of the option
public:
	/**
	 * Default constructor
	 * @param str The string representation of the option
	 */
	Option_value(std::string str) : _str(str) {}

	/**
	 * Returns the conversion of the option to a boolean.
	 * Raises an exception if the conversion failed.
	 * @return the boolean value
	 */
	bool b() const
	{
		if ( (_str == "true") || (_str == "TRUE") )
			return true;
		else if ( (_str == "false") || (_str == "FALSE") )
			return false;
		 throw std::invalid_argument( _str + " is not a boolean value" );
	}

	/**
	 * Returns the conversion of the option to an integer.
	 * Raises an exception if the conversion failed.
	 * @return the integer value
	 */
	int i() const
	{
		auto res = stoi(_str);
		return res;
	}

	/**
	 * Returns the conversion of the option to an unsigned long integer.
	 * Raises an exception if the conversion failed.
	 * @return the unsigned long integer value
	 */
	unsigned long int ul() const
	{
		auto res = stoul(_str);
		return res;
	}

	/**
	 * Returns the conversion of the option to a float.
	 * Raises an exception if the conversion failed.
	 * @return the float value
	 */
	float f() const
	{
		auto res = stof(_str);
		return res;
	}

	/**
	 * Returns the conversion of the option to a string.
	 * @return the string value
	 */
	std::string str() const
	{
		return _str;
	}
};

/**
 * An option may be single value (name is empty), or
 * a flag (with a '-' or a '--') with an optional parameter
 */
struct Option_desc
{
	std::string name;			//> Name of the option
	std::string short_name;		//> Short name for the option
	bool has_parameter_value;	//> A boolean flag to know if the option waits for a value or not
	std::string help;			//> Help message for this option
};

/**
 * The option with the expected value (when provided)
 */
struct Option
{
	Option_desc desc;		//> Description of the option
	Option_value value;		//> value of the option (if wanted)
};

/**
 * The parsed options
 */
struct Options
{
	std::vector< Option > m_options;	//> All gathered options
	std::string m_help_message;			//> Help message built while parsing options
};

typedef std::vector< Option_desc > Options_desc;
typedef std::vector< Option_value > Options_values;

/**
 * Parse the program arguments and search for input options
 * @param argc The program arguments count
 * @param argv The program arguments values
 * @param options_desc The description of allowed options
 * @return The list of found options with values (subset of \a options_desc). May be empty.
 */
inline Options parse_options(int argc, const char* argv[], const Options_desc& options_desc)
{
	static const int tab_width = 50;

	Options options;
	unsigned int count_empty = 0;
	unsigned int nb_empty_assigned = 0;
	int exit_status = 0;

	// Build help message
	options.m_help_message += "Use: " + std::string(argv[0]) + " ";
	if ( (options_desc.size() - count_empty) > 0 )
		options.m_help_message += "[OPTION] ";
	for (unsigned int i = 0; i < count_empty; ++i)
		options.m_help_message += "arg" + std::to_string(i+1) + " ";
	options.m_help_message += "\n";

	if (options_desc.size() > 0)
	{
		options.m_help_message += "\nArgument(s) usage:\n";

		int i = 1;
		auto it = options_desc.begin();
		auto it_end = options_desc.end();
		for ( ; it != it_end ; ++it)
		{
			if ((*it).name.empty()) // count empty
				++count_empty;

			std::string::size_type str_size = tab_width;
			options.m_help_message += "\t";
			if ((*it).name.empty() && (*it).short_name.empty())
			{
				std::string str_i = std::to_string(i);
				options.m_help_message += "arg" + str_i;
				str_size -= std::string("arg").size() + str_i.size();
				++i;
			}
			else
			{
				if ((*it).short_name.empty())
				{
					options.m_help_message += "--" + (*it).name;
					str_size -= std::string("--").size() + (*it).name.size();
				}
				else if ((*it).name.empty())
				{
					options.m_help_message += "-" + (*it).short_name;
					str_size -= std::string("-").size() + (*it).short_name.size();
				}
				else
				{
					options.m_help_message += "--" + (*it).name + ",-" + (*it).short_name;
					str_size -= std::string("--,-").size() + (*it).name.size() + (*it).short_name.size();
				}
				if ( (*it).has_parameter_value )
				{
					options.m_help_message += " value" ;
					str_size -= std::string(" value").size();
				}
			}
			options.m_help_message += std::string(str_size, ' ') + (*it).help + "\n";
		}
	}

	int i = 1;
	while (i < argc)
	{
		std::string str_arg = argv[i];
		if ( ("--help" == str_arg) || ("-h" == str_arg) )
			goto help;

		auto it = options_desc.begin();
		auto it_end = options_desc.end();
		for ( ; it != it_end ; ++it)
		{
			if ( (*it).name.empty() && (*it).short_name.empty() )
				continue;
			if ( (("--" + (*it).name) == str_arg) || (("-" + (*it).short_name) == str_arg) )
				break;
		}

		if (it == it_end)
		{
			if (count_empty == 0)
			{
				std::cerr << "Unknow parameter: " << str_arg << std::endl << std::endl;
				exit_status = 1;
				goto help;
			}
			it = options_desc.begin();
			auto it_bak = it;
			it_end = options_desc.end();
			unsigned int k = 0;
			for ( ; it != it_end ; ++it)
			{
				if ( !(*it).name.empty() || !(*it).short_name.empty() )
				{
					continue;
				}
				it_bak = it;
				if (k == nb_empty_assigned)
					break;
				++k;
			}
			if (it == it_end) // If we reach the end, we go back to last empty option description
				it = it_bak;
			assert((*it).name == "");
			options.m_options.push_back( { *it, Option_value(str_arg) } );
			nb_empty_assigned++;
		}
		else
		{
			if (!(*it).has_parameter_value)
				options.m_options.push_back( { *it, Option_value( std::string() ) } );
			else
			{
				++i;
				if (i > argc)
				{
					std::cerr << "Missing parameter" << std::endl << std::endl;
					exit_status = 1;
					goto help;
				}
				options.m_options.push_back( { *it, Option_value( argv[i] ) } );
			}
		}
		++i;
	}
	return options;

	// Help message
	help:
	std::cout << options.m_help_message;
	exit(exit_status);
}

/**
 * Check if the vector of options has the option \a option_name.
 * It updates the \a value with the value found for the input
 * \a option_name.
 * @param option_name The name of the option to search
 * @param options The input list of options
 * @param values The output value if \a option_name has been found
 * @return True if the \a option_name has been found, false otherwise
 */
inline bool has_option(const std::string& option_name, const Options& options, Options_values& values)
{
	Options_values::size_type values_initial_size = values.size();
	auto it = options.m_options.begin();
	auto it_end = options.m_options.end();
	for ( ; it != it_end ; ++it)
		if ( (option_name.empty() && (*it).desc.name.empty() && (*it).desc.short_name.empty())
				|| (!option_name.empty() && (((*it).desc.name == option_name) || ((*it).desc.short_name == option_name))) )
			values.push_back( (*it).value );

	return (values.size() > values_initial_size);
}

/**
 * Check if the vector of options has the option \a option_name
 * @param option_name The name of the option to search
 * @param options The input list of options
 * @return True if the \a option_name has been found, false otherwise
 */
inline bool has_option(const std::string& option_name, const Options& options)
{
	auto it = options.m_options.begin();
	auto it_end = options.m_options.end();
	for ( ; it != it_end ; ++it)
		if ( (option_name.empty() && (*it).desc.name.empty() && (*it).desc.short_name.empty())
				|| (!option_name.empty() && (((*it).desc.name == option_name) || ((*it).desc.short_name == option_name))) )
			return true;
	return false;
}

#endif /* EXAMPLES_OPTIONS_HPP_ */
