// Copyright (c) 2012 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Based on Public Domain code made released by Moritz Warning, can be obtained at
// https://github.com/mwarning/libsodium-example/blob/master/crypto_box.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

// sodium needs /dev/urandom
#include "nacl_io/nacl_io.h"
#include "sys/mount.h"
#include "sys/stat.h"

// the library we are testing
#include <sodium.h>

// the testing is done with asserts
#include <assert.h>

typedef unsigned char UCHAR;

// helper function for calling sodium and printing back results

char* to_hex( char hex[], const UCHAR bin[], size_t length )
{
  int i;
  UCHAR *p0 = (UCHAR *)bin;
  char *p1 = hex;

  for( i = 0; i < length; i++ ) {
    snprintf( p1, 3, "%02x", *p0 );
    p0 += 1;
    p1 += 2;
  }

  return hex;
}

int is_zero( const UCHAR *data, int len )
{
  int i;
  int rc;

  rc = 0;
  for(i = 0; i < len; ++i) {
    rc |= data[i];
  }

  return rc;
}

#define MAX_MSG_SIZE 1400

int encrypt(UCHAR encrypted[], const UCHAR pk[], const UCHAR sk[], const UCHAR nonce[], const UCHAR plain[], int length)
{
  int rc;
  UCHAR temp_plain[MAX_MSG_SIZE];
  UCHAR temp_encrypted[MAX_MSG_SIZE];


  if(length+crypto_box_ZEROBYTES >= MAX_MSG_SIZE) {
    return -2;
  }

  memset(temp_plain, '\0', crypto_box_ZEROBYTES);
  memcpy(temp_plain + crypto_box_ZEROBYTES, plain, length);

  rc = crypto_box(temp_encrypted, temp_plain, crypto_box_ZEROBYTES + length, nonce, pk, sk);

  if( rc != 0 ) {
    return -1;
  }

  // this check is probably superficial
  if( is_zero(temp_plain, crypto_box_BOXZEROBYTES) != 0 ) {
    return -3;
  }

  memcpy(encrypted, temp_encrypted + crypto_box_BOXZEROBYTES, crypto_box_ZEROBYTES + length);

  return crypto_box_ZEROBYTES + length - crypto_box_BOXZEROBYTES;
}

int decrypt(UCHAR plain[], const UCHAR pk[], const UCHAR sk[], const UCHAR nonce[], const UCHAR encrypted[], int length)
{
  int rc;
  UCHAR temp_encrypted[MAX_MSG_SIZE];
  UCHAR temp_plain[MAX_MSG_SIZE];

  if(length+crypto_box_BOXZEROBYTES >= MAX_MSG_SIZE) {
    return -2;
  }

  memset(temp_encrypted, '\0', crypto_box_BOXZEROBYTES);
  memcpy(temp_encrypted + crypto_box_BOXZEROBYTES, encrypted, length);

  rc = crypto_box_open(temp_plain, temp_encrypted, crypto_box_BOXZEROBYTES + length, nonce, pk, sk);

  if( rc != 0 ) {
    return -1;
  }

  // this check is probably superficial
  if( is_zero(temp_plain, crypto_box_ZEROBYTES) != 0 ) {
    return -3;
  }

  memcpy(plain, temp_plain + crypto_box_ZEROBYTES, crypto_box_BOXZEROBYTES + length);

  return crypto_box_BOXZEROBYTES + length - crypto_box_ZEROBYTES;
}

struct User {
  char* name;
  UCHAR public_key[crypto_box_PUBLICKEYBYTES];
  UCHAR secret_key[crypto_box_SECRETKEYBYTES];
};

void print_user(struct User *user)
{
  char phexbuf[2*crypto_box_PUBLICKEYBYTES+1];
  char shexbuf[2*crypto_box_SECRETKEYBYTES+1];

  printf("username: %s\n", user->name);
  printf("public key: %s\n", to_hex(phexbuf, user->public_key, crypto_box_PUBLICKEYBYTES ));
  printf("secret key: %s\n\n", to_hex(shexbuf, user->secret_key, crypto_box_SECRETKEYBYTES ));
}

// initialization

void initialize() {
  nacl_io_init();
  int rc;
  rc = sodium_init();
  assert(rc == 0);
}

void randombytes_test() {

  UCHAR a[24];
  UCHAR b[24];

  memset(a, '\0', 24);
  memset(b, '\0', 24);

  // check that is_zero is not broken
  assert(is_zero(a, 24) == 0);
  assert(is_zero(b, 24) == 0);

  randombytes(a, 24);
  randombytes(b, 24);

  // TEST: randombytes changes both variables
  assert(is_zero(a, 24) != 0);
  assert(is_zero(b, 24) != 0);
  // TEST: randobytes returned something different each time
  assert(memcmp(a,b,24) != 0);
}

void crypto_box_test() {
  int rc;
  char hexbuf[256];
  struct User alice = {"alice",
    {0x9a, 0x6a, 0x29, 0xbc, 0x58, 0x75, 0x77,
      0xe6, 0xf8, 0x0d, 0x48, 0xc0, 0xcc, 0x4c,
      0x41, 0xe5, 0xd5, 0xe1, 0x3d, 0x5e, 0xed,
      0xc2, 0x7d, 0xf1, 0xc0, 0xd8, 0x08, 0xc5,
      0xdd, 0x2e, 0xa6, 0x56},
      {0x89, 0x9d, 0x63, 0xea, 0x4c, 0x7a, 0x9b,
        0xee, 0xad, 0xf7, 0x26, 0x1d, 0x81, 0x56,
        0x38, 0x8a, 0xe2, 0x64, 0x51, 0xf0, 0xe8,
        0x1d, 0x3d, 0x9f, 0x9c, 0xde, 0xed, 0x7e,
        0xde, 0xe1, 0xe7, 0x78}
  };
  struct User bob = {"bob",
    {0x0e, 0x32, 0x48, 0x73, 0xd9, 0x96, 0x93,
      0xa7, 0x39, 0x40, 0x85, 0xc2, 0x0a, 0x72,
      0x72, 0xe7, 0xda, 0xde, 0xc9, 0x51, 0x06,
      0xb6, 0x43, 0x35, 0x37, 0x15, 0xa6, 0x77,
      0xb7, 0x6b, 0x9a, 0x63},
      {0xfc, 0x17, 0x2d, 0xdf, 0xd8, 0xb0, 0x79,
        0x2d, 0x4f, 0x0f, 0x9e, 0x03, 0x6f, 0xaa,
        0x79, 0x32, 0x50, 0xe4, 0xc9, 0x84, 0x63,
        0xbe, 0x15, 0xc8, 0x13, 0xa0, 0xcb, 0xed,
        0x2a, 0xc0, 0xb9, 0x17}
  };
  UCHAR nonce[crypto_box_NONCEBYTES] =  {0x77, 0xf4,
    0xce, 0x6d, 0x6b, 0xfd, 0x93, 0x69, 0x1e,
    0x9d, 0xd3, 0xa5, 0x99, 0xca, 0xd3, 0x61,
    0xd4, 0xbb, 0x65, 0x83, 0x99, 0x00, 0x7a,
    0x67};

  char *msg = "Hello";

  print_user(&alice);
  print_user(&bob);
  printf("message to bob: %s\n", msg);
  printf("nonce: %s\n\n", to_hex(hexbuf, nonce, crypto_box_NONCEBYTES));

  UCHAR encrypted[1000];
  rc = encrypt(encrypted, bob.public_key, alice.secret_key, nonce, (const UCHAR *)msg, strlen(msg));
  assert( rc >= 0 );
  printf("encrypted message to bob: %s\n", to_hex(hexbuf, encrypted, rc ));

  UCHAR decrypted[1000];
  rc = decrypt(decrypted, alice.public_key, bob.secret_key, nonce, encrypted, rc);
  assert( rc >= 0 );
  decrypted[rc] = '\0';
  printf("decrypted message from alice: %s\n", decrypted);

  // TEST: msg = decrypt(encrypt(msg))
  assert(memcmp(msg, decrypted, strlen(msg)) == 0);
  // TEST: msg != encrypt(msg)
  assert(memcmp(msg, encrypted, strlen(msg)) != 0);
  // TEST: encrypt(msg) == "..."
  UCHAR correct[21] = {0x72, 0xd0, 0x7e, 0xf9, 0x72,
    0x36, 0xd6, 0x5f, 0x7d, 0x37, 0xa5, 0xf0, 0x84,
    0xf4, 0x37, 0xc8, 0xe9, 0x70, 0xd0, 0xe2, 0x20};
  assert(memcmp(encrypted, correct, 21) == 0);
}

int main( int argc, char **argv )
{
  initialize();
  randombytes_test();
  crypto_box_test();
  return 0;
}