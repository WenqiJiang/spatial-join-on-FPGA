#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"



extern "C" {
    
void PE_A(
    // in
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_A_to_B,
    int* output) {

#pragma HLS INTERFACE axis port=s_B_to_A
#pragma HLS INTERFACE axis port=s_A_to_B

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    // PE A starts the loop
    s_A_to_B.write(0);

    // Receive value from B, and write to memory
    int out = s_B_to_A.read();
    output[0] = out;
}

void PE_B(
    // in
    int* input,
    hls::stream<int>& s_A_to_B,
    // out
    hls::stream<int>& s_B_to_A) {

#pragma HLS INTERFACE axis port=s_A_to_B
#pragma HLS INTERFACE axis port=s_B_to_A

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    int in_signal = s_A_to_B.read();
    int out = input[in_signal];

    s_B_to_A.write(out);
}

}
