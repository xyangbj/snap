/*
 *  Copyright (c) 2012-2017, Jyri J. Virkki
 *  All rights reserved.
 *
 *  This file is under BSD license. See LICENSE file.
 */

/*
 * Refer to bloom.h for documentation on the public interfaces.
 */

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <hls_math.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bloom.H"
#include "murmurhash2.H"

#define MAKESTRING(n) STRING(n)
#define STRING(n) #n


inline static int test_bit_set_bit(unsigned char * buf,
                                   unsigned int x, int set_bit)
{
  unsigned int byte = x >> 3;

  unsigned char c = buf[byte];        // expensive memory access

  unsigned int mask = 1 << (x % 8);

  if (c & mask) {
    return 1;
  } else {
    if (set_bit) {
      buf[byte] = c | mask;
    }
    return 0;
  }
}


//static int bloom_check_add(struct bloom * bloom,
//                           const void * buffer, int len, int add)
static int bloom_check_add(struct bloom * bloom,
                           const char buffer[32], int len, int add)
{
  if (bloom->ready == 0) {
    printf("bloom at %p not initialized!\n", (void *)bloom);
    return -1;
  }

  int hits = 0;
  register unsigned int a = murmurhash2(buffer, len, 0x9747b28c);
  register unsigned int b = murmurhash2(buffer, len, a);
  register unsigned int x;
  register unsigned int i;

  for (i = 0; i < bloom->hashes; i++) {
//#pragma HLS PIPELINE
    x = (a + i*b) % bloom->bits;
    if (test_bit_set_bit(bloom->bf, x, add)) {
      hits++;
    }
  }

  if (hits == bloom->hashes) {
    return 1;                // 1 == element already in (or collision)
  }

  return 0;
}


int bloom_init_size(struct bloom * bloom, int entries, double error,
                    unsigned int cache_size)
{
  return bloom_init(bloom, entries, error);
}


int bloom_init(struct bloom * bloom, int entries, double error)
{
	  unsigned char bf_alloc[MAX_BYTES_ALLOCATION];
	  int k;

	  bloom->ready = 0;

  if (entries < 1000 || error == 0) {
    return 1;
  }

  bloom->entries = entries;
  bloom->error = error;

  //double num = hls::log(bloom->error);
  //double denom = 0.480453013918201; // ln(2)^2
  //bloom->bpe = -(num / denom);
 
  double num = log(error);
  double denom = 0.480453013918201; // ln(2)^2
  bloom->bpe = -(num / denom);

  // Get log10 of bloom.error
  //double error_tmp;
  //int log_error;
  //error_tmp  = error;
  //for(log_error = 0; log_error < 16; log_error++) {
  	//if((int)error_tmp == 1) break;
  	//error_tmp = error_tmp * 10.;
  //}
  //printf("DBG>> error : %f - log_error : %d\n", error, log_error);
  //double num = (double)(log_error+1);
  //double denom = 0.208658092756899; // log(e) * ln(2)^2
  //bloom->bpe = (num / denom);


  double dentries = (double)entries;
  bloom->bits = (int)(dentries * bloom->bpe);

  if (bloom->bits % 8) {
    bloom->bytes = (bloom->bits / 8) + 1;
  } else {
    bloom->bytes = bloom->bits / 8;
  }

  bloom->hashes = (int)ceil(0.693147180559945 * bloom->bpe);  // ln(2)
  //bloom->hashes = (int)hls::ceil(0.693147180559945 * bloom->bpe);  // ln(2)
// Original declaration
//  bloom->bf = (unsigned char *)calloc(bloom->bytes, sizeof(unsigned char));
//  bloom->bf is declared in bloom.h - just initializing it
  if (bloom->bytes > MAX_BYTES_ALLOCATION) {
	  printf("DBG_ERROR>>> : Increase MAX_BYTES_ALLOCATION to %d (vs %d)\n",
		bloom->bytes, MAX_BYTES_ALLOCATION);
  	  return 0;
  }
else
 {
	  printf("DBG>> This test allocates  %d bytes for the bloom filter\n", bloom->bytes);
 }
  bf_init:for ( k = 0; k < bloom->bytes; k++)
#pragma HLS PIPELINE
		  bloom->bf[k] = 0;

  //if (bloom->bf == NULL) {
  //  return 1;
  //}
// end of change

  bloom->ready = 1;
  return 0;
}

//int bloom_check(struct bloom * bloom, const void * buffer, int len)
int bloom_check(struct bloom * bloom, const char buffer[32], int len)
{
  return bloom_check_add(bloom, buffer, len, 0);
}


//int bloom_add(struct bloom * bloom, const void * buffer, int len)
int bloom_add(struct bloom * bloom, const char buffer[32], int len)
{
  return bloom_check_add(bloom, buffer, len, 1);
}


void bloom_print(struct bloom * bloom)
{
  printf("bloom at %p\n", (void *)bloom);
  printf(" ->entries = %d\n", bloom->entries);
  printf(" ->error = %f\n", bloom->error);
  printf(" ->bits = %d\n", bloom->bits);
  printf(" ->bits per elem = %f\n", bloom->bpe);
  printf(" ->bytes = %d\n", bloom->bytes);
  printf(" ->hash functions = %d\n", bloom->hashes);
}


void bloom_free(struct bloom * bloom)
{
  if (bloom->ready) {
//    free(bloom->bf);
  }
  bloom->ready = 0;
}


const char * bloom_version()
{
  return MAKESTRING(BLOOM_VERSION);
}
