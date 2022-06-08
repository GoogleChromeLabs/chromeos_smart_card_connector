# Copyright 2022 Google Inc. All Rights Reserved.
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
# This file contains the implementation of the ../executable_building.mk
# interface that builds using stock Clang, for the purpose of instrumentation:
# collecting code coverage, and passing unit tests through ASan.
#
# This script assumes that LLVM and Clang are installed on the system. (We don't
# require the developer to have them by default, and not install them in
# env/initialize.sh, because these are heavyweight tools and not everyone wants
# to run the instrumentation locally.)
#
# common.mk must be included before including this file.
#


# Specifies the build configuration (default "Release", or "Debug").
CONFIG ?= Release

# A pattern rule for "dir.stamp" files in any directory.
# See also the explanation of same rule at executable_building_emscripten.mk.
%dir.stamp :
	@mkdir -p $(dir $@)
	@echo Stamp > $@

# Directory where temporary build artifacts (like .o object files) are stored.
BUILD_DIR := ./out-artifacts-$(TOOLCHAIN)/$(CONFIG)
# Directory where library .a files (built by LIB_RULE) are installed.
LIB_DIR := $(ROOT_PATH)/env/$(TOOLCHAIN)-artifacts/$(CONFIG)

# Macro that returns the .o object file path for the given source file.
# See also the explanation of same macro at executable_building_emscripten.mk.
# Arguments:
# $1 ("SOURCE") - Source file path.
define OBJ_FILE_NAME
$(BUILD_DIR)/$(basename $(subst ..,__,$(1))).o
endef

# Macro that returns the .d dependency file path for the given source file.
# See also the explanation of same macro at executable_building_emscripten.mk.
# Arguments:
# $1 ("SOURCE") - Source file path.
define DEP_FILE_NAME
$(BUILD_DIR)/$(basename $(subst ..,__,$(1))).d
endef

# Flags passed to both compiler and linker.
#
# * "fno-omit-frame-pointer", "fno-optimize-sibling-calls": Enable better stack
#   traces (docs explicitly recommended this with sanitizers).
# * "g": Enable debug symbols.
# * "m32": Build in 32-bit mode (this is also what Emscripten and NaCl
#   toolchains use).
ANALYSIS_COMMON_FLAGS := \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-g \
	-m32 \

# Flags passed to the compiler, in addition to ANALYSIS_COMMON_FLAGS.
ANALYSIS_COMPILER_FLAGS :=

# Flags passed to the linker, in addition to ANALYSIS_COMMON_FLAGS.
ANALYSIS_LINKER_FLAGS :=

ifeq ($(TOOLCHAIN),coverage)

# Add compiler and linker flags specific to coverage builds.
#
# * "fcoverage-mapping", "fprofile-instr-generate": Enable Clang's source-based
#   coverage.
ANALYSIS_COMMON_FLAGS += \
	-fcoverage-mapping \
	-fprofile-instr-generate \

else ifeq ($(TOOLCHAIN),asan_testing)

# Add compiler and linker flags specific to ASan builds.
#
# * "fsanitize=address": Use Address Sanitizer.
ANALYSIS_COMMON_FLAGS += \
	-fsanitize=address \

else

$(error Unsupported TOOLCHAIN=$(TOOLCHAIN) value.)

endif

ifeq ($(CONFIG),Release)

# Add compiler and linker flags specific to release builds.
ANALYSIS_COMMON_FLAGS += \
  -O3 \

# Add compiler flags specific to release builds.
#
# Explanation:
# NDEBUG: Disable debug-only functionality (assertions, etc.).
ANALYSIS_COMPILER_FLAGS += \
  -DNDEBUG \

else ifeq ($(CONFIG),Debug)

# Add compiler and linker flags specific to debug builds.
ANALYSIS_COMMON_FLAGS += \
  -O0 \

else

$(error Unsupported CONFIG=$(CONFIG) value.)

endif

# Documented at ../executable_building.mk.
CXX_DIALECT := c++11

# Documented at ../executable_building.mk. The implementation is mostly similar
# to the one at executable_building_emscripten.mk.
define COMPILE_RULE
$(call OBJ_FILE_NAME,$(1)): $(1) | $(dir $(call OBJ_FILE_NAME,$(1)))dir.stamp
	clang \
		-o $(call OBJ_FILE_NAME,$(1)) \
		-c \
		-MMD \
		-MP \
		$(ANALYSIS_COMMON_FLAGS) \
		$(ANALYSIS_COMPILER_FLAGS) \
		$(2) \
		$(1)
-include $(call DEP_FILE_NAME,$(1))
all: $(call OBJ_FILE_NAME,$(1))
endef

# Documented at ../executable_building.mk. The implementation is mostly similar
# to the one at executable_building_emscripten.mk.
define LIB_RULE
$(LIB_DIR)/lib$(1).a: $(foreach src,$(2),$(call OBJ_FILE_NAME,$(src)))
	@mkdir -p $(LIB_DIR)
	@rm -f $(LIB_DIR)/lib$(1).a
	llvm-ar \
		crs \
		$(LIB_DIR)/lib$(1).a \
		$(foreach src,$(2),$(call OBJ_FILE_NAME,$(src))) \
		$(3)
all: $(LIB_DIR)/lib$(1).a

# Rules for cleaning the library files on "make clean":
.PHONY: clean_lib$(1)
clean_lib$(1):
	@rm -f $(LIB_DIR)/lib$(1).a
clean: clean_lib$(1)
endef

# Documented at ../executable_building.mk. The implementation is mostly similar
# to the one at executable_building_emscripten.mk.
define LINK_EXECUTABLE_RULE
$(BUILD_DIR)/$(TARGET): $(foreach src,$(1),$(call OBJ_FILE_NAME,$(src)))
$(BUILD_DIR)/$(TARGET): $(foreach lib,$(2),$(LIB_DIR)/lib$(lib).a)
$(BUILD_DIR)/$(TARGET): $(3)
$(BUILD_DIR)/$(TARGET): | $(BUILD_DIR)/dir.stamp
$(BUILD_DIR)/$(TARGET):
	clang++ \
		-pthread \
		-o $(BUILD_DIR)/$(TARGET) \
		-L$(LIB_DIR) \
		$(foreach src,$(1),$(call OBJ_FILE_NAME,$(src))) \
		$(foreach lib,$(2),-l$(lib)) \
		$(ANALYSIS_COMMON_FLAGS) \
		$(ANALYSIS_LINKER_FLAGS) \
		$(4)
all: $(BUILD_DIR)/$(TARGET)
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET)))
endef

# Documented at ../executable_building.mk.
#
# Implementation notes:
# The resource file just needs to be copied into the out directory. The C++ code
# will read the files directly from the file system. Note that the macro
# arguments are stripped, so that COPY_TO_OUT_DIR_RULE can't break due to
# leading/trailing spaces. Also we drop the file name from the second argument,
# because COPY_TO_OUT_DIR_RULE receives the name of the target directory.
define ADD_RESOURCE_RULE
$(eval $(call COPY_TO_OUT_DIR_RULE,$(strip $(1)),$(dir $(strip $(2)))))
endef

# Rules for cleaning build files on "make clean".
.PHONY: clean_out_artifacts_$(TOOLCHAIN)
clean_out_artifacts_$(TOOLCHAIN):
	@rm -rf out-artifacts-$(TOOLCHAIN)
clean: clean_out_artifacts_$(TOOLCHAIN)
