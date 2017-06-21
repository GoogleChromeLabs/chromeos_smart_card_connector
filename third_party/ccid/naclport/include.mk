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
# /common/make/common.mk and /common/make/nacl_module_building_common.mk must be
# included before including this file.
#


CCID_VERSION := 1.4.27


CCID_LIB := google_smart_card_ccid


CCID_DIR_PATH := $(THIRD_PARTY_DIR_PATH)/ccid


#
# Location of the config file containing the list of readers supported by CCID.
#

CCID_SUPPORTED_READERS_CONFIG_PATH := \
	$(CCID_DIR_PATH)/src/readers/supported_readers.txt


#
# Helper target that generates the library's out directory.
#

$(eval $(call DEFINE_OUT_GENERATION,CCID_OUT,$(CCID_DIR_PATH)/naclport/build,$(CCID_LIB)))
