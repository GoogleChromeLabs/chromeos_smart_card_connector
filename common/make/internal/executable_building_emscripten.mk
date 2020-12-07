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
# This file contains the implementation of the ../executable_building.mk
# interface that builds using Emscripten (which generates WebAssembly binaries).
#
# common.mk must be included before including this file.
#
# Troubleshooting notes:
# * Set the "EMCC_DEBUG" environment variable to 1 in order to enable more
#   verbose output from Emscripten's tools.
#


# Sanity-check that the environment is set up.
ifeq (,$(EMSDK))
$(error You must run "source env/activate" in the repository root directory at
		least once before running "make".)
endif

# Specifies the build configuration (default "Release", or "Debug").
CONFIG ?= Release

# A pattern rule for "dir.stamp" files in any directory. The rule allows to
# abstract away calling "mkdir" in output directories. It's intended to be used
# as an order-only prerequisite - for example:
#   some_target: some_prerequisite | some_dir/dir.stamp
%dir.stamp :
	@mkdir -p $(dir $@)
	@echo Stamp > $@

# Directory where temporary build artifacts (like .o object files) are stored.
BUILD_DIR := ./out-artifacts-emscripten/$(CONFIG)
# Directory where library .a files (built by LIB_RULE) are installed.
LIB_DIR := $(ROOT_PATH)/env/emsdk/lib/$(CONFIG)

# Macro that returns the .o object file path for the given source file.
# Note that ".." are replaced with a placeholder "__", so that the object files
# stay under BUILD_DIR.
# Arguments:
# $1 ("SOURCE") - Source file path.
define OBJ_FILE_NAME
$(BUILD_DIR)/$(basename $(subst ..,__,$(1))).o
endef

# Macro that returns the .d dependency file path for the given source file. The
# dependency file is generated by the compiler and placed next to the .o object
# file.
# Arguments:
# $1 ("SOURCE") - Source file path.
define DEP_FILE_NAME
$(BUILD_DIR)/$(basename $(subst ..,__,$(1))).d
endef

# Flags passed to the Emscripten compiler/linker tools.
#
# Explanation:
# bind: Enables "Embind" (the technology used by our C++ wrappers for talking
#   with JavaScript).
# pthread: Enables Pthreads support (for C/C++ multi-threading, etc.).
# MODULARIZE: Puts Emscripten module JavaScript loading code into a factory
#   function, in order to control its loading from other JS code and to avoid
#   name conflicts with unrelated code.
# EXPORT_NAME: Name of the JavaScript function that loads the Emscripten module.
# DYNAMIC_EXECUTION: Disable dynamic code execution in Emscripten JavaScript
#   code (not doing this will cause "unsafe-eval" Content Security Policy
#   violations when running this code inside Chrome Apps/Extensions).
EMSCRIPTEN_FLAGS := \
  --bind \
  -pthread \
  -s MODULARIZE=1 \
  -s 'EXPORT_NAME="loadEmscriptenModule_$(TARGET)"' \
  -s DYNAMIC_EXECUTION=0 \

ifeq ($(CONFIG),Release)

# Add flags specific to release builds.
#
# Explanation:
# O3: Enable advanced optimizations.
# NDEBUG: Disable debug-only functionality (assertions, etc.).
EMSCRIPTEN_FLAGS += \
  -O3 \
  -DNDEBUG \

else ifeq ($(CONFIG),Debug)

# Add flags specific to debug builds.
#
# Explanation:
# O0: Disable optimizations.
# g4: Preserve maximum debug information, including source maps.
# ASSERTIONS: Enable runtime checks, like for memory allocation errors.
# DEMANGLE_SUPPORT: Demangle C++ function names in stack traces.
# DISABLE_EXCEPTION_CATCHING: Enable support for C++ exceptions.
# EXCEPTION_DEBUG: Enables printing exceptions coming from the executable.
# SAFE_HEAP: Enable memory access checks.
EMSCRIPTEN_FLAGS += \
  -O0 \
  -g4 \
  -s ASSERTIONS=2 \
  -s DEMANGLE_SUPPORT=1 \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  -s EXCEPTION_DEBUG=1 \
  -s SAFE_HEAP=1 \

else

$(error Unsupported CONFIG=$(CONFIG) value.)

endif

# Documented at ../executable_building.mk.
CXX_DIALECT := c++11

# Documented at ../executable_building.mk.
#
# Implementation notes:
# * The target for the .o file depends on the source file, so that modification
#   of the source file triggers recompilation.
# * The target also has an order-only dependency on the dir.stamp file, in order
#   to guarantee that the parent directory is created by the compilation time.
# * The compilation is performed via the Emscripten Compiler Frontend (emcc).
# * The target is added as a dependency to the "all" target, so that it's built
#   by default when no target is specified explicitly in the command line.
# * The dependency file is additionally included if it exists (generated during
#   the previous compilation due to the "-MMD" flag described below), allowing
#   to automatically trigger recompilation when some used header has changed.
#
# Explanation of parameters to emcc:
# o: Path to store the output .o file.
# c: Generate bitcode, rather than all artifacts including JavaScript.
# MMD, MP: Additionally generate the dependency file, containing information
#   about all files used by this file, i.e., all headers transitively included
#   by it. The dependency file is generated in the makefile syntax, allowing to
#   simply include this file into our makefile. System headers are not included
#   into the dependency file.
define COMPILE_RULE
$(call OBJ_FILE_NAME,$(1)): $(1) | $(dir $(call OBJ_FILE_NAME,$(1)))dir.stamp
	emcc \
		-o $(call OBJ_FILE_NAME,$(1)) \
		-c \
		-MMD \
		-MP \
		$(1) \
		$(EMSCRIPTEN_FLAGS) \
		$(2)
-include $(call DEP_FILE_NAME,$(1))
all: $(call OBJ_FILE_NAME,$(1))
endef

# Documented at ../executable_building.mk.
#
# Implementation notes:
# * The target for the .a file depends on the input .o object files, so that
#   recompilation of those triggers re-linking.
# * The recipe starts from ensuring the LIB_DIR exists.
# * The recipe also deletes the previously existing .a file, if any, in order to
#   prevent accidental reuse of old archive contents after the rebuild.
# * The linking is performed via the Emscripten "emar" (drop-in replacement for
#   the standard "ar" program).
# * The target is added as a dependency to the "all" target, so that it's built
#   by default when no target is specified explicitly in the command line.
#
# Explanation of parameters to emar:
# c: Requests to create the resulting archive.
# r: Delete previously existing members with the same names.
# s: Write an object-index into the archive, in order to speed subsequent
#    link operations.
define LIB_RULE
$(LIB_DIR)/lib$(1).a: $(foreach src,$(2),$(call OBJ_FILE_NAME,$(src)))
	@mkdir -p $(LIB_DIR)
	@rm -f $(LIB_DIR)/lib$(1).a
	emar \
		crs \
		$(LIB_DIR)/lib$(1).a \
		$(foreach src,$(2),$(call OBJ_FILE_NAME,$(src))) \
		$(3)
all: $(LIB_DIR)/lib$(1).a

# Rules for cleaning the library files when requested:
.PHONY: clean_lib$(1)
clean_lib$(1):
	@rm -f $(LIB_DIR)/lib$(1).a
clean: clean_lib$(1)
endef

# Documented at ../executable_building.mk.
#
# Implementation notes:
# * The target consists of three resulting files: .wasm, .js, .worker.js. The
#   first one is the actual binary, the second one is a JavaScript code that
#   loads it, and the third one is used for the Pthreads multi-threading
#   support. The "&" character denotes that all three files are produced by a
#   single invocation of the recipe.
# * The target depends on the input .o object files and .a static library files,
#   so that recompilation of those triggers re-linking.
# * The target also has an order-only dependency on the dir.stamp file, in order
#   to guarantee that the parent directory is created by the compilation time.
# * The linking is performed via Emscripten Compiler Frontend (emcc).
# * The targets are added as a dependency to the "all" target, so that they're
#   built by default when no target is specified explicitly in the command line.
# * The COPY_TO_OUT_DIR_RULE macro is used to copy the resulting files into the
#   "out" directory where they can be loaded by the resulting Chrome
#   App/Extension.
#
# Explanation of parameters to emcc:
# o: Path to store the output .js file (other file paths are deduced by emcc
#   from it).
# L: Add the specified directory into the .a library search paths.
# l: Link the specified .a library.
define LINK_EXECUTABLE_RULE
# Dependency on the input object files:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js: $(foreach src,$(1),$(call OBJ_FILE_NAME,$(src)))
# Dependency on the input static libraries:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js: $(foreach lib,$(2),$(LIB_DIR)/lib$(lib).a)
# Dependency on additionally specified targets:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js: $(3)
# Order-only dependency on the destination directory stamp:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js: | $(BUILD_DIR)/dir.stamp
# The recipe that performs the linking and creates the resulting files:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js &:
	emcc \
		-o $(BUILD_DIR)/$(TARGET).js \
		-L$(LIB_DIR) \
		$(foreach src,$(1),$(call OBJ_FILE_NAME,$(src))) \
		$(foreach lib,$(2),-l$(lib)) \
		$(EMSCRIPTEN_FLAGS) \
		$(4)
# Add linking into the default "all" target's prerequisite:
all: $(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js
# Copy the resulting files into the out directory:
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).js))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).wasm))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).worker.js))
endef

# Rules for cleaning build files when requested.
.PHONY: clean_out_artifacts_emscripten
clean_out_artifacts_emscripten:
	@rm -rf out-artifacts-emscripten
clean: clean_out_artifacts_emscripten
