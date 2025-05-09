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


TARGET := example_js_smart_card_client_app

include ../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(COMMON_DIR_PATH)/make/packaging_common.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/js_demo/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/common/include.mk


SOURCES_PATH := ../src


JS_COMPILER_INPUT_PATHS := \
	$(SOURCES_PATH) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_DEMO_COMPILER_INPUTS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

# Note: Change the SELF_RELOAD_ON_FATAL_ERROR's value to "false" in case you
# need to suppress the automatic self-reload of the extension on fatal failures.
JS_COMPILER_FLAGS := \
	--define='GoogleSmartCard.Logging.SELF_RELOAD_ON_FATAL_ERROR=true' \

$(eval $(call BUILD_JS_SCRIPT,background.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.BackgroundMain,$(JS_COMPILER_FLAGS)))

$(eval $(call BUILD_JS_SCRIPT,pin-dialog.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.PinDialog.Main,$(JS_COMPILER_FLAGS)))

$(eval $(JS_COMMON_BACKGROUND_PAGE_UNLOAD_PREVENTING_IFRAME_RULE))


$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/_locales))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/manifest.json))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/pin-dialog/pin-dialog.css))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/pin-dialog/pin-dialog.html))
