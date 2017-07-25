#ifndef __ACTION_BLOOM_H__
#define __ACTION_BLOOM_H__

/*
 * Copyright 2016, 2017 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <snap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOOM_ACTION_TYPE    0x10141006
#define RELEASE_LEVEL        0x00000010 
#define CONFIG_USE_NO_PTHREADS

typedef enum {
	BLOOM = 0x0,
	BLOOM_MODE_MAX = 0x1,
} bloom_mode_t;

typedef enum {
	BLOOM_SPEED = 0x0,
	BLOOM_BASIC  = 0x1,
	BLOOM_TYPE_MAX = 0x42
} test_choice_t;

typedef struct bloom_job {
	struct snap_addr in;	/* in:  input data */
	struct snap_addr src_result;	/* out:  unused */
	struct snap_addr ddr_result;	/* out:  unused  */
	uint16_t mode;		/* in:  BLOOM */
	uint16_t test_choice;	/* in:  SPEED, BASIC */
	uint32_t nb_calls;	/* in:  nb of times perf test is called */
	uint32_t count;		/* in : nb of times add/checkings to run*/
	uint32_t entries;	/* in:  nb of entries in bloom filter */
	uint32_t error_rate;  	/* in:  error rate - negative power of ten value*/
	uint32_t threads;  	/* in:  number of threads - used for sw sim only*/
	uint32_t bits;   	/* out: number of bits used for the bloom filter */
	uint32_t bytes;   	/* out: number of bytes used for the bloom filter */
	uint32_t hashes;   	/* out: number of hash used for the bloom filter */
	uint32_t collisions;   	/* out: number of collisions found by the bloom filter */
} bloom_job_t;

#ifdef __cplusplus
}
#endif

#endif	/* __ACTION_BLOOM_H__ */
