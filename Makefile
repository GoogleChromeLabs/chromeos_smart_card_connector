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

# This is a top-level makefile that builds all libraries, programs,
# extensions/apps in this project. This makefile hardcodes dependencies between
# the targets, allowing parallel builds.


# Common definitions ############################

# Declare "all" as the default target that is built when "make" is run without
# extra arguments.
all:
.PHONY: all


# Toolchain-independent targets ####################

APPLICATION_TARGETS :=

LIBRARY_TARGETS := \
	common/cpp/build \
	third_party/ccid/webport/build \
	third_party/libusb/webport/build \
	third_party/pcsc-lite/naclport/build_configuration \
	third_party/pcsc-lite/naclport/common/build \
	third_party/pcsc-lite/naclport/cpp_client/build \
	third_party/pcsc-lite/naclport/cpp_demo/build \
	third_party/pcsc-lite/naclport/server/build \
	third_party/pcsc-lite/naclport/server_clients_management/build \

example_cpp_smart_card_client_app/build: common/cpp/build
example_cpp_smart_card_client_app/build: third_party/pcsc-lite/naclport/common/build
example_cpp_smart_card_client_app/build: third_party/pcsc-lite/naclport/cpp_client/build
example_cpp_smart_card_client_app/build: third_party/pcsc-lite/naclport/cpp_demo/build
smart_card_connector_app/build: common/cpp/build
smart_card_connector_app/build: third_party/ccid/webport/build
smart_card_connector_app/build: third_party/libusb/webport/build
smart_card_connector_app/build: third_party/pcsc-lite/naclport/common/build
smart_card_connector_app/build: third_party/pcsc-lite/naclport/server/build
smart_card_connector_app/build: third_party/pcsc-lite/naclport/server_clients_management/build
third_party/ccid/webport/build: third_party/pcsc-lite/naclport/build_configuration
third_party/pcsc-lite/naclport/common/build: third_party/pcsc-lite/naclport/build_configuration
third_party/pcsc-lite/naclport/cpp_client/build: third_party/pcsc-lite/naclport/build_configuration
third_party/pcsc-lite/naclport/cpp_demo/build: third_party/pcsc-lite/naclport/build_configuration
third_party/pcsc-lite/naclport/server/build: third_party/pcsc-lite/naclport/build_configuration
third_party/pcsc-lite/naclport/server_clients_management/build: third_party/pcsc-lite/naclport/build_configuration

TEST_TARGETS := \
	common/cpp/build/tests \
	smart_card_connector_app/build/executable_module/cpp_unittests \
	third_party/libusb/webport/build/tests \

common/cpp/build/tests: common/cpp/build
smart_card_connector_app/build/executable_module/cpp_unittests: smart_card_connector_app/build
third_party/libusb/webport/build/tests: common/cpp/build
third_party/libusb/webport/build/tests: third_party/libusb/webport/build

# Toolchain related definitions #################

TOOLCHAIN ?= emscripten

# Applications with C/C++ code are built in all non-sanitizer build modes
# ("emscripten"/"pnacl" are production-suitable; "coverage" needs the built
# binaries to catch parts of the code that aren't unit-tested but still linked
# into production programs).
ifneq ($(TOOLCHAIN),asan_testing)

APPLICATION_TARGETS += \
	example_cpp_smart_card_client_app/build \
	smart_card_connector_app/build \

endif

# Use our checkout of Googletest for tests in all toolchains except NaCl (whose
# SDK provides its own version).
ifneq ($(TOOLCHAIN),pnacl)

LIBRARY_TARGETS += \
	third_party/googletest/webport/build \

common/cpp/build/tests: third_party/googletest/webport/build
smart_card_connector_app/build/executable_module/cpp_unittests: third_party/googletest/webport/build
third_party/libusb/webport/build/tests: third_party/googletest/webport/build

endif

# Enable JS targets in relevant configurations.
ifneq (,$(findstring $(TOOLCHAIN),pnacl emscripten))

APPLICATION_TARGETS += \
	example_js_smart_card_client_app/build \

LIBRARY_TARGETS += \
	common/integration_testing/build \
	example_js_standalone_smart_card_client_library \

TEST_TARGETS += \
	common/js/build/unittests \
	example_cpp_smart_card_client_app/build/js_to_cxx_tests \
	smart_card_connector_app/build/js_to_cxx_tests \
	smart_card_connector_app/build/js_unittests \
	third_party/libusb/webport/build/js_unittests \
	third_party/pcsc-lite/naclport/server_clients_management/build/js_unittests \

example_cpp_smart_card_client_app/build/js_to_cxx_tests: common/cpp/build
example_cpp_smart_card_client_app/build/js_to_cxx_tests: common/integration_testing/build
example_cpp_smart_card_client_app/build/js_to_cxx_tests: example_cpp_smart_card_client_app/build
smart_card_connector_app/build/js_to_cxx_tests: common/cpp/build
smart_card_connector_app/build/js_to_cxx_tests: common/integration_testing/build
smart_card_connector_app/build/js_to_cxx_tests: smart_card_connector_app/build

endif


# Combined "all" targets ########################

# "make" or "make all" will recursively compile all targets.
.PHONY: $(APPLICATION_TARGETS) $(LIBRARY_TARGETS) $(TEST_TARGETS)
all: $(APPLICATION_TARGETS) $(LIBRARY_TARGETS) $(TEST_TARGETS)

# Compile applications via "make package".
$(APPLICATION_TARGETS):
	+$(MAKE) --directory=$@ package
# Compile libraries and tests via "make".
$(LIBRARY_TARGETS) $(TEST_TARGETS):
	+$(MAKE) --directory=$@

define CLEAN_RULE
.PHONY: clean_$(1)
clean_$(1):
	+$(MAKE) --directory=$(1) clean
clean: clean_$(1)
endef

# "make clean" will recursively clean all files produced by other targets.
.PHONY: clean
$(foreach target,$(APPLICATION_TARGETS) $(LIBRARY_TARGETS) $(TEST_TARGETS), \
                 $(eval $(call CLEAN_RULE,$(target))))


# Test targets ##################################

# The trailing blank line is needed, so that the "foreach" result is
# multi-lined, as opposed to squashing all commands into a single line.
define DO_RUN_TEST
+$(MAKE) --directory=$(1) run_test

endef

# "make test" will compile and execute all unit and integration tests.
# Implementation note: A single rule with a sequence of commands in the recipe
# is used, so that tests don't run in parallel (otherwise they might break).
.PHONY: test
test: all
	$(foreach target,$(TEST_TARGETS),$(call DO_RUN_TEST,$(target)))
