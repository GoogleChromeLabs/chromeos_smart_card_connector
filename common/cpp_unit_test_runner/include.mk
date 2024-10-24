# Copyright 2020 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This file provides helper definitions for building and running C++ unit tests,
# written using the GoogleTest framework.

# The resulting target name. Note that this string is also hardcoded in some of
# the toolchain-specific files pulled in below.
TARGET := cpp_unit_test_runner

include $(dir $(lastword $(MAKEFILE_LIST)))/../make/common.mk
include $(ROOT_PATH)/common/make/executable_building.mk

# Common documentation for definitions provided by this file (they are
# implemented in toolchain-specific .mk files, but share the same interface):
#
# * TEST_ADDITIONAL_CXXFLAGS:
#   Additional flags to be specified when compiling test files.
#
# * TEST_ADDITIONAL_LDFLAGS:
#   Additional flags to be specified when linking the resulting executable.
#
# * TEST_RUNNER_SOURCES:
#   Test runner's own C/C++ sources to be linked into the resulting executable.
#
# * TEST_RUNNER_LIBS:
#   Test runner's own static libraries to be linked into the resulting
#   executable.
#
# * TEST_RUNNER_DEPS:
#   Test runner's own dependencies that need to be added as prerequisites for
#   the linking of the resulting executable.
#
# * run_test:
#   A target that executes the tests.

.PHONY: run_test

# Load the toolchain-specific file.
ifeq ($(TOOLCHAIN),emscripten)
include $(ROOT_PATH)/common/cpp_unit_test_runner/src/build_emscripten.mk
else ifeq ($(TOOLCHAIN),asan_testing)
include $(ROOT_PATH)/common/cpp_unit_test_runner/src/build_analysis.mk
else ifeq ($(TOOLCHAIN),coverage)
include $(ROOT_PATH)/common/cpp_unit_test_runner/src/build_analysis.mk
else
$(error Unknown TOOLCHAIN "$(TOOLCHAIN)".)
endif
