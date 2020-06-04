# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS="--disable-bsdtar --disable-bsdcpio"
EXTRA_CONFIGURE_ARGS+=" --without-iconv"

NACLPORTS_CPPFLAGS+=" -Dtimezone=_timezone"

# Because /usr/bin/xml2-config works different on Linux and Mac we need
# to treat them differently. On Mac the build works correctly with no
# changes, but Linux requires --prefix=<path_to_xml2>, which is an invalid
# flag on Mac.
if [ $OS_NAME = "Linux" ]; then
  export XML2_CONFIG_PREFIX="--prefix=\"${NACL_PREFIX}\""
fi

# Necessary for libxml2.
export LIBS="-lpthread -lm"
