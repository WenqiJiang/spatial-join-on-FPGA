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
    int* output) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A depth=512
#pragma HLS INTERFACE axis port=s_A_to_B depth=512

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    bool first_iter = true;

    for (int i = 0; i < iter; i++) {
        // PE A starts the loop
        pkt reg;
        s_A_to_B.write(reg);
        // Receive value from B, and write to memory
        if (first_iter) {
            while (s_B_to_A.empty()) {}
            first_iter = false;
        }
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
    hls::stream<pkt>& s_B_to_A) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_A_to_B depth=512
#pragma HLS INTERFACE axis port=s_B_to_A depth=512

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    bool first_iter = true;
    
    for (int i = 0; i < iter; i++) {
        if (first_iter) {
            while (s_A_to_B.empty()) {}
            first_iter = false;
        }
        pkt in_signal = s_A_to_B.read();

        pkt out;
        out.data = input[in_signal.data];
        s_B_to_A.write(out);
    }
}

}
