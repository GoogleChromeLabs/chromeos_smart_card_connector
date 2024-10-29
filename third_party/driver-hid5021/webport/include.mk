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

# This file contains common makefile definitions related to the HID 5021 driver.

# File name of the driver's bundle directory.
#
# It could be an arbitrary string unique to this driver, but we just follow what
# the driver uses on other platforms.
DRIVER5021_BUNDLE_NAME := driver-5021CL.bundle

# Path where the driver's config file is to be installed.
DRIVER5021_CONFIG_INSTALLATION_PATH := \
  $(PCSC_LITE_DRIVER_INSTALLATION_PATH)/$(DRIVER5021_BUNDLE_NAME)/Contents/Info.plist
