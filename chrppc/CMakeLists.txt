#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the Apache License, Version 2.0.
#
#  Copyright:
#     2016, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../runtime)

# Configure constants
SET(CURRENT_PROCESSED_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/constants.in.hh)
CONFIGURE_FILE(${CURRENT_PROCESSED_FILE_NAME} ${CMAKE_CURRENT_BINARY_DIR}/constants.hh @ONLY)

SET(CHRPPC_HEADERS
	constants.in.hh
	parse_error.hh
	common.hh
	dependency_graph.hh
	ast/expression.hh
	ast/body.hh
	ast/rule.hh
	ast/program.hh
	parser/reserved_keywords.hh
	visitor/expression.hh
	visitor/body.hh
	visitor/rule.hh
	visitor/occ_rule.hh
	visitor/program.hh
)

SET(CHRPPC_INLINED_SRCS
	parser/grammar.hpp
	parser/chr_grammar.hpp
	parser/chr_body_internal.hpp
	parser/chr_body_grammar.hpp
	parser/expression_internal.hpp
	parser/expression_grammar.hpp
	parser/ast_expression_builder.hpp
	parser/ast_body_builder.hpp
	parser/ast_rule_builder.hpp
	parser/ast_program_builder.hpp
	parser/actions.hpp
	parser/errors.hpp
	chrppc_parse_build.hpp
)

SET(CHRPPC_SRCS
	dependency_graph.cpp
	ast/expression.cpp
	ast/body.cpp
	ast/rule.cpp
	ast/occ_rule.cpp
	ast/program.cpp
	visitor/expression_print.cpp
	visitor/expression_print_noparent.cpp
	visitor/expression_check.cpp
	visitor/expression_apply.cpp
	visitor/body_print.cpp
	visitor/body_full_print.cpp
	visitor/body_empty.cpp
	visitor/body_apply.cpp
	visitor/body_expression_apply.cpp
	visitor/rule_print.cpp
	visitor/rule_full_print.cpp
	visitor/rule_dependency_graph.cpp
	visitor/occ_rule_print.cpp
	visitor/occ_rule_head_reorder.cpp
	visitor/occ_rule_guard_reorder.cpp
	visitor/occ_rule_update_constraint_store_indexes.cpp
	visitor/program_print.cpp
	visitor/program_set_all_persistent.cpp
	visitor/program_build_occ_rules.cpp
	visitor/program_never_stored.cpp
	visitor/program_late_storage.cpp
	visitor/abstract_code/program_abstract_code.cpp
	visitor/abstract_code/occ_rule_abstract_code.cpp
	visitor/abstract_code/body_abstract_code.cpp
	visitor/cpp_code/program_cpp_code.cpp
	visitor/cpp_code/occ_rule_cpp_code.cpp
	visitor/cpp_code/body_cpp_code.cpp
	visitor/cpp_code/expression_cpp_code.cpp
	chrppc.cpp
)

SOURCE_GROUP("Hpp Files" REGULAR_EXPRESSION ".hpp")

ADD_EXECUTABLE(chrppc ${CHRPPC_SRCS} ${CHRPPC_HEADERS} ${CHRPPC_INLINED_SRCS})
SET_TARGET_PROPERTIES(chrppc PROPERTIES COMPILE_DEFINITIONS "BUILD_CHRPP_COMPILER")

# Add install target
INSTALL(TARGETS chrppc RUNTIME DESTINATION bin)
