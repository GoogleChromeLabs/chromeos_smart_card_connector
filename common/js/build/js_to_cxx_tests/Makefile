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

# Makefile for JavaScript-to-C++ integration tests for the //common/js/ code.

TARGET := integration_tests

include ../../../../common/make/common.mk

include $(ROOT_PATH)/common/make/js_building_common.mk
include $(ROOT_PATH)/common/make/executable_building.mk
include $(ROOT_PATH)/common/cpp/include.mk
include $(ROOT_PATH)/common/js/include.mk
include $(ROOT_PATH)/common/integration_testing/include.mk


CXX_SOURCES := \

CXXFLAGS := \
  -I$(ROOT_PATH) \
  -Wall \
  -Werror \
  -Wextra \
  -pedantic \
  -std=$(CXX_DIALECT) \

LIBS := \
  google_smart_card_common \

# Include *-jstocxxtest.js and non-test files needed for them.
#
# Note: unit test files are compiled and executed differently, via
# //common/js/build/unittests/.
JS_SOURCES_PATHS := \
  $(ROOT_PATH)/common/js/src \
  !$(ROOT_PATH)/common/js/src/**-unittest.js \

# Target that compiles C++ files.
$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

# Targets that build the resulting executable module and JS tests.
$(eval $(call BUILD_JS_TO_CXX_TEST,$(CXX_SOURCES),$(LIBS),$(JS_SOURCES_PATHS)))
