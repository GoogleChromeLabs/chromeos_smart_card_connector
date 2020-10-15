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

TARGET := google_smart_card_common

include ../../make/common.mk

include $(COMMON_DIR_PATH)/make/nacl_module_building_common.mk

include ../include.mk


ROOT_SOURCES_PATH := ../src

ROOT_SOURCES_SUBDIR := google_smart_card_common

SOURCES_PATH := $(ROOT_SOURCES_PATH)/$(ROOT_SOURCES_SUBDIR)

SOURCES := \
	$(SOURCES_PATH)/external_logs_printer.cc \
	$(SOURCES_PATH)/formatting.cc \
	$(SOURCES_PATH)/logging/function_call_tracer.cc \
	$(SOURCES_PATH)/logging/hex_dumping.cc \
	$(SOURCES_PATH)/logging/logging.cc \
	$(SOURCES_PATH)/logging/syslog/syslog.cc \
	$(SOURCES_PATH)/messaging/typed_message.cc \
	$(SOURCES_PATH)/messaging/typed_message_router.cc \
	$(SOURCES_PATH)/multi_string.cc \
	$(SOURCES_PATH)/nacl_io_utils.cc \
	$(SOURCES_PATH)/numeric_conversions.cc \
	$(SOURCES_PATH)/pp_var_utils/construction.cc \
	$(SOURCES_PATH)/pp_var_utils/copying.cc \
	$(SOURCES_PATH)/pp_var_utils/debug_dump.cc \
	$(SOURCES_PATH)/pp_var_utils/extraction.cc \
	$(SOURCES_PATH)/pp_var_utils/operations.cc \
	$(SOURCES_PATH)/requesting/js_requester.cc \
	$(SOURCES_PATH)/requesting/js_request_receiver.cc \
	$(SOURCES_PATH)/requesting/remote_call_adaptor.cc \
	$(SOURCES_PATH)/requesting/remote_call_message.cc \
	$(SOURCES_PATH)/requesting/requester.cc \
	$(SOURCES_PATH)/requesting/requester_message.cc \
	$(SOURCES_PATH)/requesting/request_receiver.cc \
	$(SOURCES_PATH)/requesting/request_result.cc \
	$(SOURCES_PATH)/value.cc \
	$(SOURCES_PATH)/value_debug_dumping.cc \
	$(SOURCES_PATH)/value_nacl_pp_var_conversion.cc \

TEST_SUPPORT_SOURCES := \
	$(SOURCES_PATH)/integration_testing/integration_test_service.cc \

CXXFLAGS := \
	-I$(ROOT_SOURCES_PATH)/ \
	-Wextra \
	-std=gnu++11 \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))


$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))


INSTALLING_HEADERS := \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):external_logs_printer.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):formatting.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):logging/function_call_tracer.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):logging/hex_dumping.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):logging/logging.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):logging/mask_dumping.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):logging/syslog/syslog.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):messaging/message_listener.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):messaging/typed_message.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):messaging/typed_message_listener.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):messaging/typed_message_router.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):multi_string.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):nacl_io_utils.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):numeric_conversions.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):optional.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/construction.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/copying.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/debug_dump.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/enum_converter.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/extraction.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/operations.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):pp_var_utils/struct_converter.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/async_request.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/async_requests_storage.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/js_requester.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/js_request_receiver.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/remote_call_adaptor.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/remote_call_message.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/requester.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/requester_message.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/request_handler.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/request_id.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/request_receiver.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):requesting/request_result.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):thread_safe_unique_ptr.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):tuple_unpacking.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):unique_ptr_utils.h \
	$(ROOT_SOURCES_SUBDIR):$(SOURCES_PATH):value.h \

$(eval $(call NACL_LIBRARY_HEADERS_INSTALLATION_RULE,$(INSTALLING_HEADERS)))


test::
	+$(MAKE) --directory tests run

tests_clean::
	+$(MAKE) --directory tests clean

clean: tests_clean
