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

include $(COMMON_DIR_PATH)/make/packaging_common.mk

include $(COMMON_DIR_PATH)/make/executable_module_recursive_build.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/js_client/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/cpp_client/include.mk


SOURCES_PATH := ../src


# Constants and rules for compiling the JS files.

JS_COMPILER_INPUT_PATHS := \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_CPP_CLIENT_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
	$(SOURCES_PATH) \

# Note: Change the SELF_RELOAD_ON_FATAL_ERROR's value to "false" in case you
# need to suppress the automatic self-reload of the extension on fatal failures.
# Change the USE_SCOPED_LOGGERS's value to "true" in case the JavaScript
# code you added creates its own loggers whose names clash with the existing
# ones.
JS_COMPILER_FLAGS := \
	--define='GoogleSmartCard.Logging.SELF_RELOAD_ON_FATAL_ERROR=true' \
	--define='GoogleSmartCard.Logging.USE_SCOPED_LOGGERS=false' \

$(eval $(call BUILD_JS_SCRIPT,background.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.BackgroundMain,$(JS_COMPILER_FLAGS)))
$(eval $(call BUILD_JS_SCRIPT,window.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.WindowMain,$(JS_COMPILER_FLAGS)))
$(eval $(call BUILD_JS_SCRIPT,built-in-pin-dialog.js,$(JS_COMPILER_INPUT_PATHS),SmartCardClientApp.BuiltInPinDialog.Main,$(JS_COMPILER_FLAGS)))

ifeq ($(TOOLCHAIN),emscripten)
ifeq ($(PACKAGING),extension)
$(eval $(call BUILD_JS_SCRIPT,emscripten-offscreen-doc.js,$(JS_COMPILER_INPUT_PATHS),GoogleSmartCard.EmscriptenInOffscreenDocMain,$(JS_COMPILER_FLAGS)))
endif
endif


# Rules for invoking recursive make files that build the app's binary executable
# module (build rules for it can be found at ./executable_module/Makefile).

$(eval $(call RECURSIVELY_BUILD_EXECUTABLE_MODULE,executable_module))


# Rules for copying the static resources files into the app's out directory.

$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/_locales))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/manifest.json))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/window.html))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/window.css))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/built_in_pin_dialog/built-in-pin-dialog.css))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/built_in_pin_dialog/built-in-pin-dialog.html))

ifeq ($(TOOLCHAIN),emscripten)
ifeq ($(PACKAGING),extension)
$(eval $(call COPY_TO_OUT_DIR_RULE,$(ROOT_PATH)/common/js/src/executable-module/emscripten-offscreen-doc.html))
endif
endif
