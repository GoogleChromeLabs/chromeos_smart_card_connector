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
# Makefile for JavaScript-and-C++ integration tests.
#

TARGET := integration_tests

include ../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk
include $(COMMON_DIR_PATH)/make/executable_building.mk
include $(COMMON_DIR_PATH)/cpp/include.mk
include $(COMMON_DIR_PATH)/cpp/dependency.mk
include $(COMMON_DIR_PATH)/js/include.mk
include $(COMMON_DIR_PATH)/integration_testing/include.mk
include $(COMMON_DIR_PATH)/integration_testing/dependency.mk


SOURCE_DIR := ../../src/

CPP_SOURCES := \
	$(SOURCE_DIR)/chrome_certificate_provider/api_bridge.cc \
	$(SOURCE_DIR)/chrome_certificate_provider/api_bridge_integration_test_helper.cc \
	$(SOURCE_DIR)/chrome_certificate_provider/types.cc \

JS_COMPILER_INPUT_PATHS := \
	$(INTEGRATION_TESTING_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(SOURCE_DIR) \

# Note: INTEGRATION_TESTING_LIB has to come before CPP_COMMON_LIB, since the
# former uses symbols from the latter. And DEFAULT_NACL_LIBS is specified twice,
# because there's a circular dependency between it and INTEGRATION_TESTING_LIB.
LIBS := \
	$(DEFAULT_NACL_LIBS) \
	$(INTEGRATION_TESTING_LIB) \
	$(CPP_COMMON_LIB) \
	$(DEFAULT_NACL_LIBS) \

DEPS := \
	$(CPP_COMMON_LIB) \
	$(INTEGRATION_TESTING_LIB) \

CPPFLAGS := \
	-I$(SOURCE_DIR) \
	-I$(ROOT_PATH)/common/cpp/src \
	-pedantic \
	-std=gnu++11 \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \

$(foreach src,$(CPP_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))
$(eval $(call LINK_EXECUTABLE_RULE,$(CPP_SOURCES),$(LIBS),$(DEPS)))

$(foreach src,$(CPP_SOURCES),$(eval $(call DEPEND_COMPILE_ON_NACL_LIBRARY_HEADERS,$(src),$(INTEGRATION_TESTING_HEADERS_INSTALLATION_STAMP_FILE))))

$(eval $(call BUILD_TESTING_JS_SCRIPT,tests.js,$(JS_COMPILER_INPUT_PATHS)))

$(OUT_DIR_PATH)/index.html: $(OUT_DIR_PATH)
	@echo "<script src='tests.js'></script>" > $(OUT_DIR_PATH)/index.html
all: $(OUT_DIR_PATH)/index.html

# Target that executes the tests by starting a Chrome instance with the test
# runner's HTML page. A web server is additionally started that serves the page
# and the NaCl module's files. The user-data-dir, which is passed to Chrome for
# storing data, is cleared before each run to prevent state leakage.
#
# Note: The recipe uses variables that are defined by the NaCl's makefiles.
run_test: all
	rm -rf user-data-dir
	$(RUN_PY) \
		-C $(OUT_DIR_PATH) \
		-P "index.html?tc=$(TOOLCHAIN)&config=$(CONFIG)" \
	  $(addprefix -E ,$(CHROME_ENV)) \
	  -- \
	  "$(CHROME_PATH)" \
	  $(CHROME_ARGS) \
	  --register-pepper-plugins="$(PPAPI_DEBUG),$(PPAPI_RELEASE)" \
