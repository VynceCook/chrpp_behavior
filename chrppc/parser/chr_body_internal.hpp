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

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <tao/pegtl.hpp>

namespace chr::compiler::parser::grammar::body
{
	using namespace TAO_PEGTL_NAMESPACE;
	namespace internal
	{
		/**
		 * Function that creates a sorted array from a list of elements.
		 * @param t is the list of items to be sorted
		 * @return a sorted vector<>
		 */
		template< typename T >
		[[nodiscard]] std::vector< T > sorted_operator_vector( const std::initializer_list< T >& t )
		{
			std::vector< T > v{ t };
			const auto less = []( const auto& l, const auto& r ) { return l.name < r.name; };
			std::sort( v.begin(), v.end(), less );
			return v;
		}

		/**
		 * @brief Information about infix operators
		 *
		 * This data structure stores the information of a infix
		 * operator: its name, its left priority and its right priority.
		 */
		struct infix_info
		{
			infix_info( const std::string_view n, const unsigned lbp, const unsigned rbp ) noexcept
				: name( n ),
				left_binding_power( lbp ),
				right_binding_power( rbp )
			{
				if( right_binding_power > 0 ) {
					assert( std::min( left_binding_power, right_binding_power ) & 1 );
					assert( 2 * std::min( left_binding_power, right_binding_power ) + 1 == left_binding_power + right_binding_power );
				}
				assert( left_binding_power > 0 );
			}

			std::string name;	// Name of the operator

			unsigned left_binding_power;	// Left priority
			unsigned right_binding_power;	// Right priority
		};
	
		/**
		 * Function that captures from the input stream the corresponding
		 * operator in the ops list. If no operator is found, the function returns nullptr.
		 * @param in is the input stream
		 * @param max is the maximum length of the of the desired operator
		 * @param ops is the list of available operators
		 * @param min_precedence is the minimum precedence found earlier
		 * @return nullptr if no operator is found, the description of the operator
		 * if it exists in the \a ops list.
		 */
		template< typename ParseInput, typename OperatorInfo >
		[[nodiscard]] const OperatorInfo* match_infix( ParseInput& in, const std::size_t max_length, const std::vector< OperatorInfo >& ops, const unsigned min_precedence )
		{
			const std::size_t max = std::min( max_length, in.size( max_length ) );
			for( std::string op( in.current(), max ); !op.empty(); op.pop_back() ) {
				if( const auto i = std::find_if( ops.begin(), ops.end(), [ = ]( const OperatorInfo& info ) { return info.name == op; } ); ( i != ops.end() ) && ( i->left_binding_power >= min_precedence ) ) {
					in.bump( op.size() );
					return &*i;
				}
			}
			return nullptr;
		}

	}  // namespace internal
}  // namespace chr::compiler::parser::grammar::body
