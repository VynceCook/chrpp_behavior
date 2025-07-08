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

CMAKE_MINIMUM_REQUIRED(VERSION 3.16 FATAL_ERROR)
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Release prior to calling PROJECT()
#
IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION CHRPP_VERSION LIMIT_COUNT 1)

PROJECT(CHR++ VERSION ${CHRPP_VERSION})

SET(CMAKE_VERBOSE_MAKEFILE TRUE)

SET(EXAMPLES ON CACHE BOOL "Build examples")
# Reserved call stack for examples and tests
SET(RESERVED_CALL_STACK "10000000" CACHE STRING "Reserved call stack")

IF(UNIX)
  # determine, whether we want a static binary
  SET(STATIC_LINKING FALSE CACHE BOOL "Build a static binary?")
  # do we want static libraries?
  IF(STATIC_LINKING)
    # When STATIC_LINKING is TRUE, than cmake looks for libraries ending
    # with .a. This is for linux only!
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
    # Remove flags to get rid off all the -Wl,Bydnamic
    SET(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)
    SET(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  ENDIF(STATIC_LINKING)
ENDIF(UNIX)

# Check if C++ compiler support specific options
INCLUDE(CheckCXXCompilerFlag)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

IF (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  ADD_COMPILE_OPTIONS(-Wall -Wextra -Wno-unused-local-typedefs -pipe)
  SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb -fno-inline-functions -fPIC")
  # To make MinGW able to compile big files
  IF (MINGW)
	ADD_COMPILE_OPTIONS(-Wa,-mbig-obj)
  ENDIF(MINGW)
  IF (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
	  ADD_COMPILE_OPTIONS(-DUSE_DEPRECATED_GCC_CONSTRUCT)
  ENDIF()
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  ADD_COMPILE_OPTIONS(-Wall -Wextra -Wno-unused-local-typedefs -pipe)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g")
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  ADD_COMPILE_OPTIONS(/bigobj)
ENDIF()

# Include and build chrpp_extract_files command
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/misc)
# Configure external "misc" project
EXECUTE_PROCESS(
	COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/misc
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/misc
)
# Build external "misc" project
EXECUTE_PROCESS(
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/misc
)

# Include CHRPP compiler
ADD_SUBDIRECTORY(chrppc)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/runtime)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/runtime)

# Copy all runtime files
SET(CURRENT_PROCESSED_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/runtime/chrpp.in.hh)
CONFIGURE_FILE(${CURRENT_PROCESSED_FILE_NAME} ${CMAKE_CURRENT_BINARY_DIR}/runtime/chrpp.hh @ONLY)

IF(EXAMPLES)
	ADD_SUBDIRECTORY(examples)
ENDIF(EXAMPLES)

# Add install target
INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/runtime/ DESTINATION include/chrpp)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/runtime/ DESTINATION include/chrpp)

SET(CMAKE_CODELITE_USE_TARGETS ON)
