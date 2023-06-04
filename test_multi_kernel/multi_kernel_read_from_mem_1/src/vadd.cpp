#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

/*
Simulate the read behavior in a spatial join accelerators. 
Reading 64-byte data in a short burst, given the input addr and len streamed in.
*/


template<typename T>
inline T block_read(hls::stream<T>& s) {
#pragma HLS inline

    while (s.empty()) {}
    return s.read();
}

extern "C" {

void PE_A(
	int total_iter,
	int read_len,
    // in
	int* input_meta, 
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_read_addr,
    hls::stream<int>& s_read_len,
    hls::stream<int>& s_finish,
	int* output_data
    ) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A 
#pragma HLS INTERFACE axis port=s_read_addr 
#pragma HLS INTERFACE axis port=s_read_len 
#pragma HLS INTERFACE axis port=s_finish 

#pragma HLS INTERFACE m_axi port=input_meta offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output_data offset=slave bundle=gmem1

    // PE A starts the loop
	for (int i = 0; i < total_iter; i++) {
#pragma HLS pipeline II=1
    	s_read_addr.write(input_meta[i]);
		s_read_len.write(read_len);
	}
	s_finish.write(0);

    // Receive value from B, and write to memory
    int out = block_read<int>(s_B_to_A);
    output_data[0] = out;
}

void PE_B(
    // in
    ap_uint<512>* input_data,
    hls::stream<int>& s_read_addr,
    hls::stream<int>& s_read_len,
    hls::stream<int>& s_finish,
    // out
    hls::stream<int>& s_B_to_A
	) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_read_addr 
#pragma HLS INTERFACE axis port=s_read_len 
#pragma HLS INTERFACE axis port=s_finish 
#pragma HLS INTERFACE axis port=s_B_to_A

#pragma HLS INTERFACE m_axi port=input_data offset=slave bundle=gmem2

	ap_uint<512> local_buf[1024];
	bool break_while = false;

	while (!break_while) {

		if (!s_read_addr.empty() || !s_read_len.empty()) {

			int read_addr = block_read<int>(s_read_addr);
			int read_len = block_read<int>(s_read_len);

			for (int i = 0; i < read_len; i++) {
#pragma HLS pipeline II=1
				local_buf[i] = input_data[read_addr + i];
			}
		} else if (!s_finish.empty()) {
			int finish = block_read<int>(s_finish);
			s_B_to_A.write(local_buf[0].range(31, 0) + finish);
			break_while = true;
		}
	}
}

}