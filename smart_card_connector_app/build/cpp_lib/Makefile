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

# Builds the static library with the Smart Card Connector's core C/C++
# functionality.


TARGET := smart_card_connector_application

include ../../../common/make/common.mk
include $(ROOT_PATH)/common/make/executable_building.mk


SOURCES := \
	$(ROOT_PATH)/smart_card_connector_app/src/application.cc \

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/common/cpp/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/src \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-pedantic \
	-std=$(CXX_DIALECT) \

# Rules for compiling source files and linking them into a static library.
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))
$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
