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
    int* output
    ) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A depth=512
#pragma HLS INTERFACE axis port=s_A_to_B depth=512

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    // write output start signals
    pkt pkt_dummy_out;
    pkt_dummy_out.data = 0;
    s_A_to_B.write(pkt_dummy_out);

    // read input start signals
    while (s_B_to_A.empty()) {}
    pkt pkt_dummy_in = s_B_to_A.read();  

    for (int i = 0; i < iter; i++) {
        // PE A starts the loop
        pkt reg;
        s_A_to_B.write(reg);
        // Receive value from B, and write to memory
        // while (s_B_to_A.empty()) {}
        pkt out = s_B_to_A.read();
        output[i] = out.data + pkt_dummy_in.data;
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

    // write output start signals
    pkt pkt_dummy_out;
    pkt_dummy_out.data = 0;
    s_B_to_A.write(pkt_dummy_out);

    // read input start signals
    while (s_A_to_B.empty()) {}
    pkt pkt_dummy_in = s_A_to_B.read();  

    for (int i = 0; i < iter; i++) {
        // while (s_A_to_B.empty()) {}
        pkt in_signal = s_A_to_B.read();

        pkt out;
        out.data = input[in_signal.data + pkt_dummy_in.data];
        s_B_to_A.write(out);
    }
}

}
