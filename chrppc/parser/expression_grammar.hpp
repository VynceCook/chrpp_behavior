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

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <tao/pegtl.hpp>

#include <parse_error.hh>
#include <parser/expression_internal.hpp>
#include <ast/expression.hh>

namespace chr::compiler::parser::grammar::expression_rules
{
	// Here is the grammar to parse any infix expression and constraint call.
	using namespace TAO_PEGTL_NAMESPACE;
	namespace internal
	{
		/**
		 * @brief Operator description table
		 *
		 * The operator description table contains a set of objects to describe
		 * prefix operators and a set to describe infix and postfix operators.
		 * Each object stored in the tables describes an operator by storing
		 * its name ('+', '++', '/', '->', ...) and its priority.
		 */
		struct operator_maps
		{
			operator_maps()
				: prefix( sorted_operator_vector(
							{ prefix_info( "!", 60 ),
							prefix_info( "not", 60 ),
							prefix_info( "+", 60 ),
							prefix_info( "-", 60 ),
							prefix_info( "~", 60 ),
							prefix_info( "*", 60 ),
							prefix_info( "&", 60 ),
							prefix_info( "++", 60 ),
							prefix_info( "--", 60 ) } ) ),
				infix_postfix( sorted_operator_vector(
							{
							infix_postfix_info( ".", 71, 72 ),
							infix_postfix_info( "->", 71, 72 ),
							infix_postfix_info( "::", 74, 73 ),
                            infix_postfix_info( ".*", 37, 38 ),
							infix_postfix_info( "->*", 37, 38 ),
							infix_postfix_info( "*", 35, 36 ),
							infix_postfix_info( "/", 35, 36 ),
							infix_postfix_info( "%", 35, 36 ),
							infix_postfix_info( "+", 33, 34 ),
							infix_postfix_info( "-", 33, 34 ),
							infix_postfix_info( "<<", 31, 32 ),
							infix_postfix_info( ">>", 31, 32 ),
							infix_postfix_info( "<", 27, 28 ),
							infix_postfix_info( "<=", 27, 28 ),
							infix_postfix_info( ">", 27, 28 ),
							infix_postfix_info( ">=", 27, 28 ),
							infix_postfix_info( "==", 25, 26 ),
							infix_postfix_info( "!=", 25, 26 ),
							infix_postfix_info( "^", 21, 22 ),
							infix_postfix_info( "&&", 17, 18 ),
							infix_postfix_info( "and", 17, 18 ),
							infix_postfix_info( "||", 15, 16 ),
							infix_postfix_info( "or", 15, 16 ),
							infix_postfix_info( "?", ":", 14, 13 ),  // Special: Ternary operator.
							infix_postfix_info( "=", 12, 11 ),
							infix_postfix_info( "+=", 12, 11 ),
							infix_postfix_info( "-=", 12, 11 ),
							infix_postfix_info( "*=", 12, 11 ),
							infix_postfix_info( "/=", 12, 11 ),
							infix_postfix_info( "%=", 12, 11 ),
							infix_postfix_info( "<<=", 12, 11 ),
							infix_postfix_info( ">>=", 12, 11 ),
							infix_postfix_info( "&=", 12, 11 ),
							infix_postfix_info( "^=", 12, 11 ),
							infix_postfix_info( "|=", 12, 11 ),
							infix_postfix_info( "[", "]", 90 ),  // Special: Argument list.
							infix_postfix_info( "(", ")", 90 ),  // Special: Argument list.
							infix_postfix_info( "<[", "]>", 90 ),  // Special: Argument list.
							infix_postfix_info( "++", 90 ),
							infix_postfix_info( "--", 90 ) } ) ),
				max_prefix_length( std::max_element( prefix.begin(), prefix.end(), []( const auto& l, const auto& r ) { return l.name.size() < r.name.size(); } )->name.size() ),
				max_infix_postfix_length( std::max_element( infix_postfix.begin(), infix_postfix.end(), []( const auto& l, const auto& r ) { return l.name.size() < r.name.size(); } )->name.size() )
				{ }

			const std::vector< prefix_info > prefix;
			const std::vector< infix_postfix_info > infix_postfix;

			const std::size_t max_prefix_length;
			const std::size_t max_infix_postfix_length;
		};

		/**
		 * @brief string_view_rule PEGTL rule to match a string_view_rule
		 *
		 * Adaptation and creation of a specific rule to parse a string_view_rule
		 */
		struct string_view_rule
		{
			template< apply_mode A,
			rewind_mode M,
			template< typename... >
				class Action,
			template< typename... >
				class Control,
			typename ParseInput >
			[[nodiscard]] static bool match( ParseInput& in, const std::string_view sv ) noexcept( noexcept( match_string_view( in, sv ) ) )
			{
				return match_string_view( in, sv );
			}
		};

		// Rule verifying that a space sequence is not followed by '(', '[', '.', "::" or "->".
		// The parser goes back and returns the state of the input stream corresponding to
		// the state just before the rule was tested.
		struct pre_check_ignored
			: not_at< plus<ignored>, sor< one< '(', '[' , '.' >, TAO_PEGTL_STRING("<("), TAO_PEGTL_STRING("::"), TAO_PEGTL_STRING("->") > >
		{};

		// Forward declaration
		template< typename Literal, typename Identifier >
		struct expression;

		struct bracket_expression_error : success {};
		/**
		 * @brief Bracket-expression PEGTL rule to match a bracketed expression
		 *
		 * Adaptation and creation of a specific rule to parse an expression
		 * in brackets to trigger specific actions.
		 */
		template< typename Literal, typename Identifier >
		struct bracket_expression
		{
			template< apply_mode A,
			rewind_mode M,
			template< typename... >
				class Action,
			template< typename... >
				class Control,
			typename ParseInput,
			typename Result,
			typename Config >
			[[nodiscard]] static bool match( ParseInput& in, Result& res, const Config& cfg, const unsigned /*unused*/ )
			{
				return Control< seq< one< '(' >, sor< seq< star< ignored >, expression< Literal, Identifier >, star< ignored >, one< ')' > >, TAO_PEGTL_NAMESPACE::raise<bracket_expression_error> > > >::template match< A, M, Action, Control >( in, res, cfg, 0 );
			}
		};

		/**
		 * @brief Prefix PEGTL rule to match prefix operator
		 *
		 * The prefix operators are captured by a generic rule that
		 * is based on a table of objects that contain the name of the operator.
		 */
		template< typename Literal, typename Identifier >
		struct prefix_expression
		{
			template< apply_mode A,
			rewind_mode M,
			template< typename... >
				class Action,
			template< typename... >
				class Control,
			typename ParseInput,
			typename Result,
			typename Config >
			[[nodiscard]] static bool match( ParseInput& in, Result& res, const Config& cfg, const unsigned /*unused*/ )
			{
				PositionInfo pos(in.position());
				if( const auto* info = match_prefix( in, cfg.max_prefix_length, cfg.prefix ) ) {
					(void)Control< must< star< ignored >, expression< Literal, Identifier > > >::template match< A, M, Action, Control >( in, res, cfg, info->prefix_binding_power );
					if constexpr( A == apply_mode::action ) {
						res.prefix( info->name, pos );
					}
					return true;
				}
				return false;
			}
		};

		// Token used to raise a custom error message
		struct end_list : success {};

		/**
		 * @brief Infix-Postfix PEGTL rule to match binary and postfix operators
		 *
		 * The infix and postfix operators are captured by a generic rule that
		 * is based on a table of objects that contain the name of the operator
		 * and its priority. Some operators (->, ::, ? ...) are handled specifically.
		 */
		template< typename Literal, typename Identifier >
		struct infix_postfix_expression
		{
			template< apply_mode A,
			rewind_mode M,
			template< typename... >
				class Action,
			template< typename... >
				class Control,
			typename ParseInput,
			typename Result,
			typename Config >
			[[nodiscard]] static bool match( ParseInput& in, Result& res, const Config& cfg, const unsigned min )
			{
				if ( check_string_view(in,"==>") || check_string_view(in,"<=>") 
					|| check_string_view(in,"=>>")
					|| (!check_string_view(in,"||") && !check_string_view(in,"|=") && check_string_view(in,"|")) ) 
				{
					// We do not match with any of CHR reserved operator
					return false;
				}
				PositionInfo pos(in.position());
				if( const auto* info = match_infix_postfix( in, cfg.max_infix_postfix_length, cfg.infix_postfix, min ) ) {
					if( info->name == "?" ) {
						(void)Control< must< star< ignored >, expression< Literal, Identifier > > >::template match< A, M, Action, Control >( in, res, cfg, 0 );
						(void)Control< must< star< ignored >, string_view_rule > >::template match< A, M, Action, Control >( in, info->other );
						(void)Control< must< star< ignored >, expression< Literal, Identifier > > >::template match< A, M, Action, Control >( in, res, cfg, info->right_binding_power );
						if constexpr( A == apply_mode::action ) {
							res.ternary( info->name, info->other, pos );
						}
						return true;
					}
					// Allows you to parse a list of arguments as in the parameters of a function
					if( ( info->name == "(" ) || ( info->name == "[" ) || ( info->name == "<[" ) ) {
						const std::size_t size = res.expression_stack.size();
						(void)Control< must< star< ignored >, opt< list_must< expression< Literal, Identifier >, one< ',' >, ignored > > > >::template match< A, M, Action, Control >( in, res, cfg, 0 );
						// sor< R, raise<> > <=> must< R >
						(void)Control< sor< seq< star< ignored >, string_view_rule >, TAO_PEGTL_NAMESPACE::raise< end_list > > >::template match< A, M, Action, Control >( in, info->other );
						if constexpr( A == apply_mode::action ) {
							res.call( info->name, info->other, res.expression_stack.size() - size, pos );
						}
						return true;
					}
					if( info->is_infix() ) {
						(void)Control< must< star< ignored >, expression< Literal, Identifier > > >::template match< A, M, Action, Control >( in, res, cfg, info->right_binding_power );
						if constexpr( A == apply_mode::action ) {
							res.infix( info->name, pos );
						}
						return true;
					}
					if( info->is_postfix() ) {
						if constexpr( A == apply_mode::action ) {
							res.postfix( info->name, pos );
						}
						return true;
					}
				}
				return false;
			}
		};

		// ---------------------------------------------------------------------------
		// Parse chr_count
		// To raise an error on chr_count parsing issue
		struct chr_count_error : success {};
		// ---------------------------------------------------------------------------
		// Rule head expression wrapper in order to match constraint and call a rule action
		template< typename Literal, typename Identifier >
		struct chr_count_arg
		{
			using rule_t = chr_count_arg;
			using subs_t = type_list< sor< Literal, Identifier > >;

			template< apply_mode A,
			rewind_mode M,
			template< typename... >
				class Action,
			template< typename... >
				class Control,
			typename ParseInput,
			typename Result,
			typename ... States >
			[[nodiscard]] static bool match( ParseInput& in, Result& res, States&& ... /*unused*/ )
			{
				ast_builder::AstExpressionBuilder expr_res(res.chr_constraints);
				bool ret = Control< seq< Identifier, sor< seq< one<'('>, star<ignored>, sor< one<')'>, seq< list< sor< Literal, Identifier >, one<','>, ignored >, one<')'> > > >, TAO_PEGTL_NAMESPACE::raise< chr_count_error > > > >::template match< A, M, Action, Control >( in, expr_res );
				if (ret) {
					PositionInfo pos(in.position());
					if constexpr( A == apply_mode::action ) {
						expr_res.chr_constraint_head();
						res.expression_stack.emplace_back( std::move(expr_res.expression_stack.at( 0 )) );
					}
				}
				return ret;
			}
		};

		template< typename Literal, typename Identifier >
		struct chr_count
			: seq< TAO_PEGTL_STRING("chr_count"), sor< seq< one<'('>, star<ignored>, chr_count_arg<Literal, Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_count_error > > > {};

		// ---------------------------------------------------------------------------
		// Part of the grammar that parses the expression
		template< typename Literal, typename Identifier >
		struct first_expression
			: sor< chr_count<Literal, Identifier>, bracket_expression< Literal, Identifier >, prefix_expression< Literal, Identifier >, Literal, Identifier >
		{};

		template< typename Literal, typename Identifier >
		struct expression
			: seq< star<ignored>, first_expression< Literal, Identifier >, star< seq< pre_check_ignored, star<ignored>, infix_postfix_expression< Literal, Identifier > > > >
		{};
	}  // namespace internal
}  // namespace chr::compiler::parser::grammar::expression_rules

namespace chr::compiler::parser::grammar
{
	using namespace TAO_PEGTL_NAMESPACE;

	template< typename Literal, typename Identifier >
	struct expression
	{
		using rule_t = expression;
		using subs_t = type_list< expression_rules::internal::expression< Literal, Identifier > >;

		template< apply_mode A,
		rewind_mode M,
		template< typename... >
			class Action,
		template< typename... >
			class Control,
		typename ParseInput,
		typename Result >
		[[nodiscard]] static bool match( ParseInput& in, Result& res )
		{
			const expression_rules::internal::operator_maps cfg;
			return match< A, M, Action, Control >( in, res, cfg );
		}

		template< apply_mode A,
		rewind_mode M,
		template< typename... >
			class Action,
		template< typename... >
			class Control,
		typename ParseInput,
		typename Result,
		typename Config >
		[[nodiscard]] static bool match( ParseInput& in, Result& res, const Config& cfg )
		{
			return Control< expression_rules::internal::expression< Literal, Identifier > >::template match< A, M, Action, Control >( in, res, cfg, 0 );
		}
	};

}  // namespace chr::compiler::parser::grammar

