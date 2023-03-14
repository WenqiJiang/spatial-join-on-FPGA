#include "constants.hpp"
// #include "DRAM_utils.hpp"
// #include "join_PE.hpp"
#include "types.hpp"

void result_generator(
    // input
    int page_entry_num, // number of entries per page
    int page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
    // output
    // hls::stream<ap_uint<64> >& s_intersect_count, // per page pair
    hls::stream<int>& s_join_finish, // one single signal after everything is written
    hls::stream<result_t>& s_result_pair) {

    result_t result;
    result_t result_last;
    result_last.last = true; 

    for (int i = 0; i < page_pair_num; i++) {
        for (int j = 0; j < page_entry_num * page_entry_num; j++) {
#pragma HLS pipeline II=1
            if (j == page_entry_num * page_entry_num - 1) {
                s_result_pair.write(result_last);
            } else {
                s_result_pair.write(result);
            }
        }
    }
    s_join_finish.write(1);
}

void result_burst_buffer(
    // input
    hls::stream<result_t>& s_result_pair, 
    hls::stream<int>& s_join_finish_in, 

    // output
    hls::stream<int>& s_result_pair_burst_length, 
    hls::stream<result_t>& s_result_pair_burst, 
    hls::stream<int>& s_join_finish_out
) {

    // max_burst_length must <= the output FIFO length & AXI burst length
    const int max_burst_length = 1024 - 1;

    while (true) {

        if (!s_result_pair.empty()) {

            int burst_length = max_burst_length;

            // send data 
            for (int i = 0; i < max_burst_length; i++) {
#pragma HLS pipeline II=1

                result_t result = s_result_pair.read();
                s_result_pair_burst.write(result);

                // if last, the join PE will not produce new result immediately,
                //    as it has to load the next pair of pages
                if (result.last) {
                    burst_length = i + 1; 
                    break;
                }
            }

            // send data length, length is always > 0 because we did data empty check
            s_result_pair_burst_length.write(burst_length);
        }
        else if (!s_join_finish_in.empty()) {
            int break_signal = s_join_finish_in.read(); // must read to make dataflow work
            s_join_finish_out.write(break_signal);
            break;
        }
    }
}

void write_results(
    // input
    hls::stream<int>& s_join_finish,  // per page pair
    hls::stream<int>& s_result_pair_burst_length, 
    hls::stream<result_t>& s_result_pair_burst, 

    // out format: the first number writes total intersection count, 
    //   while the rest are intersect ID pairs
    ap_uint<64>* out_intersect) {

    ap_uint<64> total_intersect_count = 0;
    while (true) {

        if (!s_result_pair_burst_length.empty()) {

            int burst_length = s_result_pair_burst_length.read();

            for (int i = 0; i < burst_length; i++) {
#pragma HLS pipeline II=1
                ap_uint<64> out_addr = total_intersect_count + i;

                result_t result = s_result_pair_burst.read();
                ap_uint<64> result_ap_uint_64;
                result_ap_uint_64.range(31, 0) = result.obj_id_A;
                result_ap_uint_64.range(63, 32) = result.obj_id_B;

                out_intersect[out_addr] = result_ap_uint_64;
            }

            total_intersect_count += total_intersect_count;
        } 
        
        else if (!s_join_finish.empty() && s_result_pair_burst_length.empty() && s_result_pair_burst.empty()) {
            int break_signal = s_join_finish.read(); // must read to make dataflow work
            break;
        }
    }

    // write the number of intersection in the first address
    out_intersect[0] = total_intersect_count;
}

extern "C" {

void vadd(  
    int page_entry_num, // number of entries per page
    int page_num_A, // number of pages for input A
    int page_num_B, // number of pages for input B
    // in runtime (should from DRAM)
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    // out format: the first number writes total intersection count, 
    //   while the rest are intersect ID pairs
    ap_uint<64>* out_intersect 
    )
{
// Share the same AXI interface with several control signals (but they are not allowed in same dataflow)
//    https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-AXI4-Burst-Behavior

// in runtime (should from DRAM)
#pragma HLS INTERFACE m_axi port=in_pages_A offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=in_pages_B offset=slave bundle=gmem1

// out
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem2 max_write_burst_length=32

#pragma HLS dataflow


    hls::stream<result_t> s_result_pair;
#pragma HLS stream variable=s_result_pair depth=512
    
    hls::stream<int> s_result_pair_burst_length;
#pragma HLS stream variable=s_result_pair_burst_length depth=512

    hls::stream<result_t> s_result_pair_burst;
#pragma HLS stream variable=s_result_pair_burst depth=1024

    hls::stream<int> s_join_finish_result_generator_out; 
#pragma HLS stream variable=s_join_finish_result_generator_out depth=2

    hls::stream<int> s_join_finish_burst_buffer_out; 
#pragma HLS stream variable=s_join_finish_burst_buffer_out depth=2


//     hls::stream<ap_uint<64> > s_intersect_count; 
// #pragma HLS stream variable=s_intersect_count depth=512

    int page_pair_num = page_num_A * page_num_B;
    result_generator(
        // input
        page_entry_num, // number of entries per page
        page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
        // output
        // hls::stream<ap_uint<64> >& s_intersect_count, // per page pair
        s_join_finish_result_generator_out, // one single signal after everything is written
        s_result_pair);

    result_burst_buffer(
        // input
        s_result_pair, 
        s_join_finish_result_generator_out, 

        // output
        s_result_pair_burst_length, 
        s_result_pair_burst, 
        s_join_finish_burst_buffer_out
    );

    write_results(
        // input
        s_join_finish_burst_buffer_out,
        s_result_pair_burst_length, 
        s_result_pair_burst, 

        // out format: the first number writes total intersection count, 
        //   while the rest are intersect ID pairs
        out_intersect);
   
}

}
