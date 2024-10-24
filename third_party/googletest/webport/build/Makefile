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

# This Makefile builds GoogleTest and GoogleMock static libraries for usage in
# Emscripten/"analysis" builds for creating test executables.

TARGET := googletest

include ../../../../common/make/common.mk
include $(ROOT_PATH)/common/make/executable_building.mk

# Determine the type of toolchain.
ifeq ($(TOOLCHAIN),emscripten)
TOOLCHAIN_TYPE := emscripten
else ifeq ($(TOOLCHAIN),asan_testing)
TOOLCHAIN_TYPE := analysis
else ifeq ($(TOOLCHAIN),coverage)
TOOLCHAIN_TYPE := analysis
else
$(error Unsupported toolchain $(TOOLCHAIN).)
endif

# Library files that are created by this Makefile.
#
# Comments for the libraries:
# * libtest - GoogleTest's implementation.
# * libgmock - GoogleMock's implementation.
# * libgtest_main/libgmock_main - standard test runner (i.e., implementation of
#   the main() function that runs all tests).
TARGET_LIBS := \
	$(LIB_DIR)/libgmock.a \
	$(LIB_DIR)/libgmock_main.a \
	$(LIB_DIR)/libgtest.a \
	$(LIB_DIR)/libgtest_main.a \

# A make pattern for TARGET_LIBS. In this pattern, "%" denotes an arbitrary
# sequence of characters, which evaluates to ".a" in this case.
#
# We use this pattern below for writing a rule that builds all these files at
# once, instead of the regular make behavior that'll call the build script 4
# times (once for each file). Note: in Make >=4.3 there's a more intuitive
# alternative (the "&:" rules), but at the moment Make 4.3 isn't widespread
# enough.
TARGET_LIBS_PATTERN := \
	$(LIB_DIR)/libgmock% \
	$(LIB_DIR)/libgmock_main% \
	$(LIB_DIR)/libgtest% \
	$(LIB_DIR)/libgtest_main% \

# Make the target library files be built by default, even when no target is
# explicitly specified when running "make".
all: $(TARGET_LIBS)

ARTIFACTS_DIR := out-artifacts-$(TOOLCHAIN)-$(CONFIG)

# Temporary library files. Note the "d" suffix in file names in Debug builds -
# that's due to configuration inside the Googletest's CMake scripts.
ifeq ($(CONFIG),Debug)

ARTIFACTS_LIBS := \
	$(ARTIFACTS_DIR)/lib/libgmockd.a \
	$(ARTIFACTS_DIR)/lib/libgmock_maind.a \
	$(ARTIFACTS_DIR)/lib/libgtestd.a \
	$(ARTIFACTS_DIR)/lib/libgtest_maind.a \

else

ARTIFACTS_LIBS := \
	$(ARTIFACTS_DIR)/lib/libgmock.a \
	$(ARTIFACTS_DIR)/lib/libgmock_main.a \
	$(ARTIFACTS_DIR)/lib/libgtest.a \
	$(ARTIFACTS_DIR)/lib/libgtest_main.a \

endif

# A make pattern for the temporary library files. In this pattern, "%" denotes
# an arbitrary sequence of characters, which evaluates to ".a" in Release builds
# and "d.a" in Debug ones.
#
# The reason we use pattern rules is that it avoids executing a rule multiple
# times - see the doc on TARGET_LIBS_PATTERN above.
ARTIFACTS_LIBS_PATTERN := \
	$(ARTIFACTS_DIR)/lib/libgmock% \
	$(ARTIFACTS_DIR)/lib/libgmock_main% \
	$(ARTIFACTS_DIR)/lib/libgtest% \
	$(ARTIFACTS_DIR)/lib/libgtest_main% \

# The command to invoke CMake.
#
# In the Emscripten mode, use the "emcmake" wrapper, which allows to reuse the
# GoogleTest's standard CMake scripts as-is.
ifeq ($(TOOLCHAIN_TYPE),emscripten)
CMAKE_TOOL := emcmake cmake
else ifeq ($(TOOLCHAIN_TYPE),analysis)
CMAKE_TOOL := cmake
endif

# Environment variables to set when running CMake.
#
# In the Emscripten mode, we have to pass "-pthread" explicitly, both as
# compiler and linker flags.
ifeq ($(TOOLCHAIN_TYPE),emscripten)
CMAKE_ENV := CXXFLAGS="-pthread" LDFLAGS="-pthread"
else ifeq ($(TOOLCHAIN_TYPE),analysis)
CMAKE_ENV :=
endif

# Arguments passed to CMake.
#
# * CMAKE_BUILD_TYPE: Specify debug/release build.
# * GTEST_HAS_PTHREAD: Make GoogleTest/GoogleMock thread-safe via pthreads. Note
#   that the same definition must be passed when compiling test files - see
#   //common/cpp_unit_test_runner/src/build_emscripten.mk.
CMAKE_ARGS := \
	-DCMAKE_BUILD_TYPE=$(CONFIG) \
	-DGTEST_HAS_PTHREAD=1 \

ifeq ($(TOOLCHAIN_TYPE),analysis)

# Arguments specific to Coverage builds:
# * "clang++": Use clang (and not try to use gcc).
# * "m32": build in 32-bit mode (which is also what Emscripten toolchain uses by
#   default).
CMAKE_ARGS += \
	-DCMAKE_CXX_COMPILER="clang++" \
	-DCMAKE_CXX_FLAGS="-m32" \

endif

# Rule for building temporary library files, including compiling GoogleTest.
#
# Notes:
# * We use a pattern rule (note the "%" character) in order to tell Make trigger
#   this rule only once to produce all 4 files.
# * The build is performed in a temporary "out-artifacts" directory, separately
#   from source files and this Makefle.
# * After "cmake" completes, a regular "make" is used in order to actually run
#   the compilation according to prepared scripts.
#
# Explanation of parameters to cmake:
# E env: Wraps the succeeding cmake call into the "env" tool that allows to
#   specify environment variables.
# B: Build directory.
$(ARTIFACTS_LIBS_PATTERN): | $(ARTIFACTS_DIR)/dir.stamp
	$(CMAKE_ENV) $(CMAKE_TOOL) \
		../../src \
		-B $(ARTIFACTS_DIR) \
		$(CMAKE_ARGS)
	+$(MAKE) -C $(ARTIFACTS_DIR)

# Rule for creating target library files, as copies of the temporary libraries.
#
# Notes:
# * We use a pattern rule (note the "%" character) in order to tell Make trigger
#   this rule only once to produce all 4 files.
# * Debug and Release configurations are handled separately, since GoogleTest
#   adds the "d" suffix to the temporary library file names in Debug builds.
ifeq ($(CONFIG),Debug)
$(TARGET_LIBS_PATTERN): $(ARTIFACTS_LIBS)
	cp $(ARTIFACTS_DIR)/lib/libgmockd.a $(LIB_DIR)/libgmock.a
	cp $(ARTIFACTS_DIR)/lib/libgmock_maind.a $(LIB_DIR)/libgmock_main.a
	cp $(ARTIFACTS_DIR)/lib/libgtestd.a $(LIB_DIR)/libgtest.a
	cp $(ARTIFACTS_DIR)/lib/libgtest_maind.a $(LIB_DIR)/libgtest_main.a
else
$(TARGET_LIBS_PATTERN): $(ARTIFACTS_LIBS)
	cp $(ARTIFACTS_DIR)/lib/libgmock.a $(LIB_DIR)/libgmock.a
	cp $(ARTIFACTS_DIR)/lib/libgmock_main.a $(LIB_DIR)/libgmock_main.a
	cp $(ARTIFACTS_DIR)/lib/libgtest.a $(LIB_DIR)/libgtest.a
	cp $(ARTIFACTS_DIR)/lib/libgtest_main.a $(LIB_DIR)/libgtest_main.a
endif

# Make sure LIB_DIR is created before we copy .a files into it (using an
# order-only dependency).
$(TARGET_LIBS): | $(LIB_DIR)/dir.stamp

# Add the temporary build directory for deletion when running "make clean".
$(eval $(call CLEAN_RULE,$(ARTIFACTS_DIR)))
