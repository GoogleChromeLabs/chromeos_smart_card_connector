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

# This file contains helper definitions for building an executable binary that
# can be run in web-based applications.
#
# The actually used technology depends on the TOOLCHAIN environment variable.
#
# common.mk must be included before including this file.

# Common documentation for definitions provided by this file (they are
# implemented in toolchain-specific .mk files, but share the same interface):
#
# * CXX_DIALECT variable:
#   The value to be used for the "-std=" flag for C++ files.
#
# * COMPILE_RULE macro:
#   Generates object files from the given C/C++ files.
#   Arguments:
#   $1 ("SOURCES"): Source file paths,
#   $2 ("CFLAGS", optional): C/C++ compiler flags.
#
# * LIB_RULE macro:
#   Links object files that correspond to the given source files into a static
#   library file. The resulting file is put into a shared library directory.
#   Arguments:
#   $1 ("TARGET"): Library name (without the "lib" prefix or the extension),
#   $2 ("SOURCES"): Source file paths.
#
# * LINK_EXECUTABLE_RULE macro:
#   Links object files that correspond to the given source files into an
#   executable program. The resulting files are put into the out directory.
#   Arguments:
#   $1 ("SOURCES"): Source file paths,
#   $2 ("LIBS", optional): Static libraries to link against,
#   $3 ("DEPS", optional): Libraries whose building will be added as
#     dependencies of this target.
#   $4 ("LDFLAGS", optional): Linker flags.
#
# * ADD_RESOURCE_RULE macro:
#   Puts the specified resource file into the location from which it can be
#   loaded by the executable module at runtime.
#   Arguments:
#   $1 ("RESOURCE_FILE"): Path to the input resource file,
#   $2 ("TARGET_PATH"): Path under which the resource file should be exposed at
#     runtime.

# Load the toolchain-specific file.
ifeq ($(TOOLCHAIN),emscripten)
include $(COMMON_DIR_PATH)/make/internal/executable_building_emscripten.mk
else ifeq ($(TOOLCHAIN),pnacl)
include $(COMMON_DIR_PATH)/make/internal/executable_building_nacl.mk
else ifeq ($(TOOLCHAIN),asan_testing)
include $(COMMON_DIR_PATH)/make/internal/executable_building_analysis.mk
else ifeq ($(TOOLCHAIN),coverage)
include $(COMMON_DIR_PATH)/make/internal/executable_building_analysis.mk
else
$(error Unknown TOOLCHAIN "$(TOOLCHAIN)".)
endif
