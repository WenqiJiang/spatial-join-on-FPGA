#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

/*
Given that the write FIFO writes data much after than the consumer,
	check whether the program will stuck.
*/


template<typename T>
inline T block_read(hls::stream<T>& s) {
#pragma HLS inline

    while (s.empty()) {}
    return s.read();
}

template<typename T>
inline void block_write(hls::stream<T>& s, T val) {
#pragma HLS inline

    while (s.full()) {}
    s.write(val);
}

extern "C" {

void PE_A(
	int total_iter,
    // in
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_A_to_B,
    int* output) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A 
#pragma HLS INTERFACE axis port=s_A_to_B 

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0

    // PE A starts the loop
	for (int i = 0; i < total_iter; i++) {
#pragma HLS pipeline II=1
    	block_write<int>(s_A_to_B, 0);
	}

    // Receive value from B, and write to memory
    int out = block_read<int>(s_B_to_A);
    output[0] = out;
}

void PE_B(
	int total_iter,
	int sleep_delay, // number of cycles to wait before read from A
    // in
    int* input,
    hls::stream<int>& s_A_to_B,
    // out
    hls::stream<int>& s_B_to_A) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_A_to_B 
#pragma HLS INTERFACE axis port=s_B_to_A

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1

	volatile int sleep_counter = 0;
	int in_signal;
	for (int i = 0; i < total_iter; i++) {

		for (int j = 0; j < sleep_delay; j++) {
#pragma HLS pipeline II=1
			sleep_counter++;
		}
		in_signal = block_read<int>(s_A_to_B);
	}

    int out = input[in_signal];
    block_write<int>(s_B_to_A, out + sleep_counter);
}

}
