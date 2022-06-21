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


TARGET := example_js_standalone_smart_card_client_library

include ../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk


# Input sources (files and directories) for the library:
JS_COMPILER_INPUT_PATHS := \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	src/exports.js \

# Namespaces and files that Closure Compiler should take into the resulting
# library (together with their dependencies):
JS_COMPILER_ENTRY_POINTS := \
	GoogleSmartCard.PcscLiteClient.API \
	GoogleSmartCard.PcscLiteClient.Context \
	example_js_standalone_smart_card_client_library/src/exports.js \

JS_COMPILER_OUTPUT_FILE_NAME := google-smart-card-client-library.js

# Extra parameters passed to Closure Compiler.
#
# Explanation:
# * assume_function_wrapper, output_wrapper: Enclose the produced code into an
#   anonymous function (to avoid symbol collisions with any code outside the
#   library);
# * USE_SCOPED_LOGGERS: Prefix all logger names with "GoogleSmartCard.". It's
#   done to prevent logger name/setting collision with the code outside the
#   library (in case it uses Closure Library's logging too);
JS_COMPILER_ADDITIONAL_FLAGS := \
	--assume_function_wrapper \
	--define='GoogleSmartCard.Logging.USE_SCOPED_LOGGERS=true' \
	--output_wrapper="(function(){%output%})();" \

# Build the library's JS file via Closure Compiler.
$(eval $(call BUILD_JS_SCRIPT,$(JS_COMPILER_OUTPUT_FILE_NAME),$(JS_COMPILER_INPUT_PATHS),$(JS_COMPILER_ENTRY_POINTS),$(JS_COMPILER_ADDITIONAL_FLAGS)))

# Build an "ES module" flavor of the library. It's a concatenation of the
# Closure Compiler output that's built above and the es-module-exports.js file.
$(JS_BUILD_DIR_PATH)/google-smart-card-client-library-es-module.js: $(JS_BUILD_DIR_PATH) $(OUT_DIR_PATH)/$(JS_COMPILER_OUTPUT_FILE_NAME)
	cat \
		$(JS_BUILD_DIR_PATH)/$(JS_COMPILER_OUTPUT_FILE_NAME) \
		src/es-module-exports.js \
		> $(JS_BUILD_DIR_PATH)/google-smart-card-client-library-es-module.js

# Copy the created ES module .js file into ./out/.
$(eval $(call COPY_TO_OUT_DIR_RULE,\
	$(JS_BUILD_DIR_PATH)/google-smart-card-client-library-es-module.js))
