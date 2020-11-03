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

# This file contains the implementation of the ../include.mk interface that
# builds the C++ unit test runner using the Emscripten toolchain.

# Flags passed to the Emscripten compiler/linker tools in test builds.
#
# Explanation:
# MODULARIZE: Disable putting the Emscripten module JavaScript loading code into
#   a factory function, so that the test runner module is loaded automatically
#   on startup.
TEST_ADDITIONAL_EMSCRIPTEN_FLAGS := \
	-s MODULARIZE=0 \

# Documented in ../include.mk.
TEST_ADDITIONAL_CXXFLAGS := \
	$(TEST_ADDITIONAL_EMSCRIPTEN_FLAGS) \
	-I$(ROOT_PATH)/third_party/googletest/src/googlemock/include \
	-I$(ROOT_PATH)/third_party/googletest/src/googletest/include \

# Documented in ../include.mk.
TEST_ADDITIONAL_LDFLAGS := \
	$(TEST_ADDITIONAL_EMSCRIPTEN_FLAGS) \

# Documented in ../include.mk.
TEST_RUNNER_SOURCES :=

# Documented in ../include.mk.
TEST_RUNNER_LIBS := \
	gtest_main \
	gmock \
	gtest \

# Documented in ../include.mk.
TEST_RUNNER_DEPS := \

# Rules for building GoogleTest static libraries.

GOOGLETEST_LIBS := \
	$(LIB_DIR)/libgmock.a \
	$(LIB_DIR)/libgmock_main.a \
	$(LIB_DIR)/libgtest.a \
	$(LIB_DIR)/libgtest_main.a \

$(GOOGLETEST_LIBS) &:
	$(MAKE) -C $(ROOT_PATH)/third_party/googletest/webport/build

# Documented in ../include.mk.
#
# Implementation notes:
# The execution is performed via Node.js.
#
# Explanation of arguments:
# DISPLAY: Workaround against "Permission denied" Node.js issue.
# experimental-wasm-threads, experimental-wasm-bulk-memory: Needed for Pthreads
#   (multi-threading) support.
run_test: all
	DISPLAY= \
		node \
		--experimental-wasm-threads \
		--experimental-wasm-bulk-memory \
		$(OUT_DIR_PATH)/$(TARGET).js
