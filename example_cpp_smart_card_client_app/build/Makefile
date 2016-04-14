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


TARGET := example_cpp_smart_card_client_app

include ../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(COMMON_DIR_PATH)/make/app_building_common.mk

include $(COMMON_DIR_PATH)/make/nacl_module_dependency_in_app_common.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/cpp_client/include.mk


SOURCES_PATH := ../src


JS_COMPILER_INPUT_PATHS := \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_CPP_CLIENT_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(SOURCES_PATH) \

$(eval $(call BUILD_JS_SCRIPT,background.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.BackgroundMain))

$(eval $(call BUILD_JS_SCRIPT,pin-dialog.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.PinDialog.Main))


$(eval $(call ADD-NACL-MODULE-DEPENDENCY,nacl_module))


$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/_locales))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/manifest.json))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/pin-dialog/pin-dialog.css))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/pin-dialog/pin-dialog.html))
