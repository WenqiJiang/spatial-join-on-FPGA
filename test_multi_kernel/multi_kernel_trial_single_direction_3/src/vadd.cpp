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
    int* input,
    // out
    hls::stream<pkt>& s_A_to_B) {

#pragma HLS INTERFACE axis port=s_A_to_B

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    // PE A starts the loop
    for (int i = 0; i < iter; i++) {
	pkt reg;
	reg.data = input[i];
        s_A_to_B.write(reg);
    }
}

void PE_B(
    // in
    int iter,
    hls::stream<pkt>& s_A_to_B,
    // out
    int* output) {

#pragma HLS INTERFACE axis port=s_A_to_B

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1

    for (int i = 0; i < iter; i++) {
        pkt reg = s_A_to_B.read();
        output[i] = reg.data;
    }
}

}
