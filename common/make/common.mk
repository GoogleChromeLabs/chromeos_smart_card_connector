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


#
# This file contains some helper definitions that simplify writing of target
# makefiles.
#
# Note that the TARGET variable must be declared before including this makefile:
# it should contain the name of the target to be built.
#

ifeq (,$(TARGET))
$(error TARGET variable must be set before including this file)
endif


#
# Determine build configuration.
#

# Supported values: "Release" (default), "Debug".
CONFIG ?= Release
# Supported values: "pnacl" (Portable Native Client; default), "emscripten"
# (Emscripten/WebAssembly).
TOOLCHAIN ?= pnacl


#
# Returns normalized path.
#
# Arguments:
#    $1: Path to be normalized.
#

NORMALIZE_PATH = \
	$(shell python -c 'import os.path, sys; print os.path.normpath(sys.argv[1])' "$(1)")


#
# Returns a path normalized and relative to the specified another path.
#
# Arguments:
#    $1: Path to be normalized and returned relatively to another path.
#    $2: The base path for creating the relative path.
#

RELATIVE_PATH_COMMAND = \
	python \
		-c 'import os.path, sys; print os.path.relpath(sys.argv[1],sys.argv[2])' \
		"$(1)" \
		"$(2)"

RELATIVE_PATH = $(shell $(call RELATIVE_PATH_COMMAND,$(1),$(2)))


#
# Path to the source files root directory (which is one of ancestors of this
# file).
#

ROOT_PATH := $(call NORMALIZE_PATH,$(dir $(lastword $(MAKEFILE_LIST)))/../..)


#
# Path to the common/ directory.
#

COMMON_DIR_PATH := $(ROOT_PATH)/common


#
# Path to the third_party/ directory.
#

THIRD_PARTY_DIR_PATH := $(ROOT_PATH)/third_party


#
# The default target.
#
# If no targets are specified on the command line, the first target listed in
# the makefile becomes the default target. That's why the "all" target is
# defined here (with an empty recipe, which can be substituted later), so that
# there is no chance that the default target is something else.
#

all:

.PHONY: all


#
# Macro rule for adding the deletion of the specified file/directory within the
# "make clean" target.
#
# Arguments:
#    $1: Path to the file/directory.
#

define CLEAN_RULE

.PHONY: clean_$(1)

clean_$(1):
	@rm -rf "$(1)"

clean: clean_$(1)

endef


#
# OUT_DIR_PATH variable and some helper rules definitions for the directory
# where the resulting build files should be placed.
#
# This directory is re-created each build.
#
# In order to populate this directory with custom files, the
# COPY_TO_OUT_DIR_RULE macro rule can be used.
#
# Alternatively, one can add custom prerequisites to the "generate_out" target.
# But, in that case, each of these new prerequisites must ensure that it depends
# on the $(OUT_DIR_PATH) target before making any changes in the out directory.
#
# Arguments to the COPY_TO_OUT_DIR_RULE macro:
#    $1: Path to the source file.
#    $2: Optional relative path in the out directory.
#

OUT_DIR_ROOT_PATH := out

OUT_DIR_PATH := $(OUT_DIR_ROOT_PATH)/$(TARGET)

.PHONY: $(OUT_DIR_PATH) generate_out

$(eval $(call CLEAN_RULE,$(OUT_DIR_ROOT_PATH)))

$(OUT_DIR_PATH): clean_$(OUT_DIR_ROOT_PATH)
	@mkdir -p $(OUT_DIR_PATH)

generate_out: $(OUT_DIR_PATH)

all: generate_out

define COPY_TO_OUT_DIR_RULE

.PHONY: $(call NORMALIZE_PATH,$(OUT_DIR_PATH)/$(2)/$(notdir $(1)))

$(call NORMALIZE_PATH,$(OUT_DIR_PATH)/$(2)/$(notdir $(1))): $(OUT_DIR_PATH) $(1)
	@mkdir -p $(OUT_DIR_PATH)/$(2)
	@cp -pr $(1) $(OUT_DIR_PATH)/$(2)

generate_out: $(call NORMALIZE_PATH,$(OUT_DIR_PATH)/$(2)/$(notdir $(1)))

endef


#
# Race-free version of "cp" utility.
#
# Arguments:
#    $1: Path to the input file.
#    $2: Path to the resulting file.
#    $3: (optional) If non-empty, then creates the parent directories too.
#

RACE_FREE_CP = $(COMMON_DIR_PATH)/shell/race_free_cp.sh $(1) $(2) $(3)


#
# Race-free version of "mkdir" utility.
#
# Arguments:
#    $1: Path to the resulting directory to be created.
#    $2: (optional) If non-empty, then creates the parent directories too.
#

RACE_FREE_MKDIR = $(COMMON_DIR_PATH)/shell/race_free_mkdir.sh $(1) $(2)


#
# Path to the Chrome browser.
#
# By default, the value is based on guessing (according to the client OS).
#
# If the guess is wrong, or in order to use a different Chrome browser than the
# system default, the user may define the CHROME_PATH variable externally.
#

ifeq ($(shell uname -s | cut -c 1-5),Linux)
DEFAULT_CHROME_PATH := /usr/bin/google-chrome
endif
ifeq ($(shell uname),Darwin)
DEFAULT_CHROME_PATH := \
	/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome
endif
ifeq ($(shell uname -s | cut -c 1-10),MINGW32_NT)
DEFAULT_CHROME_PATH := \
	"c:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"
endif
DEFAULT_CHROME_PATH ?= failed_to_detect_DEFAULT_CHROME_PATH

CHROME_PATH ?= $(DEFAULT_CHROME_PATH)


#
# Additional environment definitions that need to be passed to Chrome.
#
# This is intended to be customized by the user in case of some special needs.
#
# For example, the browser locale may be specified by setting the "LANGUAGE"
# environment variable to the needed value.
#

CHROME_ENV ?=


#
# Additional command line flags that need to be passed to Chrome.
#
# This is intended to be customized by the user in case of some special needs.
#
# Explanation of arguments:
# no-first-run: Suppresses first-run dialogs, such as making Chrome the default
#   browser.
# password-store: Disables using the system-wide encryption storage backend, to
#   avoid notifications about system backend initialization issues.
#

CHROME_ARGS ?= \
	--no-first-run \
	--password-store=basic \


#
# Macro rule that defines helper variables and rules that allow to depend on
# building of the out directory by some other library (see also the
# COLLECT_DEPENDENCY_OUT macro rule).
#
# Arguments:
#    $1: Name of the variable that should be assigned with the path to the
#				 desired out directory.
#    $2: Path to the other library build directory.
#    $3: Target name of the other library.
#

define DEFINE_OUT_GENERATION

$(1) := $(2)/$(OUT_DIR_ROOT_PATH)/$(3)

.PHONY: generate_out_$(2)/$(OUT_DIR_ROOT_PATH)/$(3)

generate_out_$(2)/$(OUT_DIR_ROOT_PATH)/$(3):
	+$(MAKE) -C $(2) generate_out

endef


#
# Macro rule that adds rules for collecting some other's library out directory
# and putting its contents into the own out directory.
#
# Arguments:
#    $1: Path to the other library's out directory. It is assumed that there is
#				 a special target generate_out_$1 defined that provides the building of
#        this out directory (see the DEFINE_OUT_GENERATION macro rule that may
#				 be used for providing this target).
#

define COLLECT_DEPENDENCY_OUT

.PHONY: collect_dependency_out_$(1)

collect_dependency_out_$(1): $(OUT_DIR_PATH) generate_out_$(1)
	@cd $(1) && \
		find . -type f \
			-exec $(CURDIR)/$(call RACE_FREE_CP,{},$(CURDIR)/$(OUT_DIR_PATH)/{},true) \;

generate_out: collect_dependency_out_$(1)

endef


#
# Returns the items of the input list with duplicates being removed from it.
#
# Arguments:
#    $1: List of input items.
#

UNIQUE = $(shell echo $(1) | xargs -n1 | sort -u)


#
# Returns the items of the input list joined through the specified delimiter.
#
# Arguments:
#    $1: List of input items.
#    $2: Separator.
#

JOIN_WITH_DELIMITER = $(subst $(SPACE),$(2),$(1))
