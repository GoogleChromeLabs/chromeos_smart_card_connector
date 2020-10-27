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
# Currently, the only supported value is "pnacl", which corresponds to the
# Google Native Client technology.
#
# common.mk must be included before including this file.

# Default to using the "pnacl" toolchain.
TOOLCHAIN ?= pnacl

# Common documentation for definitions provided by this file (they are
# implemented in toolchain-specific .mk files, but share the same interface):
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
# * NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH variable:
#   TODO(#177): Rename to a toolchain-independent name.
#   Path where library headers are installed by the
#   NACL_LIBRARY_HEADERS_INSTALLATION_RULE macro.
#
# * NACL_LIBRARY_HEADERS_INSTALLATION_RULE macro:
#   TODO(#177): Rename to a toolchain-independent name.
#   Installs specified headers into $NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH.
#   Arguments:
#   $1 ("INSTALLING_HEADERS"): List of header descriptions, where each list item
#     should contain colon-separated destination directory (relative to
#     NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH), source directory and source
#     file path (relative to the source directory). Example:
#     "foolib:../src:a.h foolib:../src:b/c.h", which denotes installation of
#     file ../src/a.h into
#     $NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH/libfoo/a.h and of file
#     ../src/b/c.h into
#     $NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH/libfoo/b/c.h.
#
# * DEPEND_COMPILE_ON_NACL_LIBRARY_HEADERS macro:
#   TODO(#177): Rename to a toolchain-independent name.
#   Adds the specified library as a prerequisite for compilation rule of the
#   the specified file.
#   Arguments:
#   $1 ("SOURCES"): Source file paths,
#   $2 ("STAMP_FILE"): The library's header installation stamp file, defined via
#     the DEFINE_NACL_LIBRARY_HEADERS_INSTALLATION_TARGET rule.
#
# * DEFINE_NACL_LIBRARY_HEADERS_INSTALLATION_TARGET macro:
#   TODO(#177): Rename to a toolchain-independent name.
#   Defines the target for the library headers installation and sets the
#   variable containing the target name.
#   Arguments:
#   $1 ("STAMP_FILE_VAR"): Variable to be set to the stamp file name.
#   $2 ("LIB_DIR"): Path to the library's build directory.

# Load the toolchain-specific file.
ifeq ($(TOOLCHAIN),emscripten)
include $(COMMON_DIR_PATH)/make/internal/executable_building_emscripten.mk
else ifeq ($(TOOLCHAIN),pnacl)
include $(COMMON_DIR_PATH)/make/internal/executable_building_nacl.mk
else
$(error Unknown TOOLCHAIN "$(TOOLCHAIN)".)
endif
