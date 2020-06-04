# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o errexit
set -o nounset

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
NACLPORTS_ROOT="$(cd ${SCRIPT_DIR}/.. && pwd)"

PKG=${NACLPORTS_ROOT}/out/build/pkg/build_host/src/pkg
SDK_VERSION=pepper_48

WriteMetaFile() {
  echo "version = 1;" > $1
  echo "packing_format = "tbz";" >> $1
  echo "digest_format = "sha256_base32";" >> $1
  echo "digests = "digests";" >> $1
  echo "manifests = "packagesite.yaml";" >> $1
  echo "filesite = "filesite.yaml";" >> $1
  echo "digests_archive = "digests";" >> $1
  echo "manifests_archive = "packagesite";" >> $1
  echo "filesite_archive = "filesite";" >> $1
}

RunPkg() {
  local pkg_dir=$1
  local out_dir=$2
  WriteMetaFile "${pkg_dir}/meta"
  mkdir -p "${out_dir}"
  local cmd="$PKG repo -m ${pkg_dir}/meta -o ${out_dir} ${pkg_dir}"
  echo $cmd
  $cmd
  cd ${out_dir}
}

BuildRepo() {
  local gs_url="gs://naclports/builds/${SDK_VERSION}/$1/publish"
  ${SCRIPT_DIR}/download_pkg.py $1
  local REPO_DIR=${NACLPORTS_ROOT}/out/packages/prebuilt/repo
  for pkg_dir in ${NACLPORTS_ROOT}/out/packages/prebuilt/pkg/*/ ; do
    local out_dir=${REPO_DIR}/$(basename ${pkg_dir})
    RunPkg $pkg_dir $out_dir
    gsutil cp -a public-read *.tbz ${gs_url}/$(basename ${pkg_dir})
  done
}

BuildLocalRepo() {
  local REPO_DIR=${NACLPORTS_ROOT}/out/publish/
  for pkg_dir in ${REPO_DIR}/pkg_*/ ; do
    RunPkg $pkg_dir $pkg_dir
  done
}

UsageHelp() {
  echo "./build_repo.sh - Build pkg repository using \
local/remote built packages"
  echo ""
  echo "./build_repo.sh [-l] [-r REVISION]"
  echo "Either provide a revision(-r) for remote built pakcages or \
use -l for local built packages"
  echo ""
  echo "Description"
  echo "-h   show help messages"
  echo "-l   build pkg repository using local built packages"
  echo "-r   build pkg repository using remote built packages"
}

if [[ $# -lt 1 ]]; then
  UsageHelp
  exit 1
fi

OPTIND=1

while getopts "h?lr:" opt; do
  case "$opt" in
    h|\?)
        UsageHelp
        exit 0
        ;;
    l)  local=1
        ;;
    r)  local=0
        revision=$OPTARG
        ;;
  esac
done

shift $((OPTIND - 1))

if [[ ! -f ${PKG} ]]; then
  TOOLCHAIN=clang-newlib make pkg F=1 V=1
fi

if [[ ${local} = 1 ]]; then
  BuildLocalRepo
else
  BuildRepo $revision
fi
