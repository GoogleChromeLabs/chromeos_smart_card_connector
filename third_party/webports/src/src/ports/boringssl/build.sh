# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="
crypto/base64/base64_test
crypto/bio/bio_test
crypto/bn/bn_test
crypto/bytestring/bytestring_test
crypto/cipher/aead_test
crypto/cipher/aead_test
crypto/cipher/aead_test
crypto/cipher/aead_test
crypto/cipher/aead_test
crypto/cipher/aead_test
crypto/cipher/cipher_test
crypto/dh/dh_test
crypto/dsa/dsa_test
crypto/ec/example_mul
crypto/ecdsa/ecdsa_test
crypto/err/err_test
crypto/evp/example_sign
crypto/hmac/hmac_test
crypto/lhash/lhash_test
crypto/md5/md5_test
crypto/modes/gcm_test
crypto/pkcs8/pkcs12_test
crypto/rsa/rsa_test
crypto/sha/sha1_test
crypto/x509/pkcs7_test
crypto/x509v3/tab_test
crypto/x509v3/v3name_test
ssl/pqueue/pqueue_test
ssl/ssl_test
"

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}/openssl
  LogExecute install crypto/libcrypto.a ${DESTDIR_LIB}/
  LogExecute install ${SRC_DIR}/include/openssl/* -t ${DESTDIR_INCLUDE}/openssl
}

TestStep() {
  LogExecute ssl/ssl_test.sh
  LogExecute crypto/md5/md5_test.sh
  bash ../boringssl-git/util/all_tests.sh ../boringssl-git/ .sh
}
