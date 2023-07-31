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
#
# /common/make/common.mk and /common/make/executable_building.mk must be
# included before including this file.

# Common documentation for definitions provided by this file (they are
# implemented in toolchain-specific .mk files, but share the same interface):
#
# * BUILD_JS_TO_CXX_TEST macro:
#   Compiles the JavaScript test files and links the C/C++ counterpart.
#   Arguments:
#   $1 ("CXX_SOURCES"): Paths to C/C++ sources (containing helpers needed for
#     the test) to link against.
#   $2 ("LIBS"): C/C++ static libraries to link against.
#   $3 ("JS_SOURCES"): Paths to JS sources (containing the tests).
#
# * run_test target:
#   A target that executes the tests.

.PHONY: run_test


# TODO(emaxx): Drop the constants below.

INTEGRATION_TESTING_LIB := google_smart_card_integration_testing

INTEGRATION_TESTING_DIR_PATH := $(COMMON_DIR_PATH)/integration_testing

# This constant contains the list of input paths for the Closure Compiler for
# compiling the code using this library.
INTEGRATION_TESTING_JS_COMPILER_INPUT_DIR_PATHS := \
	$(INTEGRATION_TESTING_DIR_PATH) \


# Load the toolchain-specific file.
ifeq ($(TOOLCHAIN),pnacl)
include $(ROOT_PATH)/common/integration_testing/src/build_nacl.mk
else
$(error Unexpected TOOLCHAIN "$(TOOLCHAIN)".)
endif
