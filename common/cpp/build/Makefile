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

include $(COMMON_DIR_PATH)/make/executable_building.mk

include ../include.mk


SOURCES_PATH := ../src

SOURCES := \
	$(SOURCES_PATH)/public/formatting.cc \
	$(SOURCES_PATH)/public/ipc_emulation.cc \
	$(SOURCES_PATH)/public/logging/function_call_tracer.cc \
	$(SOURCES_PATH)/public/logging/hex_dumping.cc \
	$(SOURCES_PATH)/public/logging/logging.cc \
	$(SOURCES_PATH)/public/logging/syslog/syslog.cc \
	$(SOURCES_PATH)/public/messaging/typed_message.cc \
	$(SOURCES_PATH)/public/messaging/typed_message_router.cc \
	$(SOURCES_PATH)/public/multi_string.cc \
	$(SOURCES_PATH)/public/numeric_conversions.cc \
	$(SOURCES_PATH)/public/requesting/js_request_receiver.cc \
	$(SOURCES_PATH)/public/requesting/js_requester.cc \
	$(SOURCES_PATH)/public/requesting/remote_call_adaptor.cc \
	$(SOURCES_PATH)/public/requesting/remote_call_arguments_conversion.cc \
	$(SOURCES_PATH)/public/requesting/remote_call_message.cc \
	$(SOURCES_PATH)/public/requesting/request_receiver.cc \
	$(SOURCES_PATH)/public/requesting/request_result.cc \
	$(SOURCES_PATH)/public/requesting/requester.cc \
	$(SOURCES_PATH)/public/requesting/requester_message.cc \
	$(SOURCES_PATH)/public/testing_global_context.cc \
	$(SOURCES_PATH)/public/value.cc \
	$(SOURCES_PATH)/public/value_builder.cc \
	$(SOURCES_PATH)/public/value_conversion.cc \
	$(SOURCES_PATH)/public/value_debug_dumping.cc \

ifeq ($(TOOLCHAIN),pnacl)

SOURCES += \
	$(SOURCES_PATH)/public/global_context_impl_nacl.cc \
	$(SOURCES_PATH)/public/nacl_io_utils.cc \
	$(SOURCES_PATH)/public/value_nacl_pp_var_conversion.cc \

# TODO(#185): Migrate these to support the Emscripten toolchain as well.
SOURCES += \
	$(SOURCES_PATH)/public/external_logs_printer.cc \

else ifeq ($(TOOLCHAIN),emscripten)

SOURCES += \
	$(SOURCES_PATH)/public/global_context_impl_emscripten.cc \
	$(SOURCES_PATH)/public/value_emscripten_val_conversion.cc \

endif

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(SOURCES_PATH)/ \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-std=$(CXX_DIALECT) \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))


$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))


test:: all
	+$(MAKE) --directory tests run_test

tests_clean::
	+$(MAKE) --directory tests clean

clean: tests_clean
