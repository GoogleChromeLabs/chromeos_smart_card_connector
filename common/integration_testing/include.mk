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
# This file contains helper definitions for using this library.
#
# /common/make/common.mk and /common/make/executable_building.mk must be
# included before including this file.
#


INTEGRATION_TESTING_LIB := google_smart_card_integration_testing

INTEGRATION_TESTING_DIR_PATH := $(COMMON_DIR_PATH)/integration_testing

# This constant contains the list of input paths for the Closure Compiler for
# compiling the code using this library.
INTEGRATION_TESTING_JS_COMPILER_INPUT_DIR_PATHS := \
	$(INTEGRATION_TESTING_DIR_PATH) \
