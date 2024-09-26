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

TARGET := google_smart_card_driver5021cl

include ../../../common/make/common.mk
include $(ROOT_PATH)/common/make/executable_building.mk

SOURCES := \
	../src/ifdhandler.c \

CPPFLAGS := \
	-I$(ROOT_PATH)/third_party/libusb/src/libusb \
	-I$(ROOT_PATH)/third_party/pcsc-lite/src/src/PCSC \

$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))
$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
