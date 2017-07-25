#ifndef __ACTION_BLOOM_HLS_H__
#define __ACTION_BLOOM_HLS_H__

/*
 * Copyright 2017, International Business Machines
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

#include <hls_snap.H>
#include <action_bloom.h>

#define BLOOM_VERSION_MAJOR 1
#define BLOOM_VERSION_MINOR 5
#define BLOOM_VERSION (BLOOM_VERSION_MAJOR).BLOOM_VERSION_MINOR)

//---------------------------------------------------------------------
typedef struct {
        CONTROL Control;       /*  16 bytes */
        bloom_job_t Data;   /* 108 bytes */
	uint8_t padding[SNAP_HLS_JOBSIZE - sizeof(bloom_job_t)];
} action_reg;

#endif  /* __ACTION_BLOOM_HLS_H__ */
