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


TARGET := executable_module

include ../../../common/make/common.mk
include $(COMMON_DIR_PATH)/make/executable_building.mk
include $(THIRD_PARTY_DIR_PATH)/libusb/webport/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/server/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/server_clients_management/include.mk
# Depends on pcsc-lite/webport/common/include.mk.
include $(THIRD_PARTY_DIR_PATH)/ccid/webport/include.mk
# Depends on pcsc-lite/webport/common/include.mk.
include $(THIRD_PARTY_DIR_PATH)/driver-hid5021/webport/include.mk


# Common build definitions:

SOURCES_DIR := ../../src

SOURCES := \

LIBS := \
	smart_card_connector_application \
	google_smart_card_pcsc_lite_server_clients_management \
	google_smart_card_pcsc_lite_server \
	google_smart_card_pcsc_lite_common \
	google_smart_card_ccid \
	google_smart_card_libusb \
	google_smart_card_common \

DEPS :=

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-pedantic \
	-std=$(CXX_DIALECT) \

LDFLAGS :=

ifeq ($(TOOLCHAIN),emscripten)

# Emscripten specific build definitions:

SOURCES += \
	$(SOURCES_DIR)/entry_point_emscripten.cc \

else ifeq ($(TOOLCHAIN),coverage)

# Coverage-specific build definitions:

# * "empty_main": Provides an empty main() function, to avoid linking errors as
#   Emscripten builds don't need main().
SOURCES += \
	$(ROOT_PATH)/common/cpp/src/empty_main.cc \

endif

# Package resource files that the executable module needs at runtime:
$(eval $(call ADD_RESOURCE_RULE, \
	$(ROOT_PATH)/third_party/pcsc-lite/webport/server/src/fake_socket_file, \
	executable-module-filesystem/pcsc/fake_socket_file))
$(eval $(call ADD_RESOURCE_RULE, \
	$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist, \
	$(CCID_CONFIG_INSTALLATION_PATH)))
$(eval $(call ADD_RESOURCE_RULE, \
  $(ROOT_PATH)/third_party/driver-hid5021/src/Info.plist, \
  $(DRIVER5021_CONFIG_INSTALLATION_PATH)))

# Rules for compiling source files and linking them into an executable binary.
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))
$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS),$(DEPS),$(LDFLAGS)))
