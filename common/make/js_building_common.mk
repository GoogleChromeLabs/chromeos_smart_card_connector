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
# This file contains some helper definitions for building JavaScript code using
# Google Closure Compiler (see
# <https://developers.google.com/closure/compiler/>).
#
# The Google Closure Library (see
# <https://developers.google.com/closure/library/>) is additionally enabled to
# be used be the compiled JavaScript code.
#
# common.mk must be included before including this file.
#


#
# Including Google Closure Library and Compiler definitions.
#

include $(THIRD_PARTY_DIR_PATH)/closure-compiler-binary/include.mk
include $(THIRD_PARTY_DIR_PATH)/closure-library/include.mk


#
# Directory where the JavaScript built files are temporarily located, before
# copying to the out directory (this is separate from the out directory, because
# the latter is cleared on each rebuild).
#
# Note that the directory depends on both TOOLCHAIN and CONFIG variables, so
# that JavaScript files from different build configurations aren't mixed up.
#

JS_BUILD_DIR_ROOT_PATH := js_build

JS_BUILD_DIR_PATH := $(JS_BUILD_DIR_ROOT_PATH)/$(TOOLCHAIN)_$(CONFIG)

$(JS_BUILD_DIR_PATH):
	@mkdir -p $(JS_BUILD_DIR_PATH)

$(eval $(call CLEAN_RULE,$(JS_BUILD_DIR_ROOT_PATH)))


#
# Determine Closure Compiler base compilation flags.
#
# Advanced code transformations are enabled only in Release mode. Unfortunately,
# the code in Debug mode also has to be compiled and transformed - because the
# lowest Closure Compiler compilation level, WHITESPACE_ONLY, does not support
# overriding goog.define'd variables from the command line, which is crucial for
# our code.
#

ifeq ($(CONFIG),Release)

# Add Closure Compiler flags specific to release builds.
JS_BUILD_COMPILATION_FLAGS := \

else

# Add Closure Compiler flags specific to debug builds.
JS_BUILD_COMPILATION_FLAGS := \
	--debug \
	--formatting=PRETTY_PRINT \
	--formatting=PRINT_INPUT_DELIMITER \

endif

# Add common Closure Compiler flags.
#
# Explanation:
# dependency_mode=PRUNE: Skip unused JS code.
# jscomp_error *: Enable all warning classes and treat them as errors by
#   default. Note: To be on the safe side, we're specifying all possible warning
#   classes, except those that don't work for our codebase, which we explicitly
#   list via jscomp_off.
# jscomp_off deprecated: Suppress warnings about the usage of goog.inherits().
#   TODO: Switch to using ECMAScript style inheritance and promote this to
#   jscomp_error.
# jscomp_off extraRequire: Suppress warnings about unused goog.require()
#   statements. TODO: Fix the violations and promote this to jscomp_error.
# jscomp_off jsdocMissingConst: Suppress warnings about variables that could be
#   marked as const. TODO: Fix the violations and promote this to jscomp_error.
# jscomp_off lintChecks: Suppress complaints about code style. TODO: Fix the
#   code (e.g., switch to modules instead of goog.provide) and promote this to
#   jscomp_error.
# jscomp_off reportUnknownTypes: Suppressed due to many false positives in case
#   the compiler isn't smart enough to infer the variable's type.
# jscomp_off strictMissingProperties: Suppressed due to false positives on the
#   .base() calls. TODO: Replace the base() calls with the ECMAScript style
#   inheritance and promote this to jscomp_error.
# jscomp_off unknownDefines: Suppressed in order to avoid compiler complaints
#   when an unused constant is passed via --define.
JS_BUILD_COMPILATION_FLAGS += \
	--compilation_level=SIMPLE \
	--define='GoogleSmartCard.ExecutableModule.TOOLCHAIN=$(TOOLCHAIN)' \
	--define='GoogleSmartCard.Logging.USE_SCOPED_LOGGERS=false' \
	--dependency_mode=PRUNE \
	--jscomp_error='*' \
	--jscomp_off deprecated \
	--jscomp_off extraRequire \
	--jscomp_off lintChecks \
	--jscomp_off reportUnknownTypes \
	--jscomp_off strictMissingProperties \
	--jscomp_off unknownDefines \
	--use_types_for_optimization=false \
	--warning_level=VERBOSE \


# Closure Compiler compilation flags that should be additionally specified when
# compiling JavaScript files with tests.
#
# Explanation:
# dependency_mode=PRUNE_LEGACY: In addition to the standard PRUNE behavior (see
#   above), also include all JS files that don't have a goog.provide. This
#   allows unit test files to be picked up and compiled automatically.
JS_BUILD_TESTING_ADDITIONAL_COMPILATION_FLAGS := \
	$(CLOSURE_LIBRARY_TESTING_ADDITIONAL_COMPILER_FLAGS) \
	--dependency_mode=PRUNE_LEGACY \


# List of Closure Compiler input files that contain extern definitions for
# JavaScript APIs available in Chrome.
CHROME_EXTERNS_COMPILER_INPUTS := \
	$(THIRD_PARTY_DIR_PATH)/closure-compiler/src/contrib/externs/chrome.js \
	$(THIRD_PARTY_DIR_PATH)/closure-compiler/src/contrib/externs/chrome_extensions.js \
	$(THIRD_PARTY_DIR_PATH)/closure-compiler/src_additional/chrome_extensions.js \


#
# Macro rule that compiles the resulting JavaScript script, scanning the
# specified directories with the source JavaScript files and picking the
# dependencies for the specified Closure namespaces (see
# <https://developers.google.com/closure/library/docs/introduction#names>).
#
# In Debug mode, additionally creates a source map for the compiled script and
# copies all relevant source files into the out directory (preserving their
# original directory structure, relatively to the project root $(ROOT_PATH)).
#
# Arguments:
#    $1: Path to the resulting JavaScript file (relatively to the out
#        directory).
#    $2: Paths to the directories with the source JavaScript files (it should
#        not include path to the Google Closure library itself).
#    $3: Closure namespaces, from which the JavaScript code needs to be picked.
#    $4: Optional additional compiler flags.
#
# TODO(emaxx): Hide the executed commands from echoing, unless the V=1 variable
# is specified (like the NaCl SDK compilation rules do).
#

define BUILD_JS_SCRIPT

$(JS_BUILD_DIR_PATH)/$(1): $(CURDIR)/$(CLOSURE_COMPILER_JAR_PATH) $(CLOSURE_LIBRARY_SOURCES_AND_DIRS) $(call FIND_JS_SOURCES_AND_PARENT_DIRS,$(2)) $(CHROME_EXTERNS_COMPILER_INPUTS) | $(JS_BUILD_DIR_PATH)
	@rm -f $(JS_BUILD_DIR_PATH)/$(1)
	@$(call RACE_FREE_MKDIR,$(dir $(JS_BUILD_DIR_PATH)/$(1)))
	cd $(ROOT_PATH) && \
		java -server -XX:+TieredCompilation -jar $(CURDIR)/$(CLOSURE_COMPILER_JAR_PATH) \
			$(JS_BUILD_COMPILATION_FLAGS) \
			$(addprefix --entry_point , $(3)) \
			--create_source_map $(CURDIR)/$(JS_BUILD_DIR_PATH)/$(1).map \
			--output_manifest $(CURDIR)/$(JS_BUILD_DIR_PATH)/$(1).manifest \
			$(addprefix --externs $(CURDIR)/,$(CHROME_EXTERNS_COMPILER_INPUTS)) \
			$(CLOSURE_LIBRARY_COMPILER_FLAGS) \
			$(4) \
			$(CLOSURE_LIBRARY_COMPILER_INPUTS) \
			$(foreach input_dir_path, \
				$(2), \
				$(call MAKE_RELATIVE_JS_COMPILER_INPUT,$(input_dir_path))) \
			> $(CURDIR)/$(JS_BUILD_DIR_PATH)/$(1).build
	@mv $(JS_BUILD_DIR_PATH)/$(1).build $(JS_BUILD_DIR_PATH)/$(1)
	@if [ "$(CONFIG)" != "Release" ]; then \
		echo "//# sourceMappingURL=$(1).map" >> $(JS_BUILD_DIR_PATH)/$(1) ;\
	fi

.PHONY: generate_js_build_out_$(JS_BUILD_DIR_PATH)/$(1)

generate_js_build_out_$(JS_BUILD_DIR_PATH)/$(1): $(OUT_DIR_PATH) $(JS_BUILD_DIR_PATH)/$(1)
	@if [ "$(CONFIG)" != "Release" ]; then \
		cp -p $(JS_BUILD_DIR_PATH)/$(1).map $(OUT_DIR_PATH)/$(1).map ;\
		while read INPUT_JS_FILE; do \
			$(call RACE_FREE_CP,$(ROOT_PATH)/$$$${INPUT_JS_FILE},$(OUT_DIR_PATH)/$$$${INPUT_JS_FILE},true) ;\
			done < $(JS_BUILD_DIR_PATH)/$(1).manifest ;\
	fi

generate_out: generate_js_build_out_$(JS_BUILD_DIR_PATH)/$(1)

$(eval $(call COPY_TO_OUT_DIR_RULE,$(JS_BUILD_DIR_PATH)/$(1),$(dir $(1))))

endef


#
# Macro rule that compiles the resulting JavaScript file with the tests,
# scanning the specified directories with the source JavaScript files and
# picking the dependencies for the specified Closure namespaces.
#
# Arguments:
#    $1: Path to the resulting JavaScript file (relatively to the out
#        directory).
#    $2: Paths to the directories with the source JavaScript files (it should
#        not include path to the Google Closure library itself).
#    $3: Optional additional compiler flags.
#

define BUILD_TESTING_JS_SCRIPT

$(eval $(call BUILD_JS_SCRIPT,$(1),$(2),,$(JS_BUILD_TESTING_ADDITIONAL_COMPILATION_FLAGS) $(3)))

endef


#
# Macro rule that compiles the resulting JavaScript file with unit tests (see
# also the BUILD_TESTING_JS_SCRIPT macro rule), creates HTML file with unit
# tests runner, and adds run_test target that executes the unit tests.
#
# Arguments:
#    $1: Paths to the directories with the source JavaScript files (it should
#        not include path to the Google Closure library itself).
#    $2: Optional additional compiler flags.
#

define BUILD_JS_UNITTESTS

$(eval $(call BUILD_TESTING_JS_SCRIPT,unittests.js,$(1),$(2)))

$(OUT_DIR_PATH)/index.html: $(OUT_DIR_PATH)
	@echo "<script src='unittests.js'></script>" > $(OUT_DIR_PATH)/index.html

generate_out: $(OUT_DIR_PATH)/index.html

run_test: all
	$(CHROME_ENV) $(CHROME_PATH) $(OUT_DIR_PATH)/index.html $(CHROME_ARGS) \
		--user-data-dir=$(OUT_DIR_ROOT_PATH)/temp_chrome_profile

endef


#
# Function that returns list of all JavaScript files under the specified
# locations, and, additionally, their parent directories.
#
# This function is intended to be used as make rules prerequisites.
#

FIND_JS_SOURCES_AND_PARENT_DIRS = \
	$(shell find $(filter-out !%,$(1)) -type d -o -type f -name "*.js" -exec dirname {} \; -exec echo {} \;)


#
# Function that transforms the specified compiler input from a relative path
# (relative to the repository root) into a path relative to the current working
# directory.
#
# This function also takes care to handle the negation masks (like '!foo/**bar')
# correctly.
#

MAKE_RELATIVE_JS_COMPILER_INPUT = \
	$(findstring !,$(1))$(call RELATIVE_PATH,$(CURDIR)/$(subst !,,$(1)),$(ROOT_PATH))
