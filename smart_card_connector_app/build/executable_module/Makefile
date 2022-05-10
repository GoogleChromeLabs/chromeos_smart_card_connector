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
include $(COMMON_DIR_PATH)/cpp/dependency.mk

include $(THIRD_PARTY_DIR_PATH)/ccid/webport/include.mk
include $(THIRD_PARTY_DIR_PATH)/ccid/webport/dependency.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/webport/include.mk
include $(THIRD_PARTY_DIR_PATH)/libusb/webport/dependency.mk

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

SOURCES += \
	$(ROOT_PATH)/common/cpp/src/empty_main.cc \

endif


# Preparation of static files for the executable module file system:

ifeq ($(TOOLCHAIN),emscripten)

# On Emscripten, the files that need to be exposed to the executable module have
# to be packaged ("preloaded") into a .data file.

# Add a flag to the Emscripten linker to package the specified directory into a
# .data file. The "@" character is used in order to specify the paths inside the
# archive.
LDFLAGS += \
	--preload-file \
	$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file@/executable-module-filesystem/pcsc/fake_socket_file \
	--preload-file \
	$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist@/executable-module-filesystem/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist \

# Make sure the changes in the packaged files trigger re-linking.
DEPS += \
	$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file \
	$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist \

# Declare the target for the .data file to depend on the .wasm file, so that
# targets that depend on the .data file only run after the file gets created by
# the linker.
$(BUILD_DIR)/$(TARGET).data: $(BUILD_DIR)/$(TARGET).wasm

# Copy the .data file into the out directory, so that it's put into the
# resulting app directory by the parent makefile.
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).data))

else ifeq ($(TOOLCHAIN),pnacl)

# On Native Client, the files that need to be exposed to the executable module
# have to be copied into the resulting extension/app as-is. The nacl_io
# library's "httpfs" file system is used by the C++ initialization code in order
# to set up access to these files at runtime.

# Add rules for copying the files into the out directory.
$(eval $(call COPY_TO_OUT_DIR_RULE,$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file,executable-module-filesystem/pcsc/))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist,executable-module-filesystem/pcsc/drivers/ifd-ccid.bundle/Contents/))

# Create the manifest file that is used by the nacl_io library in order to
# support operations like listing directories in the "httpfs" file system.
#
# Implementation note: The Python script's output is first redirected into a
# temporary ".build" file, so that a failure in the script leaves the manifest
# file not existing or not modified (so that next invocations of make don't skip
# it).
$(OUT_DIR_PATH)/nacl_io_manifest.txt: generate_out
	$(NACL_SDK_ROOT)/tools/genhttpfs.py \
		--srcdir "$(OUT_DIR_PATH)" \
		--recursive . \
		> nacl_io_manifest.txt.build
	@mv nacl_io_manifest.txt.build $(OUT_DIR_PATH)/nacl_io_manifest.txt
$(eval $(call CLEAN_RULE,nacl_io_manifest.txt.build))
all: $(OUT_DIR_PATH)/nacl_io_manifest.txt

endif


# Common build rules:

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES),$(LIBS),$(DEPS),$(LDFLAGS)))


$(eval $(call COLLECT_DEPENDENCY_OUT,$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_OUT)))
