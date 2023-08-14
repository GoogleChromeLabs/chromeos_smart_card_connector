# Copyright 2020 Google Inc. All Rights Reserved.
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
# This makefile builds and runs JavaScript unit tests for the Smart Card
# Connector.
#


TARGET := js_smart_card_connector_unittests

include ../../../common/make/common.mk
include $(COMMON_DIR_PATH)/make/js_building_common.mk
include $(COMMON_DIR_PATH)/js/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/js_client/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/common/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/include.mk

# The list of source paths that Closure Compiler should compile for these unit
# tests.
# At high level, it's:
# * Code-under-test and its tests under smart_card_connector_app/.
# * All libraries it uses (including transitive dependencies).
# * Exclusion pattern for C++-to-JS tests (they're run in a separate target with
#   special setup).
JS_COMPILER_INPUT_PATHS := \
  $(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
  $(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
  $(PCSC_LITE_JS_CLIENT_COMPILER_INPUT_DIR_PATHS) \
  $(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \
  $(ROOT_PATH)/third_party/libusb/webport/src \
  $(ROOT_PATH)/smart_card_connector_app/src \
  !$(ROOT_PATH)/smart_card_connector_app/src/!**jstocxxtest.js \

$(eval $(call BUILD_JS_UNITTESTS,$(JS_COMPILER_INPUT_PATHS)))
