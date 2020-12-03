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


TARGET := nacl_module

include ../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include $(COMMON_DIR_PATH)/cpp/include.mk
include $(COMMON_DIR_PATH)/cpp/dependency.mk

include $(THIRD_PARTY_DIR_PATH)/ccid/naclport/include.mk
include $(THIRD_PARTY_DIR_PATH)/ccid/naclport/dependency.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/naclport/include.mk
include $(THIRD_PARTY_DIR_PATH)/libusb/naclport/dependency.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/dependency.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server/dependency.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server_clients_management/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server_clients_management/dependency.mk


# Common build definitions:

SOURCES_DIR := ../../src

SOURCES := \
	$(SOURCES_DIR)/application.cc \

LIBS := \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_LIB) \
	$(PCSC_LITE_SERVER_LIB) \
	$(PCSC_LITE_COMMON_LIB) \
	$(CCID_LIB) \
	$(LIBUSB_LIB) \
	$(CPP_COMMON_LIB) \
	$(DEFAULT_NACL_LIBS) \

DEPS :=

CXXFLAGS := \
	-I$(ROOT_PATH)/common/cpp/src \
	-I$(ROOT_PATH)/third_party/libusb/naclport/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src \
	-I$(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/src \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-pedantic \
	-std=$(CXX_DIALECT) \

ifeq ($(TOOLCHAIN),pnacl)

# Native Client specific build definitions:

SOURCES += \
	$(SOURCES_DIR)/entry_point_nacl.cc \

LIBS += \
	nacl_io \

DEPS += \
	nacl_io \

$(eval $(call DEPEND_RULE,nacl_io))

endif


# Common build rules:

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS),$(DEPS)))


$(eval $(call COLLECT_DEPENDENCY_OUT,$(CCID_OUT)))
$(eval $(call COLLECT_DEPENDENCY_OUT,$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_OUT)))
$(eval $(call COLLECT_DEPENDENCY_OUT,$(PCSC_LITE_SERVER_OUT)))
