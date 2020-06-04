/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>

#include "vorbis/vorbisfile.h"

/* Opaque structure passed through the callbacks. */
typedef struct ogg_handle {
  void* buffer;
  size_t size;
  off_t read_pos;
} ogg_handle;

/*  reference:

typedef struct {
  size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void *datasource);
  int    (*seek_func)  (void *datasource, ogg_int64_t offset, int whence);
  int    (*close_func) (void *datasource);
  long   (*tell_func)  (void *datasource);
} ov_callbacks;
*/

static size_t audio_read(void* buffer, size_t size, size_t nmemb, void* f) {
  size_t actual_size = size * nmemb;
  if (((ogg_handle*)f)->size - ((ogg_handle*)f)->read_pos < actual_size)
    actual_size = ((ogg_handle*)f)->size - ((ogg_handle*)f)->read_pos;
  if (actual_size != size * nmemb) {
    nmemb = actual_size / size;
  }
  memcpy(buffer, ((ogg_handle*)f)->buffer + ((ogg_handle*)f)->read_pos,
         size * nmemb);
  ((ogg_handle*)f)->read_pos += size * nmemb;
  return nmemb;
}

static int audio_seek(void* f, ogg_int64_t off, int whence) {
  switch (whence) {
    case SEEK_SET:
      ((ogg_handle*)f)->read_pos = off;
      break;
    case SEEK_CUR:
      ((ogg_handle*)f)->read_pos += off;
      break;
    case SEEK_END:
      ((ogg_handle*)f)->read_pos = (off_t)(((ogg_handle*)f)->size) + off;
      break;
  }
  return ((ogg_handle*)f)->read_pos;
}

static int audio_close(void* f) {
  return 0;
}

static long audio_tell(void* f) {
  return ((ogg_handle*)f)->read_pos;
}

void DecodeOggBuffer(void* inBuffer,
                     size_t size,
                     char** outBuffer,
                     int* outBufferSize,
                     int* outChannels,
                     int* outRate) {
  OggVorbis_File ogg;

  ogg_handle oh;
  oh.buffer = inBuffer;
  oh.size = size;
  oh.read_pos = 0;

  /* Use custom callbacks to read the ogg file from a buffer in the absence of
   * ordinary POSIX file functions.
   */
  ov_callbacks callbacks = {
      (size_t (*)(void*, size_t, size_t, void*))audio_read,
      (int (*)(void*, ogg_int64_t, int))audio_seek,
      (int (*)(void*))audio_close,
      (long (*)(void*))audio_tell,
  };

  ov_open_callbacks(&oh, &ogg, NULL, 0, callbacks);
  vorbis_info* info = ov_info(&ogg, -1);
  printf("ogg file, channels: %d, rate: %ld\n", info->channels, info->rate);
  ogg_int64_t num_samples = ov_pcm_total(&ogg, -1);
#define SAMPLE_SIZE sizeof(short) /* assume 16-bit samples */
  printf("\tnum_samples: %lld buffer size: %lld\n", num_samples,
         info->channels * SAMPLE_SIZE * num_samples);

  int buf_size = num_samples * SAMPLE_SIZE * info->channels;
  /* Caller is responsible for freeing the resulting PCM buffer. */
  char* pcm_buffer = (char*)malloc(buf_size);
  int pos = 0;
  while (pos < buf_size) {
    int ret = ov_read(&ogg, pcm_buffer + pos, buf_size - pos, 0, 2, 1, NULL);
    if (ret == OV_HOLE || ret == OV_EBADLINK || ret == OV_EINVAL)
      continue;
    pos += ret;
  }

  *outBuffer = pcm_buffer;
  *outBufferSize = buf_size;
  *outChannels = info->channels;
  *outRate = info->rate;
}
