#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

void PE_A(
    // in
    hls::stream<int>& s_C_to_A,
    // out
    hls::stream<int>& s_A_to_B,
    ap_uint<64>* out_intersect) {

    // PE A starts the loop
    s_A_to_B.write(1);

    // End by 
    int out = s_C_to_A.read();
    out_intersect[0] = out;
}

void PE_B(
    // in
    ap_uint<64>* layer_cache,
    hls::stream<int>& s_A_to_B,
    // out
    hls::stream<int>& s_B_to_C) {

    int in = s_A_to_B.read();
    int out = layer_cache[in];

    s_B_to_C.write(out);
}

void PE_C(
    // in
    hls::stream<int>& s_B_to_C,
    // out
    hls::stream<int>& s_C_to_A) {

    s_C_to_A.write(s_B_to_C.read());
}


extern "C" {

void vadd(  
    int max_level_A,
    int max_level_B,
    int root_id_A,
    int root_id_B,
    // in runtime (should from DRAM)
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    // out (intermediate)
    ap_uint<64>* layer_cache,
    // out (result) format: the first number writes total intersection count, 
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
#pragma HLS INTERFACE m_axi port=layer_cache  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem3

#pragma HLS dataflow
    
    hls::stream<int> s_A_to_B; 
#pragma HLS stream variable=s_A_to_B depth=512
    hls::stream<int> s_B_to_C; 
#pragma HLS stream variable=s_A_to_B depth=512
    hls::stream<int> s_C_to_A; 
#pragma HLS stream variable=s_A_to_B depth=512

    PE_A(
        // in
        s_C_to_A,
        // out
        s_A_to_B,
        out_intersect);

    PE_B(
        // in
        layer_cache,
        s_A_to_B,
        // out
        s_B_to_C);

    PE_C(
        // in
        s_B_to_C,
        // out
        s_C_to_A);
}

}
