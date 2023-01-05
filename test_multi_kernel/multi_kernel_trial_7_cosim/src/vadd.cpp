#include "vadd.hpp"


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
    hls::stream<pkt>& s_B_to_A) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_A_to_B depth=512
#pragma HLS INTERFACE axis port=s_B_to_A depth=512

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    for (int i = 0; i < iter; i++) {
        // while (s_A_to_B.empty()) {}
        pkt in_signal = s_A_to_B.read();

        pkt out;
        out.data = input[in_signal.data];
        s_B_to_A.write(out);
    }
}

}
