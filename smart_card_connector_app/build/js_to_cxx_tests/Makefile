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
include $(ROOT_PATH)/common/cpp/include.mk
include $(ROOT_PATH)/common/js/include.mk
include $(ROOT_PATH)/common/integration_testing/include.mk
include $(ROOT_PATH)/third_party/ccid/webport/include.mk
include $(ROOT_PATH)/third_party/libusb/webport/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/common/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/server/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/include.mk


SOURCE_DIR := $(ROOT_PATH)/smart_card_connector_app/src/

# TODO(emaxx): application.cc is also compiled for other targets
# (../executable_module/ and ../executable_module/cpp_unittests/). Get rid of
# duplicate compilation by putting it into a static library.
CXX_SOURCES := \
  $(SOURCE_DIR)/application.cc \
  $(SOURCE_DIR)/application_integration_test_helper.cc \

CXXFLAGS := \
  -I$(ROOT_PATH) \
  -I$(ROOT_PATH)/common/cpp/src \
  -I$(ROOT_PATH)/common/integration_testing/src \
  -I$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src \
  -I$(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/src \
  -Wall \
  -Werror \
  -Wextra \
  -Wno-zero-length-array \
  -pedantic \
  -std=$(CXX_DIALECT) \

LIBS := \
  $(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_LIB) \
  $(PCSC_LITE_SERVER_LIB) \
  $(PCSC_LITE_COMMON_LIB) \
  $(CCID_LIB) \
  $(LIBUSB_LIB) \
  $(CPP_COMMON_LIB) \
  $(DEFAULT_NACL_LIBS) \

JS_SOURCES_PATHS := \
  $(SOURCE_DIR)/**-jstocxxtest.js \
  $(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \

# Target that compiles C++ files.
$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

# Targets that build the resulting executable module and JS tests.
$(eval $(call BUILD_JS_TO_CXX_TEST,$(CXX_SOURCES),$(LIBS),$(JS_SOURCES_PATHS)))
