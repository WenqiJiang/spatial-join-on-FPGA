#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

void input_generator(
    // input
    int page_entry_num, // number of entries per page
    int page_num_A, // number of pages for input A
    int page_num_B, // number of pages for input B
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    // output
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B
    ) {
    
    obj_t obj_A;
    obj_t obj_B;

    for (int a = 0; a < page_num_A; a++) {
        for (int b = 0; b < page_num_B; b++) {
            for (int i = 0; i < page_entry_num; i++) {
#pragma HLS pipeline II=1
                s_page_A.write(obj_A);
                s_page_B.write(obj_B);
            }
        }
    }
}

void result_consumer(
    // input
    int page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
    hls::stream<int>& s_join_finish,  // per page pair
    hls::stream<result_t>& s_result_pair, 
    // out format: the first number writes total intersection count, 
    //   while the rest are intersect ID pairs
    ap_uint<64>* out_intersect) {

    ap_uint<64> total_intersect_count = 0;
    ap_uint<64> result_ap_uint_64;
    
    while (true) {
#pragma HLS pipeline II=1

        if (!s_result_pair.empty()) {
            total_intersect_count += 1; 
            result_t result = s_result_pair.read();
            result_ap_uint_64.range(31, 0) = result.obj_id_A;
            result_ap_uint_64.range(63, 32) = result.obj_id_B;
            // no write to the memory
            // ap_uint<64> out_addr = total_intersect_count + 1;
            // out_intersect[out_addr] = result_ap_uint_64;
        } else if (!s_join_finish.empty() && s_result_pair.empty()) {
            int break_signal = s_join_finish.read(); // must read to make dataflow work
            break;
        }
    }

    // write the number of intersection in the first address
    out_intersect[0] = total_intersect_count;
    out_intersect[1] = result_ap_uint_64;
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

////////////////////     First Half: ADC     ////////////////////

    hls::stream<obj_t> s_page_A;
#pragma HLS stream variable=s_page_A depth=512
    hls::stream<obj_t> s_page_B;
#pragma HLS stream variable=s_page_B depth=512

    input_generator(
        // input
        page_entry_num, // number of entries per page
        page_num_A, // number of pages for input A
        page_num_B, // number of pages for input B
        in_pages_A,
        in_pages_B,
        // output
        s_page_A,
        s_page_B);

    hls::stream<result_t> s_result_pair;
#pragma HLS stream variable=s_result_pair depth=512

    hls::stream<int> s_join_finish; 
#pragma HLS stream variable=s_join_finish depth=2

//     hls::stream<ap_uint<64> > s_intersect_count; 
// #pragma HLS stream variable=s_intersect_count depth=512

    int page_pair_num = page_num_A * page_num_B;
    join_page(
        // input
        page_entry_num, // number of entries per page
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        s_page_A,
        s_page_B,
        // output
        // s_intersect_count, 
        s_join_finish,
        s_result_pair);

    result_consumer(
        // input
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        s_join_finish,  // per page pair
        s_result_pair, 
        // out format: the first number writes total intersection count, 
        //   while the rest are intersect ID pairs
        out_intersect);    
}

}
