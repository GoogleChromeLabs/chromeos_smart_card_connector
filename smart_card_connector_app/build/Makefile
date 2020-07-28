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
# This makefile builds the Smart Card Connector app.
#
# Upon successful build, the resulting app (including all resources that may be
# needed during run time) is stored under the ./out/ directory.
#
# You may use the "make run" command to start an instance of Chrome that would
# automatically load and run the app.
#
# You may also use the "make package" command to build a packaged app .CRX file
# and a .ZIP archive suitable for uploading at Chrome WebStore. (Please note
# that in both cases, the resulting app will have an ID different from ID of the
# app published by Google on Web Store. Only project maintainers who have
# corresponding access in the Chrome Developer Dashboard can update the app
# published by Google.)
#
# For more details regarding these special make targets, refer to the
# common/make/app_building_common.mk file.
#


TARGET := smart_card_connector_app

include ../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(COMMON_DIR_PATH)/make/app_building_common.mk

include $(COMMON_DIR_PATH)/make/nacl_module_dependency_in_app_common.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/naclport/include.mk
include $(THIRD_PARTY_DIR_PATH)/ccid/naclport/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server_clients_management/include.mk


SOURCES_PATH := ../src


#
# Constants and rules for compiling the background.js file.
#

JS_COMPILER_BACKGROUND_INPUT_PATHS := \
	$(SOURCES_PATH) \
	$(LIBUSB_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_SERVER_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

$(eval $(call BUILD_JS_SCRIPT,background.js,$(JS_COMPILER_BACKGROUND_INPUT_PATHS),GoogleSmartCard.ConnectorApp.BackgroundMain))


#
# Constants and rules for compiling the window.js file.
#

JS_COMPILER_WINDOW_INPUT_PATHS := \
	$(SOURCES_PATH) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_SERVER_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

$(eval $(call BUILD_JS_SCRIPT,window.js,$(JS_COMPILER_WINDOW_INPUT_PATHS),GoogleSmartCard.ConnectorApp.Window.Main))


#
# Rules for invoking recursive make files that build the app's NaCl module
# (the actual recursive rules can be found at ./nacl_module/Makefile).
#

$(eval $(call ADD-NACL-MODULE-DEPENDENCY,nacl_module))


#
# Rules for building the app's manifest (see
# <https://developer.chrome.com/apps/manifest> for reference) from a template.
#

$(OUT_DIR_PATH)/manifest.json: $(SOURCES_PATH)/create_app_manifest.py $(SOURCES_PATH)/manifest.json.template $(CCID_SUPPORTED_READERS_CONFIG_PATH) $(OUT_DIR_PATH)
	$(SOURCES_PATH)/create_app_manifest.py \
		--manifest-template-path="$(SOURCES_PATH)/manifest.json.template" \
		--ccid-supported-readers-config-path="$(CCID_SUPPORTED_READERS_CONFIG_PATH)" \
		--target-file-path="$(OUT_DIR_PATH)/manifest.json"

generate_out: $(OUT_DIR_PATH)/manifest.json


#
# Rules for creating the human readable list of supported readers.
#

.PHONY: package

package: human_readable_supported_readers.txt

human_readable_supported_readers.txt: $(SOURCES_PATH)/create_human_readable_supported_readers.py $(CCID_SUPPORTED_READERS_CONFIG_PATH)
	$(SOURCES_PATH)/create_human_readable_supported_readers.py \
		--ccid-supported-readers-config-path="$(CCID_SUPPORTED_READERS_CONFIG_PATH)" \
		--target-file-path=human_readable_supported_readers.txt


#
# Rules for copying the static resources files into the app's out directory.
#

$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/_locales))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/icon.png))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/managed_storage_schema.json))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/window.html))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/window.css))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/card-present.png))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/about-window.html))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/about-window.css))
