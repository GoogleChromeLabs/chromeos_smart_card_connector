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


TARGET := google_smart_card_pcsc_lite_common

include ../../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include ../include.mk


SOURCES_PATH := ../src

SOURCES := \
	$(SOURCES_PATH)/constants.cc \
	$(SOURCES_PATH)/public/pcsc_lite_tracing_wrapper.cc \
	$(SOURCES_PATH)/public/scard_debug_dump.cc \
	$(SOURCES_PATH)/public/scard_structs_serialization.cc \

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-zero-length-array \
	-std=$(CXX_DIALECT) \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))

$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
