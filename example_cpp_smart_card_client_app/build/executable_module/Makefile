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
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/cpp_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/cpp_demo/include.mk


SOURCES_DIR := ../../src

# List of sources to compile:
SOURCES := \
	$(SOURCES_DIR)/application.cc \
	$(SOURCES_DIR)/built_in_pin_dialog/built_in_pin_dialog_server.cc \
	$(SOURCES_DIR)/chrome_certificate_provider/api_bridge.cc \
	$(SOURCES_DIR)/chrome_certificate_provider/types.cc \
	$(SOURCES_DIR)/ui_bridge.cc \

ifeq ($(TOOLCHAIN),emscripten)

# Sources specific to Emscripten builds:
SOURCES += \
	$(SOURCES_DIR)/entry_point_emscripten.cc \

else ifeq ($(TOOLCHAIN),coverage)

# Sources specific to Coverage builds:
#
# * "empty_main": Provides an empty main() function, to avoid linking errors as
#   Emscripten builds don't need main().
SOURCES += \
	$(ROOT_PATH)/common/cpp/src/empty_main.cc \

endif

LIBS := \
	google_smart_card_pcsc_lite_cpp_demo \
	google_smart_card_pcsc_lite_client \
	google_smart_card_pcsc_lite_common \
	google_smart_card_pcsc_lite_client \
	google_smart_card_common \

CPPFLAGS := \
	-I$(SOURCES_DIR) \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/common/cpp/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/webport/cpp_client/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/webport/cpp_demo/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/src/src/PCSC \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-pedantic \
	-std=$(CXX_DIALECT) \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS)))
