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

ARTIFACTS_DIR := out-artifacts-$(TOOLCHAIN)-$(CONFIG)

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

all: build.stamp

# Rule that builds GoogleTest using CMake.
#
# Notes:
# * The build is performed in a temporary "out-artifacts" directory, separately
#   from source files and this Makefle.
# * After "cmake" completes, a regular "make" is used in order to actually run
#   the compilation according to prepared scripts.
# * The create *.a static libraries are copied into the resulting libraries dir.
# * If all previous steps succeeded, we generate the "build.stamp" file to
#   designate the successful completion.
#
# Explanation of parameters to cmake:
# E env: Wraps the succeeding cmake call into the "env" tool that allows to
#   specify environment variables.
# B: Build directory.
build.stamp: | $(ARTIFACTS_DIR)/dir.stamp $(LIB_DIR)/dir.stamp
	rm -f build.stamp
	$(CMAKE_ENV) $(CMAKE_TOOL) \
		../../src \
		-B $(ARTIFACTS_DIR) \
		$(CMAKE_ARGS)
	+$(MAKE) -C $(ARTIFACTS_DIR)
	cp $(ARTIFACTS_DIR)/lib/*.a $(LIB_DIR)/
	touch build.stamp

# Add the temporary build directory for deletion when running "make clean".
$(eval $(call CLEAN_RULE,$(ARTIFACTS_DIR)))
