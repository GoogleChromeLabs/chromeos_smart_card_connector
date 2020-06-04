#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SCRIPT_DIR=$(dirname "${BASH_SOURCE}")
CONFIGURE_SCRIPT=$1
shift 1
set -e

"${SCRIPT_DIR}/patch_configure.py" "${CONFIGURE_SCRIPT}"

if [ -z "${NACL_CROSS_PREFIX}" ]; then
  NACL_ENV_IMPORT=1
  . "${SCRIPT_DIR}/nacl-env.sh"
fi

# TODO(sbc): this code is currently duplicated in common.sh.
PatchConfigSub() {
  # Replace the package's config.sub one with an up-do-date copy
  # that includes nacl support.  We only do this if the string
  # 'nacl)' is not already contained in the file.
  local DEFAULT_CONFIG_SUB="$(dirname ${CONFIGURE_SCRIPT})/config.sub"
  local CONFIG_SUB=${CONFIG_SUB:-${DEFAULT_CONFIG_SUB}}
  if [ ! -f ${CONFIG_SUB} ]; then
    if [ -n "${CONFIG_SUB_MISSING}" ]; then
      return
    fi
    echo "Failed to find config.sub (${CONFIG_SUB})."
    echo "Please specify using \$CONFIG_SUB."
    exit 1
  fi
  if grep -q 'nacl)' ${CONFIG_SUB} /dev/null; then
    echo "${CONFIG_SUB} supports NaCl"
  else
    echo "Patching config.sub"
    /bin/cp -f ${SCRIPT_DIR}/config.sub ${CONFIG_SUB}
  fi
}

PatchConfigSub

CONF_HOST=${NACL_CROSS_PREFIX}
if [ "${NACL_ARCH}" = "pnacl" -o "${NACL_ARCH}" = "emscripten" ]; then
  # The PNaCl tools use "pnacl-" as the prefix, but config.sub
  # does not know about "pnacl".  It only knows about "le32-nacl".
  # Unfortunately, most of the config.subs here are so old that
  # it doesn't know about that "le32" either.  So we just say "nacl".
  CONF_HOST="nacl"
fi

NaClEnvExport
echo "${CONFIGURE_SCRIPT}" --host=${CONF_HOST} --prefix=${NACL_PREFIX} "$@"
exec "${CONFIGURE_SCRIPT}" --host=${CONF_HOST} --prefix=${NACL_PREFIX} "$@"
