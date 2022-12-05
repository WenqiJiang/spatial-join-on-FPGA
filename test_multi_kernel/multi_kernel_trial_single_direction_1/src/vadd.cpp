#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"



extern "C" {
    
void PE_A(
    // in
    int* input,
    // out
    hls::stream<int>& s_A_to_B) {

#pragma HLS INTERFACE axis port=s_A_to_B

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    // PE A starts the loop
    s_A_to_B.write(input[0]);
}

void PE_B(
    // in
    hls::stream<int>& s_A_to_B,
    // out
    int* output) {

#pragma HLS INTERFACE axis port=s_A_to_B

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1

    output[0] = s_A_to_B.read();
}

}
