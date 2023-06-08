#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

#include "hls_burst_maxi.h"

/*
Simulate the read behavior in a spatial join accelerators. 
Reading 64-byte data in a short burst, given the input addr and len streamed in.

Compared to 1: using hls_burst_maxi
	https://docs.xilinx.com/r/2022.1-English/ug1399-vitis-hls/Using-Manual-Burst

Compared to 2: using manual prefetching (request multiple)
*/


template<typename T>
inline T block_read(hls::stream<T>& s) {
#pragma HLS inline

    while (s.empty()) {}
    return s.read();
}

extern "C" {

void PE_A(
	int total_iter, // number of write requests
	int write_len,
    // in
	int* input_meta, // doing nothing in this case
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_write_data,
    hls::stream<int>& s_write_len,
    hls::stream<int>& s_finish,
	int* output_data_A
    ) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A 
#pragma HLS INTERFACE axis port=s_write_data 
#pragma HLS INTERFACE axis port=s_write_len 
#pragma HLS INTERFACE axis port=s_finish 

#pragma HLS INTERFACE m_axi port=input_meta offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output_data_A offset=slave bundle=gmem1

    // PE A starts the loop
	for (int i = 0; i < total_iter; i++) {
		for (int j = 0; j < write_len; j++) {
#pragma HLS pipeline II=1
    		s_write_data.write(0);
		}
		s_write_len.write(write_len);
	}
	s_finish.write(0);

    // Receive value from B, and write to memory
    int out = block_read<int>(s_B_to_A);
    output_data_A[0] = out;
}

void PE_B(
    // in
    hls::burst_maxi<ap_uint<512> > output_data_B,
    hls::stream<int>& s_write_data,
    hls::stream<int>& s_write_len,
    hls::stream<int>& s_finish,
    // out
    hls::stream<int>& s_B_to_A
	) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
// #pragma HLS INTERFACE axis port=s_write_data 
#pragma HLS INTERFACE axis port=s_write_len 
#pragma HLS INTERFACE axis port=s_finish 
#pragma HLS INTERFACE axis port=s_B_to_A

#pragma HLS INTERFACE m_axi port=output_data_B latency=1 num_write_outstanding=32 max_write_burst_length=16 offset=slave bundle=gmem2

	ap_uint<512> local_buf[1024];
	bool break_while = false;
	int max_write_requests = 16;
	int burst_length_cache[max_write_requests];
	int write_count = 0;
	
	while (!break_while) {

		if (!s_write_len.empty()) {

			//    1. send prefetch request (if not enough outstanding requests are in the pipeline)
			//    2. reading data
			int current_request_count = 0;
			int current_write_len = 0;
			while (!s_write_len.empty() && 
				current_request_count < max_write_requests ){

				int write_len = block_read<int>(s_write_len);
				current_write_len += write_len;
				burst_length_cache[current_request_count] = write_len;
				current_request_count++;
			}

			if (current_write_len > 0) {
				output_data_B.write_request(write_count, current_write_len);
				for (int bid = 0; bid < current_request_count; bid++) {
					int burst_length = burst_length_cache[bid];
					for (int i = 0; i < burst_length; i++) {
#pragma HLS pipeline II=1
						output_data_B.write(s_write_data.read());
					}
				}
				write_count += current_write_len;
				output_data_B.write_response();
			}
		} else if (!s_finish.empty()) {
			int finish = block_read<int>(s_finish);
			s_B_to_A.write(local_buf[0].range(31, 0) + finish);
			break_while = true;
		}
	}
}

}
