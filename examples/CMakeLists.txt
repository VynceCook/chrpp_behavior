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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:${RESERVED_CALL_STACK}")
ELSEIF (MINGW)
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,${RESERVED_CALL_STACK}")
ELSEIF (WIN32 AND (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang))
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,${RESERVED_CALL_STACK}")
ENDIF()

SET(CHRPP_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/backtrack.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/constraint_store.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/constraint_store_index.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/constraint_stores_iterator.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/history.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/statistics.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/trace.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/shared_obj.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/bt_list.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/bt_interval.hh
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/robin_growth_policy.h
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/robin_hash.h
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/robin_map.h
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/robin_set.h
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/xxh3.h
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/xxhash.h
)

SET(CHRPP_INLINED_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/constraint_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/constraint_store_index.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/logical_var.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/options.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/strategy.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/bt_list.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/utils.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/bt_interval.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../runtime/third_party/xxhash.hpp
)

SET(CHRPP_EXAMPLES_FILES
	behavior.chrpp
)

SOURCE_GROUP("Hpp Files" REGULAR_EXPRESSION ".hpp")
SOURCE_GROUP("Chrpp Files" REGULAR_EXPRESSION ".chrpp")

SET(chrppc_parameters "")

SET(ENABLE_TRACE OFF CACHE BOOL "Set to true to generate output with trace properties.")
IF (ENABLE_TRACE)
	ADD_DEFINITIONS(-DENABLE_TRACE)
	SET(chrppc_parameters ${chrppc_parameters} --trace)
ENDIF(ENABLE_TRACE)

SET(ENABLE_LINE_ERROR ON CACHE BOOL "Enable friendly line error in chrpp source file")
IF(ENABLE_LINE_ERROR)
	SET(chrppc_parameters ${chrppc_parameters} --enable-line_error)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-line_error)
ENDIF()

SET(ENABLE_CONSTRAINT_STORE_INDEX ON CACHE BOOL "Enable the use of an indexing data structure for managing constraint store")
IF(ENABLE_CONSTRAINT_STORE_INDEX)
	SET(chrppc_parameters ${chrppc_parameters} --enable-constraint_store_index)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-constraint_store_index)
ENDIF()

SET(ENABLE_WARNING_UNUSED_RULE ON CACHE BOOL "Enable warning about unused ruled detection")
IF(ENABLE_WARNING_UNUSED_RULE)
	SET(chrppc_parameters ${chrppc_parameters} --enable-warning_unused_rule)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-warning_unused_rule)
ENDIF()

SET(ENABLE_NEVER_STORED ON CACHE BOOL "Enable never stored optimization")
IF(ENABLE_NEVER_STORED)
	SET(chrppc_parameters ${chrppc_parameters} --enable-never_stored)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-never_stored)
ENDIF()

SET(ENABLE_HEAD_REORDER ON CACHE BOOL "Enable head reorder optimization")
IF(ENABLE_HEAD_REORDER)
	SET(chrppc_parameters ${chrppc_parameters} --enable-head_reorder)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-head_reorder)
ENDIF()

SET(ENABLE_GUARD_REORDER ON CACHE BOOL "Enable guard reorder optimization")
IF(ENABLE_GUARD_REORDER)
	SET(chrppc_parameters ${chrppc_parameters} --enable-guard_reorder)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-guard_reorder)
ENDIF()

SET(ENABLE_OCCURRENCES_REORDER ON CACHE BOOL "Enable occurrences reorder optimization")
IF(ENABLE_OCCURRENCES_REORDER)
	SET(chrppc_parameters ${chrppc_parameters} --enable-occurrences_reorder)
ELSE()
	SET(chrppc_parameters ${chrppc_parameters} --disable-occurrences_reorder)
ENDIF()

SET(ENABLE_STATISTICS ON CACHE BOOL "Enable runtime statistics")
IF(ENABLE_STATISTICS)
	ADD_DEFINITIONS(-DENABLE_STATISTICS)
ENDIF()

SET(ENABLE_MEMORY_STATISTICS OFF CACHE BOOL "Enable runtime memory statistics")
IF(ENABLE_MEMORY_STATISTICS)
	ADD_DEFINITIONS(-DENABLE_MEMORY_STATISTICS)
ENDIF()

FOREACH (chrpp_file ${CHRPP_EXAMPLES_FILES})
	GET_FILENAME_COMPONENT(chrpp_file_name ${chrpp_file} NAME_WE)
	EXECUTE_PROCESS(
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../misc/chrpp_extract_files ${CMAKE_CURRENT_SOURCE_DIR}/${chrpp_file} 
		OUTPUT_VARIABLE CHR_AUTO_GEN_FILES
	)

	ADD_CUSTOM_COMMAND(OUTPUT ${CHR_AUTO_GEN_FILES}
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../chrppc/chrppc ${chrppc_parameters} --output_dir ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${chrpp_file}
		DEPENDS chrppc ${chrpp_file}
	)

	ADD_EXECUTABLE(${chrpp_file_name} ${CHR_AUTO_GEN_FILES} ${chrpp_file} ${CHRPP_HEADERS} ${CHRPP_INLINED_SRCS})
	# Add install target
	INSTALL(TARGETS ${chrpp_file_name} RUNTIME DESTINATION bin)
ENDFOREACH ()
