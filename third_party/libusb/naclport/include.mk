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
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA


#
# This file contains helper definitions for using this library.
#
# /common/make/common.mk and /common/make/executable_building.mk must be
# included before including this file.
#


LIBUSB_LIB := google_smart_card_libusb


LIBUSB_DIR_PATH := $(THIRD_PARTY_DIR_PATH)/libusb


LIBUSB_JS_COMPILER_INPUT_DIR_PATHS := $(LIBUSB_DIR_PATH)/naclport/src


LIBUSB_INSTALLED_HEADER_DIR_NAME := google_smart_card_libusb/libusb-1.0

LIBUSB_INCLUDE_PATH := \
	$(NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH)/$(LIBUSB_INSTALLED_HEADER_DIR_NAME)


#
# Helper target that installs the library headers.
#
# This target can be used in dependant libraries.
#

$(eval $(call DEFINE_NACL_LIBRARY_HEADERS_INSTALLATION_TARGET,LIBUSB_HEADERS_INSTALLATION_STAMP_FILE,$(LIBUSB_DIR_PATH)/naclport/build))
