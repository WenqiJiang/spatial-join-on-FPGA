#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

extern "C" {

void executor(  
    ///// input /////
    int root_id_A,
    int root_id_B,
    int page_bytes,
    // input streams
    //   to layer cache controller
    hls::stream<int>& axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& axis_layer_cache_read_addr, 
    hls::stream<int>& axis_layer_cache_write_addr, 
    //   to node reading PE
    hls::stream<pair_t>& axis_page_ID_pair_read_nodes,
    //   to the write results PE
    hls::stream<int>& axis_join_finish,   
    // input from DRAM
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,

    ///// output /////
    // output streams
    //   from layer cache controller
    hls::stream<pair_t>& axis_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& axis_intersect_count_directory_scheduler, // for write request, return count  
    
    // in/out (intermediate)
    ap_uint<64>* layer_cache,
    // out (result) format: the first number writes total intersection count, 
    //   while the rest are intersect ID pairs
    ap_uint<64>* out_intersect 
    )
{
// Share the same AXI interface with several control signals (but they are not allowed in same dataflow)
//    https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-AXI4-Burst-Behavior

// input streams
#pragma HLS INTERFACE axis port=axis_read_write_control
#pragma HLS INTERFACE axis port=axis_layer_cache_read_addr
#pragma HLS INTERFACE axis port=axis_layer_cache_write_addr
#pragma HLS INTERFACE axis port=axis_page_ID_pair_read_nodes
#pragma HLS INTERFACE axis port=axis_join_finish

// in runtime (should from DRAM)
#pragma HLS INTERFACE m_axi port=in_pages_A offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=in_pages_B offset=slave bundle=gmem1

// output streams
#pragma HLS INTERFACE axis port=axis_page_pair_scheduler
#pragma HLS INTERFACE axis port=axis_intersect_count_directory_scheduler

// out 
#pragma HLS INTERFACE m_axi port=layer_cache  offset=slave bundle=gmem2 max_write_burst_length=64
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem3 max_write_burst_length=64

#pragma HLS dataflow

////////////////////     First Half: ADC     ////////////////////

    hls::stream<node_meta_t> s_meta_A;
#pragma HLS stream variable=s_meta_A depth=512
    hls::stream<node_meta_t> s_meta_B;
#pragma HLS stream variable=s_meta_B depth=512

    hls::stream<obj_t> s_page_A;
#pragma HLS stream variable=s_page_A depth=512
    hls::stream<obj_t> s_page_B;
#pragma HLS stream variable=s_page_B depth=512
    
    hls::stream<int> s_join_finish[6]; 
#pragma HLS stream variable=s_join_finish depth=2

    hls::stream<int> s_intersect_count_directory;
#pragma HLS stream variable=s_intersect_count_directory depth=512

    hls::stream<result_t> s_result_pair_directory;
#pragma HLS stream variable=s_result_pair_directory depth=1024

    hls::stream<int> s_intersect_count_leaf; 
#pragma HLS stream variable=s_intersect_count_leaf depth=512

    hls::stream<result_t> s_result_pair_leaf;
#pragma HLS stream variable=s_result_pair_leaf depth=1024

    hls::stream<int> s_result_pair_leaf_burst_length;
#pragma HLS stream variable=s_result_pair_leaf_burst_length depth=512

    hls::stream<result_t> s_result_pair_leaf_burst;
#pragma HLS stream variable=s_result_pair_leaf_burst depth=1024

    hls::stream<int> s_result_pair_directory_burst_length;
#pragma HLS stream variable=s_result_pair_directory_burst_length depth=512

    hls::stream<result_t> s_result_pair_directory_burst;
#pragma HLS stream variable=s_result_pair_directory_burst depth=1024


    pass_termination_signal(
        // from AXIS
        axis_join_finish,
        // to internal PEs
        s_join_finish[0]
    ); 

    read_nodes(
        // input
        page_bytes,
        in_pages_A,
        in_pages_B,
        axis_page_ID_pair_read_nodes,
        s_join_finish[0],
        // output
        s_meta_A,
        s_meta_B,
        s_page_A,
        s_page_B,
        s_join_finish[1]
    );

    burst_buffer(
        // input
        s_result_pair_directory, 
        s_join_finish[1],

        // output
        s_result_pair_directory_burst_length, 
        s_result_pair_directory_burst, 
        s_join_finish[2]
    );

    layer_cache_memory_controller(
        // input
        //   argument
        root_id_A,
        root_id_B,
        //   memory 
        layer_cache,
        //   from join PE
        s_intersect_count_directory, 
        s_result_pair_directory_burst_length, 
        s_result_pair_directory_burst, 
        //   from scheduler
        axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
        axis_layer_cache_read_addr, 
        axis_layer_cache_write_addr, 
        s_join_finish[2],
        // output
        //   to scheduler
        axis_page_pair_scheduler,      // for read request, return pair
        axis_intersect_count_directory_scheduler, // for write request, return count
        s_join_finish[3]
    );

    join_page(
        // input
        s_meta_A,
        s_meta_B,
        s_page_A,
        s_page_B,
        s_join_finish[3],
        // output
        //   for directory nodes: 
        s_intersect_count_directory, // per page pair
        s_result_pair_directory,
        //   for leaf nodes: 
        s_intersect_count_leaf, // per page pair
        s_result_pair_leaf,
        s_join_finish[4]
    );

    burst_buffer(
        // input
        s_result_pair_leaf, 
        s_join_finish[4], 

        // output
        s_result_pair_leaf_burst_length, 
        s_result_pair_leaf_burst, 
        s_join_finish[5]
    );

    write_results(
        // input
        //   from join PE
        s_intersect_count_leaf, // per page pair
        s_result_pair_leaf_burst_length, 
        s_result_pair_leaf_burst, 
        //   from the scheduler
        s_join_finish[5],  // final end signal 
        // out
        //    out format: the first number writes total intersection count, 
        //                while the rest are intersect ID pairs
        out_intersect
    );
}

}
