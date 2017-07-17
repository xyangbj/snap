#ifndef __ACTION_BFS_NVME_H__
#define __ACTION_BFS_NVME_H__

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
#include <snap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BFS_NVME_ACTION_TYPE 0x10141024

#ifndef CACHELINE_BYTES
#define CACHELINE_BYTES 128
#endif
unsigned int * g_out_ptr;

#define END_SIGN (uint64_t)(-1)
#define END_SIGN_REL (uint64_t) 0x7FFFFFFFF
#define END_SIGN_PROP (uint64_t) 0xFFFFFFFFF

//RelRecordFmt -- Formatted
//64bytes
typedef struct __attribute__((__packed__))  RelRecordFmt
{
    uint8_t in_use;
    uint32_t first_node; 
    uint32_t second_node; 
    uint32_t rel_type;
    uint32_t first_prev_rel_id;
    uint32_t first_next_rel_id;
    uint32_t second_prev_rel_id;
    uint32_t second_next_rel_id;
    uint32_t next_prop_id;  //data
    uint8_t  extra;         //first_in_chain_markers; 
    uint8_t paddings[30];
} RelRecordFmt_t;

// Every field in 64bit, easy to manipulate in C
typedef struct RelRecord
{
    uint64_t first;
    uint64_t second;
    uint64_t first_prev;
    uint64_t first_next;
    uint64_t second_prev;
    uint64_t second_next;
    uint64_t next_prop;
    short    first_in_endchain;
    short    first_in_startchain;
} RelRecord_t;

//NodeRecord --Formatted
//16bytes
typedef struct __attribute__((__packed__)) NodeRecordFmt
{
    uint8_t in_use;
    uint32_t next_rel_id;  
    uint32_t next_prop_id; //data
    uint8_t labels[5];
    uint8_t extra;         //dense?  
    uint8_t paddings[1];
} NodeRecordFmt_t;

// Every field in 64bit, easy to manipulate in C
typedef struct NodeRecord
{
    uint64_t next_rel;
    uint64_t last_rel; //temparal use.
    uint64_t next_prop;
} NodeRecord_t;

//Example Node properties and Edge properties
#define NAME_LENGTH 5
typedef struct NodeProperty
{
    uint8_t name[64];
    uint32_t age;
    uint8_t category[60]; 
    uint8_t location[64];
} NodeProperty_t;

typedef struct RelProperty 
{
    uint64_t first;
    uint64_t second;
    uint8_t relationship[48];
} RelProperty_t;


//-----------------------------------------
// Action needs to construct NodeRecord array and RelRecord Array in DDR.
// RelProperty and NodeProperty will be stored in NVMe storage.
typedef struct bfs_job {
    uint64_t NodeRecord_host;
    uint64_t RelRecord_host;
    uint64_t NodeProperty_host; //Do we need to copy them from Host to NVMe?
    uint64_t RelProperty_host;  //Do we need to copy them from Host to NVMe?
    uint64_t node_num; 
    uint64_t rel_num;


    // input conditions    
    uint64_t start_root;      //try to do multi-source BFS from
                              //the adjacent vertices of start_root. 

    uint64_t qualifier;       //Used to match with NodeRecord.labels

    // output traverse track in Host
    struct snap_addr output_traverse;

    // Control and Status register
    uint32_t step;
    uint32_t status_pos;
} bfs_job_t;

//-----------------------------------------
//   Queue
//-----------------------------------------

typedef uint64_t ElementType;
typedef struct QNode {
    ElementType    data;
    struct QNode   *next;
} QNode_t;

typedef struct Queue {
    struct QNode * front;
    struct QNode * rear;
} Queue_t;

#ifdef __cplusplus
}
#endif
#endif	/* __ACTION_BFS_NVME_H__ */
