# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

project(examples LANGUAGES NONE)

string(REPLACE " " "" PRODUCT_NAME_NOSPACE ${PRODUCT_NAME})
if (BUILD_CLI)
  set(DOTNET_SUB_DIRECTORY "add_subdirectory(dotnet)")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGeodeNative.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/Find${PRODUCT_NAME_NOSPACE}.cmake @ONLY)

install(DIRECTORY .
  DESTINATION examples
  USE_SOURCE_PERMISSIONS
  PATTERN "*.in" EXCLUDE
  PATTERN "dotnet" EXCLUDE)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt
  DESTINATION examples)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FindGeode.cmake
  DESTINATION examples/cmake)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/Find${PRODUCT_NAME_NOSPACE}.cmake
  DESTINATION examples/cmake)

add_subdirectory(cpp)
if (BUILD_CLI)
  install(DIRECTORY dotnet
    DESTINATION examples
    USE_SOURCE_PERMISSIONS
    PATTERN "*.in" EXCLUDE)

  add_subdirectory(dotnet)
endif()
add_subdirectory(utilities)
