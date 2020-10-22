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


ADDITIONAL_TEST_LIBS_PREFIX := \
	google_smart_card_common \

ADDITIONAL_TEST_LIBS_SUFFIX := \
	nacl_io \


ADDITIONAL_TEST_DEPS = \
	nacl_io \
	google_smart_card_common:.. \


TEST_SOURCES_PATH := ../../src

TEST_SOURCES_SUBDIR := google_smart_card_common

TEST_SOURCES := \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/formatting_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/logging/hex_dumping_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/messaging/typed_message_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/messaging/typed_message_router_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/numeric_conversions_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/pp_var_utils/construction_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/pp_var_utils/enum_converter_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/pp_var_utils/struct_converter_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/requesting/async_request_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/requesting/async_requests_storage_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/value_conversion_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/value_debug_dumping_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/value_nacl_pp_var_conversion_unittest.cc \
	$(TEST_SOURCES_PATH)/$(TEST_SOURCES_SUBDIR)/value_unittest.cc \

ADDITIONAL_TEST_CPPFLAGS := \
	-I$(TEST_SOURCES_PATH) \


include ../../../tests_runner/build.mk
