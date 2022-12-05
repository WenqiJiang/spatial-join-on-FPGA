#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

//////////////////////////////////////////////////////////////////////////////
// Cannot export XO, found 'ap_fifo' top level interface on argument 's_B_to_A' 
//   that is not supported by the XO format.
//////////////////////////////////////////////////////////////////////////////

extern "C" {
    
void PE_A(
    // in
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_A_to_B,
    int* output) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE ap_fifo port=s_B_to_A depth=512
#pragma HLS INTERFACE ap_fifo port=s_A_to_B depth=512

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    // PE A starts the loop
    s_A_to_B.write(0);

    // Receive value from B, and write to memory
    while (s_B_to_A.empty()) {}
    int out = s_B_to_A.read();
    output[0] = out;
}

void PE_B(
    // in
    int* input,
    hls::stream<int>& s_A_to_B,
    // out
    hls::stream<int>& s_B_to_A) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE ap_fifo port=s_A_to_B depth=512
#pragma HLS INTERFACE ap_fifo port=s_B_to_A depth=512

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

    while (s_A_to_B.empty()) {}
    int in_signal = s_A_to_B.read();
    int out = input[in_signal];

    s_B_to_A.write(out);
}

}
