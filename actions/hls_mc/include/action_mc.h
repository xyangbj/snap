#ifndef __ACTION_MC_H__
#define __ACTION_MC_H__

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

#define MC_ACTION_TYPE 0x10141010

typedef struct mc_job_t {
	struct snap_addr in;	/* input data */
	struct snap_addr out;   /* output table */
        int seed;
        float result;
} mc_job_t;

#ifdef __cplusplus
}
#endif

#endif	/* __ACTION_MEMCOPY_H__ */
