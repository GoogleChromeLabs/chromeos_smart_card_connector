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

# Makefile for the C++ library tests.
#
# The makefile is based on the helper definitions provided by
# "cpp_unit_test_runner".

include ../../../cpp_unit_test_runner/include.mk

ROOT_SOURCES_PATH := ../../src
ROOT_SOURCES_SUBDIR := google_smart_card_common
SOURCES_PATH := $(ROOT_SOURCES_PATH)/$(ROOT_SOURCES_SUBDIR)

SOURCES := \
	$(SOURCES_PATH)/formatting_unittest.cc \
	$(SOURCES_PATH)/ipc_emulation_unittest.cc \
	$(SOURCES_PATH)/logging/hex_dumping_unittest.cc \
	$(SOURCES_PATH)/logging/logging_unittest.cc \
	$(SOURCES_PATH)/messaging/typed_message_router_unittest.cc \
	$(SOURCES_PATH)/multi_string_unittest.cc \
	$(SOURCES_PATH)/numeric_conversions_unittest.cc \
	$(SOURCES_PATH)/requesting/async_request_unittest.cc \
	$(SOURCES_PATH)/requesting/async_requests_storage_unittest.cc \
	$(SOURCES_PATH)/requesting/remote_call_arguments_conversion_unittest.cc \
	$(SOURCES_PATH)/value_conversion_unittest.cc \
	$(SOURCES_PATH)/value_debug_dumping_unittest.cc \
	$(SOURCES_PATH)/value_unittest.cc \
	$(ROOT_SOURCES_PATH)/public/value_builder_unittest.cc \

ifeq ($(TOOLCHAIN),emscripten)

SOURCES += \
	$(SOURCES_PATH)/value_emscripten_val_conversion_unittest.cc \

else ifeq ($(TOOLCHAIN),pnacl)

SOURCES += \
	$(SOURCES_PATH)/value_nacl_pp_var_conversion_unittest.cc \

endif

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(ROOT_SOURCES_PATH)/ \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-std=$(CXX_DIALECT) \
	$(TEST_ADDITIONAL_CXXFLAGS) \

LIBS := \
	google_smart_card_common \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES) $(TEST_RUNNER_SOURCES),\
	$(LIBS) $(TEST_RUNNER_LIBS),$(TEST_RUNNER_DEPS),$(TEST_ADDITIONAL_LDFLAGS)))
