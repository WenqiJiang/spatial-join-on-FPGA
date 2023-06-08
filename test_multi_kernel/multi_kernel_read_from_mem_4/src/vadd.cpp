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


#define N_JOIN_PE 16

template<typename T>
inline T block_read(hls::stream<T>& s) {
#pragma HLS inline

    while (s.empty()) {}
    return s.read();
}

extern "C" {

void PE_A(
	int total_iter,
	int send_interval,
    // in
	int* input_addr, 
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_read_addr,
    hls::stream<int>& s_join_PE_ID,
    hls::stream<int>& s_finish,
	int* output_data
    ) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_B_to_A 
#pragma HLS INTERFACE axis port=s_read_addr 
#pragma HLS INTERFACE axis port=s_join_PE_ID 
#pragma HLS INTERFACE axis port=s_finish 

#pragma HLS INTERFACE m_axi port=input_addr latency=1 num_read_outstanding=32 max_read_burst_length=16 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output_data offset=slave bundle=gmem1

	int addr_cache_size = 1000; // cache the first 1024 addr and repetitively send them
	int addr_cache[addr_cache_size];
	for (int i = 0; i < addr_cache_size; i++) {
		addr_cache[i] = input_addr[i];
	}

	volatile int cnt; 
    // PE A starts the loop
	for (int i = 0; i < total_iter; i++) {
		for (int j = 0; j < send_interval; j++) { cnt++; }
    	s_read_addr.write(input_addr[i]);
		s_join_PE_ID.write(i % N_JOIN_PE);
	}
	s_finish.write(0);

    // Receive value from B, and write to memory
    int out = block_read<int>(s_B_to_A);
    output_data[0] = out + cnt;
}

void PE_B(
    // in
	int read_len, 
	int page_size_per_axi,
    hls::burst_maxi<ap_uint<512> > input_data_A,
    hls::burst_maxi<ap_uint<512> > input_data_B,
    hls::stream<int>& s_read_addr,
    hls::stream<int>& s_join_PE_ID,
    hls::stream<int>& s_finish,
    // out
    hls::stream<int>& s_B_to_A
	) {
// HLS Interface manual: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-interface
#pragma HLS INTERFACE axis port=s_read_addr 
#pragma HLS INTERFACE axis port=s_join_PE_ID 
#pragma HLS INTERFACE axis port=s_finish 
#pragma HLS INTERFACE axis port=s_B_to_A

#pragma HLS INTERFACE m_axi port=input_data_A latency=1 num_read_outstanding=32 max_read_burst_length=16 offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=input_data_B latency=1 num_read_outstanding=32 max_read_burst_length=16 offset=slave bundle=gmem3

	ap_uint<512> local_buf[1024];
	bool break_while = false;
	int max_prefetch_request = 16;
	int join_PE_ID_cache[max_prefetch_request];

	while (!break_while) {

		if (!s_read_addr.empty() || !s_join_PE_ID.empty()) {

			//    1. send prefetch request (if not enough outstanding requests are in the pipeline)
			//    2. reading data
			int current_request_count = 0;
			
			while ((!s_read_addr.empty() || !s_join_PE_ID.empty()) && 
				current_request_count < max_prefetch_request ){

				join_PE_ID_cache[current_request_count] = block_read<int>(s_join_PE_ID);
				int read_addr = block_read<int>(s_read_addr);
				input_data_A.read_request(read_addr * page_size_per_axi, read_len);
				input_data_B.read_request(read_addr * page_size_per_axi, read_len);
				current_request_count++;
			}

			for (int j = 0; j < current_request_count; j++) {
				int join_PE_ID = join_PE_ID_cache[j];
				for (int i = 0; i < read_len; i++) {
#pragma HLS pipeline II=1
					local_buf[join_PE_ID] = input_data_A.read() + input_data_B.read();
				}
			}
		} else if (!s_finish.empty()) {
			int finish = block_read<int>(s_finish);
			s_B_to_A.write(local_buf[0].range(31, 0) + finish);
			break_while = true;
		}
	}
}

}
