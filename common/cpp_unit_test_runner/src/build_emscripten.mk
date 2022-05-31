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

# Documented in ../include.mk.
#
# Explanation:
# GTEST_HAS_PTHREAD: Make GoogleTest/GoogleMock thread-safe via pthreads. Note
#   that this must match the flag passed when building GoogleTest/GoogleMock -
#   see //third_party/googletest/webport/build/Makefile.
TEST_ADDITIONAL_CXXFLAGS := \
	-DGTEST_HAS_PTHREAD=1 \
	-I$(ROOT_PATH)/third_party/googletest/src/googlemock/include \
	-I$(ROOT_PATH)/third_party/googletest/src/googletest/include \

# Documented in ../include.mk.
#
# Explanation:
# EXIT_RUNTIME: Quit the program when the main() exits - otherwise the execution
#   will hang.
# EXPORT_NAME: Restore the standard value "Module" for this flag, overriding the
#   setting specified in
#   //common/make/internal/executable_building_emscripten.mk. Otherwise loading
#   the test under Node.js will fail.
# MODULARIZE: Disable putting the Emscripten module JavaScript loading code into
#   a factory function, overriding the setting specified in
#   //common/make/internal/executable_building_emscripten.mk. Otherwise the test
#   runner won't be loaded automatically when running the file in Node.js.
# PROXY_TO_PTHREAD: Run the main() function, and hence all test bodies, in a
#   separate thread. Otherwise, multi-threaded tests will hang as background
#   tests aren't created until the main thread yields.
TEST_ADDITIONAL_LDFLAGS := \
	-s EXIT_RUNTIME \
	-s EXPORT_NAME=Module \
	-s MODULARIZE=0 \
	-s PROXY_TO_PTHREAD \

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
#
# Note that we're using a pattern rule (the "%" character matches the ".a"
# common substring). It's done in order to tell Make execute the recipe only
# once in order to produce all four files. A more intuitive approach would be
# using the "&:" syntax, but it's only introduced in Make 4.3, which isn't
# widespread enough yet.

GOOGLETEST_LIBS_PATTERN := \
	$(LIB_DIR)/libgmock% \
	$(LIB_DIR)/libgmock_main% \
	$(LIB_DIR)/libgtest% \
	$(LIB_DIR)/libgtest_main% \

$(GOOGLETEST_LIBS_PATTERN):
	$(MAKE) -C $(ROOT_PATH)/third_party/googletest/webport/build

# Documented in ../include.mk.
#
# Implementation notes:
# The execution is performed via Node.js. We chdir into the out directory, so
# that the .data file can be loaded at runtime without specifying any path (the
# .data file is only used when the test uses resource files).
#
# Explanation of arguments:
# DISPLAY: Workaround against "Permission denied" Node.js issue.
# experimental-wasm-threads, experimental-wasm-bulk-memory: Needed for Pthreads
#   (multi-threading) support.
run_test: all
	cd $(OUT_DIR_PATH) && DISPLAY= \
		node \
		--experimental-wasm-threads \
		--experimental-wasm-bulk-memory \
		$(TARGET).js
