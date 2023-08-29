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
include $(COMMON_DIR_PATH)/cpp/include.mk
include $(THIRD_PARTY_DIR_PATH)/ccid/webport/include.mk
include $(THIRD_PARTY_DIR_PATH)/libusb/webport/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server_clients_management/include.mk


# Common build definitions:

SOURCES_DIR := ../../src

SOURCES := \

LIBS := \
	smart_card_connector_application \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_LIB) \
	$(PCSC_LITE_SERVER_LIB) \
	$(PCSC_LITE_COMMON_LIB) \
	$(CCID_LIB) \
	$(LIBUSB_LIB) \
	$(CPP_COMMON_LIB) \
	$(DEFAULT_NACL_LIBS) \

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

else ifeq ($(TOOLCHAIN),pnacl)

# Native Client specific build definitions:

SOURCES += \
	$(SOURCES_DIR)/entry_point_nacl.cc \

LIBS += \
	nacl_io \

DEPS += \
	nacl_io \

$(eval $(call DEPEND_RULE,nacl_io))

else ifeq ($(TOOLCHAIN),coverage)

# Coverage-specific build definitions:

# * "empty_main": Provides an empty main() function, to avoid linking errors as
#   NaCl/Emscripten builds don't need main().
SOURCES += \
	$(ROOT_PATH)/common/cpp/src/empty_main.cc \

endif

# Package resource files that the executable module needs at runtime:
$(eval $(call ADD_RESOURCE_RULE, \
	$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file, \
	executable-module-filesystem/pcsc/fake_socket_file))
$(eval $(call ADD_RESOURCE_RULE, \
	$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist, \
	executable-module-filesystem/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist))

# Rules for compiling source files and linking them into an executable binary.
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))
$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS),$(DEPS),$(LDFLAGS)))

# Rules for copying PC/SC-Lite config files into the resulting out/ folder.
$(eval $(call COLLECT_DEPENDENCY_OUT,$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_OUT)))


# Rules for running unit tests.
test:: all
	+$(MAKE) --directory cpp_unittests run_test

# Rules for cleaning unit test build artifacts on "make clean".
tests_clean::
	+$(MAKE) --directory cpp_unittests clean
clean: tests_clean
