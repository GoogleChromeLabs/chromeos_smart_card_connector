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
# builds the C++ unit test runner using the Native Client toolchain.

# Documented in ../include.mk.
TEST_ADDITIONAL_CXXFLAGS :=
# Documented in ../include.mk.
TEST_ADDITIONAL_LDFLAGS :=

# Path to the test runner's own source files.
TEST_RUNNER_SOURCES_DIR := $(ROOT_PATH)/common/cpp_unit_test_runner/src
# Documented in ../include.mk.
TEST_RUNNER_SOURCES := \
	$(TEST_RUNNER_SOURCES_DIR)/entry_point_nacl.cc \

# C++ flags to be used when compiling the test runner's own C++ source files.
TEST_RUNNER_CXXFLAGS := \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-std=$(CXX_DIALECT) \

# Add rules for compiling the test runner's own C++ source files.
$(foreach src,$(TEST_RUNNER_SOURCES),\
	$(eval $(call COMPILE_RULE,$(src),$(TEST_RUNNER_CXXFLAGS))))

# Documented in ../include.mk.
TEST_RUNNER_LIBS := \
	gmock \
	gtest \
	ppapi_simple_cpp \
	$(DEFAULT_NACL_LIBS) \
	nacl_io \

# Documented in ../include.mk.
TEST_RUNNER_DEPS := \
	ppapi_simple_cpp \
	nacl_io \

# Rules for building the test runner's own dependencies.
$(eval $(call DEPEND_RULE,nacl_io))
$(eval $(call DEPEND_RULE,ppapi_simple_cpp))

# Rules for copying the test runner's own static files:

TEST_RUNNER_STATIC_FILES := \
	$(NACL_SDK_ROOT)/getting_started/part2/common.js \
	$(TEST_RUNNER_SOURCES_DIR)/index_nacl.html \

$(foreach static_file,$(TEST_RUNNER_STATIC_FILES),\
	$(eval $(call COPY_TO_OUT_DIR_RULE,$(static_file))))

# TODO
$(OUT_DIR_PATH)/nacl_io_manifest.txt: generate_out
	$(NACL_SDK_ROOT)/tools/genhttpfs.py \
		--srcdir "$(OUT_DIR_PATH)" \
		--recursive . \
		> nacl_io_manifest.txt.build
	@mv nacl_io_manifest.txt.build $(OUT_DIR_PATH)/nacl_io_manifest.txt
$(eval $(call CLEAN_RULE,nacl_io_manifest.txt.build))
all: $(OUT_DIR_PATH)/nacl_io_manifest.txt

# Documented in ../include.mk.
#
# Implementation notes:
# The tests are executed by starting a Chrome instance with the test runner's
# HTML page. A web server is additionally started that serves the page and the
# NaCl module's files.
#
# Note: The recipe uses variables that are defined by the NaCl's makefiles.
run_test: all
	$(RUN_PY) \
		-C $(OUT_DIR_PATH) \
		-P "index_nacl.html?tc=$(TOOLCHAIN)&config=$(CONFIG)" \
		$(addprefix -E ,$(CHROME_ENV)) \
		-- \
		"$(CHROME_PATH)" \
		$(CHROME_ARGS) \
		--register-pepper-plugins="$(PPAPI_DEBUG),$(PPAPI_RELEASE)" \
