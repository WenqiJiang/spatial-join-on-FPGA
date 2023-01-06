#pragma once

#include "types.hpp"

template<typename T>
inline T block_read(hls::stream<T>& s) {
#pragma HLS inline

    while (s.empty()) {}
    return s.read();
}

// pair_t to ap_uint<64>
ap_uint<64> pack_pair(pair_t input) {
#pragma HLS inline

    ap_uint<64> output_ap_uint_64;
    ap_uint<32> id_A_uint = *((ap_uint<32>*) (&input.id_A));
    ap_uint<32> id_B_uint = *((ap_uint<32>*) (&input.id_B));
    output_ap_uint_64.range(31, 0) = id_A_uint;
    output_ap_uint_64.range(63, 32) = id_B_uint;

    return output_ap_uint_64;
}

// ap_uint<64> to pair_t 
pair_t unpack_pair(ap_uint<64> input) {
#pragma HLS inline

    pair_t output;
    ap_uint<32> id_A_uint = input.range(31, 0);
    ap_uint<32> id_B_uint = input.range(63, 32);
    output.id_A = id_A_uint;
    output.id_B = id_B_uint;

    return output;
}

template<int num_stream>
void replicate_termination_signal(
    hls::stream<int>& axis_join_finish,
    hls::stream<int> (&s_join_finish_replicated)[num_stream]) {

    int end = block_read<int>(axis_join_finish);
    for (int s = 0; s < num_stream; s++) {
#pragma HLS unroll
        s_join_finish_replicated[s].write(end);
    }
}