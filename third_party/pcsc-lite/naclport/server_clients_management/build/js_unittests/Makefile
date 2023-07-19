# Copyright 2018 Google Inc. All Rights Reserved.
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
# This makefile builds and runs JavaScript unit tests for the
# "server_clients_management" component of the PC/SC-Lite naclport.
#


TARGET := js_pcsc_lite_server_clients_management_unittests

include ../../../../../../common/make/common.mk

include $(ROOT_PATH)/common/make/js_building_common.mk
include $(ROOT_PATH)/common/js/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/common/include.mk
include $(ROOT_PATH)/third_party/pcsc-lite/naclport/server_clients_management/include.mk


# Compile all JavaScript files rooting in the tested component's directory
# (including test files), and also compile the common library files (excluding
# that library's test files).
#
# The path constants used below come from the include.mk files.
JS_COMPILER_INPUT_PATHS := \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_SOURCES_PATH) \
	$(PCSC_LITE_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

$(eval $(call BUILD_JS_UNITTESTS,$(JS_COMPILER_INPUT_PATHS)))
