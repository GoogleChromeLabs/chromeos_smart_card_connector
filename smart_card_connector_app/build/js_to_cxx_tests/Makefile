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


SOURCE_DIR := $(ROOT_PATH)/smart_card_connector_app/src/

# TODO(emaxx): Add C++ helpers for controlling the PC/SC-Lite server.
CXX_SOURCES := \

CXXFLAGS := \

LIBS := \

JS_SOURCES_PATH := $(SOURCE_DIR)/**-jstocxxtest.js

# Target that compiles C++ files.
$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

# Targets that build the resulting executable module and JS tests.
$(eval $(call BUILD_JS_TO_CXX_TEST,$(CXX_SOURCES),$(LIBS),$(JS_SOURCES_PATH)))
