# Copyright 2016 Google Inc.
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
# This file contains helper definitions for using this library.
#
# /common/make/common.mk must be included before including this file.
#


CLOSURE_LIBRARY_RELATIVE_DIR_PATH := closure-library/src

CLOSURE_LIBRARY_DIR_PATH := \
	$(THIRD_PARTY_DIR_PATH)/$(CLOSURE_LIBRARY_RELATIVE_DIR_PATH)


#
# List of all source files and directories in the Closure Library.
#
# This variable is intended to be used as make rules prerequisites.
#

CLOSURE_LIBRARY_SOURCES_AND_DIRS := \
	$(shell find $(CLOSURE_LIBRARY_DIR_PATH) -type d -o -type f -name "*.js")


#
# Function for obtaining the input list for the Closure Compiler that correspond
# to the Closure Library.
#
# Test and perf files are excluded from compilation, because some of them may
# contain duplicate symbols and some not correspond to some strict validation
# rules.
#

CLOSURE_LIBRARY_COMPILER_INPUTS = \
	$(call RELATIVE_PATH,$(CURDIR)/$(CLOSURE_LIBRARY_DIR_PATH),$(ROOT_PATH))/closure \
	!$(call RELATIVE_PATH,$(CURDIR)/$(CLOSURE_LIBRARY_DIR_PATH),$(ROOT_PATH))/**_test.js \
	!$(call RELATIVE_PATH,$(CURDIR)/$(CLOSURE_LIBRARY_DIR_PATH),$(ROOT_PATH))/**_perf.js \
	!$(call RELATIVE_PATH,$(CURDIR)/$(CLOSURE_LIBRARY_DIR_PATH),$(ROOT_PATH))/closure/bin/**.js \


#
# Flags for the Closure Compiler for the different compilation modes.
#

CLOSURE_LIBRARY_COMPILER_FLAGS := \
	--define goog.DISALLOW_TEST_ONLY_CODE=true \
	--define goog.ENABLE_CHROME_APP_SAFE_SCRIPT_LOADING=true \
	--define goog.ENABLE_DEBUG_LOADER=false \
	--define goog.LOAD_MODULE_USING_EVAL=false \
	--define goog.asserts.ENABLE_ASSERTS=true \
	--define goog.debug.LOGGING_ENABLED=true \
	--hide_warnings_for '$(CLOSURE_LIBRARY_RELATIVE_DIR_PATH)' \

ifeq ($(CONFIG),Release)

CLOSURE_LIBRARY_COMPILER_FLAGS += \
	--define goog.DEBUG=false \

else

CLOSURE_LIBRARY_COMPILER_FLAGS += \
	--define goog.DEBUG=true \

endif

CLOSURE_LIBRARY_TESTING_ADDITIONAL_COMPILER_FLAGS := \
	--define goog.DISALLOW_TEST_ONLY_CODE=false \
