# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


#
# This file contains helper definitions for using this library.
#
# /common/make/common.mk and /common/make/nacl_module_building_common.mk must be
# included before including this file.
#


CPP_COMMON_LIB := google_smart_card_common


CPP_COMMON_DIR_PATH := $(COMMON_DIR_PATH)/cpp


COMMON_SYSLOG_INCLUDE_PATH := \
	$(NACL_LIBRARY_HEADERS_INSTALLATION_DIR_PATH)/google_smart_card_common/logging/syslog



#
# Helper target that installs the library headers.
#
# This target can be used in dependant libraries.
#

$(eval $(call DEFINE_NACL_LIBRARY_HEADERS_INSTALLATION_TARGET,CPP_COMMON_HEADERS_INSTALLATION_STAMP_FILE,$(CPP_COMMON_DIR_PATH)/build))
