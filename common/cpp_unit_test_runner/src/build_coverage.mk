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
# builds the C++ unit test runner using the "coverage" toolchain.
#
# Note that the implementation is mostly similar to the one at
# build-emscripten.mk.

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

# Execute the test binary. Write the collected coverage profile into
# ./{Debug|Release}.profraw, so that we can later merge these profiles from all
# runs and build a summarized report.
run_test: all
	LLVM_PROFILE_FILE="$(CURDIR)/$(CONFIG).profraw" \
		$(OUT_DIR_PATH)/$(TARGET)
