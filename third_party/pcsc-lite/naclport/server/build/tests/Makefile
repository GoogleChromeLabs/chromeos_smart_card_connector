# Copyright 2022 Google Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

include ../../../../../../common/cpp_unit_test_runner/include.mk

SOURCES := \
  ../../src/google_smart_card_pcsc_lite_server/global_unittest.cc \

CXXFLAGS := \
  -I$(ROOT_PATH) \
  -I$(ROOT_PATH)/common/cpp/src \
  -pedantic \
  -Wall \
  -Werror \
  -Wextra \
  -std=$(CXX_DIALECT) \
  $(TEST_ADDITIONAL_CXXFLAGS) \

# The libraries are listed in the topological order: a library should only use
# symbols from subsequent ones.
LIBS := \
  google_smart_card_pcsc_lite_server \
  google_smart_card_ccid \
  google_smart_card_libusb \
  google_smart_card_common \

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

LIBS += \
  nacl_io \

$(OUT_DIR_PATH)/nacl_io_manifest.txt: generate_out
	$(NACL_SDK_ROOT)/tools/genhttpfs.py \
		--srcdir "$(OUT_DIR_PATH)" \
		--recursive . \
		> $(OUT_DIR_ROOT_PATH)/nacl_io_manifest.txt
	@mv \
		$(OUT_DIR_ROOT_PATH)/nacl_io_manifest.txt \
		$(OUT_DIR_PATH)/nacl_io_manifest.txt
all: $(OUT_DIR_PATH)/nacl_io_manifest.txt

endif

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LINK_EXECUTABLE_RULE,$(SOURCES) $(TEST_RUNNER_SOURCES),\
  $(LIBS) $(TEST_RUNNER_LIBS),$(TEST_RUNNER_DEPS),\
  $(LDFLAGS) $(TEST_ADDITIONAL_LDFLAGS)))

$(eval $(call COPY_TO_OUT_DIR_RULE,$(ROOT_PATH)/third_party/pcsc-lite/naclport/server/src/fake_socket_file,executable-module-filesystem/pcsc/))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(ROOT_PATH)/third_party/ccid/webport/build/Info.plist,executable-module-filesystem/pcsc/drivers/ifd-ccid.bundle/Contents/))
