# Copyright 2023 Google Inc. All Rights Reserved.
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

# Makefile for JavaScript-to-C++ integration tests.

TARGET := integration_tests

include ../../../common/make/common.mk

include $(ROOT_PATH)/common/make/js_building_common.mk
include $(ROOT_PATH)/common/make/executable_building.mk
include $(ROOT_PATH)/common/js/include.mk
include $(ROOT_PATH)/common/integration_testing/include.mk
include $(ROOT_PATH)/third_party/libusb/webport/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/common/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/js_client/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/server/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/include.mk
# Depends on pcsc-lite/naclport/common/include.mk.
include $(ROOT_PATH)/third_party/ccid/webport/include.mk
# Depends on pcsc-lite/naclport/common/include.mk.
include $(THIRD_PARTY_DIR_PATH)/driver-hid5021/webport/include.mk


SOURCE_DIR := $(ROOT_PATH)/smart_card_connector_app/src/

# TODO(emaxx): testing_smart_card_simulation.cc is also compiled for other
# targets (for ../executable_module/cpp_unittests/). Get rid of duplicate
# compilation by putting them into static libraries.
CXX_SOURCES := \
  $(ROOT_PATH)/common/cpp/src/logging_integration_test_helper.cc \
  $(SOURCE_DIR)/application_integration_test_helper.cc \
  $(SOURCE_DIR)/testing_smart_card_simulation.cc \

CXXFLAGS := \
  -I$(ROOT_PATH) \
  -Wall \
  -Werror \
  -Wextra \
  -Wno-zero-length-array \
  -pedantic \
  -std=$(CXX_DIALECT) \

LIBS := \
  smart_card_connector_application \
  google_smart_card_pcsc_lite_server_clients_management \
  google_smart_card_pcsc_lite_server \
  google_smart_card_pcsc_lite_common \
  google_smart_card_ccid \
  google_smart_card_libusb \
  google_smart_card_common \
  $(DEFAULT_NACL_LIBS) \

# In NaCl builds, extra libraries are needed.
ifeq ($(TOOLCHAIN),pnacl)

LIBS += nacl_io
$(eval $(call DEPEND_RULE,nacl_io))

endif

# Include *-jstocxxtest.js and non-test files needed for them.
JS_SOURCES_PATHS := \
  $(SOURCE_DIR) \
  !$(SOURCE_DIR)/**-unittest.js \
  $(ROOT_PATH)/third_party/libusb/webport/src \
  !$(ROOT_PATH)/third_party/libusb/webport/src/**test.js \
  $(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
  $(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
  $(PCSC_LITE_SERVER_JS_COMPILER_INPUT_DIR_PATHS) \
  $(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \

# Target that compiles C++ files.
$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

# Package resource files that the C/C++ code needs at runtime:
# * fake_socket_file: it's needed to trick the PC/SC daemon code into thinking
#   it can open a socket IPC communication.
$(eval $(call ADD_RESOURCE_RULE, \
  $(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file, \
  executable-module-filesystem/pcsc/fake_socket_file))
# * Info.plist: these are driver configs; at runtime, the PC/SC daemon code
#   reads parameters from these configs to call into the drivers.
$(eval $(call ADD_RESOURCE_RULE, \
  $(ROOT_PATH)/third_party/ccid/webport/build/Info.plist, \
  $(CCID_CONFIG_INSTALLATION_PATH)))
$(eval $(call ADD_RESOURCE_RULE, \
  $(ROOT_PATH)/third_party/driver-hid5021/src/Info.plist, \
  $(DRIVER5021_CONFIG_INSTALLATION_PATH)))

# Targets that build the resulting executable module and JS tests.
$(eval $(call BUILD_JS_TO_CXX_TEST,$(CXX_SOURCES),$(LIBS),$(JS_SOURCES_PATHS)))
