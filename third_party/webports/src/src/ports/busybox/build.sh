# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BusyBoxDisable() {
  # Switch one option in the busybox config from yes to no.
  sed -i.bak "s/$*=y/$*=n/" .config
}

ConfigureStep() {
  SetupCrossEnvironment

  LogExecute make -f ${SRC_DIR}/Makefile defconfig BUILD_LIBBUSYBOX=y \
    KBUILD_SRC=${SRC_DIR}

  BusyBoxDisable CONFIG_ACPID
  BusyBoxDisable CONFIG_ARP
  BusyBoxDisable CONFIG_ARPING
  BusyBoxDisable CONFIG_BLOCKDEV
  BusyBoxDisable CONFIG_BRCTL
  BusyBoxDisable CONFIG_EJECT
  BusyBoxDisable CONFIG_ETHER_WAKE
  BusyBoxDisable CONFIG_FBSPLASH
  BusyBoxDisable CONFIG_FDFLUSH
  BusyBoxDisable CONFIG_FDISK
  BusyBoxDisable CONFIG_FEATURE_IFUPDOWN_IP_BUILTIN
  BusyBoxDisable CONFIG_FEATURE_INETD_RPC
  BusyBoxDisable CONFIG_FEATURE_IP_LINK
  BusyBoxDisable CONFIG_FEATURE_MOUNT_LOOP
  BusyBoxDisable CONFIG_FEATURE_SUID
  BusyBoxDisable CONFIG_FREE
  BusyBoxDisable CONFIG_FREERAMDISK
  BusyBoxDisable CONFIG_FSTRIM
  BusyBoxDisable CONFIG_HDPARM
  BusyBoxDisable CONFIG_IFCONFIG
  BusyBoxDisable CONFIG_IFENSLAVE
  BusyBoxDisable CONFIG_IFPLUGD
  BusyBoxDisable CONFIG_IP
  BusyBoxDisable CONFIG_LOSETUP
  BusyBoxDisable CONFIG_MKFS_EXT2
  BusyBoxDisable CONFIG_MKFS_VFAT
  BusyBoxDisable CONFIG_MOUNT
  BusyBoxDisable CONFIG_NANDDUMP
  BusyBoxDisable CONFIG_NANDWRITE
  BusyBoxDisable CONFIG_NBDCLIENT
  BusyBoxDisable CONFIG_RAIDAUTORUN
  BusyBoxDisable CONFIG_ROUTE
  BusyBoxDisable CONFIG_SLATTACH
  BusyBoxDisable CONFIG_TUNCTL
  BusyBoxDisable CONFIG_UBIATTACH
  BusyBoxDisable CONFIG_UBIDETACH
  BusyBoxDisable CONFIG_UBIMKVOL
  BusyBoxDisable CONFIG_UBIRMVOL
  BusyBoxDisable CONFIG_UBIRSVOL
  BusyBoxDisable CONFIG_UBIUPDATEVOL
  BusyBoxDisable CONFIG_UDHCPC
  BusyBoxDisable CONFIG_UDHCPD
  BusyBoxDisable CONFIG_UMOUNT
  BusyBoxDisable CONFIG_UPTIME
  BusyBoxDisable CONFIG_WATCHDOG

  if [ "${TOOLCHAIN}" = "glibc" -a "${NACL_ARCH}" = "arm" ]; then
    BusyBoxDisable CONFIG_INIT
    BusyBoxDisable CONFIG_LINEEDIT
    BusyBoxDisable CONFIG_FEATURE_UTMP
    BusyBoxDisable CONFIG_FEATURE_VI_USE_SIGNALS
  fi
}

BuildStep() {
  export CROSS_COMPILE=${NACL_CROSS_PREFIX}-
  export CPPFLAGS="${NACLPORTS_CPPFLAGS}"
  export EXTRA_CFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}"
  export EXTRA_LDFLAGS="${NACLPORTS_LDFLAGS}"
  EXTRA_LDFLAGS+=" -lppapi_simple -lnacl_io -lppapi"
  DefaultBuildStep
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/busybox"
  MakeDir ${ASSEMBLY_DIR}

  cp busybox ${ASSEMBLY_DIR}/busybox_${NACL_ARCH}${NACL_EXEEXT}

  ChangeDir ${ASSEMBLY_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${ASSEMBLY_DIR}/busybox_*${NACL_EXEEXT} \
      -s . \
      -o busybox.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py busybox.nmf

  InstallNaClTerm ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/manifest.json ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_16.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_48.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_128.png ${ASSEMBLY_DIR}
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip busybox-${VERSION}.zip busybox
}
