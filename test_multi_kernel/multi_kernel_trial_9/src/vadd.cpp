#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces-without-Side-Channels
typedef ap_axiu<32, 0, 0, 0> pkt;


extern "C" {
    
void PE_A(
    // in
    int iter,
    hls::stream<pkt>& s_B_to_A,
    // out
    hls::stream<pkt>& s_A_to_B,
    int* output,
    // start signals
    hls::stream<int>& s_A_start,
    hls::stream<int>& s_B_start
    ) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A 
#pragma HLS INTERFACE axis port=s_A_to_B 

#pragma HLS INTERFACE axis port=s_A_start 
#pragma HLS INTERFACE axis port=s_B_start 

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    // start detection 
    s_A_start.write(1);
    while (s_B_start.empty()) {}
    int start = s_B_start.read();   

    for (int i = 0; i < iter; i++) {
        // PE A starts the loop
        pkt reg;
        s_A_to_B.write(reg);
        // Receive value from B, and write to memory
        // while (s_B_to_A.empty()) {}
        pkt out = s_B_to_A.read();
        output[i] = out.data;
    }
}

void PE_B(
    // in
    int iter,
    int* input,
    hls::stream<pkt>& s_A_to_B,
    // out
    hls::stream<pkt>& s_B_to_A,
    // start signals
    hls::stream<int>& s_A_start,
    hls::stream<int>& s_B_start) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_A_to_B 
#pragma HLS INTERFACE axis port=s_B_to_A 

#pragma HLS INTERFACE axis port=s_A_start 
#pragma HLS INTERFACE axis port=s_B_start 

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    // start detection 
    s_B_start.write(1);
    while (s_A_start.empty()) {}
    int start = s_A_start.read();   

    for (int i = 0; i < iter; i++) {
        // while (s_A_to_B.empty()) {}
        pkt in_signal = s_A_to_B.read();

        pkt out;
        out.data = input[in_signal.data];
        s_B_to_A.write(out);
    }
}

}
