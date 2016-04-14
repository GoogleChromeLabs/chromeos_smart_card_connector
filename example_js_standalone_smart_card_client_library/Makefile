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


JS_COMPILER_INPUT_PATHS := \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	exports.js \

JS_COMPILER_TARGET_NAMESPACES := \
	GoogleSmartCard.PcscLiteClient.API \
	GoogleSmartCard.PcscLiteClient.Context \

JS_COMPILER_OUTPUT_FILE_NAME := google-smart-card-client-library.js

JS_COMPILER_ADDITIONAL_FLAGS := \
	--output_wrapper="(function(){%output%})();" \

$(eval $(call BUILD_JS_SCRIPT,$(JS_COMPILER_OUTPUT_FILE_NAME),$(JS_COMPILER_INPUT_PATHS),$(JS_COMPILER_TARGET_NAMESPACES),$(JS_COMPILER_ADDITIONAL_FLAGS)))
