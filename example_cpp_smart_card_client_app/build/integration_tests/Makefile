# Copyright 2020 Google Inc. All Rights Reserved.
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
# Makefile for JavaScript-and-C++ integration tests.
#

TARGET := integration_tests

include ../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk
include $(COMMON_DIR_PATH)/make/executable_building.mk
include $(COMMON_DIR_PATH)/cpp/include.mk
include $(COMMON_DIR_PATH)/js/include.mk
include $(COMMON_DIR_PATH)/integration_testing/include.mk


SOURCE_DIR := ../../src/

CXX_SOURCES := \
	$(SOURCE_DIR)/chrome_certificate_provider/api_bridge.cc \
	$(SOURCE_DIR)/chrome_certificate_provider/api_bridge_integration_test_helper.cc \
	$(SOURCE_DIR)/chrome_certificate_provider/types.cc \

CXXFLAGS := \
	-I$(SOURCE_DIR) \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/common/cpp/src \
	-I$(ROOT_PATH)/common/integration_testing/src \
	-pedantic \
	-std=$(CXX_DIALECT) \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \

LIBS := \

JS_SOURCES_PATH := $(SOURCE_DIR)

# Target that compiles C++ files.
$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

# Targets that build the resulting executable module and JS tests.
$(eval $(call BUILD_JS_TO_CXX_TEST,$(CXX_SOURCES),$(LIBS),$(JS_SOURCES_PATH)))
