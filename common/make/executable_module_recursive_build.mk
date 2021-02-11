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

# This file contains helper definitions for performing recursive make builds of
# executable modules (which are themselves built via executable_building.mk).
#
# common.mk must be included before including this file.

# Macro rule that builds the specified executable module (as a recursive make
# invocation) and copies the latter's resulting build artifacts into the "out"
# directory.
#
# Arguments:
# $1 ("MODULE_NAME"): The executable module's name (note that, by convention,
#   it's both the name of the module's build directory name and the value of the
#   TARGET variable inside the module's makefile).

define RECURSIVELY_BUILD_EXECUTABLE_MODULE

.PHONY: $(1)

$(1):
	+$(MAKE) --directory $(1)

clean:
	+$(MAKE) --directory $(1) clean

.PHONY: generate_out_executable_module_$(1)

generate_out_executable_module_$(1): $(OUT_DIR_PATH) $(1)
	@cp -pr $(1)/out/$(1)/* $(OUT_DIR_PATH)

generate_out: generate_out_executable_module_$(1)

endef
