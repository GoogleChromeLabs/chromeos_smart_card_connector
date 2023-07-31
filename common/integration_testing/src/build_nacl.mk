# Copyright 2023 Google Inc. All Rights Reserved.
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

# This file contains the implementation of the ../include.mk interface that
# builds the JS-to-C++ test runner using the Native Client toolchain.

# Internal constant containing the list of additional C/C++ static libraries to
# link against.
#
# Note: INTEGRATION_TESTING_LIB has to come before CPP_COMMON_LIB, since the
# former uses symbols from the latter. And DEFAULT_NACL_LIBS is specified twice,
# because there's a circular dependency between it and INTEGRATION_TESTING_LIB.
JS_TO_CXX_TEST_NACL_LIBS := \
	$(DEFAULT_NACL_LIBS) \
	$(INTEGRATION_TESTING_LIB) \
	$(CPP_COMMON_LIB) \
	$(DEFAULT_NACL_LIBS) \

JS_TO_CXX_TEST_JS_COMPILER_INPUTS := \
	$(INTEGRATION_TESTING_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

# Documented in ../include.mk.
define BUILD_JS_TO_CXX_TEST

# Target that generates the NaCl executable module containing compiled C/C++
# helpers.
$(eval $(call LINK_EXECUTABLE_RULE,$(1),$(JS_TO_CXX_TEST_NACL_LIBS) $(2)))

# Target that generates tests.js containing compiled JS tests and helpers.
$(eval $(call BUILD_TESTING_JS_SCRIPT,tests.js,$(JS_TO_CXX_TEST_JS_COMPILER_INPUTS) $(3)))

endef

# Target that generates index.html.
#
# It's essentially a simple wrapper that executes the contents of tests.js.
$(OUT_DIR_PATH)/index.html: $(OUT_DIR_PATH)
	@echo "<script src='tests.js'></script>" > $(OUT_DIR_PATH)/index.html
all: $(OUT_DIR_PATH)/index.html

# Target that executes the tests by starting a Chrome instance with the test
# runner's HTML page. A web server is additionally started that serves the page
# and the NaCl module's files. The user-data-dir, which is passed to Chrome for
# storing data, is cleared before each run to prevent state leakage.
#
# Note: The recipe uses variables that are defined by NaCl's makefiles.
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
