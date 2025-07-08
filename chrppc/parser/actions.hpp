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
#include <cstring>

#include <tao/pegtl.hpp>
#include <ast/expression.hh>

namespace chr::compiler::parser
{
	namespace pegtl = TAO_PEGTL_NAMESPACE;

	/**
	 * @brief Action to perform on matched rule
	 *
	 * Class representing an action to be performed for an element of the grammar (a rule).
	 * The class has an instantiated template argument for each element of the grammar.
	 * The default case does nothing and each specific action must be within a class
	 * of the same name for which the template argument has been specialized.
	 */
	template< typename Rule >
	struct action
		: pegtl::nothing< Rule >
	{};

	/**
	 * Specialisation of the _action_ class for a literal.
	 */
	template<>
	struct action< literal >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstExpressionBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.number( in.string(), pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for the chr_tag_begin
	 */
	template<>
	struct action< chr_tag_begin >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res._start_line_number = pos.line;
		}
	};

	/**
	 * Specialisation of the _action_ class for the chr_tag_end
	 */
	template<>
	struct action< chr_tag_end >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res._empty = false;
			res._end_line_number = pos.line;
		}
	};

	/**
	 * Specialisation of the _action_ class for a property name.
	 */
	template<>
	struct action< property_name >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			res.prg.set_name( in.string() );
		}
	};

	/**
	 * Specialisation of the _action_ class for a property auto persistent.
	 */
	template<>
	struct action< property_auto_persistent >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			std::string str_upper_in = in.string();
			std::transform(str_upper_in.begin(), str_upper_in.end(), str_upper_in.begin(), 
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
			if (str_upper_in == "TRUE")
				res.prg.set_auto_persistent( true );
			else
				res.prg.set_auto_persistent( false );
		}
	};

	/**
	 * Specialisation of the _action_ class for a property_auto_catch_failure.
	 */
	template<>
	struct action< property_auto_catch_failure >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			std::string str_upper_in = in.string();
			std::transform(str_upper_in.begin(), str_upper_in.end(), str_upper_in.begin(), 
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
			if (str_upper_in == "TRUE")
				res.prg.set_auto_catch_failure( true );
			else
				res.prg.set_auto_catch_failure( false );
		}
	};

	/**
	 * Specialisation of the _action_ class for a property parameters.
	 */
	template<>
	struct action< property_parameters >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			std::vector< std::tuple<std::string,std::string> > params;
			// Split in.string() according to ','
			std::stringstream ss (in.string());
			std::string item;
			while (getline (ss, item, ','))
			{
				// Right trim string
				item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) {
							return !std::isspace(ch);
							}).base(), item.end());
				auto const pos = item.find_last_of(' ');
				params.push_back(std::make_tuple(item.substr(0,pos),item.substr(pos + 1)));
			}
			res.prg.set_parameters( params );
		}
	};

	/**
	 * Specialisation of the _action_ class for a property template_parameters.
	 */
	template<>
	struct action< property_template_parameters >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			std::vector< std::tuple<std::string,std::string> > params;
			// Split in.string() according to ','
			std::stringstream ss (in.string());
			std::string item;
			while (getline (ss, item, ','))
			{
				// Right trim string
				item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) {
							return !std::isspace(ch);
							}).base(), item.end());
				auto const pos = item.find_last_of(' ');
				params.push_back(std::make_tuple(item.substr(0,pos),item.substr(pos + 1)));
			}
			res.prg.set_template_parameters( params );
		}
	};

	/**
	 * Specialisation of the _action_ class for an identifier.
	 */
	template<>
	struct action< pegtl::identifier >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstExpressionBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.identifier( in.string(), pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for an identifier.
	 */
	template<>
	struct action< grammar::Identifier_with_space >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstExpressionBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.identifier( in.string(), pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_count
	 */
	template< typename U, typename V >
	struct action< grammar::expression_rules::internal::chr_count<U,V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input&, ast_builder::AstExpressionBuilder& res, States&&... /*unused*/ )
		{
			res.chr_count();
		}
	};

	/**
	 * Specialisation of the _action_ class for a constraint_call_pragma_value.
	 */
	template<>
	struct action< grammar::body::chr_keyword >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.chr_keyword( in.string(), pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_behavior
	 */
	template<>
	struct action< grammar::body::chr_behavior_empty_arg >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.empty_body( pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_try_bt
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_try_bt<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.chr_try( true, pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_try
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_try<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.chr_try( false, pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_behavior
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_behavior<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.behavior( pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_exists
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_exists<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.exists_forall( "exists", pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_exists_it
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_exists_it<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.exists_forall_it( "exists", pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_forall
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_forall<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.exists_forall( "forall", pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_forall_it
	 */
	template< typename U, typename V >
	struct action< grammar::body::chr_forall_it<U, V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.exists_forall_it( "forall", pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a constraint_call_pragma_value.
	 */
	template<>
	struct action< grammar::body::constraint_call_pragma_value >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstBodyBuilder& res, States&&... /*unused*/ )
		{
			if (in.string() == "catch_failure")
			{
				PositionInfo pos(in.position());
				res.add_pragma( Pragma::catch_failure, pos );
			}
		}
	};

	/**
	 * Specialisation of the _action_ class for clearing a rule name.
	 */
	template<>
	struct action< grammar::rule_name_clear >
	{
		template< typename Input, typename... States >
		static void apply( const Input&, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			res.clear_name();
		}
	};

	/**
	 * Specialisation of the _action_ class for a rule name.
	 */
	template< typename U, typename V >
	struct action< grammar::rule_name<U,V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			res.set_name(in.string());
		}
	};

	/**
	 * Specialisation of the _action_ class for a rule head constraint pragma.
	 */
	template<>
	struct action< grammar::rule_head_pragma_value >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			assert(res.head_keep.size() > 0);
			PositionInfo pos(in.position());
			if (in.string() == "passive")
				res.add_head_pragma( Pragma::passive );
			else if (in.string() == "bang")
				res.add_head_pragma( Pragma::bang );
			else if (in.string() == "no_history")
				res.add_head_pragma( Pragma::no_history );
			else
				throw ParseError("parse error, unknown Pragma", pos);
		}
	};

	/**
	 * Specialisation of the _action_ class for the head split part operator
	 */
	template<>
	struct action< grammar::rule_head_split_op >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			res.set_split_head(in.position());
		}
	};

	/**
	 * Specialisation of the _action_ class for the rule operator
	 */
	template<>
	struct action< grammar::rule_operator >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.set_rule_operator(in.string(),pos);
			if (!res.split_head && (res.rule_op == "<=>"))
				res.head_del.swap(res.head_keep);
		}
	};

	/**
	 * Specialisation of the _action_ class for recording a rule guard.
	 */
	template<>
	struct action< grammar::rule_guard_op >
	{
		template< typename Input, typename... States >
		static void apply( const Input&, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			res.valid_guard();
		}
	};

	/**
	 * Specialisation of the _action_ class for clearing a rule guard.
	 */
	template<>
	struct action< grammar::rule_guard_clear >
	{
		template< typename Input, typename... States >
		static void apply( const Input&, ast_builder::AstRuleBuilder& res, States&&... /*unused*/ )
		{
			res.guard.clear();
		}
	};

	/**
	 * Specialisation of the _action_ class for a constraint declaration pragma.
	 */
	template<>
	struct action< grammar::constraint_decl_pragma_value >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			if (in.string() == "persistent")
				res.add_chr_constraint_pragma( Pragma::persistent );
			else if (in.string() == "no_reactivate")
				res.add_chr_constraint_pragma( Pragma::no_reactivate );
			else
				throw ParseError("parse error, unknown Pragma", pos);
		}
	};

	/**
	 * Specialisation of the _action_ class for a constraint_arg_type_mode
	 */
	template<>
	struct action< grammar::constraint_arg_type_mode >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstExpressionBuilder& res, States&&... /*unused*/ )
		{
			PositionInfo pos(in.position());
			res.identifier( in.string(), pos );
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_include_file_name
	 */
	template<>
	struct action< grammar::chr_include_file_name >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			res._include_file_name = in.string();
		}
	};

	/**
	 * Specialisation of the _action_ class for a chr_include
	 */
	template< typename U, typename V >
	struct action< grammar::chr_include<U,V> >
	{
		template< typename Input, typename... States >
		static void apply( const Input& in, ast_builder::AstProgramBuilder& res, States&&... /*unused*/ )
		{
			std::string file_name = res._include_file_name;
			try {
				file_input f_in(res._include_file_name);
				try {
					parse< grammar::chr_statements<U,V>, chr::compiler::parser::action >( f_in, res );
				}
				catch( const TAO_PEGTL_NAMESPACE::parse_error& e ) {
					throw ParseError(
							(R"_STR(from included file ")_STR" + file_name + R"_STR(")_STR").c_str(),
							in.position(),
							ParseError(std::string(e.message()).c_str(),PositionInfo(e.positions().front())),
							f_in.line_at(e.positions().front()));
				}
				catch( const chr::compiler::ParseError& e ) {
					throw ParseError(
							(R"_STR(from included file ")_STR" + file_name + R"_STR(")_STR").c_str(),
							in.position(),
							e,
							f_in.line_at(e.positions().front()));
				}
			}
			catch( const std::filesystem::filesystem_error& e ) {
				throw TAO_PEGTL_NAMESPACE::parse_error(R"_STR(filesystem error: no such file ")_STR" + file_name + R"_STR(")_STR",in);
			}
		}
	};

}  // namespace chr::compiler::parser

