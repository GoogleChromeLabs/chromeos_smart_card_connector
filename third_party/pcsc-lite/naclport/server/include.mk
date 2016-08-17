# Copyright 2016 Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#
# This file contains helper definitions for using this library.
#
# /common/make/common.mk and /common/make/nacl_module_building_common.mk must be
# included before including this file.
#


PCSC_LITE_SERVER_LIB := google_smart_card_pcsc_lite_server


PCSC_LITE_SERVER_DIR_PATH := $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server


PCSC_LITE_SERVER_INSTALLED_HEADERS_DIR_NAME := \
	google_smart_card_pcsc_lite_server

PCSC_LITE_SERVER_INCLUDE_PATH := \
	$(NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH)/$(PCSC_LITE_SERVER_INSTALLED_HEADERS_DIR_NAME)/PCSC \


PCSC_LITE_SERVER_JS_COMPILER_INPUT_DIR_PATHS := \
	$(PCSC_LITE_SERVER_DIR_PATH)/src


$(eval $(call DEFINE_OUT_GENERATION,PCSC_LITE_SERVER_OUT,$(PCSC_LITE_SERVER_DIR_PATH)/build,$(PCSC_LITE_SERVER_LIB)))


#
# Helper target that installs the library headers.
#
# This target can be used in dependant libraries.
#

$(eval $(call DEFINE_NACL_LIBRARY_HEADERS_INSTALLATION_TARGET,PCSC_LITE_SERVER_HEADERS_INSTALLATION_STAMP_FILE,$(PCSC_LITE_SERVER_DIR_PATH)/build))
