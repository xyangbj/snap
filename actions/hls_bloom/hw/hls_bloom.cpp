
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


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <hls_math.h>

#include "bloom.H"
#include "action_bloom.H"

/** ***************************************************************************
 * A few simple tests to check if it works at all.
 *
 */
//static int basic()
static int basic(struct bloom * bloom)
{
  printf("----- basic -----\n");

  //struct bloom bloom;
  char rc=0;

  /* test that entering an error equal to 1 is rejected */
//  assert(bloom_init(&bloom, 0, 1.0) == 1);
  rc |= (bloom_init(bloom, 0, 1.0) == 1 ? 0 : 1);
  /* test that entering an error equal to 0 is rejected */
//  assert(bloom_init(&bloom, 10, 0) == 1);
  rc |= (bloom_init(bloom, 10, 0) == 1 ? 0 : 1);
  /* test that bloom wasn't initialized yet */
//  assert(bloom.ready == 0);
  rc |= (bloom->ready == 0 ? 0 : 1);

  /* test that adding entries before init is rejected */
//  assert(bloom_add(&bloom, "hello world", 11) == -1);
  rc |= (bloom_add(bloom, "hello world", 11) == -1 ? 0 : 1);
  /* test that adding entries before init is rejected */
//  assert(bloom_check(&bloom, "hello world", 11) == -1);
  rc |= (bloom_check(bloom, "hello world", 11) == -1 ? 0 : 1);

//  bloom_free(&bloom);

//  assert(bloom_init(&bloom, 1002, 0.1) == 0);
  rc |= (bloom_init(bloom, 1002, 0.1) == 0 ? 0 : 1);

//  assert(bloom.ready == 1);
    rc |= (bloom->ready == 1 ? 0 : 1);

  bloom_print(bloom);

  /* check that "hello world" is NOT found - add it if not found*/
  //assert(bloom_check(&bloom, "hello world", 11) == 0);
  rc |= (bloom_check(bloom, "hello world", 11) == 0 ? 0 : 1);

  //assert(bloom_add(&bloom, "hello world", 11) == 0);
  rc |= (bloom_add(bloom, "hello world", 11) == 0 ? 0 : 1);

  /* check that "hello world" is found */
  //assert(bloom_check(&bloom, "hello world", 11) == 1);
  rc |= (bloom_check(bloom, "hello world", 11) == 1 ? 0 : 1);

  /* add once more "hello world" */
  //assert(bloom_add(&bloom, "hello world", 11) > 0);
  rc |= (bloom_add(bloom, "hello world", 11) > 0 ? 0 : 1);

  /* add twice just "hello" */
  //assert(bloom_add(&bloom, "hello", 5) == 0);
  rc |= (bloom_add(bloom, "hello", 5) == 0 ? 0 : 1);
  //assert(bloom_add(&bloom, "hello", 5) > 0);
  rc |= (bloom_add(bloom, "hello", 5) > 0 ? 0 : 1);

  /* check that "hello" is found */
  //assert(bloom_check(&bloom, "hello", 5) == 1);
  rc |= (bloom_check(bloom, "hello", 5) == 1 ? 0 : 1);
  //bloom_free(&bloom);

  return rc;
}
/** ***************************************************************************
 * Simple loop to compare performance.
 *
 */
//static int perf_loop(int entries, int count)
static int perf_loop(int entries, int count,
		unsigned int error_rate, struct bloom * bloom)
{
  char rc;
  unsigned int j;
  double error = 1.0;
  printf("----- perf_loop -----\n");

  //struct bloom bloom;
  //assert(bloom_init(&bloom, entries, 0.001) == 0);
  for(j = 0; j < error_rate; j++)
	  error = error / 10.;
  rc = (bloom_init(bloom, entries, error) == 0 ? 0 : 1);
  bloom_print(bloom);

  int i;
  int collisions = 0;

  //struct timeval tp;
  //gettimeofday(&tp, NULL);
  //long before = (tp.tv_sec * 1000L) + (tp.tv_usec / 1000L);

  for (i = 0; i < count; i++) {
//#pragma HLS UNROLL factor=16      // PARALLELIZATION FACTOR
    //if (bloom_add(&bloom, (const char *) &i, sizeof(int))) { collisions++; }
	char buffer[32];
	for (int k = 0; k < 32; k++)
#pragma HLS UNROLL
		buffer[k] = (char)((i) >> k*8);

	if (bloom_add(bloom, buffer, sizeof(int))) { collisions++; }
  }

  //gettimeofday(&tp, NULL);
  //long after = (tp.tv_sec * 1000L) + (tp.tv_usec / 1000L);

  printf("Added %d elements of size %d,  (collisions=%d)\n",
         count, (int)sizeof(int), collisions);

  printf("%d,%d\n", entries, bloom->bytes);

  bloom_print(bloom);
  bloom_free(bloom);

  return collisions;
}


//-----------------------------------------------------------------------------
//--- MAIN PROGRAM ------------------------------------------------------------
//-----------------------------------------------------------------------------

void process_action( action_reg *Action_Register)
{
	int rc = 1;
        uint32_t run_number;
        uint32_t entries, count;
        uint32_t collisions =0;
        uint32_t collisions_tmp =0;
        uint32_t error_rate, error;
        uint32_t nb_calls;
	struct bloom bloom;
        double double_error;

	switch(Action_Register->Data.test_choice) {
	case(BLOOM_SPEED):	// perf test
		nb_calls = Action_Register->Data.nb_calls;
		count = Action_Register->Data.count;
		entries = Action_Register->Data.entries;
		error_rate = Action_Register->Data.error_rate;

                for (run_number = 0; run_number < nb_calls; run_number++) {
                       uint64_t collisions_tmp;

           	       collisions_tmp = perf_loop(entries, count, error_rate, &bloom);
                       collisions += collisions_tmp;
                }

		rc = 0;
		break;
	case(BLOOM_BASIC): 	// basic tests
            	rc = basic(&bloom); // no parameters
		collisions = 0;
		break;

	default:
		rc = 1;
		break;
	}

     	Action_Register->Data.collisions = collisions;
  	Action_Register->Data.entries = bloom.entries;
	// Get log10 of bloom.error
        double_error = bloom.error;
	for(error = 0; error < 16; error++) {
		double_error = double_error * 10.;
		if(double_error == 1.) break;
	}
  	Action_Register->Data.error_rate = error;
  	Action_Register->Data.bits = bloom.bits;
  	Action_Register->Data.bytes = bloom.bytes;
  	Action_Register->Data.hashes = bloom.hashes;


   	/* Final output register writes */
    if (rc == 0) {
    	printf("Test completed OK !!\n");
    	Action_Register->Control.Retc = SNAP_RETC_SUCCESS;
    }
    else
    {
    	printf(" ==> Test completed with errors!! <==\n");
    	Action_Register->Control.Retc = SNAP_RETC_FAILURE;
    }

}


//--- INTERFACE LEVEL -------------------------------------------------
/**
 * Remarks: Using pointers for the din_gmem, ... parameters is requiring to
 * to set the depth=... parameter via the pragma below. If missing to do this
 * the cosimulation will not work, since the width of the interface cannot
 * be determined. Using an array din_gmem[...] works too to fix that.
 */
// This example doesn't use FPGA DDR.
// Need to set Environment Variable "SDRAM_USED=FALSE" before compilation.
void hls_action(snap_membus_t *din_gmem,
		    snap_membus_t *dout_gmem,
		    //snap_membus_t *d_ddrmem, 
		    action_reg *Action_Register,
		    action_RO_config_reg *Action_Config)
{

// Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE m_axi port=dout_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg           offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg          offset=0x040

//DDR memory Interface 
//#pragma HLS INTERFACE m_axi port=d_ddrmem bundle=card_mem0 offset=slave depth=512
//#pragma HLS INTERFACE s_axilite port=d_ddrmem bundle=ctrl_reg           offset=0x050

// Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg      offset=0x010
#pragma HLS DATA_PACK variable=Action_Register
#pragma HLS INTERFACE s_axilite port=Action_Register bundle=ctrl_reg    offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg

	/* Hardcoded numbers */
        switch (Action_Register->Control.flags) {
        case 0:
        	Action_Config->action_type   = (snapu32_t) BLOOM_ACTION_TYPE;
        	Action_Config->release_level = (snapu32_t) RELEASE_LEVEL;
        	Action_Register->Control.Retc = (snapu32_t)0xe00f;
        	return;
        	break;
        default:
        	Action_Register->Control.Retc = (snapu32_t)0x0;
        	process_action(Action_Register);
        	break;
        }

        return;
}

//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------


#ifdef NO_SYNTH

/**
 * FIXME We need to use hls_action from here to get the real thing
 * simulated. For now let's take the short path and try without it.
 *
 * Works only for the TEST set of parameters.
 */
int main(void)
{
	short i, j, rc=0;

	snap_membus_t din_gmem[1]; 	// Unused
	snap_membus_t dout_gmem[1];	// Unused
	//snap_membus_t d_ddrmem[1];	// Unused
	action_reg Action_Register;
	action_RO_config_reg Action_Config;


	// Get Config registers
	Action_Register.Control.flags = 0;
	hls_action(din_gmem, dout_gmem, //d_ddrmem,
			    &Action_Register, &Action_Config);

	// Process the action
	Action_Register.Control.flags = 1;

	//********SPEED TESTS*******
	Action_Register.Data.test_choice = BLOOM_SPEED; 	//speed test
	Action_Register.Data.nb_calls = 1;
	Action_Register.Data.count = 1000000;
	Action_Register.Data.entries = 1000000;
	Action_Register.Data.error_rate = 2; 	 // 2 means error = 0.01

	hls_action(din_gmem, dout_gmem, //d_ddrmem,
			    &Action_Register, &Action_Config);
	printf(" ==> 1 test call : collisions = %d\n",
			(long long) Action_Register.Data.collisions);

	printf(" ===============================================\n");
	//********BASIC TESTS*******
	Action_Register.Data.test_choice = BLOOM_BASIC; 	//basic test
	Action_Register.Data.nb_calls = 1;	// unused
	Action_Register.Data.entries = 1; 	// unused

	hls_action(din_gmem, dout_gmem, //d_ddrmem,
			    &Action_Register, &Action_Config);

	if (Action_Register.Control.Retc == SNAP_RETC_FAILURE) {
				printf(" ==> RETURN CODE FAILURE <==\n");
				return 1;
	}
	else
		printf(" ==> RETURN CODE OK <==\n");


	printf(">> ACTION TYPE = %8lx - RELEASE_LEVEL = %8lx <<\n",
			(unsigned int)Action_Config.action_type,
			(unsigned int)Action_Config.release_level);

	return rc;
}

#endif // end of NO_SYNTH flag
