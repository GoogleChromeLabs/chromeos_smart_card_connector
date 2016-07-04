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

include $(THIRD_PARTY_DIR_PATH)/closure-compiler/include.mk
include $(THIRD_PARTY_DIR_PATH)/closure-library/include.mk


#
# Including Google Chrome Extensions API definitions.
#

include $(THIRD_PARTY_DIR_PATH)/chrome/include.mk


#
# Directory where the JavaScript built files are temporarily located, before
# copying to the out directory (this is separate from the out directory, because
# the latter is cleared on each rebuild).
#

JS_BUILD_DIR_ROOT_PATH := js_build

JS_BUILD_DIR_PATH := $(JS_BUILD_DIR_ROOT_PATH)/$(CONFIG)

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

JS_BUILD_COMPILATION_FLAGS := \

else

JS_BUILD_COMPILATION_FLAGS := \
	--debug \
	--formatting=PRETTY_PRINT \
	--formatting=PRINT_INPUT_DELIMITER \

endif

JS_BUILD_COMPILATION_FLAGS += \
	--closure_entry_point=goog \
	--compilation_level=SIMPLE \
	--jscomp_error=accessControls \
	--jscomp_error=ambiguousFunctionDecl \
	--jscomp_error=checkEventfulObjectDisposal \
	--jscomp_error=checkRegExp \
	--jscomp_error=checkTypes \
	--jscomp_error=checkVars \
	--jscomp_error=conformanceViolations \
	--jscomp_error=const \
	--jscomp_error=constantProperty \
	--jscomp_error=deprecated \
	--jscomp_error=deprecatedAnnotations \
	--jscomp_error=duplicateMessage \
	--jscomp_error=es3 \
	--jscomp_error=es5Strict \
	--jscomp_error=externsValidation \
	--jscomp_error=fileoverviewTags \
	--jscomp_error=globalThis \
	--jscomp_error=inferredConstCheck \
	--jscomp_error=invalidCasts \
	--jscomp_error=misplacedTypeAnnotation \
	--jscomp_error=missingGetCssName \
	--jscomp_error=missingProperties \
	--jscomp_error=missingProvide \
	--jscomp_error=missingRequire \
	--jscomp_error=missingReturn \
	--jscomp_error=msgDescriptions \
	--jscomp_error=newCheckTypes \
	--jscomp_error=nonStandardJsDocs \
	--jscomp_error=suspiciousCode \
	--jscomp_error=strictModuleDepCheck \
	--jscomp_error=typeInvalidation \
	--jscomp_error=undefinedNames \
	--jscomp_error=undefinedVars \
	--jscomp_error=unknownDefines \
	--jscomp_error=unusedLocalVariables \
	--jscomp_error=unusedPrivateMembers \
	--jscomp_error=uselessCode \
	--jscomp_error=useOfGoogBase \
	--jscomp_error=visibility \
	--jscomp_off lintChecks \
	--jscomp_off reportUnknownTypes \
	--manage_closure_dependencies \
	--only_closure_dependencies \
	--warning_level=VERBOSE \


#
# Closure Compiler compilation flags that should be additionally specified when
# compiling JavaScript files with tests.
#

JS_BUILD_TESTING_ADDITIONAL_COMPILATION_FLAGS := \
	$(CLOSURE_LIBRARY_TESTING_ADDITIONAL_COMPILER_FLAGS) \
	--compilation_level=SIMPLE \
	--only_closure_dependencies=false \


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
# FIXME(emaxx): Hide the executed commands from echoing, unless the V=1 variable
# is specified (like the NaCl SDK compilation rules do).
#

define BUILD_JS_SCRIPT

$(JS_BUILD_DIR_PATH)/$(1): $(CLOSURE_LIBRARY_SOURCES_AND_DIRS) $(call FIND_JS_SOURCES_AND_PARENT_DIRS,$(2)) $(CHROME_API_EXTERNS_COMPILER_INPUTS) | $(JS_BUILD_DIR_PATH)
	@rm -f $(JS_BUILD_DIR_PATH)/$(1)
	@$(call RACE_FREE_MKDIR,$(dir $(JS_BUILD_DIR_PATH)/$(1)))
	cd $(ROOT_PATH) && \
		java -server -XX:+TieredCompilation -jar $(CURDIR)/$(CLOSURE_COMPILER_JAR_PATH) \
			$(JS_BUILD_COMPILATION_FLAGS) \
			$(addprefix --closure_entry_point , $(3)) \
			--create_source_map $(CURDIR)/$(JS_BUILD_DIR_PATH)/$(1).map \
			--output_manifest $(CURDIR)/$(JS_BUILD_DIR_PATH)/$(1).manifest \
			$(addprefix --externs $(CURDIR)/,$(CHROME_API_EXTERNS_COMPILER_INPUTS)) \
			$(CLOSURE_LIBRARY_COMPILER_FLAGS) \
			$(4) \
			$(CLOSURE_LIBRARY_COMPILER_INPUTS) \
			$(foreach input_dir_path, \
				$(2), \
				$(call RELATIVE_PATH,$(CURDIR)/$(input_dir_path),$(ROOT_PATH))) \
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
# Function that returns list of all JavaScript files under the specified
# locations, and, additionally, their parent directories.
#
# This function is intended to be used as make rules prerequisites.
#
FIND_JS_SOURCES_AND_PARENT_DIRS = \
	$(shell find $(1) -type d -o -type f -name "*.js" -exec dirname {} \; -exec echo {} \;)
