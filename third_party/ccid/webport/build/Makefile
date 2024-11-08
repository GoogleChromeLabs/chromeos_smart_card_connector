# Copyright 2016 Google Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# This makefile builds the port of CCID library under the Emscripten.
#
# Upon successful build, the resulting static library is installed into the
# Emscripten's libraries subdirectory.

TARGET := google_smart_card_ccid

include ../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include $(THIRD_PARTY_DIR_PATH)/libusb/webport/include.mk

include $(THIRD_PARTY_DIR_PATH)/pcsc-lite/webport/common/include.mk

include ../include.mk


CCID_SOURCES_PATH := ../../src/src

CCID_WEBPORT_SOURCES_PATH := ../src


# Rules for invoking the source files compilation into object files

# * rename all IFDH...() functions to avoid symbol clashes with other drivers;
#   TODO: objcopy --redefine-sym would be more elegant, but is blocked on
#   https://github.com/llvm/llvm-project/issues/50623 .
# * HAVE_PTHREAD definition enables the support of working with multiple readers
#   simultaneously;
# * log_msg and log_xxd are redefined in order to not collide with the symbols
#   from the PC/SC-Lite server-side libraries;
# * PCSCLITE_HP_DROPDIR constant points to the directory containing the configs
#   for all PC/SC-Lite server drivers;
COMMON_CPPFLAGS := \
	-DIFDHCloseChannel=CCID_IFDHCloseChannel \
	-DIFDHControl=CCID_IFDHControl \
	-DIFDHCreateChannel=CCID_IFDHCreateChannel \
	-DIFDHCreateChannelByName=CCID_IFDHCreateChannelByName \
	-DIFDHGetCapabilities=CCID_IFDHGetCapabilities \
	-DIFDHICCPresence=CCID_IFDHICCPresence \
	-DIFDHPowerICC=CCID_IFDHPowerICC \
	-DIFDHSetCapabilities=CCID_IFDHSetCapabilities \
	-DIFDHSetProtocolParameters=CCID_IFDHSetProtocolParameters \
	-DIFDHTransmitToICC=CCID_IFDHTransmitToICC \
	-DHAVE_PTHREAD=1 \
	-Dlog_msg=ccid_log_msg \
	-Dlog_xxd=ccid_log_xxd \
	-DPCSCLITE_HP_DROPDIR='"$(PCSC_LITE_DRIVER_INSTALLATION_PATH)"' \
	-Wall \

CCID_SOURCES := \
	$(CCID_SOURCES_PATH)/ccid.c \
	$(CCID_SOURCES_PATH)/ccid_usb.c \
	$(CCID_SOURCES_PATH)/commands.c \
	$(CCID_SOURCES_PATH)/debug.c \
	$(CCID_SOURCES_PATH)/ifdhandler.c \
	$(CCID_SOURCES_PATH)/strlcpy.c \
	$(CCID_SOURCES_PATH)/utils.c \

# * BUNDLE constant contains the name of subdirectory under the PC/SC-Lite
#   server driver configs directory (see also the PCSCLITE_HP_DROPDIR constant);
# * USE_SYSLOG enables using syslog functions for logging, which in this web
#   port are redefined in order to log both to stderr and to JavaScript console
#   (see the /common/cpp/ library);
# * VERSION constant contains the CCID version;
# * __linux definition allows to use some header files that are usually provided
#   under Linux (but are actually provided by the Emscripten SDK too);
# * The "macro-redefined" warning diagnostic is disabled because of some
#   non-clean code;
CCID_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-DBUNDLE='"$(CCID_BUNDLE_NAME)"' \
	-DUSE_SYSLOG \
	-DVERSION='"$(CCID_VERSION)"' \
	-D__linux \
	-I$(CCID_WEBPORT_SOURCES_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \
	-I$(ROOT_PATH)/common/cpp/src/public/logging/syslog \
	-I$(ROOT_PATH)/third_party/libusb/src/libusb \
	-Wno-macro-redefined \

$(foreach src,$(CCID_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CCID_CPPFLAGS))))

CCID_OPENCT_SOURCES := \
	$(CCID_SOURCES_PATH)/openct/buffer.c \
	$(CCID_SOURCES_PATH)/openct/checksum.c \
	$(CCID_SOURCES_PATH)/openct/proto-t1.c \

CCID_OPENCT_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-I$(CCID_WEBPORT_SOURCES_PATH) \
	-I$(CCID_SOURCES_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \

$(foreach src,$(CCID_OPENCT_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CCID_OPENCT_CPPFLAGS))))

CCID_TOWITOKO_SOURCES := \
	$(CCID_SOURCES_PATH)/towitoko/atr.c \
	$(CCID_SOURCES_PATH)/towitoko/pps.c \

CCID_TOWITOKO_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-I$(CCID_WEBPORT_SOURCES_PATH) \
	-I$(CCID_SOURCES_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \
	-I$(ROOT_PATH)/third_party/libusb/src/libusb \

$(foreach src,$(CCID_TOWITOKO_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CCID_TOWITOKO_CPPFLAGS))))

# Rules for compiling webport helper C++ sources:

CCID_WEBPORT_SOURCES := \
	$(CCID_WEBPORT_SOURCES_PATH)/ccid_pcsc_driver_adaptor.cc \

# * Pass the "CCID_SO_INSTALLATION_PATH" constant to the C++ code.
# * Specify include search paths.
CCID_WEBPORT_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-DCCID_SO_INSTALLATION_PATH='"$(CCID_SO_INSTALLATION_PATH)"' \
	-I$(ROOT_PATH) \
	-I$(CCID_SOURCES_PATH) \
	-I$(PCSC_LITE_ORIGINAL_HEADERS_DIR_PATH) \

$(foreach src,$(CCID_WEBPORT_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CCID_WEBPORT_CPPFLAGS))))


# Variable containing the list of all source files whose compiled object files
# will have to be linked together

SOURCES := \
	$(CCID_SOURCES) \
	$(CCID_OPENCT_SOURCES) \
	$(CCID_TOWITOKO_SOURCES) \
	$(CCID_WEBPORT_SOURCES) \


# Rules for building the driver config file and putting it into the resulting
# out directory under the PC/SC-Lite server driver configs directory

Info.plist: $(CCID_SOURCES_PATH)/create_Info_plist.pl $(CCID_SUPPORTED_READERS_CONFIG_PATH) $(CCID_SOURCES_PATH)/Info.plist.src
	@rm -f Info.plist
	$(CCID_SOURCES_PATH)/create_Info_plist.pl \
		$(CCID_SUPPORTED_READERS_CONFIG_PATH) \
		$(CCID_SOURCES_PATH)/Info.plist.src \
		--target=$(CCID_SO_NAME) \
		--version=$(CCID_VERSION) > Info.plist.build
	@mv Info.plist.build Info.plist

all: Info.plist
$(eval $(call CLEAN_RULE,Info.plist.build))
$(eval $(call CLEAN_RULE,Info.plist))


# Rules for linking of the compiled object files and installing the resulting
# static library into the shared libraries directory.

$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
