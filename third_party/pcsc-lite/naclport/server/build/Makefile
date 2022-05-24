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

# This makefile builds the port of server side of PC/SC-Lite under the Native
# Client (for the details, see the README file).
#
# Upon successful build, the resulting static library is installed into the NaCl
# SDK libraries directory, and the public headers are installed into the NaCl
# SDK common headers directory.

TARGET := google_smart_card_pcsc_lite_server

include ../../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include $(COMMON_DIR_PATH)/cpp/include.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/webport/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/naclport/common/include.mk

include ../include.mk


SOURCES_PATH := ../src

PCSC_LITE_SOURCES_PATH := $(PCSC_LITE_DIR_PATH)/src/src


# Rules for invoking the source files compilation into object files

# * FAKE_PCSC_NACL_SOCKET_FILE_NAME constant points to the file containing the
#   fake socket file (it's used for tricking the original PC/SC-Lite code which
#   calls stat() for this file - for the details see file
#   src/winscard_msg_nacl.cc).
COMMON_CPPFLAGS := \
	-DFAKE_PCSC_NACL_SOCKET_FILE_NAME='"executable-module-filesystem/pcsc/fake_socket_file"' \
	-I$(SOURCES_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \
	-I$(PCSC_LITE_SOURCES_PATH) \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/common/cpp/src \

# * DEBUG_HOTPLUG definition enables debug logging for the reader drivers
#   loading code in the original PC/SC-Lite code;
# * HAVE_LIBUSB enables reader hot plug implementation based on the libusb
#   library (its NaCl port is located under the ../../../../libusb directory);
# * HAVE_SYSLOG_H enables emitting log messages through syslog (which is faked
#   to use the common logging primitives from the /common/cpp library);
# * PCSC_ARCH constant tells to use the code corresponding to Linux platform
#   (actually, in the current NaCl port no functionality is influenced by this
#   constant);
# * PCSCLITE_HP_DROPDIR constant points to the directory containing the configs
#   for all PC/SC-Lite server drivers; use a relative path, so that it works
#   both inside the Smart Card Connector app and in unit tests;
# * USE_IPCDIR constant is just a stub for making the original PC/SC-Lite code
#   compiling;
# * USE_USB definition enables the PC/SC-Lite support of USB readers (as
#   opposed to the other alternative - serial bus devices)
PCSC_LITE_COMMON_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-DDEBUG_HOTPLUG \
	-DHAVE_LIBUSB \
	-DHAVE_SYSLOG_H \
	-DPCSC_ARCH='"Linux"' \
	-DPCSCLITE_HP_DROPDIR='"executable-module-filesystem/pcsc/drivers"' \
	-DUSE_IPCDIR='"IPCDIR_is_not_expected_to_be_used"' \
	-DUSE_USB \
	-I$(ROOT_PATH)/common/cpp/src/google_smart_card_common/logging/syslog \
	-I$(ROOT_PATH)/third_party/libusb/src/libusb \

# Part of the list of the original PC/SC-Lite daemon source files to be
# compiled (see also PCSC_LITE_SERVER_SVC_SOURCES,
# PCSC_LITE_SERVER_DEBUGLOG_SOURCES, PCSC_LITE_SERVER_READERFACTORY_SOURCES
# variables).
PCSC_LITE_SERVER_SOURCES := \
	$(PCSC_LITE_SOURCES_PATH)/atrhandler.c \
	$(PCSC_LITE_SOURCES_PATH)/error.c \
	$(PCSC_LITE_SOURCES_PATH)/eventhandler.c \
	$(PCSC_LITE_SOURCES_PATH)/ifdwrapper.c \
	$(PCSC_LITE_SOURCES_PATH)/prothandler.c \
	$(PCSC_LITE_SOURCES_PATH)/utils.c \
	$(PCSC_LITE_SOURCES_PATH)/winscard.c \

# * PCSCD definition is required to enable compilation of the code that is
#   required for compiling the original PC/SC-Lite daemon source files.
# * SCard* functions are redefined in order to not collide with the symbols
#   from the PC/SC-Lite server-side libraries
PCSC_LITE_SERVER_CPPFLAGS := \
	$(PCSC_LITE_COMMON_CPPFLAGS) \
	-DPCSCD \
	-DSCardBeginTransaction=SCardBeginTransactionServer \
	-DSCardConnect=SCardConnectServer \
	-DSCardControl=SCardControlServer \
	-DSCardDisconnect=SCardDisconnectServer \
	-DSCardEndTransaction=SCardEndTransactionServer \
	-DSCardEstablishContext=SCardEstablishContextServer \
	-DSCardGetAttrib=SCardGetAttribServer \
	-DSCardReconnect=SCardReconnectServer \
	-DSCardReleaseContext=SCardReleaseContextServer \
	-DSCardSetAttrib=SCardSetAttribServer \
	-DSCardStatus=SCardStatusServer \
	-DSCardTransmit=SCardTransmitServer \

$(foreach src,$(PCSC_LITE_SERVER_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_CPPFLAGS))))

# Part of the list of the original PC/SC-Lite daemon source files to be
# compiled: some files require special additional compilation flags (see also
# the PCSC_LITE_SERVER_SOURCES variable)
PCSC_LITE_SERVER_SVC_SOURCES := \
	$(PCSC_LITE_SOURCES_PATH)/winscard_svc.c \

# * close redefinition makes the original PC/SC-Lite code use the function from
#   the emulated socketpair library (because the emulated socketpair library
#   generates fake file descriptors, and calling the real close function with
#   them may cause bad effects);
# * no-return-type suppresses spurious compiler errors in cases when it doesn't
#   recognize calls to library functions that terminate the thread or the
#   program and bypass returning from a non-void function.
PCSC_LITE_SERVER_SVC_CPPFLAGS := \
	$(PCSC_LITE_SERVER_CPPFLAGS) \
	-Dclose=ServerCloseSession \
	-Wno-return-type \

$(foreach src,$(PCSC_LITE_SERVER_SVC_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_SVC_CPPFLAGS))))

# Special group just for debuglog.c.
PCSC_LITE_SERVER_DEBUGLOG_SOURCES := \
	$(PCSC_LITE_SOURCES_PATH)/debuglog.c \

# * suppress spurious compiler errors due to usage of non-portable in printf
#   formats.
PCSC_LITE_SERVER_DEBUGLOG_CPPFLAGS := \
	$(PCSC_LITE_SERVER_CPPFLAGS) \
	-Wno-format \

$(foreach src,$(PCSC_LITE_SERVER_DEBUGLOG_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_DEBUGLOG_CPPFLAGS))))

# Special group just for readerfactory.c
PCSC_LITE_SERVER_READERFACTORY_SOURCES := \
	$(PCSC_LITE_SOURCES_PATH)/readerfactory.c \

# Calls to RFAddReader and RFRemoveReader (readerfactory.c) are hooked to get
# better information on reader status.
PCSC_LITE_SERVER_READERFACTORY_CPPFLAGS := \
	$(PCSC_LITE_SERVER_CPPFLAGS) \
	-DRFAddReader=RFAddReaderOriginal \
	-DRFRemoveReader=RFRemoveReaderOriginal \

$(foreach src,$(PCSC_LITE_SERVER_READERFACTORY_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_READERFACTORY_CPPFLAGS))))

# Special group just for hotplug_libusb.c.
PCSC_LITE_SERVER_HOTPLUG_LIBUSB_SOURCES := \
	$(PCSC_LITE_SOURCES_PATH)/hotplug_libusb.c \

# * mock out the pipe(), read(), write() and close() standard library functions
#   with our fake implementations, since the standard pipe() has poor semantics
#   under Emscripten (it always creates a nonblocking pipe, even when there's no
#   O_NONBLOCK flag specified); read() and close() need to be mocked out, since
#   our fake pipe() implementation returns fake file descriptors (based on
#   simple counters).
PCSC_LITE_SERVER_HOTPLUG_LIBUSB_CPPFLAGS := \
	$(PCSC_LITE_SERVER_CPPFLAGS) \
	-Dclose=GoogleSmartCardIpcEmulationClose \
	-Dpipe=GoogleSmartCardIpcEmulationPipe \
	-Dread=GoogleSmartCardIpcEmulationRead \
	-Dwrite=GoogleSmartCardIpcEmulationWrite \

$(foreach src,$(PCSC_LITE_SERVER_HOTPLUG_LIBUSB_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_HOTPLUG_LIBUSB_CPPFLAGS))))

# List of the original PC/SC-Lite client library source files to be compiled
PCSC_LITE_CLIENT_SOURCES := \
	$(PCSC_LITE_SOURCES_PATH)/winscard_clnt.c \

PCSC_LITE_SERVER_CLIENT_CPPFLAGS := \
	$(PCSC_LITE_COMMON_CPPFLAGS) \

$(foreach src,$(PCSC_LITE_CLIENT_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_CLIENT_CPPFLAGS))))

# List of additional source files to be compiled:
# * Files named as *_nacl* provide replacement for some parts of the original
#   PC/SC-Lite implementation;
# * Other files provide helper libraries and the implementation of the public
#   interface of the library
PCSC_LITE_SERVER_NACL_SOURCES := \
	$(SOURCES_PATH)/auth_nacl.cc \
	$(SOURCES_PATH)/cmath.cc \
	$(SOURCES_PATH)/dyn_nacl.cc \
	$(SOURCES_PATH)/pcscdaemon_nacl.cc \
	$(SOURCES_PATH)/public/pcsc_lite_server_web_port_service.cc \
	$(SOURCES_PATH)/readerfactory_nacl.cc \
	$(SOURCES_PATH)/server_sockets_manager.cc \
	$(SOURCES_PATH)/sys_nacl.cc \
	$(SOURCES_PATH)/unistd_nacl.cc \
	$(SOURCES_PATH)/winscard_msg_nacl.cc \

PCSC_LITE_SERVER_NACL_CXXFLAGS := \
	$(COMMON_CPPFLAGS) \
	-std=$(CXX_DIALECT) \

$(foreach src,$(PCSC_LITE_SERVER_NACL_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(PCSC_LITE_SERVER_NACL_CXXFLAGS))))


# Variable containing the list of all source files whose compiled object files
# will have to be linked together

SOURCES := \
	$(PCSC_LITE_SERVER_SOURCES) \
	$(PCSC_LITE_SERVER_SVC_SOURCES) \
	$(PCSC_LITE_SERVER_DEBUGLOG_SOURCES) \
	$(PCSC_LITE_SERVER_HOTPLUG_LIBUSB_SOURCES) \
	$(PCSC_LITE_CLIENT_SOURCES) \
	$(PCSC_LITE_SERVER_NACL_SOURCES) \
	$(PCSC_LITE_SERVER_READERFACTORY_SOURCES) \


# Rules for linking of the compiled object files and installing the resulting
# static library into the NaCl SDK libraries directory

$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
