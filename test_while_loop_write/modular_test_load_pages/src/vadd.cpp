#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

void input_consumer(
    // input
    int page_entry_num, // number of entries per page
    int page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B,
    // output
    ap_uint<64>* out_intersect ) {

    obj_t page_A[MAX_PAGE_ENTRY_NUM];
    obj_t page_B[MAX_PAGE_ENTRY_NUM];

    for (int i = 0; i < page_pair_num; i++) {
        // load the two pages 
        for (int j = 0; j < page_entry_num; j++) {
#pragma HLS pipeline II=1
            page_A[j] = s_page_A.read();
            page_B[j] = s_page_B.read();
        }
    }

    out_intersect[0] = 0;
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

    input_loader(
        // input
        page_entry_num, // number of entries per page
        page_num_A, // number of pages for input A
        page_num_B, // number of pages for input B
        in_pages_A,
        in_pages_B,
        // output
        s_page_A,
        s_page_B);

    int page_pair_num = page_num_A * page_num_B;
    input_consumer(
        // input
        page_entry_num, // number of entries per page
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        s_page_A,
        s_page_B,
        // output
        out_intersect);
}

}
