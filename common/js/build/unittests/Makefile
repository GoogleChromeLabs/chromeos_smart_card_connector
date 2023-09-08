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


TARGET := js_common_unittests

include ../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/js/include.mk


# Include *-unittest.js and non-test files needed for them.
#
# Note: "jstocxxtest" files are compiled and executed differently, via
# //common/js/build/js_to_cxx_tests/.
JS_COMPILER_INPUT_PATHS := \
	$(ROOT_PATH)/common/js/src \
	!$(ROOT_PATH)/common/js/src/**-jstocxxtest.js \

$(eval $(call BUILD_JS_UNITTESTS,$(JS_COMPILER_INPUT_PATHS)))
