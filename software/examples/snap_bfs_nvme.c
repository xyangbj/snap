/*
 * Simple Breadth-first-search in C
 *
 * Use Adjacency list to describe a graph:
 *        https://en.wikipedia.org/wiki/Adjacency_list
 *
 * Wikipedia's pages are based on "CC BY-SA 3.0"
 * Creative Commons Attribution-ShareAlike License 3.0
 * https://creativecommons.org/licenses/by-sa/3.0/
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <action_bfs_nvme.h>
#include <snap_hls_if.h>


/*
 * BFS: breadth first search
 *    Simple demo to traverse a graph stored in adjcent table.
 *
 *    A directed graph with vertexs (or called node) and edges (or called arc)
 *    The adjacent table format is:
 *    vertex_list[0]   -> {edge, vertex_index} -> {edge, vertex_index} -> ... -> NULL
 *    vertex_list[1]   -> {edge, vertex_index} -> {edge, vertex_index} -> ... -> NULL
 *    ...
 *    vertex_list[N-1] -> {edge, vertex_index} -> {edge, vertex_index} -> ... -> NULL
 *
 * Function:
 *    Starting from each vertex node (called 'root'),
 *      and search all of the vertexes that it can reach.
 *      Visited nodes are recorded in obuf.
 *
 * Implementation:
 *    We ask FPGA to visit the host memory to traverse this data structure.
 *    1. We need to set a BFS_ACTION_TYPE, this is the ACTION ID.
 *    2. We need to fill in 108 bytes configuration space.
 *    Host will send this field to FPGA via MMIO-32.
 *          This field is completely user defined. see 'bfs_job_t'
 *    3. Call snap APIs
 *
 * Notes:
 *    When 'timeout' is reached, PSLSE will send ha_jcom=LLCMD (0x45) and uncompleted transactions will be killed.
 *
 */

static const char *version = GIT_VERSION;
int verbose_flag = 0;
static void usage(const char *prog)
{
    printf("Usage: %s [-h] [-v, --verbose] [-V, --version]\n"
            "  -C, --card <cardno> can be (0...3)\n"
            "  -i, --input_file <graph.txt>       Input graph file. (Not Available Now!!!) \n"
            "  -o, --output_file <traverse.bin>   Output traverse result file.\n"
            "  -t, --timeout <seconds>       When graph is large, need to enlarge it.\n"
            "  -n, --rand_nodes <N>          Generate a random graph with the number\n"
            "  -s, --start_root <num>        Traverse starting node index [0...N-1], default 0\n"
            "  -v, --verbose                 Show more information on screen.\n"
            "                                Automatically turned off when vertex number > 20\n"
            "  -V, --version                 Git version\n"
            "  -I, --irq                     Enable Interrupts\n"
            "\n"
            "Example:\n"
            "  snap_bfs_nvme   (Traverse a small sample graph and show result on screen)\n"
            "\n",
            prog);
}



/*---------------------------------------------------
 *       Create Adjacent Table
 *---------------------------------------------------*/
//static int create_file_graph( /*AdjList * adj, const char * input_file*/)
//{
//    int rc = 0;
////    printf("input_file is %s\n", input_file);
//    return rc;
//}





static int create_graph_data(NodeProperty_t * np_array, RelProperty_t * rp_array,
        uint64_t node_num, uint64_t *rel_num, uint64_t max_rel_num)
{
    int rc = 0;
    uint64_t i, j;
    short k;

    // Initialize the NodeProperty with some random data
    // Data!
    for (i = 0; i < node_num; i++) {

        //Example data
        for (k = 0; k < NAME_LENGTH; k++)
            np_array[i].name[k] = 'A' + rand()%26;
        np_array[i].name[++k] = '\0';
        np_array[i].age = rand()%100;

    }

    // For each node, decide the "fanout" and create relationships (edges)
    // 
    uint64_t second, fanout;
    uint64_t e;
    for (i = 0; i < node_num; i++) {
        fanout = (rand() % (node_num/2)); //Example setting!!
                                          //Be careful at RAND_MAX in stdlib.h
                                          //FIXME

        for (j = 0; j < fanout; j++) {

            if (e >= max_rel_num) //Stop if max_rel_num is hit
                break;

            do {
                second = rand()%node_num; //Randomly picked up 
                                          //Be careful at RAND_MAX in stdlib.h
                                          //FIXME
            } while (second == i); 
            //An edge to itself is not allowed
            //But duplicated edge between two nodes exist, have not been handled.

            //Relationship Property
            rp_array[e].first = i;
            rp_array[e].second = second;

            //Example data
            for (k = 0; k < NAME_LENGTH; k++)
                rp_array[e].relationship[k] = 'a' + rand()%26;
            rp_array[e].relationship[++k] = '\0';
            
            e++;
        }
    }

    *rel_num = e;
    printf("Create random graph data done.\n");

    return rc;
}

static int create_graph_records(RelProperty_t * rp_array,
        NodeRecord_t * nr_list, RelRecord_t * rr_list,
        NodeRecordFmt_t * nr_list_fmt, RelRecordFmt_t * rr_list_fmt,
        uint64_t node_num, uint64_t rel_num)
{
    
    int rc = 0;
    uint64_t i;
    uint64_t first;
    short k;
    
    //Initialize 
    for (i = 0; i < node_num; i++) {
        nr_list[i].next_rel = END_SIGN;
        nr_list[i].next_prop = i;
        nr_list[i].last_rel = END_SIGN;
    }

    //Go through rp_array
    //Only implementint the starting chain. 
    //TODO 
    for (i = 0; i < rel_num; i++) {

        first = rp_array[i].first;
        rr_list[i].first = rp_array[i].first;
        rr_list[i].second = rp_array[i].second;

        if (nr_list[first].last_rel == END_SIGN) {
            //first edge in this chain
            nr_list[first].next_rel = i;
            nr_list[first].last_rel = i;
            rr_list[i].second_prev = END_SIGN;
            rr_list[i].second_next = END_SIGN;
        } else {
            //append to the tail
            rr_list[nr_list[first].last_rel].second_next = i;
            rr_list[i].second_prev = nr_list[first].last_rel;
            //i becomes the new tail
            rr_list[i].second_next = END_SIGN;
            nr_list[first].last_rel = i;
        }
    }

    ////////////////////////////////////////
    //Pack to Neo4j standard format
    // Node
    for (i = 0; i < node_num; i++) {
        nr_list_fmt[i].in_use = (((nr_list[i].next_prop >>32) & 0xF) << 4) |
                                (((nr_list[i].next_rel >>32) & 0x7) << 1)  |
                                0;
        nr_list_fmt[i].next_rel_id = (nr_list[i].next_rel & 0xFFFFFFFF);
        nr_list_fmt[i].next_prop_id = (nr_list[i].next_prop & 0xFFFFFFFF);
        
        for (k = 0; k < 4; k++) {
            nr_list_fmt[i].labels[k] = '0' + rand()%10;
        }
        nr_list_fmt[i].labels[k] = '\0';
        nr_list_fmt[i].extra = 0;
    }

    //Edge/Releationship
    for (i = 0; i < rel_num; i++) {
        rr_list_fmt[i].in_use = (((rr_list[i].next_prop >> 32) & 0xF) << 4) |
                                (((rr_list[i].first >>32 ) & 0x7) << 1) | 
                                0; 
        rr_list_fmt[i].rel_type = (((rr_list[i].second >> 32) & 0x7) << 28) |
                                  (((rr_list[i].first_prev >> 32) & 0x7) << 25) |
                                  (((rr_list[i].first_next >> 32) & 0x7) << 22) |
                                  (((rr_list[i].second_prev >> 32) & 0x7) << 19) |
                                  (((rr_list[i].second_next >> 32) & 0x7) << 16) |
                                  0; //type
        rr_list_fmt[i].first_node  = (rr_list[i].first & 0xFFFFFFFF);
        rr_list_fmt[i].second_node = (rr_list[i].second & 0xFFFFFFFF);
        rr_list_fmt[i].first_prev_rel_id = (rr_list[i].first_prev & 0xFFFFFFFF);
        rr_list_fmt[i].first_next_rel_id = (rr_list[i].first_next & 0xFFFFFFFF);
        rr_list_fmt[i].second_prev_rel_id = (rr_list[i].second_prev & 0xFFFFFFFF);
        rr_list_fmt[i].second_next_rel_id = (rr_list[i].second_next & 0xFFFFFFFF);

        rr_list_fmt[i].next_prop_id = (rr_list[i].next_prop & 0xFFFFFFFF);
        rr_list_fmt[i].extra = 1; //firstInStartNodeChain
    }
    return rc; 

}

static void print_graph(NodeProperty_t *np_array, 
        NodeRecord_t *nr_list, RelRecord_t *rr_list, uint64_t node_num)
{
    if (node_num > 20) {
        printf ("Will not print for a too big graph. Return..\n");
        return;
    }

    uint64_t i;
    uint64_t rel; //Relationship ID
    for (i = 0; i < node_num; i++) {
        
        rel = nr_list[i].next_rel;

        if(rel != END_SIGN)
            printf("Node %ld (name=%s), connects edge %ld\n", i, np_array[i].name, rel);
        else
            printf("Node %ld (name=%s), connects to END_SIGN\n", i, np_array[i].name);


        while(rel != END_SIGN) {
            printf("       links to edge %3ld (Node %3ld to %3ld) {Relationship Prev %3ld, Next %3ld}.\n",
                     rel, rr_list[rel].first,  rr_list[rel].second, 
                     rr_list[rel].second_prev, rr_list[rel].second_next);
            rel = rr_list[rel].second_next;
        }
    }
}




/*---------------------------------------------------
 *       Delete Adjacent Table when exit
 *---------------------------------------------------*/
static void destroy_graph(NodeProperty_t *np_array,  RelProperty_t *rp_array,
        NodeRecord_t * nr_list, RelRecord_t *rr_list,
        NodeRecordFmt_t * nr_list_fmt, RelRecordFmt_t *rr_list_fmt)
{

    free(np_array);
    free(rp_array);
    free(nr_list);
    free(rr_list);
    free(nr_list_fmt);
    free(rr_list_fmt);
}


/*---------------------------------------------------
 *       Hook Configuration
 *---------------------------------------------------*/

static void snap_prepare_bfs(struct snap_job *job,
        bfs_job_t *bjob_in,
        bfs_job_t *bjob_out,
        uint64_t nr_list,
        uint64_t rr_list,
        
        uint64_t np_array, //Do we need to copy them from Host to NVMe?
        uint64_t rp_array,  //Do we need to copy them from Host to NVMe?

        uint64_t node_num,
        uint64_t rel_num,
        uint32_t step,
        uint64_t start_root,
        uint64_t qualifier, 

        void *addr_out,
        uint16_t type_out)
{

    fprintf(stdout, "----------------  Config Space ----------- \n");
    fprintf(stdout, "output_address = %p\n", addr_out);
    fprintf(stdout, "graph vertex number = %ld\n", node_num);
    fprintf(stdout, "graph edge number = %ld\n", rel_num);
    fprintf(stdout, "start BFS traversing at %ld\n", start_root);
    fprintf(stdout, "qualifier = %ld\n", qualifier);
    fprintf(stdout, "------------------------------------------ \n");


    snap_addr_set(&bjob_in->output_traverse, addr_out, 0,
            type_out, SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_DST | SNAP_ADDRFLAG_END );

    bjob_in->NodeRecord_host = nr_list;
    bjob_in->RelRecord_host  = rr_list;
    bjob_in->NodeProperty_host = np_array;
    bjob_in->RelProperty_host = rp_array;
    bjob_in->node_num = node_num;
    bjob_in->rel_num = rel_num;


    bjob_in->step = step;
    bjob_in->start_root = start_root;

    bjob_in->qualifier = qualifier;

    bjob_in->status_pos = 0;

    snap_job_set(job, bjob_in, sizeof(*bjob_in),
            bjob_out, sizeof(*bjob_out));
}

static int run_one_step(struct snap_action *action,
        struct snap_job *cjob,
        unsigned long timeout,
        uint64_t step)
{
    int rc;
    struct timeval etime, stime;

    gettimeofday(&stime, NULL);
    rc = snap_action_sync_execute_job(action, cjob, timeout);
    if (rc != 0) {
        fprintf(stderr, "err: job execution %d: %s!\n\n\n", rc,
                strerror(errno));
        return rc;
    }
    gettimeofday(&etime, NULL);
    fprintf(stdout, "Step %ld took %lld usec\n",
            step, (long long)timediff_usec(&etime, &stime));

    fprintf(stdout, "Step %ld RETC=%x\n", step, cjob->retc);
    return rc;
}

/*---------------------------------------------------
 *       MAIN
 *---------------------------------------------------*/
int main(int argc, char *argv[])
{
    //General variables for snap call
    int ch;
    int rc = 0;
    int card_no = 0;
    struct snap_card *card = NULL;
    struct snap_action *action = NULL;
    char device[128];
    struct snap_job cjob;
    uint32_t page_size = sysconf(_SC_PAGESIZE);
    int exit_code = EXIT_SUCCESS;
    snap_action_flag_t action_irq = 0;


    unsigned long timeout = 10000;
    const char *input_file = NULL;
    const char *output_file = NULL;
    uint64_t node_n, rel_n, max_rel_n, start_root;
    uint64_t qualifier;

    node_n  = 6;
    start_root = 0;
    qualifier = 0; //No qualifier;
    //Otherwise should be 8 chars, and in "000????0"

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "card",	 required_argument, NULL, 'C' },
            { "input_file",	 required_argument, NULL, 'i' },
            { "output_file", required_argument, NULL, 'o' },
            { "rand_nodes",	 required_argument, NULL, 'n' },
            { "start_root",	 required_argument, NULL, 's' },
            { "timeout",	 required_argument, NULL, 't' },
            { "version",	 no_argument,	    NULL, 'V' },
            { "verbose",	 no_argument,	    NULL, 'v' },
            { "help",	 no_argument,	    NULL, 'h' },
            { "irq",	 no_argument,	    NULL, 'I' },
            { 0,		 no_argument,	    NULL, 0   },
        };

        ch = getopt_long(argc, argv,
                "C:i:o:t:n:s:VvhI",
                long_options, &option_index);
        if (ch == -1)	/* all params processed ? */
            break;

        switch (ch) {
            /* which card to use */
            case 'C':
                card_no = strtol(optarg, (char **)NULL, 0);
                break;
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 't':
                timeout = strtol(optarg, (char **)NULL, 0);
                break;
            case 'V':
                printf("%s\n", version);
                exit(EXIT_SUCCESS);
            case 'v':
                verbose_flag++;
                break;
            case 'n':
                node_n = strtol(optarg, (char **)NULL, 0);
                break;
            case 's':
                start_root = strtol(optarg, (char **)NULL, 0);
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case 'I':	/* irq */
		action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind != argc) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }



    //Action specfic
    bfs_job_t bjob_in;
    bfs_job_t bjob_out;

    //Output buffer
    uint8_t type_out = SNAP_ADDRTYPE_HOST_DRAM;
    uint32_t * obuf = 0x0ull;
    uint32_t nodes_out;
    uint32_t i, j, k;
    FILE *ofp;


    //////////////////////////////////////////////////////////////////////
    // Construct the graph, and set to ibuf.
    NodeProperty_t * np_array;
    RelProperty_t * rp_array;
    NodeRecord_t * nr_list;
    RelRecord_t * rr_list;
    NodeRecordFmt_t * nr_list_fmt;
    RelRecordFmt_t * rr_list_fmt;


    fprintf(stdout, "DEBUG: page_size is %d\n", page_size);
    fprintf(stdout, "DEBUG: timeout is %ld\n",timeout);

    fprintf(stdout, "input_file is %s\n", input_file);
    //if(input_file != NULL)
    //    rc = create_file_graph (/*&adj, input_file*/); // TODO dummy function
    //else

    max_rel_n = node_n * (node_n - 1)/2; //We set a limit

    np_array = memalign(page_size, node_n * sizeof(NodeProperty_t));
    rp_array = memalign(page_size, max_rel_n * sizeof(RelProperty_t));
    if (np_array == NULL || rp_array == NULL) {
        rc = -1;
        return rc;
    }

    rc = create_graph_data(np_array, rp_array, node_n, &rel_n, max_rel_n);
    printf("node_num = %ld, rel_num = %ld\n", node_n, rel_n);

    nr_list = memalign(CACHELINE_BYTES, node_n * sizeof(NodeRecord_t));
    rr_list = memalign(CACHELINE_BYTES, rel_n * sizeof(RelRecord_t));
    nr_list_fmt = memalign(CACHELINE_BYTES, node_n * sizeof(NodeRecordFmt_t));
    rr_list_fmt = memalign(CACHELINE_BYTES, rel_n * sizeof(RelRecordFmt_t));
    if (nr_list == NULL || rr_list == NULL || nr_list_fmt == NULL || rr_list_fmt == NULL) {
        printf("Failed to malloc\n");
        rc = -1;
        return rc;
    }

    rc = create_graph_records(rp_array, nr_list, rr_list, nr_list_fmt, rr_list_fmt, node_n, rel_n);
    print_graph(np_array, nr_list, rr_list, node_n);

    if(rc < 0)
        goto out_error;



    // create obuf
    // obuf is 1024bit  aligned.
    // Format:
    // 1024b: Root: | {visit_node}, {visit_node}, .............................{visit_node} |
    // 1024b:       | {visit_node}, {visit_node}, ....,  {FF....cnt}, {dummy}, ..., {dummy} |
    //
    // Each {} is uint32_t, can fill 32 nodes in a row.

    nodes_out = (node_n/32+1)*32;
    //nodes_out = node_n * (node_n/32+1)*32;
    printf("nodes out space = %d nodes. \n", nodes_out);
    obuf = memalign(page_size, sizeof(uint32_t) * nodes_out);


    //////////////////////////////////////////////////////////////////////
    //     Start Working Flow
    //////////////////////////////////////////////////////////////////////

    fprintf(stdout, "snap_kernel_attach start...\n");

    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);
    card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM,
            SNAP_DEVICE_ID_SNAP);
    if (card == NULL) {
        fprintf(stderr, "err: failed to open card %u: %s\n",
                card_no, strerror(errno));
        goto out_error;
    }

    action = snap_attach_action(card, BFS_NVME_ACTION_TYPE, action_irq, 60);
    if (action == NULL) {
        fprintf(stderr, "err: failed to attach action %u: %s\n",
                card_no, strerror(errno));
        goto out_error1;
    }

    
    uint32_t step;

    // Step1: Copy Host Data to NVMe storage
    step = 1;
    snap_prepare_bfs(&cjob, &bjob_in, &bjob_out,
        (unsigned long)nr_list_fmt, 
        (unsigned long)rr_list_fmt,
        (unsigned long)np_array,
        (unsigned long)rp_array,
        node_n,
        rel_n,
        step,
        start_root,
        qualifier,
        obuf, type_out);


    rc |= run_one_step(action, &cjob, timeout, step);
    if (rc != 0)
        goto out_error2;

    // Step2: Doing BFS from Storage
    step = 2;
    snap_prepare_bfs(&cjob, &bjob_in, &bjob_out,
        (unsigned long)nr_list_fmt, 
        (unsigned long)rr_list_fmt,
        (unsigned long)np_array,
        (unsigned long)rp_array,
        node_n,
        rel_n,
        step,
        start_root,
        qualifier,
        obuf, type_out);

    rc |= run_one_step(action, &cjob, timeout, step);
    if (rc != 0)
        goto out_error2;



    //////////////////////////////////////////////////////////////////////
    //     Output BFS results
    //////////////////////////////////////////////////////////////////////
    //TODO: still old style

    fprintf(stdout, "------------------------------------------ \n");
    fprintf(stdout, "Write out position to 0x%x\n", bjob_out.status_pos);

    if(output_file == NULL ) {
        //print on screen

        i = 0;  //uint32 count
        j = 0;  //vertex    index
        fprintf(stdout, "Visiting node (%d): ", j);

        while(i < nodes_out) {
            k = obuf[i];

            //End sign is {FF....cnt} in a word.
            if((k>>24) == 0xFF) {
                fprintf (stdout, "End. Cnt = %d\n", (k&0x00FFFFFF));
                i = i + 32 - (i%32); //Skip following empty.
                j++;
                if(i < nodes_out) //For next node:
                    fprintf(stdout, "Visiting node (%d): ", j);

            }
            else {
                fprintf (stdout, "%d, ", k);
                i++;
            }
            if (i > 600) {
                fprintf(stdout, "\n .... will not print too many lines. Stop.\n");
                break;
            }

        }
    }
    else {
        //output into file
        fprintf(stdout, "Output to file %s\n", output_file);
        ofp = fopen(output_file, "w+");
        if(!ofp) {
            fprintf(stderr, "err: Cannot open file %s\n", output_file);
            goto out_error;
        }
        rc = fwrite(obuf, nodes_out, 4, ofp);
        if (rc < 0)
            goto out_error;
    }

    snap_detach_action(action);
    snap_card_free(card);
    free(obuf);
    destroy_graph(np_array, rp_array, nr_list, rr_list, nr_list_fmt, rr_list_fmt);
    exit(exit_code);

out_error2:
    snap_detach_action(action);
out_error1:
    snap_card_free(card);
out_error:
    destroy_graph(np_array, rp_array, nr_list, rr_list, nr_list_fmt, rr_list_fmt);
    free(obuf);
    exit(EXIT_FAILURE);
}
