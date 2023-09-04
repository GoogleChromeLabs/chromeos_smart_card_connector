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

TARGET := google_smart_card_pcsc_lite_server_clients_management

include ../../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include $(COMMON_DIR_PATH)/make/js_building_common.mk

include $(COMMON_DIR_PATH)/cpp/include.mk

include $(COMMON_DIR_PATH)/js/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk
include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/server/include.mk

include ../include.mk


SOURCES_PATH := ../src


SOURCES := \
        $(SOURCES_PATH)/admin_policy_getter.cc \
	$(SOURCES_PATH)/client_request_processor.cc \
	$(SOURCES_PATH)/clients_manager.cc \
	$(SOURCES_PATH)/client_handles_registry.cc \
	$(SOURCES_PATH)/google_smart_card_pcsc_lite_server_clients_management/backend.cc \
	$(SOURCES_PATH)/google_smart_card_pcsc_lite_server_clients_management/ready_message.cc \

CXXFLAGS := \
	-I$(ROOT_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \
	-std=$(CXX_DIALECT) \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CXXFLAGS))))


$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))


$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/known_client_apps.json,pcsc_lite_server_clients_management))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/permissions_checking/user-prompt-dialog.html,pcsc_lite_server_clients_management))
$(eval $(call COPY_TO_OUT_DIR_RULE,$(SOURCES_PATH)/permissions_checking/user-prompt-dialog.css,pcsc_lite_server_clients_management))


JS_COMPILER_INPUT_PATHS := \
	$(JS_COMMON_JS_COMPILER_INPUT_DIR_PATHS) \
	$(PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_JS_COMPILER_INPUT_DIR_PATHS) \

JS_COMPILER_FLAGS := \
	--define='GoogleSmartCard.Logging.SELF_RELOAD_ON_FATAL_ERROR=true' \

$(eval $(call BUILD_JS_SCRIPT,pcsc_lite_server_clients_management/user-prompt-dialog.js,$(JS_COMPILER_INPUT_PATHS),GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptDialog.Main,$(JS_COMPILER_FLAGS)))
