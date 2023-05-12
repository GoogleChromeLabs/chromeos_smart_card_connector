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
# This makefile builds the Smart Card Connector app/extension.
#
# Upon successful build, the resulting app/extension (including all resources
# that may be needed during run time) is stored under the ./out/ directory.
#
# You may use the "make run" command to start an instance of Chrome that would
# automatically load and run the app.
#
# You may also use the "make package" command to build a .ZIP archive suitable
# for uploading at Chrome WebStore, and "make package_crx" command for creating
# a packaged app/extension .CRX file.
# (Please note that in both cases, the resulting app/extension will have an ID
# different from ID of the app/extension published by Google on Web Store. Only
# project maintainers who have corresponding access in the Chrome Developer
# Dashboard can update the app/extension published by Google.)
#
# For more details regarding these special make targets, refer to the
# common/make/packaging_common.mk file.
#


TARGET := smart_card_connector_app

include ../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(COMMON_DIR_PATH)/make/packaging_common.mk

include $(COMMON_DIR_PATH)/make/executable_module_recursive_build.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/webport/include.mk
include $(THIRD_PARTY_DIR_PATH)/ccid/webport/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server_clients_management/include.mk


SOURCES_PATH := ../src

JS_COMPILER_FLAGS := \
	--define='GoogleSmartCard.Logging.SELF_RELOAD_ON_FATAL_ERROR=true' \
	--externs $(CURDIR)/../src/externs/chrome_smart_card_provider_private.js


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

$(eval $(call BUILD_JS_SCRIPT,background.js,$(JS_COMPILER_BACKGROUND_INPUT_PATHS),GoogleSmartCard.ConnectorApp.BackgroundMain,$(JS_COMPILER_FLAGS)))


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

$(eval $(call BUILD_JS_SCRIPT,window.js,$(JS_COMPILER_WINDOW_INPUT_PATHS),GoogleSmartCard.ConnectorApp.Window.Main,$(JS_COMPILER_FLAGS)))


#
# Rules for invoking recursive make files that build the app's executable module
# (the actual recursive rules can be found at ./executable_module/Makefile).
#

$(eval $(call RECURSIVELY_BUILD_EXECUTABLE_MODULE,executable_module))


#
# Rules for building the app's manifest (see
# <https://developer.chrome.com/apps/manifest> for reference) from a template.
#

$(OUT_DIR_PATH)/manifest.json: $(SOURCES_PATH)/create_app_manifest.py $(SOURCES_PATH)/manifest.json.template $(CCID_SUPPORTED_READERS_CONFIG_PATH) $(OUT_DIR_PATH)
	$(SOURCES_PATH)/create_app_manifest.py \
		--manifest-template-path="$(SOURCES_PATH)/manifest.json.template" \
		--ccid-supported-readers-config-path="$(CCID_SUPPORTED_READERS_CONFIG_PATH)" \
		--target-file-path="$(OUT_DIR_PATH)/manifest.json" \
		--enable-condition="CONFIG=$(CONFIG)" \
		--enable-condition="PACKAGING=$(PACKAGING)"

generate_out: $(OUT_DIR_PATH)/manifest.json


#
# Rules for creating the human readable list of supported readers.
#

all: human_readable_supported_readers.txt

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


# Rules for the background page unload preventing feature. This feature is only
# used in the Emscripten builds - see the src/background.js file for
# explanation. Here we add rules that compile the .js file for the iframe and
# copy it, together with an .html file, to the out directory.

ifeq ($(TOOLCHAIN),emscripten)
$(eval $(JS_COMMON_BACKGROUND_PAGE_UNLOAD_PREVENTING_IFRAME_RULE))
endif


# Rules for running unit tests.
# We run C++ and JS tests sequentially to avoid any runtime conflict.
test:: all
	+$(MAKE) --directory executable_module test
	+$(MAKE) --directory js_unittests run_test

# Rules for cleaning unit test build artifacts on "make clean".
js_unittests_clean::
	+$(MAKE) --directory js_unittests clean
clean: js_unittests_clean
