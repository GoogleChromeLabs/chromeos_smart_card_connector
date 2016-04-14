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
# This file contains some helper definitions for building Chrome Apps.
#
# common.mk must be included before including this file.
#


#
# Special "run" target that starts a Chrome instance with a temporary profile
# and an App loaded from the out directory and running.
#
# In order to use a different Chrome browser than the system default, the
# CHROME_PATH variable may be defined externally.
#
# Additional environment variables that need to be passed to Chrome (e.g.
# "LANGUAGE" variable for changing browser's locale) can be passed through the
# CHROME_ENV variable.
#
# Additional command line flags that need to be passed to Chrome can be
# specified through the CHROME_ARGS variable.
#

ifeq ($(shell uname -s | cut -c 1-5),Linux)
DEFAULT_CHROME_PATH := /usr/bin/google-chrome
endif
ifeq ($(shell uname),Darwin)
DEFAULT_CHROME_PATH := \
	/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome
endif
ifeq ($(shell uname -s | cut -c 1-10),MINGW32_NT)
DEFAULT_CHROME_PATH := \
	"c:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"
endif
DEFAULT_CHROME_PATH ?= failed_to_detect_DEFAULT_CHROME_PATH

USER_DATA_DIR_PATH := ./user-data-dir

CHROME_PATH ?= $(DEFAULT_CHROME_PATH)

CHROME_ENV ?=

CHROME_ARGS += \
	--enable-nacl \
	--enable-pnacl \
	--no-first-run \
	--user-data-dir=$(USER_DATA_DIR_PATH) \
	--load-and-launch-app=$(OUT_DIR_PATH) \

.PHONY: run

run: all
	$(CHROME_ENV) $(CHROME_PATH) $(CHROME_ARGS)

$(eval $(call CLEAN_RULE,$(USER_DATA_DIR_PATH)))


#
# Special "package" target that creates a packaged App .CRX file and a .ZIP
# archive suitable for uploading at Chrome WebStore (see
# <https://developer.chrome.com/webstore/publish>).
#
# The ID of the generated App packaged in the .CRX file is controlled by the
# private key .PEM file. The PEM file, if missing, is automatically generated
# with some random contents - so, in order to keep ID the same, the PEM file
# should be preserved.
#
# The .ZIP archive intended for WebStore, in contrast, has no signature, as
# WebStore stores the private key internally and signs the App package itself.
#

.PHONY: package

$(TARGET).pem:
	@rm -f $(TARGET).pem
	openssl genrsa -out "$(TARGET).pem" 2048

$(TARGET).crx: all $(TARGET).pem
	@rm -f $(TARGET).crx
	$(THIRD_PARTY_DIR_PATH)/crxmake/src/crxmake.sh "$(OUT_DIR_PATH)" "$(TARGET).pem"

$(TARGET)__webstore.zip: all
	@rm -f "$(TARGET)__webstore.zip"
	cd "$(OUT_DIR_PATH)" && zip -qr -9 -X "$(CURDIR)/$(TARGET)__webstore.zip" .

package: $(TARGET).crx $(TARGET)__webstore.zip

$(eval $(call CLEAN_RULE,$(TARGET).crx))
$(eval $(call CLEAN_RULE,$(TARGET)__webstore.zip))
