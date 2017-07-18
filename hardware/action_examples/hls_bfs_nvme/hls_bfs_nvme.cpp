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
 */

/*
 * Adopt SNAP's framework for FPGA hardware action part.
 * Fit for Xilinx HLS compiling constraints.
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

/* Version
 * 2017/6/26    1.0  VertexData and EdgeData are stored in DDR now.
 */

#include <string.h>
#include "ap_int.h"
#include <hls_stream.h>
#include "action_bfs_nvme_hls.H"

#define HW_RELEASE_LEVEL       0x00000010


//--------------------------------------------------------------------------------------------
snapu32_t read_bulk ( snap_membus_t *src_mem,
        snapu64_t      byte_address,
        snapu32_t      byte_to_transfer,
        snap_membus_t *buffer)
{
    snapu32_t xfer_size;
    xfer_size = MIN(byte_to_transfer, (snapu32_t) MAX_NB_OF_BYTES_READ);
// Patch to Issue#320 - memcopy doesn't handle 4Kbytes xfer
    int xfer_size_in_words;
    if(xfer_size %BPERDW == 0)
        xfer_size_in_words = xfer_size/BPERDW;
    else
        xfer_size_in_words = (xfer_size/BPERDW) + 1;

    //memcpy(buffer,
    //       (snap_membus_t *) (src_mem + (byte_address >> ADDR_RIGHT_SHIFT)),
    //       xfer_size);
    rb_loop: for (int k=0; k< xfer_size_in_words; k++)
#pragma HLS PIPELINE
        buffer[k] = (src_mem + (byte_address >> ADDR_RIGHT_SHIFT))[k];
    
    return xfer_size;
}
void read_single (snap_membus_t * src_mem, snapu64_t byte_address, snap_membus_t * data)
{
#pragma HLS INLINE
    *data = (src_mem + (byte_address >> ADDR_RIGHT_SHIFT))[0];
}


snapu32_t write_bulk (snap_membus_t *tgt_mem,
        snapu64_t      byte_address,
        snapu32_t      byte_to_transfer,
        snap_membus_t *buffer)
{
    snapu32_t xfer_size;
    xfer_size = MIN(byte_to_transfer, (snapu32_t)  MAX_NB_OF_BYTES_READ);
    // Patch to Issue#320 - memcopy doesn't handle 4Kbytes xfer
    int xfer_size_in_words;
    if(xfer_size %BPERDW == 0)
        xfer_size_in_words = xfer_size/BPERDW;
    else
        xfer_size_in_words = (xfer_size/BPERDW) + 1;
    
    //memcpy((snap_membus_t *)(tgt_mem + (byte_address >> ADDR_RIGHT_SHIFT)), buffer, xfer_size);

    wb_loop: for (int k=0; k<xfer_size_in_words; k++)
#pragma HLS PIPELINE
          (tgt_mem + (byte_address >> ADDR_RIGHT_SHIFT))[k] = buffer[k];
    return xfer_size;
}
void write_single (snap_membus_t * tgt_mem, snapu64_t byte_address, snap_membus_t data)
{
#pragma HLS INLINE
    (tgt_mem + (byte_address >> ADDR_RIGHT_SHIFT))[0] = data;
}


//--------------------------------------------------------------------------------------------
void write_out_buf (snap_membus_t  * tgt_mem, snapu64_t address, snapu32_t * out_buf)
{
    snap_membus_t lines[2];
    //Convert it to one cacheline.
    lines[0](31,0) = out_buf[0];
    lines[0](63,32) = out_buf[1];
    lines[0](95,64) = out_buf[2];
    lines[0](127,96) = out_buf[3];
    lines[0](159,128) = out_buf[4];
    lines[0](191,160) = out_buf[5];
    lines[0](223,192) = out_buf[6];
    lines[0](255,224) = out_buf[7];
    lines[0](287,256) = out_buf[8];
    lines[0](319,288) = out_buf[9];
    lines[0](351,320) = out_buf[10];
    lines[0](383,352) = out_buf[11];
    lines[0](415,384) = out_buf[12];
    lines[0](447,416) = out_buf[13];
    lines[0](479,448) = out_buf[14];
    lines[0](511,480) = out_buf[15];
    lines[1](31,0) = out_buf[16];
    lines[1](63,32) = out_buf[17];
    lines[1](95,64) = out_buf[18];
    lines[1](127,96) = out_buf[19];
    lines[1](159,128) = out_buf[20];
    lines[1](191,160) = out_buf[21];
    lines[1](223,192) = out_buf[22];
    lines[1](255,224) = out_buf[23];
    lines[1](287,256) = out_buf[24];
    lines[1](319,288) = out_buf[25];
    lines[1](351,320) = out_buf[26];
    lines[1](383,352) = out_buf[27];
    lines[1](415,384) = out_buf[28];
    lines[1](447,416) = out_buf[29];
    lines[1](479,448) = out_buf[30];
    lines[1](511,480) = out_buf[31];

    write_bulk(tgt_mem, address, BPERCL, lines);
}
//--------------------------------------------------------------------------------------------
//--- MAIN PROGRAM ---------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void hls_action(snap_membus_t  *din_gmem, 
        snap_membus_t  *dout_gmem,
        snap_membus_t  *d_ddrmem,
        action_reg *Action_Register, action_RO_config_reg *Action_Config)
{
    // Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din_gmem bundle=host_mem offset=slave depth=512 \
    max_read_burst_length=32 max_write_burst_length=32
#pragma HLS INTERFACE m_axi port=dout_gmem bundle=host_mem offset=slave depth=512 \
    max_read_burst_length=32 max_write_burst_length=32
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg 		offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg 		offset=0x040

    //DDR memory Interface
#pragma HLS INTERFACE m_axi port=d_ddrmem bundle=card_mem0 offset=slave depth=512 \
    max_read_burst_length=32 max_write_burst_length=32
#pragma HLS INTERFACE s_axilite port=d_ddrmem bundle=ctrl_reg 		offset=0x050

    // Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg	offset=0x010
#pragma HLS DATA_PACK variable=Action_Register
#pragma HLS INTERFACE s_axilite port=Action_Register bundle=ctrl_reg	offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg


    snapu32_t step;
    
    // local RAMs
    // Need to calculate the size to fit into Block RAM limit
    ap_uint <1>             visited[MAX_NODE_NUM];
    ap_uint <REL_BITWIDTH>  nr_list_local[MAX_NODE_NUM];  //Only stores rel_id
    snap_membus_t           buf_rel_prefetch[PREFETCH_LINES];  //Speculative prefetch
    snap_membus_t           buf_gmem[MAX_NB_OF_BYTES_READ/BPERDW]; //Used to do memcopy

    ap_uint <REL_BITWIDTH>  bound1, bound2;
    //Variables
    snapu64_t nr_src_address, nr_tgt_address;
    snapu64_t rr_src_address, rr_tgt_address;
    snapu64_t fetch_address, commit_address;

    ap_uint<REL_BITWIDTH>  rel_num, rel, next_rel;
    ap_uint<NODE_BITWIDTH> node_num, nn, root, current, second;
    ap_uint<NODE_BITWIDTH> vnode_cnt, vnode_idx;
    ap_uint<2> kk; //For some membus split
        
    short buf_id;
    snap_membus_t one_rr;

    uint32_t read_bytes;
    uint32_t bytes_to_transfer;
    uint32_t xfer_offset; //Here cannot take use of full 8GB FIXME bug
    
    //OUTPUT  FIXME
    snapu32_t buf_out[32];   //To fill a cacheline and write to output_traverse.

    hls::stream <Q_t> Q;
#pragma HLS stream depth=65536 variable=Q
    //TODO Caution!!! pragma doesn't recognize MAX_NODE_NUM macro.



    /* Required Action Type Detection */
    switch (Action_Register->Control.flags) {
        case 0:
            Action_Config->action_type = (snapu32_t)BFS_NVME_ACTION_TYPE;
            Action_Config->release_level = (snapu32_t)HW_RELEASE_LEVEL;
            Action_Register->Control.Retc = (snapu32_t)0xe00f;
            return;
        default:
            break;
    }



    // Step1 Memcopy NodeRecord and RelRecord
    node_num     = Action_Register->Data.node_num;
    nr_src_address = Action_Register->Data.NodeRecord_host;
    nr_tgt_address = NodeRecord_ADDR_DDR; 

    rel_num       = Action_Register->Data.rel_num;
    rr_src_address = Action_Register->Data.RelRecord_host;
    rr_tgt_address = RelRecord_ADDR_DDR;
    
    commit_address = Action_Register->Data.output_traverse.addr;
    root           = Action_Register->Data.start_root;
    step           = Action_Register->Data.step;
    if(step(1,0) == 1) //For a better timing
    {

        if (node_num == 0 || node_num > MAX_NODE_NUM || root >= node_num) {
            //Illegal inputs
            //node_num = 0 meaningless.
            //Hardware cannot handle too many nodes in this particular impl.
            Action_Register->Control.Retc = SNAP_RETC_FAILURE;
            return;
        }


        //Copy NodeRecord_host
        //And fill nr_list_local
        //sizeof(NodeRecordFmt_t) = 16 
        bytes_to_transfer = node_num * sizeof(NodeRecordFmt_t);
        xfer_offset = 0;

        while (bytes_to_transfer > 0) {
            read_bytes = read_bulk  (din_gmem, nr_src_address + xfer_offset,  bytes_to_transfer, buf_gmem);

            for (nn = 0; nn < (read_bytes/16); nn ++) {
                kk = nn (1,0); 
                nr_list_local[xfer_offset/16 + nn] = buf_gmem [nn/4](39 + kk*128, 8 + kk*128);
                //HW doesn't support so many nodes! 32bits enough

                //Shall we handle labels[5] here also? 
                //And compare with qualifier. Marked not-matched as "visited"
            }
            write_bulk (d_ddrmem, nr_tgt_address + xfer_offset,  read_bytes, buf_gmem);
            xfer_offset += read_bytes;
            bytes_to_transfer -= read_bytes;
        }

        //Copy RelRecord
        //sizeof(NodeRecordFmt_t) = 64
        //happens to be equal to BPERDW
        bytes_to_transfer = rel_num * sizeof(RelRecordFmt_t);
        xfer_offset = 0;
        while (bytes_to_transfer > 0) {
            read_bytes = read_bulk  (din_gmem, rr_src_address + xfer_offset, bytes_to_transfer, buf_gmem);
            write_bulk (d_ddrmem, rr_tgt_address + xfer_offset,  read_bytes, buf_gmem);
            xfer_offset += read_bytes;
            bytes_to_transfer -= read_bytes;
        }
        
        Action_Register->Control.Retc = (snapu32_t) SNAP_RETC_SUCCESS;
    }
    else if (step(1,0) == 2)
    {
        // Traversing.
        // byte address received need to be aligned with port width

        //Starting traverse
        //Enqueue
        Q.write(root);

        for (nn = 0; nn < node_num; nn++) {
#pragma HLS UNROLL factor=128
            visited[nn] = 0;
        }

        // First fill output
        visited[root] = 1;
        buf_out[0]  = root;
        vnode_cnt   = 1;
        vnode_idx   = 1;

        bound1 = 0;
        bound2 = 0;
        while (!Q.empty())
        { 
            //Deque
            current = Q.read();
            next_rel = nr_list_local[current];

            while (next_rel != END_SIGN_HW) {

                fetch_address = RelRecord_ADDR_DDR + next_rel*sizeof(RelRecordFmt_t); //byte address!
                //Hit: fetch_address in [bound1, bound2)
                
                if( fetch_address < bound1 || fetch_address >= bound2 ) {
                    //miss, need a new read 
                    read_bulk(d_ddrmem, fetch_address, PREFETCH_LINES * BPERDW, buf_rel_prefetch);
                    bound1 = fetch_address;
                    bound2 = fetch_address + PREFETCH_LINES * BPERDW; 
                }

                buf_id = (fetch_address - bound1) >> ADDR_RIGHT_SHIFT ; //RelRecordFmt = BPERDW
                one_rr = buf_rel_prefetch[buf_id];

                second   = one_rr(71, 40);   //second_node, 32bits enough
                next_rel = one_rr(231, 200); //second_next_rel_id

                if(!visited[second]) {
                    visited[second] = 1;
                    
                    //Enqueue
                    Q.write(second);

                    //Output result
                    buf_out[vnode_idx] = second;
                    vnode_cnt ++;
                    vnode_idx ++;

                    //Commit buf_out if a cacheline is fulfilled
                    if((vnode_idx * sizeof(snapu32_t)) >= BPERCL) {
                        write_out_buf(dout_gmem, commit_address, buf_out);

                        vnode_idx = 0;
                        commit_address += BPERCL;
                    }
                }
            }
        }

        //Last output
        buf_out[vnode_idx] = 0xFF000000 + vnode_cnt; //0xFF is a mark of END.
        write_out_buf(dout_gmem, commit_address, buf_out);
        vnode_idx = 0;
        commit_address += BPERCL; //One cacheline

        //Update register
        Action_Register->Control.Retc         = SNAP_RETC_SUCCESS;
        Action_Register->Data.status_pos      = commit_address(31,0);
    }
    return;
}


