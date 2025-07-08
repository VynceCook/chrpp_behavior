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

namespace chr::compiler::parser::grammar
{
	namespace pegtl = TAO_PEGTL_NAMESPACE;
	using namespace pegtl;

	// ---------------------------------------------------------------------------
	// Parse rule head pragmas
	struct rule_head_pragma_value
			: sor< TAO_PEGTL_KEYWORD("bang"), TAO_PEGTL_KEYWORD("passive"), TAO_PEGTL_KEYWORD("no_history") > {};
	struct rule_head_pragma_list
			: seq< one< '{' > , star<ignored>, list< rule_head_pragma_value, one< ',' >, ignored >, star<ignored>, one< '}' > > {};
	struct rule_head_pragma_values
			: sor< rule_head_pragma_list, rule_head_pragma_value > {};
	struct rule_head_pragmas
			: if_must< one< '#' >, star<ignored>, rule_head_pragma_values > {};

	// ---------------------------------------------------------------------------
	// Parse CHR rule
	struct rule_name_clear
			: success {};
	template< typename Literal, typename Identifier >
	struct rule_name
			: Identifier {};
	template< typename Literal, typename Identifier >
	struct rule_name_optional
			: sor< seq< not_at< chr_keyword >, rule_name<Literal, Identifier>, star<ignored>, one< '@' > >, rule_name_clear > {};

	// Token used to raise a custom error message
	struct constraint_args_end_list : success {} ;

	// ---------------------------------------------------------------------------
	// Rule head expression wrapper in order to match constraint and call a rule action
	template< typename Literal, typename Identifier >
	struct rule_head_constraint
	{
		using rule_t = rule_head_constraint;
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
			bool ret = Control< seq< Identifier, sor< seq< one<'('>, star<ignored>, sor< one<')'>, seq< list< sor< Literal, Identifier >, one<','>, ignored >, one<')'> > > >, TAO_PEGTL_NAMESPACE::raise< constraint_args_end_list > > > >::template match< A, M, Action, Control >( in, expr_res );
			if (ret) {
				PositionInfo pos(in.position());
				if constexpr( A == apply_mode::action ) {
					expr_res.chr_constraint_head();
					res.add_head( std::move(expr_res.expression_stack.at( 0 )) );
				}
			}
			return ret;
		}
	};

	struct rule_head_split_op
			: one<'\\'> {};
	template< typename Literal, typename Identifier >
	struct rule_head_constraint_with_pragmas
			: seq< rule_head_constraint<Literal, Identifier>, star<ignored>, opt< rule_head_pragmas > > {};
	template< typename Literal, typename Identifier >
	struct rule_head
			: list< rule_head_constraint_with_pragmas<Literal, Identifier>, one< ',' >, ignored > {};

	// ---------------------------------------------------------------------------
	// Rule guard expression wrapper in order to match expression and call a rule action
	template< typename Literal, typename Identifier >
	struct rule_guard_element
	{
		using rule_t = rule_guard_element;
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
					res.add_guard_part( std::move(expr_res.expression_stack.at( 0 )) );
				}
			}
			return ret;
		}
	};

	struct rule_guard_clear
			: success {};
	struct rule_guard_op
			: one< '|' > {};
	template< typename Literal, typename Identifier >
	struct rule_guard
			: seq< list< rule_guard_element<Literal, Identifier>, one< ',' >, ignored >, star<ignored>, rule_guard_op > {};
	template< typename Literal, typename Identifier >
	struct rule_guard_optional
			: sor< try_catch< rule_guard<Literal, Identifier> >, rule_guard_clear > {};

	// ---------------------------------------------------------------------------
	// Rule body wrapper
	template< typename Literal, typename Identifier >
	struct rule_body
	{
		using rule_t = rule_body;
		using subs_t = type_list< body::internal::constraint_call_sequence< Literal, Identifier > >;

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
			ast_builder::AstBodyBuilder body_res(res.chr_constraints);
			bool ret = Control< body::constraint_call_sequence< Literal, Identifier > >::template match< A, M, Action, Control >( in, body_res );
			if (ret) {
				PositionInfo pos(in.position());
				if constexpr( A == apply_mode::action ) {
					res.set_body( std::move(body_res.body_stack.at( 0 )) );
				}
			}
			return ret;
		}
	};

	struct rule_operator
		: sor< TAO_PEGTL_KEYWORD("==>"), TAO_PEGTL_KEYWORD("=>>"), TAO_PEGTL_KEYWORD("<=>") > {};

	namespace internal {
		template< typename Literal, typename Identifier >
		struct chr_rule
    			: seq< star<ignored>, rule_name_optional<Literal, Identifier>, star<ignored>, rule_head<Literal, Identifier>, star<ignored>, opt< rule_head_split_op, star<ignored>, rule_head<Literal, Identifier> >, star<ignored>, rule_operator, star<ignored>, rule_guard_optional<Literal, Identifier>, star<ignored>, rule_body<Literal, Identifier>, star<ignored>, two< ';' > > {};
	};

	// ---------------------------------------------------------------------------
	// CHR rule wrapper
	template< typename Literal, typename Identifier >
	struct rule
	{
		using rule_t = rule;
		using subs_t = type_list< body::internal::constraint_call_sequence< Literal, Identifier > >;

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
			ast_builder::AstRuleBuilder rule_res(res.prg.rules().size(),res.prg.chr_constraints());
			bool ret = Control< internal::chr_rule<Literal,Identifier> >::template match< A, M, Action, Control >( in, rule_res );
			if (ret) {
				PositionInfo pos(in.position());
				if constexpr( A == apply_mode::action ) {
					res.add_rule( rule_res.build_rule() );
				}
			}
			return ret;
		}
	};

	// ---------------------------------------------------------------------------
	// Parse CHR constraint decl pragmas
	struct constraint_decl_pragma_value
			: sor< TAO_PEGTL_KEYWORD("no_reactivate"), TAO_PEGTL_KEYWORD("persistent") > {};
	struct constraint_decl_pragma_list
			: seq< one< '{' > , star<ignored>, list< constraint_decl_pragma_value, one< ',' >, ignored >, star<ignored>, one< '}' > > {};
	struct constraint_decl_pragma_values
			: sor< constraint_decl_pragma_list, constraint_decl_pragma_value > {};
	struct constraint_decl_pragmas
			: if_must< one< '#' >, star<ignored>, constraint_decl_pragma_values > {};

	struct constraint_arg_type_mode
			: sor< one< '+' >,  one< '-' >, one< '?' >, TAO_PEGTL_NAMESPACE::raise< constraint_arg_type_mode > > {};
	// Token used to raise a custom error message
	struct constraint_arg_type_error : success {};

	// To parse CPP types like: unsigned long int
	struct Identifier_with_space : seq< identifier_first, star< sor< one<' '>, identifier_other > > >{ };

	template< typename Literal, typename Identifier >
	struct constraint_arg_type
			: seq< constraint_arg_type_mode, star<ignored>, sor< try_catch<expression<Literal,Identifier_with_space>>, TAO_PEGTL_NAMESPACE::raise< constraint_arg_type_error > > > {};

	// ---------------------------------------------------------------------------
	// CHR constraint declaration wrapper
	template< typename Literal, typename Identifier >
	struct constraint_decl
	{
		using rule_t = constraint_decl;
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
			ast_builder::AstExpressionBuilder expr_res(res.prg.chr_constraints());
			bool ret = Control< seq< Identifier, one<'('>, star<ignored>, sor< one<')'>, seq< list< constraint_arg_type<Literal, Identifier>, one< ',' >, ignored >, one<')'> >, TAO_PEGTL_NAMESPACE::raise< constraint_arg_type_error > > > >::template match< A, M, Action, Control >( in, expr_res );
			if (ret) {
				PositionInfo pos(in.position());
				if constexpr( A == apply_mode::action ) {
					expr_res.chr_constraint_decl();
					res.add_chr_constraint( std::move(expr_res.expression_stack.at( 0 )) );
				}
			}
			return ret;
		}
	};

	struct constraint_decl_tag
			: seq< TAO_PEGTL_KEYWORD("chr_constraint"), star<ignored>, one<'>'> > {};
	template< typename Literal, typename Identifier >
	struct constraint_decl_with_pragmas
			: seq< star<ignored>, constraint_decl<Literal, Identifier>, star<ignored>, opt< constraint_decl_pragmas > > {};
	template< typename Literal, typename Identifier >
	struct constraint_decl_list
			: seq< constraint_decl_tag, list_must< constraint_decl_with_pragmas<Literal, Identifier>, one< ',' >, ignored > > {};

	// ---------------------------------------------------------------------------
	// Parse CHR include
	struct chr_include_file_name
			: plus< sor< ranges< 'a', 'z', 'A', 'Z', '0', '9', '_' >, one<'/'>, one<'.'>, one<'-'> > > {};
	struct chr_include_file
			: sor< seq< TAO_PEGTL_KEYWORD("name"), star<ignored>, one<'='>, star<ignored>, one<'"'>, chr_include_file_name, one<'"'> >, TAO_PEGTL_NAMESPACE::raise<chr_include_file> > {};
	template< typename Literal, typename Identifier >
	struct chr_include
			: seq< TAO_PEGTL_KEYWORD("chr_include"), star<ignored>, chr_include_file, star<ignored>, one<'>'> > {};

	// ---------------------------------------------------------------------------
	// CHR statement
	struct chr_statement_tag_error : success {};

	template< typename Literal, typename Identifier >
	struct chr_statement_tag
		: seq< one<'<'>, star<ignored>, sor< at<chr_tag_end>, chr_include<Literal,Identifier>, constraint_decl_list<Literal,Identifier >, TAO_PEGTL_NAMESPACE::raise<chr_statement_tag_error> > > {};

	template< typename Literal, typename Identifier >
	struct chr_statement
		: sor< chr_statement_tag<Literal, Identifier>, rule<Literal, Identifier> > {};

	template< typename Literal, typename Identifier >
	struct chr_statements
		: list< seq<chr_statement<Literal,Identifier>, discard>, star<ignored> > {};

}  // namespace chr::compiler::parser::grammar
