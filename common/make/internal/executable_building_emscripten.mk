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


# Check that the environment is set up.
ifeq (,$(EMSDK))
$(error You must run "source env/activate" in the repository root directory at \
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

# Flags passed to the Emscripten compiler and linker tools.
#
# Explanation:
# pthread: Enables Pthreads support (for C/C++ multi-threading, etc.).
# DISABLE_EXCEPTION_CATCHING: Enable support for C++ exceptions.
EMSCRIPTEN_COMMON_FLAGS := \
  -pthread \
  -s DISABLE_EXCEPTION_CATCHING=0 \

# Flags passed to the Emscripten compiler tools, in addition to
# `EMSCRIPTEN_COMMON_FLAGS`.
EMSCRIPTEN_COMPILER_FLAGS := \

# Flags passed to the Emscripten linker tools, in addition to
# `EMSCRIPTEN_COMMON_FLAGS`.
#
# Explanation:
# bind: Enables "Embind" (the technology used by our C++ wrappers for talking
#   with JavaScript).
# ABORTING_MALLOC: Immediately abort on memory allocation failures (instead of
#   letting malloc and similar return null, which is the default behavior with
#   ALLOW_MEMORY_GROWTH=1).
# ALLOW_MEMORY_GROWTH: Increase the amount of available memory dynamically at
#   runtime (as opposed to be capped by the default limit of 16 MiB).
# DYNAMIC_EXECUTION: Disable dynamic code execution in Emscripten JavaScript
#   code (not doing this will cause "unsafe-eval" Content Security Policy
#   violations when running this code inside Chrome Apps/Extensions).
# ENVIRONMENT: Only generate support code for running in needed Web environments
#   and skip support for others.
# EXPORT_NAME: Name of the JavaScript function that loads the Emscripten module.
# INCOMING_MODULE_JS_API: Names of Emscripten Module attributes that we need at
#   runtime (code for other attributes is optimized away).
# MIN_CHROME_VERSION: Skip generating Emscripten support code for very old
#   Chrome versions. (The exact boundary is chosen similarly to
#   minimum_chrome_version in
#   smart_card_connector_app/src/manifest.json.template.)
# MIN_*_VERSION: Disable support code for non-Chrome browsers as we're never
#   expected to run anywhere besides Chrome.
# MODULARIZE: Puts Emscripten module JavaScript loading code into a factory
#   function, in order to control its loading from other JS code and to avoid
#   name conflicts with unrelated code.
# PTHREAD_POOL_SIZE_STRICT: Suppress runtime warnings when a new worker has to
#   start for a new thread (this warning is confusing and is mostly useful in
#   early days of development).
# no-pthreads-mem-growth: Suppress the linker warning about the performance of
#   the "Pthreads + ALLOW_MEMORY_GROWTH" combination.
EMSCRIPTEN_LINKER_FLAGS := \
  --bind \
  --post-js=$(ROOT_PATH)/common/js/src/executable-module/emscripten-module-js-epilog.js.inc \
  -s ABORTING_MALLOC=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s DYNAMIC_EXECUTION=0 \
  -s ENVIRONMENT=web,worker \
  -s 'EXPORT_NAME="loadEmscriptenModule_$(TARGET)"' \
  -s INCOMING_MODULE_JS_API=[buffer,instantiateWasm,onAbort,preRun,print,printErr,wasmMemory] \
  -s MIN_CHROME_VERSION=96 \
  -s MIN_EDGE_VERSION=-1 \
  -s MIN_FIREFOX_VERSION=-1 \
  -s MIN_IE_VERSION=-1 \
  -s MIN_SAFARI_VERSION=-1 \
  -s MODULARIZE=1 \
  -s PTHREAD_POOL_SIZE_STRICT=0 \
  -Wno-pthreads-mem-growth \

ifeq ($(CONFIG),Release)

# Add compiler and linker flags specific to release builds.
#
# Explanation:
# O3: Enable advanced optimizations.
# flto: Use link-time optimizations. Note: this typically produces larger
#   executables, however they should presumably be faster.
EMSCRIPTEN_COMMON_FLAGS += \
  -O3 \
  -flto \

# Add compiler flags specific to release builds.
#
# Explanation:
# NDEBUG: Disable debug-only functionality (assertions, etc.).
EMSCRIPTEN_COMPILER_FLAGS += \
  -DNDEBUG \

# Add linker flags specific to release builds.
#
# Explanation:
# closure=1: Enable Closure Compiler based optimizations of the JavaScript code
#   emitted by Emscripten.
EMSCRIPTEN_LINKER_FLAGS += \
	--closure=1 \

else ifeq ($(CONFIG),Debug)

# Add compiler and linker flags specific to debug builds.
#
# Explanation:
# O0: Disable optimizations.
# g: Preserve debug information, including DWARF data.
EMSCRIPTEN_COMMON_FLAGS += \
  -O0 \
  -g \

# Add linker flags specific to debug builds.
#
# Explanation:
# ASSERTIONS: Enable runtime checks, like for memory allocation errors.
# DEMANGLE_SUPPORT: Demangle C++ function names in stack traces.
# SAFE_HEAP: Enable memory access checks.
# TOTAL_STACK: Increase the initial stack size (Emscripten's default 64KB are
#   very tight for Debug builds, and while some code in this project calls
#   pthread_attr_setstacksize() its parameters aren't chosen with Emscripten
#   Debug's heavy stack consumption in mind).
# Wno-limited-postlink-optimizations: Suppress a warning about limited
#   optimizations.
EMSCRIPTEN_LINKER_FLAGS += \
  -s ASSERTIONS=2 \
  -s DEMANGLE_SUPPORT=1 \
  -s SAFE_HEAP=1 \
  -s TOTAL_STACK=1048576 \
  -Wno-limited-postlink-optimizations \

else

$(error Unsupported CONFIG=$(CONFIG) value.)

endif

# Documented at ../executable_building.mk.
CXX_DIALECT := c++11

# The list of linker flags and dependencies for adding resource files (initially
# empty, and added by ADD_RESOURCE_RULE).
EMSCRIPTEN_RESOURCE_FILE_LDFLAGS :=
EMSCRIPTEN_RESOURCE_FILE_DEPS :=

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
		$(EMSCRIPTEN_COMMON_FLAGS) \
		$(EMSCRIPTEN_COMPILER_FLAGS) \
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
# Make sure LIB_DIR is created (using an order-only dependency).
$(LIB_DIR)/lib$(1).a: | $(LIB_DIR)/dir.stamp

# Library build rule
$(LIB_DIR)/lib$(1).a: $(foreach src,$(2),$(call OBJ_FILE_NAME,$(src)))
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
#   support.
# * The "%" character is used as a trick to tell Make run the recipe only once
#   in order to produce all three files (technically, "%" means it's a pattern
#   rule, and in this case it's matching to a single dot "."). Note: in
#   Make >=4.3 there's a more intuitive alternative (the "&:" rules), but at the
#   moment Make 4.3 isn't widespread enough.
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
# Dependency on input resource files:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js: $(EMSCRIPTEN_RESOURCE_FILE_DEPS)
# Order-only dependency on the destination directory stamp:
$(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js: | $(BUILD_DIR)/dir.stamp
# The recipe that performs the linking and creates the resulting files:
$(BUILD_DIR)/$(TARGET)%js $(BUILD_DIR)/$(TARGET)%wasm $(BUILD_DIR)/$(TARGET).worker%js:
	emcc \
		-o $(BUILD_DIR)/$(TARGET).js \
		-L$(LIB_DIR) \
		$(foreach src,$(1),$(call OBJ_FILE_NAME,$(src))) \
		$(foreach lib,$(2),-l$(lib)) \
		$(EMSCRIPTEN_COMMON_FLAGS) \
		$(EMSCRIPTEN_LINKER_FLAGS) \
		$(EMSCRIPTEN_RESOURCE_FILE_LDFLAGS) \
		$(4)
# Add linking into the default "all" target's prerequisite:
all: $(BUILD_DIR)/$(TARGET).js $(BUILD_DIR)/$(TARGET).wasm $(BUILD_DIR)/$(TARGET).worker.js
# Copy the resulting files into the out directory:
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).js))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).wasm))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(BUILD_DIR)/$(TARGET).worker.js))
endef

# Documented at ../executable_building.mk.
#
# Implementation notes:
# * On Emscripten, the files that need to be exposed to the executable module
#   have to be packaged ("preloaded") into a .data file. This happens during the
#   linking stage, hence this macro only modifies the variables that are used by
#   LINK_EXECUTABLE_RULE.
# * The "@" character in Emscripten's CLI syntax is a separator between input
#   and output paths; we also have to strip the arguments to avoid accidental
#   spaces around it.
# * We also add a dependency on the "copy_emscripten_data_to_out" helper target
#   that copies the .data file created during linking into the "out" folder.
define ADD_RESOURCE_RULE
EMSCRIPTEN_RESOURCE_FILE_LDFLAGS += --preload-file $(strip $(1))@$(strip $(2))
EMSCRIPTEN_RESOURCE_FILE_DEPS += $(1)
generate_out: copy_emscripten_data_to_out
endef

# Rules for copying the .data file created during linking into the out folder.
#
# We don't do it inside LINK_EXECUTABLE_RULE, because the .data file is only
# created when there's at least one resource file, and with Make syntax it's
# terribly difficult to encode such conditional logic inside a macro.
.PHONY: copy_emscripten_data_to_out
copy_emscripten_data_to_out: $(OUT_DIR_PATH) $(BUILD_DIR)/$(TARGET).wasm
	@cp -pr $(BUILD_DIR)/$(TARGET).data $(OUT_DIR_PATH)/

# Rules for cleaning build files when requested.
.PHONY: clean_out_artifacts_emscripten
clean_out_artifacts_emscripten:
	@rm -rf out-artifacts-emscripten
clean: clean_out_artifacts_emscripten
