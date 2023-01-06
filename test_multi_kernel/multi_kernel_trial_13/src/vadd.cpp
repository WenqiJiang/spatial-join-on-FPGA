#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces-without-Side-Channels
// typedef ap_axiu<32, 0, 0, 0> pkt;

template<typename T>
inline T block_read(hls::stream<T>& s) {
#pragma HLS inline

    while (s.empty()) {}
    return s.read();
}

void PE_B_1(
    int iter, 
    int* input,
    hls::stream<int>& axis_A_to_B,
    hls::stream<int>& s_B1_to_B2) {

    // This is the PE reading AXIS, thus need explicit blocking read
    
    int in_signal;
    int out_signal;

    for (int i = 0; i < iter - 1; i++) {
#pragma HLS pipeline II=1
        in_signal = block_read<int>(axis_A_to_B);
        out_signal = in_signal + 1;
        s_B1_to_B2.write(out_signal); // create dependency 
    }

    // last iteration
    in_signal = block_read<int>(axis_A_to_B);
    out_signal = input[in_signal] + 1;
    s_B1_to_B2.write(out_signal);
}

void PE_B_2(
    int iter,
    hls::stream<int>& s_B1_to_B2,
    hls::stream<int>& s_B2_to_B3) {
   
    // This is the internal PE, on need explicit blocking read
    
    int in_signal;
    int out_signal;

    for (int i = 0; i < iter; i++) {
#pragma HLS pipeline II=1
        in_signal = s_B1_to_B2.read();
        out_signal = in_signal + 1;
        s_B2_to_B3.write(out_signal); 
    }
}

void PE_B_3(
    int iter,
    hls::stream<int>& s_B2_to_B3,
    hls::stream<int>& axis_B_to_A) {

    // This is the AXIS write PE, no need explicit blocking read 
    
    int in_signal;
    int out_signal;

    for (int i = 0; i < iter; i++) {
#pragma HLS pipeline II=1
        in_signal = s_B2_to_B3.read();
        out_signal = in_signal + 1;
        axis_B_to_A.write(out_signal); 
    }
}

extern "C" {
    
void PE_A(
    // in
    int iter,
    hls::stream<int>& axis_B_to_A,
    // out
    hls::stream<int>& axis_A_to_B,
    int* output) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=axis_B_to_A depth=512
#pragma HLS INTERFACE axis port=axis_A_to_B depth=512

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    int reg;
    int out; 

    reg = 0;


    for (int i = 0; i < iter; i++) {
#pragma HLS pipeline II=1
        // PE A starts the loop
        axis_A_to_B.write(reg);
        // Receive value from B, and write to memory
        out = block_read<int>(axis_B_to_A);
    }
    
    output[0] = out;
}

void PE_B(
    // in
    int iter,
    int* input,
    hls::stream<int>& axis_A_to_B,
    // out
    hls::stream<int>& axis_B_to_A) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=axis_A_to_B depth=512
#pragma HLS INTERFACE axis port=axis_B_to_A depth=512

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

#pragma HLS dataflow

    hls::stream<int> s_B1_to_B2;
#pragma HLS stream variable=s_B1_to_B2 depth=2

    hls::stream<int> s_B2_to_B3;
#pragma HLS stream variable=s_B2_to_B3 depth=2

    PE_B_1(
        iter, 
        input,
        axis_A_to_B,
        s_B1_to_B2);

    PE_B_2(
        iter,
        s_B1_to_B2,
        s_B2_to_B3);

    PE_B_3(
        iter,
        s_B2_to_B3,
        axis_B_to_A);

}

}
