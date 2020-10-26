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
# Including of this file creates a NaCl module building dependency on this
# library.
#
# /common/make/common.mk, /common/make/executable_building.mk and include.mk
# must be included before including this file.
#


$(eval $(call DEPEND_RULE,$(CCID_LIB),$(CCID_DIR_PATH)/naclport/build))
