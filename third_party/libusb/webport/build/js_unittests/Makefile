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
# This makefile builds and runs JavaScript unit tests for the libusb webport.
#


TARGET := js_libusb_unittests

include ../../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk
include ../../include.mk


# Compile all JavaScript files rooting in the tested component's directory
# (including test files), and also compile the common library files (excluding
# that library's test files).
#
# The path constants used below come from the include.mk files.
JS_COMPILER_INPUT_PATHS := \
	$(LIBUSB_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

$(eval $(call BUILD_JS_UNITTESTS,$(JS_COMPILER_INPUT_PATHS)))
