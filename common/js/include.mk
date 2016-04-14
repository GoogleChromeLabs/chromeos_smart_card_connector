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
# This file contains helper definitions for using this library.
#
# /common/make/common.mk and /common/make/js_building_common.mk must be included
# before including this file.
#


JS_COMMON_DIR_PATH := $(COMMON_DIR_PATH)/js

JS_COMMON_SOURCES_PATH := $(JS_COMMON_DIR_PATH)/src


# This constant contains the list of input paths for the Closure Compiler for
# compiling the code using this library.
JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS := $(JS_COMMON_SOURCES_PATH)


# This macro adds rules for preparing and copying to the out directory the files
# that are necessary for the background page unload preventing feature of the
# library (see the code under the src/background-page-unload-preventing
# directory).
define JS_COMMON_BACKGROUND_PAGE_UNLOAD_PREVENTING_IFRAME_RULE

$(eval $(call BUILD_JS_SCRIPT,background-page-unload-preventing-iframe.js,$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS),GoogleSmartCard.BackgroundPageUnloadPreventing.IFrameMain))

$(eval $(call COPY_TO_OUT_DIR_RULE,$(JS_COMMON_SOURCES_PATH)/background-page-unload-preventing/background-page-unload-preventing-iframe.html))

endef
