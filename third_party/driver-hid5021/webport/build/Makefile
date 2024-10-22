# Copyright 2024 Google Inc.
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

TARGET := google_smart_card_driver_hid5021

include ../../../../common/make/common.mk
include $(ROOT_PATH)/common/make/executable_building.mk

# C/C++ compiler flags used for all source files below:
#
# * rename all IFDH...() functions to avoid symbol clashes with other drivers.
COMMON_CPPFLAGS := \
	-DIFDHCloseChannel=HID5021_IFDHCloseChannel \
	-DIFDHControl=HID5021_IFDHControl \
	-DIFDHCreateChannel=HID5021_IFDHCreateChannel \
	-DIFDHGetCapabilities=HID5021_IFDHGetCapabilities \
	-DIFDHICCPresence=HID5021_IFDHICCPresence \
	-DIFDHPowerICC=HID5021_IFDHPowerICC \
	-DIFDHSetCapabilities=HID5021_IFDHSetCapabilities \
	-DIFDHSetProtocolParameters=HID5021_IFDHSetProtocolParameters \
	-DIFDHTransmitToICC=HID5021_IFDHTransmitToICC \

# Rules for compiling the driver's C sources:

DRIVER_SOURCES := \
	../../src/ifdhandler.c \

DRIVER_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-I$(ROOT_PATH)/third_party/libusb/src/libusb \
	-I$(ROOT_PATH)/third_party/pcsc-lite/src/src/PCSC \

$(foreach src,$(DRIVER_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(DRIVER_CPPFLAGS))))

# Rules for compiling the webport helper sources:

WEBPORT_SOURCES := \
	../src/hid5021_pcsc_driver_adaptor.cc \

WEBPORT_CPPFLAGS := \
	$(COMMON_CPPFLAGS) \
	-I$(ROOT_PATH) \
	-I$(ROOT_PATH)/third_party/pcsc-lite/src/src/PCSC \

$(foreach src,$(WEBPORT_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(WEBPORT_CPPFLAGS))))

# Rules for producing the resulting static library:

SOURCES := \
	$(DRIVER_SOURCES) \
	$(WEBPORT_SOURCES) \

$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
