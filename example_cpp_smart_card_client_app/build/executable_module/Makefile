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


# Base name of the resulting binary. For example, in Emscripten/WebAssembly
# builds the resulting binary file will be called "executable_module.wasm".
TARGET := executable_module

include ../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include $(COMMON_DIR_PATH)/cpp/include.mk
include $(COMMON_DIR_PATH)/cpp/dependency.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/dependency.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/cpp_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/cpp_client/dependency.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/cpp_demo/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/cpp_demo/dependency.mk


SOURCES_DIR := ../../src

SOURCES := \
	$(SOURCES_DIR)/application.cc \
	$(SOURCES_DIR)/built_in_pin_dialog/built_in_pin_dialog_server.cc \
	$(SOURCES_DIR)/chrome_certificate_provider/api_bridge.cc \
	$(SOURCES_DIR)/chrome_certificate_provider/types.cc \
	$(SOURCES_DIR)/ui_bridge.cc \

ifeq ($(TOOLCHAIN),emscripten)

SOURCES += \
	$(SOURCES_DIR)/entry_point_emscripten.cc \

else ifeq ($(TOOLCHAIN),pnacl)

SOURCES += \
	$(SOURCES_DIR)/entry_point_nacl.cc \

endif

LIBS := \
	$(PCSC_LITE_CPP_DEMO_LIB) \
	$(PCSC_LITE_CLIENT_LIB) \
	$(PCSC_LITE_COMMON_LIB) \
	$(PCSC_LITE_CLIENT_LIB) \
	$(CPP_COMMON_LIB) \
	$(DEFAULT_NACL_LIBS) \

CPPFLAGS := \
	-I$(SOURCES_DIR) \
	-I$(ROOT_PATH)/common/cpp/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/naclport/cpp_client/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/naclport/cpp_demo/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/src/src/PCSC \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-pedantic \
	-std=gnu++11 \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS)))
