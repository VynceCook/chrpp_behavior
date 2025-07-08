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

#include <tao/pegtl.hpp>
#include <ast/program.hh>

namespace chr::compiler::parser
{
	namespace pegtl = TAO_PEGTL_NAMESPACE;
	using namespace pegtl;

	// ---------------------------------------------------------------------------
	// Comment and ignored characters
	struct comment
		: seq< two<'/'>, until< eolf > >
	{};

	struct ignored
		: sor< space, comment >
	{};

	// ---------------------------------------------------------------------------
	// Constants
	struct cpp_constant_number
		: sor< TAO_PEGTL_STRING("true"), TAO_PEGTL_STRING("false"), seq< plus< sor< digit, one<'.'> > >, opt< plus< one< 'u', 'f', 'l', 'U', 'L' > > > > > {};
	// A constant char is encapsulated by '.
	struct cpp_constant_char
		: seq< one< '\'' >, ranges< 32, 38, 40, 126 >, one< '\'' > > {};
	// A constant string is encapsulated by ".
	struct cpp_constant_string
		: seq< one< '\"' >, star< ranges< 32, 33, 35, 126 > >, one< '\"' > > {};

	/**
	 * @brief literal rule to match a literal
	 *
	 * A literal is a constant (number, true, false, char or string)
	 */
	struct literal
		: sor< cpp_constant_char, cpp_constant_string, cpp_constant_number >
	{};

	// ---------------------------------------------------------------------------
	// Program properties
	struct prop_name				: TAO_PEGTL_KEYWORD("name") {};
	struct prop_auto_persistent		: TAO_PEGTL_KEYWORD("auto_persistent") {};
	struct prop_auto_catch_failure	: TAO_PEGTL_KEYWORD("auto_catch_failure") {};
	struct prop_parameters			: TAO_PEGTL_KEYWORD("parameters") {};
	struct prop_template_parameters	: TAO_PEGTL_KEYWORD("template_parameters") {};

	// ---------------------------------------------------------------------------
	// Keywords
	struct keyword_chr				: sor< TAO_PEGTL_KEYWORD("chr"), TAO_PEGTL_KEYWORD("CHR") > {};
	struct keyword_new_constraint	: TAO_PEGTL_KEYWORD("chr_constraint") {};
	struct keyword_chr_include		: TAO_PEGTL_KEYWORD("chr_include") {};
	struct chr_keyword : sor< keyword_new_constraint, keyword_chr_include > {};

	// ---------------------------------------------------------------------------
	// CHR properties
	struct property_name : sor< seq< identifier_first, star< identifier_other > >, TAO_PEGTL_NAMESPACE::raise<property_name> > {};
	struct property_auto_persistent : sor< TAO_PEGTL_ISTRING("true"), TAO_PEGTL_ISTRING("false"), TAO_PEGTL_NAMESPACE::raise<property_auto_persistent> > {}; 
	struct property_auto_catch_failure : sor< TAO_PEGTL_ISTRING("true"), TAO_PEGTL_ISTRING("false"), TAO_PEGTL_NAMESPACE::raise<property_auto_catch_failure> > {}; 
	struct property_parameters : sor< plus< not_one< '"'> >, TAO_PEGTL_NAMESPACE::raise<property_parameters> > {};
	struct property_template_parameters : sor< plus< not_one< '"'> >, TAO_PEGTL_NAMESPACE::raise<property_template_parameters> > {};

	struct property
		: sor<
			seq< prop_name, star<ignored>, one<'='>, star<ignored>, one<'"'>, property_name, one<'"'> >,
			seq< prop_auto_persistent, star<ignored>, one<'='>, star<ignored>, one<'"'>, property_auto_persistent, one<'"'> >,
			seq< prop_auto_catch_failure, star<ignored>, one<'='>, star<ignored>, one<'"'>, property_auto_catch_failure, one<'"'> >,
			seq< prop_parameters, star<ignored>, one<'='>, star<ignored>, one<'"'>, property_parameters, one<'"'> >,
			seq< prop_template_parameters, star<ignored>, one<'='>, star<ignored>, one<'"'>, property_template_parameters, one<'"'> >
		> {};
	struct properties
		: list< property, star<ignored> > {};

	// ---------------------------------------------------------------------------
	// CHR starting/ending tags
	struct chr_tag_begin
		: seq< one<'<'>, keyword_chr, at< one<' ', '>'> >, star<ignored>, sor< one<'>'>, sor< seq< properties, star<ignored>, one<'>'> >, TAO_PEGTL_NAMESPACE::raise<property> > >, star<ignored> > {};
	// The '<' at the start of the tag is consumed by grammar::chr_statement
	struct chr_tag_end
		: seq< one<'/'>, keyword_chr, one<'>'> > {};
}

#include <parser/ast_expression_builder.hpp>
#include <parser/ast_body_builder.hpp>
#include <parser/ast_rule_builder.hpp>
#include <parser/ast_program_builder.hpp>
#include <parser/expression_grammar.hpp>
#include <parser/chr_body_grammar.hpp>
#include <parser/chr_grammar.hpp>

namespace chr::compiler::parser::grammar
{
	struct program
		: seq< until< chr_tag_begin >, star<ignored>, must< chr_statements< literal, TAO_PEGTL_NAMESPACE::identifier >, chr_tag_end > >
	{};
}  // namespace chr::compiler::parser

#include <parser/actions.hpp>
#include <parser/errors.hpp>

