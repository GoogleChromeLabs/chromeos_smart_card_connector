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

include $(COMMON_DIR_PATH)/make/nacl_module_building_common.mk

include $(COMMON_DIR_PATH)/cpp/include.mk

include ../include.mk


LIBUSB_NACL_SOURCES_PATH := ../src

LIBUSB_SOURCES_PATH := ../../src


C_SOURCES := \
	$(LIBUSB_SOURCES_PATH)/libusb/strerror.c \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_error_name.c \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_get_string_descriptor_ascii.c \

CXX_SOURCES := \
	$(LIBUSB_NACL_SOURCES_PATH)/chrome_usb/api_bridge.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/chrome_usb/types.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/google_smart_card_libusb/global.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_contexts_storage.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_opaque_types.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_over_chrome_usb.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/libusb_tracing_wrapper.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/usb_transfer_destination.cc \
	$(LIBUSB_NACL_SOURCES_PATH)/usb_transfers_parameters_storage.cc \

SOURCES := \
	$(C_SOURCES) \
	$(CXX_SOURCES) \

CPPFLAGS := \
	-I$(LIBUSB_NACL_SOURCES_PATH) \
	-I$(LIBUSB_SOURCES_PATH)/libusb \
	-Wextra \
	-Wno-sign-compare \

CXXFLAGS := \
	-std=gnu++11 \

$(foreach src,$(C_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS))))

$(foreach src,$(CXX_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CPPFLAGS) $(CXXFLAGS))))

$(foreach src,$(CXX_SOURCES),$(eval $(call DEPEND_COMPILE_ON_NACL_LIBRARY_HEADERS,$(src),$(CPP_COMMON_HEADERS_INSTALLATION_STAMP_FILE))))


$(eval $(call NACL_LIBRARY_BUILD_RULE,$(SOURCES)))


INSTALLING_HEADERS := \
	$(LIBUSB_INSTALLED_HEADER_DIR_NAME):$(LIBUSB_SOURCES_PATH)/libusb:libusb.h \
	google_smart_card_libusb:$(LIBUSB_NACL_SOURCES_PATH)/google_smart_card_libusb:global.h \

$(eval $(call NACL_LIBRARY_HEADERS_INSTALLATION_RULE,$(INSTALLING_HEADERS)))


test::
	+$(MAKE) --directory tests run

tests_clean::
	+$(MAKE) --directory tests clean

clean: tests_clean
