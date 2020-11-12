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
# This file contains some helper definitions for including the Chrome Native
# Client modules in Chrome Apps.
# TODO(#177): Generalize the file to support Emscripten as well.
#
# common.mk must be included before including this file.
#


ifeq (,$(NACL_SDK_ROOT))
$(error You must specify the NACL_SDK_ROOT environment variable with a path to \
		a downloaded NaCl SDK directory)
endif


#
# Macro rule that adds some auxiliary rules for depending on the specified
# Native Client module.
#
# These rules provide automatic re-building of the Native Client module when
# necessary, copying its built binaries into the out directory.
#

define ADD-NACL-MODULE-DEPENDENCY

.PHONY: $(1)

$(1):
	+$(MAKE) --directory $(1)

clean:
	+$(MAKE) --directory $(1) clean

.PHONY: generate_out_nacl_module_$(1)

generate_out_nacl_module_$(1): $(OUT_DIR_PATH) $(1)
	@cp -pr $(1)/out/$(1)/* $(OUT_DIR_PATH)

generate_out: generate_out_nacl_module_$(1)

endef


#
# In order to mount App resources as an "httpfs" file system in NaCl module and
# support all usual file system operations with it (like listing directories
# contents), a special manifest listing all the files has to be created.
#

NACL_IO_MANIFEST_PATH := $(OUT_DIR_PATH)/nacl_io_manifest.txt

$(NACL_IO_MANIFEST_PATH): generate_out
	$(NACL_SDK_ROOT)/tools/genhttpfs.py --srcdir "$(OUT_DIR_PATH)" --recursive . > nacl_io_manifest.txt.build
	@mv nacl_io_manifest.txt.build $(NACL_IO_MANIFEST_PATH)

$(eval $(call CLEAN_RULE,nacl_io_manifest.txt.build))

all: $(NACL_IO_MANIFEST_PATH)
