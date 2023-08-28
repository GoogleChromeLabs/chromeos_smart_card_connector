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

# Below are rules for compiling the "entry point", i.e. our implementation of
# the interface expected by the NaCl framework (`pp::CreateModule()`).
#
# Note that we don't put this file into a static library, since it'd lead to
# circular dependencies.
JS_TO_CXX_TEST_ENTRY_POINT_SOURCE := \
	$(ROOT_PATH)/common/integration_testing/src/entry_point_nacl.cc

JS_TO_CXX_TEST_ENTRY_POINT_FLAGS := \
	-I$(ROOT_PATH) \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-std=$(CXX_DIALECT) \

$(eval $(call COMPILE_RULE,$(JS_TO_CXX_TEST_ENTRY_POINT_SOURCE),\
	$(JS_TO_CXX_TEST_ENTRY_POINT_FLAGS)))

# Internal constant containing the list of additional C/C++ static libraries to
# link against.
#
# The libraries are listed in the dependency order (i.e., a consumer of some
# symbols comes before their provider).
JS_TO_CXX_TEST_NACL_LIBS := \
	$(INTEGRATION_TESTING_LIB) \
	$(CPP_COMMON_LIB) \
	$(DEFAULT_NACL_LIBS) \

# Internal constant containing the list of additional JS sources to be passed to
# Closure Compiler.
JS_TO_CXX_TEST_JS_COMPILER_INPUTS := \
	$(INTEGRATION_TESTING_JS_COMPILER_INPUT_DIR_PATHS) \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \

# Documented in ../include.mk.
define BUILD_JS_TO_CXX_TEST

# Target that generates the NaCl executable module containing compiled C/C++
# helpers.
$(eval $(call LINK_EXECUTABLE_RULE,$(1) $(JS_TO_CXX_TEST_ENTRY_POINT_SOURCE),\
	$(2) $(JS_TO_CXX_TEST_NACL_LIBS)))

# Target that generates tests.js containing compiled JS tests and helpers.
$(eval $(call BUILD_TESTING_JS_SCRIPT,tests.js,\
	$(3) $(JS_TO_CXX_TEST_JS_COMPILER_INPUTS)))

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
