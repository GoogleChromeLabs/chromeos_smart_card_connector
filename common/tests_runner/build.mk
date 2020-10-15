# Copyright 2016 Google Inc. All Rights Reserved.
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


#
# Makefile for the tests runner, that is capable of executing both C++ tests
# through the Google Test framework (including the support of NaCl-specific
# code) and JavaScript tests (FIXME(emaxx): TBD).
#
# The tests are run by the "make run" command (this target is provided by the
# NaCl SDK build scripts).
#


TARGET := tests_runner

TESTS_RUNNER_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

include $(TESTS_RUNNER_DIR)/../make/common.mk

PAGE := $(OUT_DIR_PATH)/index.html

include $(COMMON_DIR_PATH)/make/nacl_module_building_common.mk


#
# Add rules for additional dependencies that the including test may have
# specified through the ADDITIONAL_TEST_DEPS variable (it should contain a
# space-separated list of dependency names with, optionally, directories
# specified after a colon).
#

ADDITIONAL_TEST_DEPS ?=

TESTS_RUNNER_DEPS := \
	ppapi_simple_cpp \

EXTRACT_DEPENDENCY_NAME = $(firstword $(subst :, ,$(1)))

EXTRACT_DEPENDENCY_PATH = $(word 2,$(subst :, ,$(1)))

define DEPEND_RULE_WITH_PATH
$(eval $(call DEPEND_RULE,$(call EXTRACT_DEPENDENCY_NAME,$(1)),$(call EXTRACT_DEPENDENCY_PATH,$(1))))
endef

$(foreach dep_with_dir,$(TESTS_RUNNER_DEPS) $(ADDITIONAL_TEST_DEPS),$(eval $(call DEPEND_RULE_WITH_PATH,$(dep_with_dir))))

DEPS := \
	$(TESTS_RUNNER_DEPS) \
	$(foreach dep_with_dir,$(ADDITIONAL_TEST_DEPS),$(call EXTRACT_DEPENDENCY_NAME,$(dep_with_dir))) \


#
# Create a list of libraries to be linked, taking into account the additional
# libraries that the including test may have specified through the
# ADDITIONAL_TEST_LIBS_PREFIX and ADDITIONAL_TEST_LIBS_SUFFIX variables.
#
# Note that, due to PPAPI libraries internal structure, ppapi_simple_cpp must be
# linked before ppapi and ppapi_cpp, while nacl_io must be linked after them.
#

ADDITIONAL_TEST_LIBS_PREFIX ?=

ADDITIONAL_TEST_LIBS_SUFFIX ?=

TESTS_RUNNER_LIBS_PREFIX := \
	gmock \
	gtest \
	ppapi_simple_cpp \

LIBS := \
	$(ADDITIONAL_TEST_LIBS_PREFIX) \
	$(TESTS_RUNNER_LIBS_PREFIX) \
	$(DEFAULT_NACL_LIBS) \
	$(TESTS_RUNNER_LIBS_SUFFIX) \
	$(ADDITIONAL_TEST_LIBS_SUFFIX) \


#
# Create compiler flags list, taking into account the additional flags that the
# including test may have specified through the ADDITIONAL_TEST_CPPFLAGS
# variable.
#

CPPFLAGS := \
	-pedantic \
	-std=gnu++11 \
	-Wall \
	-Wextra \
	-Wno-zero-length-array \
	$(ADDITIONAL_TEST_CPPFLAGS)


#
# Create the list of sources.
#
# The including test may specify its source files through the TEST_SOURCES
# variable.
#

TEST_SOURCES ?=

TESTS_RUNNER_SOURCES_DIR := $(TESTS_RUNNER_DIR)/src

TESTS_RUNNER_SOURCES := \
	$(TESTS_RUNNER_SOURCES_DIR)/tests_runner.cc \

SOURCES := \
	$(TESTS_RUNNER_SOURCES) \
	$(TEST_SOURCES) \


#
# Add the compilation rules for all sources.
#

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))


#
# Add the rules for building the NaCl module with the C++ tests.
#

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS),$(DEPS)))


#
# Add the rules for copying the tests runner static files.
#

TESTS_RUNNER_STATIC_FILES := \
	$(NACL_SDK_ROOT)/getting_started/part2/common.js \
	$(TESTS_RUNNER_SOURCES_DIR)/index.html \

$(foreach static_file,$(TESTS_RUNNER_STATIC_FILES),$(eval $(call COPY_TO_OUT_DIR_RULE,$(static_file))))


#
# Hack: change CURDIR to the out directory, so that the "run" target starts
# Chrome with the correct working directory, and re-adjust path to the loaded
# page accordingly.
#

CURDIR := $(OUT_DIR_PATH)

PAGE := index.html
