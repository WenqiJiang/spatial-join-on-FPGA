#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

void pure_write(
    // input
    long page_entry_num, // number of entries per page
    long page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
    // output
    ap_uint<64>* out_intersect) {

    ap_uint<64> result;
    for (long i = 0; i < page_pair_num * page_entry_num * page_entry_num; i++)  {
#pragma HLS pipeline II=1
        out_intersect[i] = result;
    }
}

extern "C" {

void vadd(  
    int page_entry_num, // number of entries per page
    int page_num_A, // number of pages for input A
    int page_num_B, // number of pages for input B
    // in runtime (should from DRAM)
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    // out format: the first number writes total intersection count, 
    //   while the rest are intersect ID pairs
    ap_uint<64>* out_intersect 
    )
{
// Share the same AXI interface with several control signals (but they are not allowed in same dataflow)
//    https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-AXI4-Burst-Behavior

// in runtime (should from DRAM)
#pragma HLS INTERFACE m_axi port=in_pages_A offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=in_pages_B offset=slave bundle=gmem1

// out
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem2 num_write_outstanding=32 max_write_burst_length=16

#pragma HLS dataflow

    long page_pair_num = ((long) page_num_A) * ((long) page_num_B);

    pure_write(
        // input
        page_entry_num, // number of entries per page
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        // output
        out_intersect);

}

}
