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

namespace chr::compiler::parser
{
	namespace pegtl = TAO_PEGTL_NAMESPACE;
	using namespace pegtl;

	/**
	 * Custom error control of CHR parser
	 *
	 * Template class which is used to provide adapted messages when
	 * errors occur. Each specialization of the template class corresponds
	 * to a case where a useful information can be provided to the user.
	 */
	template< typename Rule >
	struct error_control : public normal< Rule > {
	   static const std::string error_message;	///< The error message

	   /**
		* Overload of the raise function to print our own error message.
		* @param in The input stream
		*/
	   template< typename Input, typename ... States >
	   [[noreturn]] static void raise( const Input & in, States && ... )
	   {
		  throw parse_error( error_message, in );
	   }
	};

	// ---------------------------------------------------------------------------
	// Default custom message
	template< typename Dummy > inline const std::string error_control< Dummy >::error_message = "parse error";

	// ---------------------------------------------------------------------------
	// Custom messages
	template<> inline const std::string error_control< chr::compiler::parser::grammar::expression_rules::internal::expression<chr::compiler::parser::literal, tao::pegtl::ascii::identifier> >::error_message = "parse error matching expression";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::expression_rules::internal::bracket_expression_error >::error_message = "parse error matching bracketed expression, unexpected character encountered";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::expression_rules::internal::end_list >::error_message = "parse error matching expression, expected one of the end list tokens ')', ']' or ']>'";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::constraint_args_end_list >::error_message = "parse error matching expression, expected one of the end list tokens ')', ']' or ']>'";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::constraint_arg_type_mode >::error_message = "parse error matching mode (unknown), mode must be either '+', '-' or '?'";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::constraint_arg_type_error >::error_message = "parse error matching CHR constraint argument";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::constraint_decl_pragma_values >::error_message = "parse error matching pragma, unknown pragma in constraint declaration";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::rule_head_pragma_values >::error_message = "parse error matching pragma, unknown pragma in constraint head";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::body::constraint_call_pragma_values >::error_message = "parse error matching pragma, unknown pragma in body constraint call";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::rule_guard_op >::error_message = "parse error matching guard, expected '|' but other symbol found";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::body::chr_try_error >::error_message = "parse error matching CHR try (probably a wrong number of arguments)";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::body::chr_behavior_error >::error_message = "parse error matching behavior, exists or forall (probably a wrong number of arguments)";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::chr_statement_tag_error >::error_message = "parse error matching CHR statement (or rule), unknown CHR statement";
	template<> inline const std::string error_control< chr::compiler::parser::grammar::chr_include_file >::error_message = "parse error matching CHR include file, ill-formed attribute";
	
	template<> inline const std::string error_control< chr::compiler::parser::property >::error_message = "parse error matching CHR property";
	template<> inline const std::string error_control< chr::compiler::parser::property_name >::error_message = R"_STR(parse error matching value of CHR property "name")_STR";
	template<> inline const std::string error_control< chr::compiler::parser::property_auto_persistent >::error_message = R"_STR(parse error matching value of CHR property "auto_persistent", value can only be true or false)_STR";
	template<> inline const std::string error_control< chr::compiler::parser::property_auto_catch_failure >::error_message = R"_STR(parse error matching value of CHR property "catch_failure", value can only be true or false)_STR";
	template<> inline const std::string error_control< chr::compiler::parser::property_parameters >::error_message = R"_STR(parse error matching value of CHR property "parameters")_STR";
	template<> inline const std::string error_control< chr::compiler::parser::property_template_parameters >::error_message = R"_STR(parse error matching value of CHR property "template_parameters")_STR";
}
