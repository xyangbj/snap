/*
 * Simple Breadth-first-search in C
 *
 * Based on Pseudo code of:
 *        https://en.wikipedia.org/wiki/Breadth-first_search
 *
 * Use Adjacency list to describe a graph:
 *        https://en.wikipedia.org/wiki/Adjacency_list
 *
 * And takes Queue structure:
 *        https://en.wikipedia.org/wiki/Queue_%28abstract_data_type%29
 *
 * Wikipedia's pages are based on "CC BY-SA 3.0"
 * Creative Commons Attribution-ShareAlike License 3.0
 * https://creativecommons.org/licenses/by-sa/3.0/
 *
 * Attribution:
 * You must give appropriate credit, provide a link to
 * the license, and indicate if changes were made. You may do so in
 * any reasonable manner, but not in any way that suggests the
 * licensor endorses you or your use.
 *
 * ShareAlike:
 * If you remix, transform, or build upon the material, you must
 * distribute your contributions under the same license as the original.
 */


 /*
 * Adopt SNAP's framework for software action part.
 */

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

/*
 * Example to use the FPGA to traverse a graph with breadth-first-search
 * method.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libsnap.h>
#include <linux/types.h>	/* __be64 */
#include <asm/byteorder.h>

#include <snap_internal.h>
#include <snap_tools.h>
#include <action_bfs_nvme.h>


static int mmio_write32(struct snap_card *card,
        uint64_t offs, uint32_t data)
{
    act_trace("  %s(%p, %llx, %x)\n", __func__, card,
            (long long)offs, data);
    return 0;
}

static int mmio_read32(struct snap_card *card,
        uint64_t offs, uint32_t *data)
{
    act_trace("  %s(%p, %llx, %x)\n", __func__, card,
            (long long)offs, *data);
    return 0;
}
//-------------------------------------
//   Queue functions
//-------------------------------------
static Queue_t * InitQueue()
{
    Queue_t *q= (Queue_t*)malloc (sizeof(Queue_t));
    if (!q)
    {
        printf("ERROR: failed to malloc queue.\n");
        return NULL;
    }
    q -> front = NULL;
    q -> rear  = NULL;
    return q;
}

static void DestoryQueue( Queue_t *q)
{
    free (q);
}

static int QueueEmpty(Queue_t * q)
{
    return (q->front == NULL);
}

/*
   static void PrintQueue(Queue_t *q)
   {
   printf("Queue is ");
   if (QueueEmpty(q))
   {
   printf ("empty.\n");
   return;
   }

   QNode * node = q->front;
   while (node != NULL)
   {
   printf ("%d, ", node->data);
   node = node->next;
   }
   printf("\n");
   }
   */
static void EnQueue (Queue_t *q, ElementType element)
{
    QNode_t * node_ptr = (QNode_t *) malloc (sizeof(QNode_t));
    if (!node_ptr)
    {
        printf("ERROR: failed to malloc a queue node.\n");
        return;
    }

    node_ptr->data = element;
    node_ptr->next = NULL;

    //Append to the tail

    if (q->front == NULL)
        q->front = node_ptr;

    if (q->rear == NULL)
    {
        q->rear = node_ptr;
    }
    else
    {
        q->rear->next = node_ptr;
        q->rear = node_ptr;
    }


    //PrintQueue(q);
}



static void DeQueue (Queue_t *q, ElementType *ep)
{
    // printf("Dequeue\n");
    if (QueueEmpty (q))
    {
        printf ("ERROR: DeQueue: queue is empty.\n");
        return;
    }

    QNode_t *temp = q->front;
    if(q->front == q->rear) //only one element
    {
        q->front = NULL;
        q->rear = NULL;
    }
    else
    {
        q->front = q->front->next;
    }

    *ep = temp->data;
    free(temp);
}


//-------------------------------------
//    breadth first search
//-------------------------------------

// Output Traverse Result
// put one visited vertex to the place of g_out_ptr.
// Last vertex (is_tail=1) will follow an END sign (FFxxxxxx)
// And with the total number of vertices
// FIXME: old, only supports uint32
static void output_node(unsigned int vertex, int is_tail)
{
    if(is_tail == 0)
    {
        *g_out_ptr = vertex;
        g_out_ptr ++;
        //   printf("Visit Node %d\n", vertex);
    }
    else
    {
        *g_out_ptr = 0xFF000000 + vertex; //here vertex means cnt
        // printf("End. %x\n", *g_out_ptr);
        g_out_ptr += 32 - (((unsigned long long )g_out_ptr & 0x7C) >> 2); //Make paddings.

    }
}
static void decode_NodeRecord( NodeRecordFmt_t in, NodeRecord_t *out)
{
    out->next_rel = in.next_rel_id + (((uint64_t)(in.in_use & 0xE)) << 31);
    out->next_prop = in.next_prop_id + (((uint64_t)(in.in_use & 0xF0)) << 28);
}
static void decode_RelRecord (RelRecordFmt_t in, RelRecord_t *out)
{
    out->first = in.first_node + (((uint64_t)(in.in_use & 0xE)) << 31);
    out->next_prop = in.next_prop_id + (((uint64_t)(in.in_use & 0xF0)) << 28);

    out->second = in.second_node + (((uint64_t)(in.rel_type & 0x70000000)) << 4);
    out->first_prev = in.first_prev_rel_id + (((uint64_t)(in.rel_type & 0xE000000)) << 7);
    out->first_next = in.first_next_rel_id + (((uint64_t)(in.rel_type & 0x1C00000)) << 10);
    out->second_prev = in.second_prev_rel_id + (((uint64_t)(in.rel_type & 0x380000)) << 13);
    out->second_next = in.second_next_rel_id + (((uint64_t)(in.rel_type & 0x70000)) << 16);
}

//Breadth-first-search from a start_root
static void bfs (NodeRecordFmt_t * nr_list_fmt, RelRecordFmt_t * rr_list_fmt,
        uint64_t node_num, uint64_t root, uint64_t qualifier)
{

    Queue_t *Q;
    uint8_t * visited;
    visited = malloc (node_num * sizeof(uint8_t));

    uint64_t i, rel, node;
    NodeRecord_t nr;
    RelRecord_t  rr;

    uint64_t current = 0; 
    uint64_t cnt = 0;

    //initilize to all zero.
    for (i = 0; i < node_num; i++)
        visited[i] = 0;

    //Starting from root
    Q = InitQueue();
    visited[root] = 1;
    output_node( root,0);
    cnt++;
    EnQueue(Q, root);


    //BFS
    while (! QueueEmpty(Q))
    {
        DeQueue(Q, &current);
        decode_NodeRecord(nr_list_fmt[current], &nr);

        rel = nr.next_rel;
        while(rel != END_SIGN_REL)
        {
            decode_RelRecord(rr_list_fmt[rel], &rr);
            node = rr.second;

            if(!visited[node])
            {
                visited[node] = 1;
                output_node((uint32_t)node, 0); //TODO
                cnt++;

                EnQueue(Q, node);
            }
            rel = rr.second_next;
        } //till to NULL of the edge list
    }
    output_node((uint32_t)cnt, 1); //Indicate a tail //TODO

    free(visited);
    DestoryQueue(Q);
}
//------------------------------------
//    action main
//------------------------------------


static int action_main(struct snap_sim_action *action,
        void *job, unsigned int job_len __unused)
{
    int rc = 0;
    bfs_job_t *js = (bfs_job_t *)job;


    //NodeProperty_t * np_array = (NodeProperty_t *) js->NodeProperty_host;
    //RelProperty_t  * rp_array = (RelProperty_t *)  js->RelProperty_host;

    NodeRecordFmt_t * nr_list_fmt = (NodeRecordFmt_t *) js->NodeRecord_host;
    RelRecordFmt_t  * rr_list_fmt = (RelRecordFmt_t *)  js->RelRecord_host;
   
    //Nothing to do in step1.
    
    if (js->step == 2) {
        g_out_ptr = (unsigned int *)js->output_traverse.addr;

        bfs(nr_list_fmt, rr_list_fmt, js->node_num, js->start_root, js->qualifier);
        js->status_pos = (unsigned int)((unsigned long long) g_out_ptr & 0xFFFFFFFFull);
    }
    if (rc == 0)
        goto out_ok;
    else
        goto out_err;

out_ok:
    action->job.retc = SNAP_RETC_SUCCESS;
    return 0;

out_err:
    action->job.retc = SNAP_RETC_FAILURE;

    return 0;
}

static struct snap_sim_action action = {
    .vendor_id = SNAP_VENDOR_ID_ANY,
    .device_id = SNAP_DEVICE_ID_ANY,
    .action_type = BFS_NVME_ACTION_TYPE,

    .job = { .retc = SNAP_RETC_FAILURE, },
    .state = ACTION_IDLE,
    .main = action_main,
    .priv_data = NULL,	/* this is passed back as void *card */
    .mmio_write32 = mmio_write32,
    .mmio_read32 = mmio_read32,

    .next = NULL,
};

static void _init(void) __attribute__((constructor));

static void _init(void)
{
    snap_action_register(&action);
}
