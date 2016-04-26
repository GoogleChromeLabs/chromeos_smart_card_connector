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


TARGET := smart_card_connector_app

include ../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(COMMON_DIR_PATH)/make/app_building_common.mk

include $(COMMON_DIR_PATH)/make/nacl_module_dependency_in_app_common.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/naclport/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server_clients_management/include.mk


SOURCES_PATH := ../src


JS_COMPILER_BACKGROUND_INPUT_PATHS := \
	$(SOURCES_PATH) \
	$(LIBUSB_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

$(eval $(call BUILD_JS_SCRIPT,background.js,$(JS_COMPILER_BACKGROUND_INPUT_PATHS),GoogleSmartCard.ConnectorApp.BackgroundMain))


JS_COMPILER_WINDOW_INPUT_PATHS := \
	$(SOURCES_PATH) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

$(eval $(call BUILD_JS_SCRIPT,window.js,$(JS_COMPILER_WINDOW_INPUT_PATHS),GoogleSmartCard.ConnectorApp.Window.Main))


$(eval $(call ADD-NACL-MODULE-DEPENDENCY,nacl_module))


$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/_locales))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/icon.png))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/managed_storage_schema.json))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/manifest.json))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/window.html))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/window.css))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/about-window.html))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/about-window.css))
