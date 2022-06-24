# Copyright 2022 Google Inc. All Rights Reserved.
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
# builds the C++ unit test runner using the "analysis" toolchains (sanitizers,
# coverage).
#
# Note that the implementation is mostly similar to the one at
# build-emscripten.mk.

# Allow passing additional parameters to GTest via setting this environment
# variable. E.g.:
#   GTEST_ARGS="--gtest_filter=*Foo" make test
GTEST_ARGS ?=

TEST_ADDITIONAL_CXXFLAGS := \
	-DGTEST_HAS_PTHREAD=1 \
	-I$(ROOT_PATH)/third_party/googletest/src/googlemock/include \
	-I$(ROOT_PATH)/third_party/googletest/src/googletest/include \

TEST_ADDITIONAL_LDFLAGS :=

TEST_RUNNER_SOURCES :=

TEST_RUNNER_LIBS := \
	gtest_main \
	gmock \
	gtest \

TEST_RUNNER_DEPS := \

GOOGLETEST_LIBS_PATTERN := \
	$(LIB_DIR)/libgmock% \
	$(LIB_DIR)/libgmock_main% \
	$(LIB_DIR)/libgtest% \
	$(LIB_DIR)/libgtest_main% \

$(GOOGLETEST_LIBS_PATTERN):
	$(MAKE) -C $(ROOT_PATH)/third_party/googletest/webport/build

TEST_ENV :=

ifeq ($(TOOLCHAIN),coverage)

# Write the collected coverage profile into ./{Debug|Release}.profraw, so that
# we can later merge these profiles from all runs and build a summarized report.
# Use "CURDIR" instead of ".", to use an absolute path that doesn't depend on
# the program's current directory.
TEST_ENV += \
	LLVM_PROFILE_FILE="$(CURDIR)/$(CONFIG).profraw" \

endif

# Execute the test binary.
#
# Chdir into the "out/$TARGET" directory, so that resource files are available
# under relative paths (./executable-module-filesystem/...). (We only need this
# for the "analysis" toolchains, because other toolchains provide built-in ways
# of exposing resource files through a virtual file system.)
run_test: all
	cd $(OUT_DIR_PATH) && \
		$(TEST_ENV) ./$(TARGET) $(GTEST_ARGS)
