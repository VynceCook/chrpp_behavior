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

FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../VERSION CHRPP_VERSION LIMIT_COUNT 1)

PROJECT(CHR_EXTRACT_FILES VERSION ${CHRPP_VERSION})

SET(CMAKE_VERBOSE_MAKEFILE TRUE)

# Check if C++ compiler support specific options
INCLUDE(CheckCXXCompilerFlag)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

ADD_EXECUTABLE(chrpp_extract_files chrpp_extract_files.cpp)
# Add install target
INSTALL(TARGETS chrpp_extract_files RUNTIME DESTINATION bin)

