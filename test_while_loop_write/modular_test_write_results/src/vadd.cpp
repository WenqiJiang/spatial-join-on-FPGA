#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

void result_generator(
    // input
    int page_entry_num, // number of entries per page
    int page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
    // output
    // hls::stream<ap_uint<64> >& s_intersect_count, // per page pair
    hls::stream<int>& s_join_finish, // one single signal after everything is written
    hls::stream<result_t>& s_result_pair) {

    result_t result;
    for (int i = 0; i < page_pair_num; i++) {
        for (int j = 0; j < page_entry_num * page_entry_num; j++) {
#pragma HLS pipeline II=1
            s_result_pair.write(result);
        }
    }
    s_join_finish.write(1);
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
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem2

#pragma HLS dataflow


    hls::stream<result_t> s_result_pair;
#pragma HLS stream variable=s_result_pair depth=512

    hls::stream<int> s_join_finish; 
#pragma HLS stream variable=s_join_finish depth=2

//     hls::stream<ap_uint<64> > s_intersect_count; 
// #pragma HLS stream variable=s_intersect_count depth=512

    int page_pair_num = page_num_A * page_num_B;
    result_generator(
        // input
        page_entry_num, // number of entries per page
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        // output
        // hls::stream<ap_uint<64> >& s_intersect_count, // per page pair
        s_join_finish, // one single signal after everything is written
        s_result_pair);

    write_results(
        // input
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        s_join_finish,  // per page pair
        s_result_pair, 
        // out format: the first number writes total intersection count, 
        //   while the rest are intersect ID pairs
        out_intersect);    
}

}
