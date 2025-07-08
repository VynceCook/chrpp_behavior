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
#include <parser/chr_body_internal.hpp>

namespace chr::compiler::parser::grammar::body
{
	namespace pegtl = TAO_PEGTL_NAMESPACE;
	using namespace pegtl;

	template< typename Literal, typename Identifier >
	struct body_expression_wrapper
	{
		using rule_t = body_expression_wrapper;
		using subs_t = type_list< expression< Literal, Identifier > >;

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
			bool ret = Control< expression< Literal, Identifier > >::template match< A, M, Action, Control >( in, expr_res );
			if (ret) {
				PositionInfo pos(in.position());
				if constexpr( A == apply_mode::action ) {
					res.cpp_expression( std::move(expr_res.expression_stack.at(0)), pos );
				}
			}
			return ret;
		}
	};

	namespace internal {
		// Forward declaration
		template< typename Literal, typename Identifier >
		struct constraint_call_sequence;
	} // namespace internal

	// ---------------------------------------------------------------------------
	// Parse constraint_call pragmas
	struct constraint_call_pragma_value
			: sor< TAO_PEGTL_KEYWORD("catch_failure") > {};
	struct constraint_call_pragma_list
			: seq< one< '{' > , star<ignored>, list< constraint_call_pragma_value, one< ',' >, ignored >, star<ignored>, one< '}' > > {};
	struct constraint_call_pragma_values
			: sor< constraint_call_pragma_list, constraint_call_pragma_value > {};
	struct constraint_call_pragmas
			: if_must< one< '#' >, star<ignored>, constraint_call_pragma_values > {};

	// ---------------------------------------------------------------------------
	// Parse CPP expression
	template< typename Literal, typename Identifier >
	struct cpp_expression
			: body_expression_wrapper< Literal, Identifier > {};

	template< typename Literal, typename Identifier >
	struct cpp_expression_with_pragmas
			: seq< body_expression_wrapper< Literal, Identifier >, star<ignored>, opt< constraint_call_pragmas > > {};

	// ---------------------------------------------------------------------------
	// Parse CHR keyword
	struct chr_keyword_name
		: sor< TAO_PEGTL_STRING("success"), TAO_PEGTL_STRING("failure"), TAO_PEGTL_STRING("stop") > {};
	struct chr_keyword
		: seq< chr_keyword_name, opt< seq< one<'('>, star<ignored>, one<')'> > > > {};

	// ---------------------------------------------------------------------------
	// Parse CHR try
	struct chr_try_error
		: success {};
	template< typename Literal, typename Identifier >
	struct chr_try_bt
		: seq< TAO_PEGTL_STRING("try_bt"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_try_error > > > {};
	template< typename Literal, typename Identifier >
	struct chr_try
		: seq< TAO_PEGTL_STRING("try"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_try_error > > > {};

	// ---------------------------------------------------------------------------
	// Parse behavior
	struct chr_behavior_error
		: success {};
	struct chr_behavior_empty_arg
		: seq< star<ignored>, one<','> > {};
	template< typename Literal, typename Identifier >
	struct chr_behavior_opt_arg
		: sor<
			chr_behavior_empty_arg,
			seq< star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<','> >
		>
			{};
	template< typename Literal, typename Identifier >
	struct chr_behavior
		: seq< TAO_PEGTL_STRING("behavior"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				chr_behavior_opt_arg<Literal, Identifier>,
				chr_behavior_opt_arg<Literal, Identifier>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				chr_behavior_opt_arg<Literal, Identifier>,
				chr_behavior_opt_arg<Literal, Identifier>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_behavior_error > > > {};

	// ---------------------------------------------------------------------------
	// Parse exists
	template< typename Literal, typename Identifier >
	struct chr_exists
		: seq< TAO_PEGTL_STRING("exists"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_behavior_error > > > {};

	// ---------------------------------------------------------------------------
	// Parse exists_it
	template< typename Literal, typename Identifier >
	struct chr_exists_it
		: seq< TAO_PEGTL_STRING("exists_it"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_behavior_error > > > {};

	// ---------------------------------------------------------------------------
	// Parse for_all
	template< typename Literal, typename Identifier >
	struct chr_forall
		: seq< TAO_PEGTL_STRING("forall"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_behavior_error > > > {};

	// ---------------------------------------------------------------------------
	// Parse for_all_it
	template< typename Literal, typename Identifier >
	struct chr_forall_it
		: seq< TAO_PEGTL_STRING("forall_it"), sor< seq< one<'('>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, cpp_expression<Literal,Identifier>, star<ignored>, one<','>,
				star<ignored>, internal::constraint_call_sequence<Literal,Identifier>, star<ignored>, one<')'> >, TAO_PEGTL_NAMESPACE::raise< chr_behavior_error > > > {};

	// ---------------------------------------------------------------------------
	// Parse constraint call
	template< typename Literal, typename Identifier >
	struct chr_reserved_constraint
		: sor< chr_try_bt<Literal,Identifier>, chr_try<Literal,Identifier>, chr_behavior<Literal,Identifier>, chr_exists_it<Literal,Identifier>, chr_exists<Literal,Identifier>, chr_forall_it<Literal,Identifier>, chr_forall<Literal,Identifier> > {};

	template< typename Literal, typename Identifier >
	struct constraint_call
			: sor< chr_keyword, chr_reserved_constraint<Literal,Identifier>, cpp_expression_with_pragmas<Literal,Identifier> > {};

	namespace internal
	{
		/**
		 * @brief Operator description table
		 *
		 * The operator description table contains a set of objects to describe
		 * infix operators.
		 * Each object stored in the tables describes an operator by storing
		 * its name (',', ';') and its priority.
		 */
		struct operator_maps
		{
			operator_maps()
				: infix( sorted_operator_vector(
							{
							infix_info( ";", 33, 34 ),
							infix_info( ",", 35, 36 ) } ) ),
				max_infix_length( std::max_element( infix.begin(), infix.end(), []( const auto& l, const auto& r ) { return l.name.size() < r.name.size(); } )->name.size() )
				{ }

			const std::vector< infix_info > infix;
			const std::size_t max_infix_length;
		};

		/**
		 * @brief Bracket-statement rule to match a bracketed constraint set
		 *
		 * Adaptation and creation of a specific rule to parse an expression
		 * in brackets to trigger specific actions.
		 */
		template< typename Literal, typename Identifier >
		struct bracket_constraint_call_sequence
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
				return Control< if_must< one< '(' >, star< ignored >, constraint_call_sequence< Literal, Identifier >, star< ignored >, one< ')' > > >::template match< A, M, Action, Control >( in, res, cfg, 0 );
			}
		};

		/**
		 * @brief Infix chr statement
		 *
		 * The infix chr operators (, and ;) are captured by a generic rule that
		 * is based on a table of objects that contain the name of the operator
		 * and its priority.
		 */
		template< typename Literal, typename Identifier >
		struct infix_chr_statement
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
					if ( expression_rules::internal::check_string_view(in,";;") )
					{
						// We do not want to match end of rule here
						return false;
					}
					PositionInfo pos(in.position());
					if( const auto* info = match_infix( in, cfg.max_infix_length, cfg.infix, min ) ) {
						if ( info->name == ";;" )
						{
							// We do not want to match end of rule here
							return false;
						}
						(void)Control< must< star< ignored >, constraint_call_sequence< Literal, Identifier > > >::template match< A, M, Action, Control >( in, res, cfg, info->right_binding_power );
						if constexpr( A == apply_mode::action ) {
							res.sequence( info->name, pos );
						}
						return true;
					}
					return false;
				}
		};

		// ---------------------------------------------------------------------------
		// Parse body sequence composed of constraint calls separated from others with (, or ;)

		// Part of the grammar that parses the constraint_call sequence
		template< typename Literal, typename Identifier >
		struct first_constraint_call
			: sor< bracket_constraint_call_sequence< Literal, Identifier >, constraint_call< Literal, Identifier > >
		{};

		template< typename Literal, typename Identifier >
		struct constraint_call_sequence
			: seq< star<ignored>, first_constraint_call< Literal, Identifier >, star<ignored>, star< internal::infix_chr_statement< Literal, Identifier > > >
		{};
	} // namespace internal

	template< typename Literal, typename Identifier >
	struct constraint_call_sequence
	{
		using rule_t = constraint_call_sequence;
		using subs_t = type_list< internal::constraint_call_sequence< Literal, Identifier > >;

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
			const internal::operator_maps cfg;
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
			return Control< internal::constraint_call_sequence< Literal, Identifier > >::template match< A, M, Action, Control >( in, res, cfg, 0 );
		}
	};
}  // namespace chr::compiler::parser::grammar::body
