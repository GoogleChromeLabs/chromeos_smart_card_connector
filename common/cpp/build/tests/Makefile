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
# Makefile for the C++ library tests.
#
# The tests are executed through the separate tests runner, and therefore its
# makefile has to be used - with our specific options plugged into it through a
# bunch of make variables.
#

include ../../../cpp_unit_test_runner/include.mk

ROOT_SOURCES_PATH := ../../src
ROOT_SOURCES_SUBDIR := google_smart_card_common
SOURCES_PATH := $(ROOT_SOURCES_PATH)/$(ROOT_SOURCES_SUBDIR)

SOURCES := \
	$(SOURCES_PATH)/formatting_unittest.cc \
	$(SOURCES_PATH)/logging/hex_dumping_unittest.cc \
	$(SOURCES_PATH)/messaging/typed_message_router_unittest.cc \
	$(SOURCES_PATH)/numeric_conversions_unittest.cc \
	$(SOURCES_PATH)/requesting/async_request_unittest.cc \
	$(SOURCES_PATH)/requesting/async_requests_storage_unittest.cc \
	$(SOURCES_PATH)/value_conversion_unittest.cc \
	$(SOURCES_PATH)/value_debug_dumping_unittest.cc \
	$(SOURCES_PATH)/value_unittest.cc \

ifeq ($(TOOLCHAIN),pnacl)

SOURCES += \
	$(SOURCES_PATH)/pp_var_utils/construction_unittest.cc \
	$(SOURCES_PATH)/pp_var_utils/enum_converter_unittest.cc \
	$(SOURCES_PATH)/pp_var_utils/struct_converter_unittest.cc \
	$(SOURCES_PATH)/value_nacl_pp_var_conversion_unittest.cc \

endif

CXXFLAGS := \
	-I$(ROOT_SOURCES_PATH)/ \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-std=gnu++11 \
	$(TEST_ADDITIONAL_CXXFLAGS) \

LIBS := \
	google_smart_card_common \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES) $(TEST_RUNNER_SOURCES),\
	$(LIBS) $(TEST_RUNNER_LIBS),$(TEST_RUNNER_DEPS),$(TEST_ADDITIONAL_LDFLAGS)))
