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


TARGET := google_smart_card_libusb

include ../../../../common/make/common.mk

include $(COMMON_DIR_PATH)/make/executable_building.mk

include $(COMMON_DIR_PATH)/cpp/include.mk

include ../include.mk


LIBUSB_NACL_SOURCES_PATH := ../src

LIBUSB_SOURCES_PATH := ../../src


C_SOURCES := \
	$(LIBUSB_SOURCES_PATH)/libusb/strerror.c \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_error_name.c \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_get_string_descriptor_ascii.c \

CXX_SOURCES := \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_contexts_storage.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_js_proxy.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_js_proxy_constants.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_js_proxy_data_model.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_opaque_types.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_tracing_wrapper.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/public/libusb_web_port_service.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/usb_transfer_destination.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/usb_transfers_parameters_storage.cc \

SOURCES := \
	$(C_SOURCES) \
	$(CXX_SOURCES) \

CPPFLAGS := \
	-I$(ROOT_PATH) \
	-I$(LIBUSB_NACL_SOURCES_PATH) \
	-I$(LIBUSB_SOURCES_PATH)/libusb \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-sign-compare \

CXXFLAGS := \
	-std=$(CXX_DIALECT) \

$(foreach src,$(C_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))

$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS) $(CXXFLAGS))))


$(eval $(call LIB_RULE,$(TARGET),$(SOURCES)))
