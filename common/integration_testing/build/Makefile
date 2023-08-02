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

# This makefile builds the "google_smart_card_integration_testing" static
# library, which contains common code for JS-to-C++ tests.

TARGET := google_smart_card_integration_testing

include ../../make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include ../include.mk


ROOT_SOURCES_PATH := ../src
ROOT_SOURCES_SUBDIR := google_smart_card_integration_testing
SOURCES_PATH := $(ROOT_SOURCES_PATH)/$(ROOT_SOURCES_SUBDIR)

SOURCES := \
	$(SOURCES_PATH)/integration_test_helper.cc \
	$(SOURCES_PATH)/integration_test_service.cc \

CXXFLAGS := \
	-I$(ROOT_SOURCES_PATH) \
	-I$(ROOT_PATH)/common/cpp/src \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-std=$(CXX_DIALECT) \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
