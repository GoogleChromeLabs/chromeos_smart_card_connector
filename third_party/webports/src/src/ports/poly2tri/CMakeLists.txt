# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
cmake_minimum_required(VERSION 2.8)
INCLUDE_DIRECTORIES(./poly2tri ./poly2tri/common ./poly2tri/sweep)
FILE(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/poly2tri/*.cc)
ADD_LIBRARY(poly2tri ${sources})
SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
INSTALL(TARGETS poly2tri ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
SET(HEADER_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
INSTALL(DIRECTORY poly2tri DESTINATION "${HEADER_INSTALL_DIR}" FILES_MATCHING PATTERN "*.h")
