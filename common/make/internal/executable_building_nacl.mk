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
# This file contains the implementation of the ../executable_building.mk
# interface that builds using Chrome Native Client.
#
# common.mk must be included before including this file.
#


ifeq (,$(NACL_SDK_ROOT))
$(error You must specify the NACL_SDK_ROOT environment variable with a path to \
		a downloaded NaCl SDK directory)
endif


#
# This is a special variable for the NaCl toolchain scripts: by default, only
# "pnacl" (Portable NaCl) is targeted.
#
# (For the difference between NaCl and PNaCl, refer to this document:
# <https://developer.chrome.com/native-client/nacl-and-pnacl>).
#

VALID_TOOLCHAINS ?= pnacl


#
# Load NaCl SDK toolchain definitions.
#

include $(NACL_SDK_ROOT)/tools/common.mk


#
# This variable should be included in the list of linked libraries ("LIBS") when
# invoking the corresponding macro rule.
#
# Note that these libraries usually have to be the last in the list.
#

DEFAULT_NACL_LIBS := \
	ppapi \
	ppapi_cpp \
	pthread \


# Documented at ../executable_building.mk.
#
# Note: "c++11" is not used, since it lacks some features in PNaCl, like the
# `isascii()` standard library function.
CXX_DIALECT := gnu++11


#
# Documented at ../binary_executable_building.mk.
#

define LINK_EXECUTABLE_RULE

$(eval $(call NACL_MODULE_LINK_INTERNAL_RULE,$(1),$(2),$(3),$(4)))

$(eval $(call NACL_MODULE_COPY_BINARIES_INTERNAL_RULE))

$(eval $(call NMF_RULE,$(TARGET),--pnacl-optlevel=0 --pnacl-debug-optlevel=0))

$(eval $(call COPY_TO_OUT_DIR_RULE,$(OUTDIR)/$(TARGET).nmf))

endef


#
# Auxiliary macro rule that adds rules for linking the resulting NaCl binaries.
#
# The PNaCl workflow uses both an unstripped and finalized/stripped binary; on
# NaCl, only produce a stripped binary in Release builds.
#

ifneq (,$(or $(findstring pnacl,$(TOOLCHAIN)),$(findstring Release,$(CONFIG))))

define NACL_MODULE_LINK_INTERNAL_RULE
$(eval $(call LINK_RULE,$(TARGET)_unstripped,$(1),$(2),$(3),$(4)))
$(eval $(call STRIP_RULE,$(TARGET),$(TARGET)_unstripped))
endef

else

define NACL_MODULE_LINK_INTERNAL_RULE
$(eval $(call LINK_RULE,$(TARGET),$(1),$(2),$(3),$(4)))
endef

endif


#
# Auxiliary macro rule that adds rules for copying the resulting NaCl binaries
# into the out directory.
#
# The unstripped binaries are copied only in Debug builds, and finalized/
# stripped binaries are copied always.
#
# FIXME(emaxx): Instead of "pexe" it should use the right extension: "nexe" for
# NaCl, "pexe" for PNaCl.
#

ifneq (,$(findstring Release,$(CONFIG)))

define NACL_MODULE_COPY_BINARIES_INTERNAL_RULE
$(eval $(call COPY_TO_OUT_DIR_RULE,$(OUTDIR)/$(TARGET).pexe))
endef

else

define NACL_MODULE_COPY_BINARIES_INTERNAL_RULE
$(eval $(call COPY_TO_OUT_DIR_RULE,$(OUTDIR)/$(TARGET)_unstripped.bc))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(OUTDIR)/$(TARGET)_unstripped.pexe))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(OUTDIR)/$(TARGET).pexe))
endef

endif

# Rule for creating a "nacl_io_manifest.txt" manifest file, which is used for
# exposing resource files to the NaCl code through a virtual file system
# ("httpfs", provided by nacl_io).
#
# Implementation note: The Python script's output is first redirected into a
# temporary ".build" file, so that a failure in the script leaves the manifest
# file not existing or not modified (so that next invocations of make don't skip
# it).
$(OUT_DIR_PATH)/nacl_io_manifest.txt: generate_out
	$(NACL_SDK_ROOT)/tools/genhttpfs.py \
		--srcdir "$(OUT_DIR_PATH)" \
		--recursive . \
		> nacl_io_manifest.txt.build
	@mv nacl_io_manifest.txt.build $(OUT_DIR_PATH)/nacl_io_manifest.txt
$(eval $(call CLEAN_RULE,nacl_io_manifest.txt.build))
all: $(OUT_DIR_PATH)/nacl_io_manifest.txt

#
# By default, NaCl SDK "clean" rules leave the root output directory named as
# the selected NaCl toolchain (e.g. "pnacl"). Here are provided additional rules
# for removal of these directories too.
#

$(foreach toolchain,$(VALID_TOOLCHAINS),$(eval $(call CLEAN_RULE,$(toolchain))))
