#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ap_int.h"
#include "action_mc_hls.h"
#include <hls_stream.h>

#define NUM_ENG 50
#define RELEASE_LEVEL       0x00000010

static int do_rand_f(int *seed)
{
#pragma HLS PIPELINE
#pragma HLS INLINE
    int hi, lo, xx;
    
    xx = (*seed % 0x7ffffffe) + 1;
    hi = xx / 127773;
    lo = xx % 127773;
    xx = 16807 * lo - 2836 * hi;
    if (xx < 0)
        xx += 0x7fffffff;
    xx--;
    *seed = xx;
    return (xx);
}

static int next = 1;

int rand_f(void)
{
#pragma HLS PIPELINE
#pragma HLS INLINE
    return (do_rand_f(&next));
}

/*void srand(unsigned seed)
 {
	next = seed;
 }*/

float max(float a, float b)
{
#pragma HLS PIPELINE
    return (a<b)?b:a;     // or: return comp(a,b)?b:a; for version (2)
}

float cal_unit(float gauss_bm, float mid_value, float S_adjust)
{
#pragma HLS PIPELINE
    float S_cur = 0.0;
    //float S_adjust = S * exp(T*(r-0.5*v*v));
    S_cur = S_adjust * exp(mid_value*gauss_bm);
    return S_cur;
}

//void gaussian_box_muller(int first_flag, int select_flag[0], int lfsr1[3], int lfsr2[3], float random_value[0])
void gaussian_box_muller(float gauss_bm[NUM_ENG])
{
#pragma HLS INLINE
    float num1 = 0.0;
    float num2 = 0.0;
    float euclid_sq = 0.0;
    int t;
    //int randnum1 = 0;
    //int randnum2 = 0; 
    //randnum1 = rand_f();
    //randnum2 = rand_f();     
gaussian_box_muller_label10:for (t=0; t<NUM_ENG; t++)
{
gaussian_box_muller_label11:do {
//#pragma HLS loop_tripcount min=1 max=5 avg=3
    num1 = 2.0 * (float)rand_f() / (float)(RAND_MAX)-1;
    num2 = 2.0 * (float)rand_f() / (float)(RAND_MAX)-1;
    //printf("x: %f y: %f\n", x, y);
    euclid_sq = num1*num1 + num2*num2;
} while (euclid_sq >= 1.0);
    
    gauss_bm[t]= num1*sqrtf(-2*logf(euclid_sq)/euclid_sq);
}
}

float calc_parall(float Tmp[NUM_ENG])
{
    int i;
    float payoff_sum = 0.0;
    calc_parall_label0:for (i=0; i<NUM_ENG; i++)
    {
#pragma HLS UNROLL
#pragma HLS PIPELINE
        payoff_sum = payoff_sum + Tmp[i];
    }
    return payoff_sum;
}


//double MC(double gauss_bm[NUM_ENG], double S, double K, double r, double v, double T, int cp_flag)
//float MC(float gauss_bm[NUM_ENG], int cp_flag)
float MC(int cp_flag, float gauss_bm[NUM_ENG], float mid_value, float S_adjust, float K)
//float MC(int cp_flag, float mid_value, float S_adjust)
{
#pragma HLS PIPELINE
    int i, t;
    float Tmp[NUM_ENG];
    float payoff_sum = 0.0;
#pragma HLS ARRAY_PARTITION variable=gauss_bm complete dim=1
#pragma HLS ARRAY_PARTITION variable=Tmp complete dim=1     
MC_label1:for (t=0; t<NUM_ENG; t++)
{
#pragma HLS UNROLL
#pragma HLS PIPELINE 
    if(cp_flag == 1)
    {
        Tmp[t] = max((cal_unit(gauss_bm[t], mid_value, S_adjust) - K), 0.0);
    }
    else
    {
        Tmp[t] = max((K - cal_unit(gauss_bm[t], mid_value, S_adjust)), 0.0);
    }
}
    payoff_sum = calc_parall(Tmp);
    return payoff_sum;
}

//Pricing a European vanilla call option with a Monte Carlo method
//void monte_carlo_price(double sum, double gauss_bm[NUM_ENG], double S, double K, double r, double v, double T, int cp_flag)
//void monte_carlo_price(float gauss_bm[NUM_ENG], int cp_flag, float sum[0])
//void monte_carlo_price(int cp_flag, float gauss_bm[NUM_ENG],float sum[0])
/*void monte_carlo_price(int cp_flag, int num_sims[0], float S[0], float K[0], float r[0], float v[0], float T[0], float sum[0])
{
    //my_srandom(seed);
    float payoff_sum = 0.0;
    float pre_payoff_sum = 0.0;
    float gauss_bm[NUM_ENG];
    float mid_value1 = exp(sqrt(v[0]*v[0]*T[0]));
    float S_adjust = S[0] * exp(T[0]*(r[0]-0.5*v[0]*v[0]));
    float mid_value2 = exp(-r[0]*T[0]);
    float para = K[0];
    int cnt = num_sims[0];
    int i;
    
    for(i=0; i<cnt;i=i+100)
    {
        gaussian_box_muller(gauss_bm);
        pre_payoff_sum = MC(cp_flag, gauss_bm, mid_value1, S_adjust, para);
        payoff_sum = pre_payoff_sum + payoff_sum;
        //cnt -= 100;
    }
    //payoff_sum = MC(cp_flag, mid_value1, S_adjust);
    //printf("%f \n", payoff_sum / (double)(NUM_ENG));
    //printf("%f \n", (payoff_sum / (double)(NUM_ENG))* exp(-r*T));
    sum[0] = (payoff_sum / (float)(cnt)) * mid_value2;
    //return sum;
}*/

/*static snapu32_t write_bulk (snap_membus_t *tgt_mem,
                             snapu64_t      byte_address,
                             snapu32_t      byte_to_transfer,
                             snap_membus_t *buffer)
{
    snapu32_t xfer_size;
    xfer_size = MIN(byte_to_transfer, (snapu32_t)  MAX_NB_OF_BYTES_READ);
    memcpy((snap_membus_t *)(tgt_mem + (byte_address >> ADDR_RIGHT_SHIFT)), buffer, xfer_size);
    return xfer_size;
}

void write_out_buf (snap_membus_t  * tgt_mem, snapu64_t address, float * out_buf)
{
    snap_membus_t lines;
    //Convert it to one cacheline.
    lines(31,0) = out_buf[0];
    lines(63,32) = 0;
    lines(95,64) = 0;
    lines(127,96) = 0;
    lines(159,128) = 0;
    lines(191,160) = 0;
    lines(223,192) = 0;
    lines(255,224) = 0;
    lines(287,256) = 0;
    lines(319,288) = 0;
    lines(351,320) = 0;
    lines(383,352) = 0;
    lines(415,384) = 0;
    lines(447,416) = 0;
    lines(479,448) = 0;
    lines(511,480) = 0;
    
    write_bulk(tgt_mem, address, BPERCL, &lines);
}*/

void hls_action(snap_membus_t  *din_gmem,
                snap_membus_t  *dout_gmem,
                //snap_membus_t  *d_ddrmem,
                action_reg            *Action_Register,
                action_RO_config_reg  *Action_Config)
{
    // Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE m_axi port=dout_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg 		offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg 		offset=0x040
    
    //DDR memory Interface
//#pragma HLS INTERFACE m_axi port=d_ddrmem bundle=card_mem0 offset=slave depth=512
//#pragma HLS INTERFACE s_axilite port=d_ddrmem bundle=ctrl_reg 		offset=0x050
    
    // Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg	offset=0x010
#pragma HLS DATA_PACK variable=Action_Register
#pragma HLS INTERFACE s_axilite port=Action_Register bundle=ctrl_reg	offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg
    
    short rc = 0;
    snapu32_t result_size=0;
    //snapu64_t input_address;
    //snapu64_t output_address;
    snapu32_t ReturnCode = SNAP_RETC_SUCCESS;
    
    // byte address received need to be aligned with port width
    //input_address  = Action_Register->Data.in.addr;
    //output_address = Action_Register->Data.out.addr;

    
    /* Required Action Type Detection */
    switch (Action_Register->Control.flags) {
        case 0:
            Action_Config->action_type = (snapu32_t)MC_ACTION_TYPE;
            Action_Config->release_level = (snapu32_t)RELEASE_LEVEL;
            Action_Register->Control.Retc = (snapu32_t)0xe00f;
            return;
        default:
            break;
    }

    next = Action_Register->Data.seed;
    //my_srandom(seed);
    // First we create the parameter list
    int num_sims = 200;   // Number of simulated asset paths
    float S = 100.0;  // Option price
    float K = 100.0;  // Strike price
    float r = 0.05;   // Risk-free rate (5%)
    float v = 0.2;    // Volatility of the underlying (20%)
    float T = 1.0;    // One year until expiry
    float sum = 0.0;
    int cp_flag =1;

    /*int num_sims = Action_Register->Data.NUM_RUNS;   // Number of simulated asset paths
    float S = Action_Register->Data.S;  // Option price
    float K = Action_Register->Data.K;  // Strike price
    float r = Action_Register->Data.r;   // Risk-free rate (5%)
    float v = Action_Register->Data.v;    // Volatility of the underlying (20%)
    float T = Action_Register->Data.T;    // One year until expiry
    int cp_flag = Action_Register->Data.cp_flag;
    float sum = 0.0;*/
    
    float payoff_sum = 0.0;
    float pre_payoff_sum = 0.0;
    float gauss_bm[NUM_ENG];
    float mid_value1 = sqrt(v*v*T);
    float S_adjust = S * exp(T*(r-0.5*v*v));
    float mid_value2 = exp(-r*T);
    float para = K;
    int cnt = num_sims;
    int i;
   
    for(i=0; i<cnt;i=i+NUM_ENG)
    {
        gaussian_box_muller(gauss_bm);
        pre_payoff_sum = MC(cp_flag, gauss_bm, mid_value1, S_adjust, para);
        payoff_sum = pre_payoff_sum + payoff_sum;
        //cnt -= 100;
    }
    //payoff_sum = MC(cp_flag, mid_value1, S_adjust);
    //printf("%f \n", payoff_sum / (double)(NUM_ENG));
    //printf("%f \n", (payoff_sum / (double)(NUM_ENG))* exp(-r*T));
    sum = (payoff_sum / (float)(cnt)) * mid_value2;
    //return sum;
    Action_Register->Data.result = sum;
    //write_out_buf(dout_gmem,output_address, &sum);
    if (rc != 0)
        ReturnCode = SNAP_RETC_FAILURE;
    
    Action_Register->Control.Retc = ReturnCode;
    return;
    
}

