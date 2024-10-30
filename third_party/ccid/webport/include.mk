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


#
# This file contains helper definitions for using this library.
#
# /common/make/common.mk, /common/make/executable_building.mk and
# /third_party/pcsc-lite/webport/common/include.mk must be included before
# including this file.
#


CCID_VERSION := 1.5.5


CCID_DIR_PATH := $(THIRD_PARTY_DIR_PATH)/ccid


# File name of the CCID driver's bundle directory.
#
# It could be an arbitrary string, but we just follow what CCID uses on other
# platforms.
CCID_BUNDLE_NAME := ifd-ccid.bundle

# File name of the CCID driver's .so file.
#
# This value must match the one specified in the "CFBundleExecutable" parameter
# of the Info.plist config.
#
# Note that we don't really create this file in our webport, since all drivers
# are linked statically, however it's still used by the code to distinguish
# between calls made to different drivers.
CCID_SO_NAME := libccid.so

# Path where the CCID driver's config file is to be installed.
CCID_CONFIG_INSTALLATION_PATH := \
	$(PCSC_LITE_DRIVER_INSTALLATION_PATH)/$(CCID_BUNDLE_NAME)/Contents/Info.plist

# Path where the CCID driver's .so file is expected to be installed.
#
# Note that the .so file doesn't really exist in our webport, since all drivers
# are linked statically, however it's still used by the code to distinguish
# between calls made to different drivers.
CCID_SO_INSTALLATION_PATH := \
	$(PCSC_LITE_DRIVER_INSTALLATION_PATH)/$(CCID_BUNDLE_NAME)/Contents/$(PCSC_LITE_ARCHITECTURE)/$(CCID_SO_NAME)


#
# Location of the config file containing the list of readers supported by CCID.
#

CCID_SUPPORTED_READERS_CONFIG_PATH := \
	$(CCID_DIR_PATH)/src/readers/supported_readers.txt
