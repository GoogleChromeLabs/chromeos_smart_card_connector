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
# Emscripten builds for creating test executables.
#
# Note: This Makefile isn't supported in Native Client builds, since NaCl's
# webports project already provides a NaCl-ported version of GoogleTest.

TARGET := googletest

include ../../../../common/make/common.mk
include $(ROOT_PATH)/common/make/executable_building.mk

ifneq ($(TOOLCHAIN),emscripten)
$(error This Makefile is only supported in Emscripten builds.)
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

# Make the target library files be built by default, even when no target is
# explicitly specified when running "make".
all: $(TARGET_LIBS)

# Temporary library files. Note the "d" suffix in file names in Debug builds -
# that's due to configuration inside the Googletest's CMake scripts.
ifeq ($(CONFIG),Debug)

ARTIFACTS_LIBS := \
	out-artifacts/lib/libgmockd.a \
	out-artifacts/lib/libgmock_maind.a \
	out-artifacts/lib/libgtestd.a \
	out-artifacts/lib/libgtest_maind.a \

else

ARTIFACTS_LIBS := \
	out-artifacts/lib/libgmock.a \
	out-artifacts/lib/libgmock_main.a \
	out-artifacts/lib/libgtest.a \
	out-artifacts/lib/libgtest_main.a \

endif

# Rule for building temporary library files, including compiling GoogleTest.
#
# Notes:
# * The "&" character after the list of targets denotes that all of them are
#   built together by a single invocation of the recipe.
# * The build is performed in a temporary "out-artifacts" directory, separately
#   from source files and this Makefle.
# * Emscripten's wrapper "emcmake" is used for running "cmake", which allows to
#   reuse the GoogleTest's standard CMake scripts as-is, but with some runtime
#   fixes in order to use Emscripten toolchain instead of the system compiler.
# * After "cmake" completes, a regular "make" is used in order to actually run
#   the compilation according to prepared scripts.
#
# Explanation of parameters to cmake:
# E env: Wraps the succeeding cmake call into the "env" tool that allows to
#   specify environment variables.
# CXXFLAGS: Enables Emscripten Pthreads support (despite that it's not used by
#   GoogleTest itself due to the other parameter below, Emscripten requires this
#   flag to be set if a library is linked later with other files that have it).
# LDFLAGS: Enables Emscripten Pthreads support.
# B: Build directory.
# CMAKE_BUILD_TYPE: Specify debug/release build.
# gtest_disable_pthreads: Disables pthreads-based functionality in GoogleTest
#   that doesn't behave well under Emscripten (it deadlocks on startup).
$(ARTIFACTS_LIBS) &:
	rm -rf out-artifacts
	mkdir out-artifacts
	emcmake \
		cmake \
		-E env \
		CXXFLAGS="-pthread" \
		LDFLAGS="-pthread" \
		cmake \
		../../src \
		-B out-artifacts \
		-DCMAKE_BUILD_TYPE=$(CONFIG) \
		-Dgtest_disable_pthreads=on
	+$(MAKE) -C out-artifacts

# Rule for creating target library files, as copies of the temporary libraries.
#
# Notes:
# * Debug and Release configurations are handled separately, since GoogleTest
#   adds the "d" suffix to the temporary library file names in Debug builds.
ifeq ($(CONFIG),Debug)
$(TARGET_LIBS) &: $(ARTIFACTS_LIBS)
	cp out-artifacts/lib/libgmockd.a $(LIB_DIR)/libgmock.a
	cp out-artifacts/lib/libgmock_maind.a $(LIB_DIR)/libgmock_main.a
	cp out-artifacts/lib/libgtestd.a $(LIB_DIR)/libgtest.a
	cp out-artifacts/lib/libgtest_maind.a $(LIB_DIR)/libgtest_main.a
else
$(TARGET_LIBS) &: $(ARTIFACTS_LIBS)
	cp out-artifacts/lib/libgmock.a $(LIB_DIR)/libgmock.a
	cp out-artifacts/lib/libgmock_main.a $(LIB_DIR)/libgmock_main.a
	cp out-artifacts/lib/libgtest.a $(LIB_DIR)/libgtest.a
	cp out-artifacts/lib/libgtest_main.a $(LIB_DIR)/libgtest_main.a
endif

# Add the temporary build directory for deletion when running "make clean".
$(eval $(call CLEAN_RULE,out-artifacts))
