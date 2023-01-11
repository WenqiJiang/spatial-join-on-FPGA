#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

extern "C" {

void executer(  
    ///// input /////
    int root_id_A,
    int root_id_B,
    // input streams
    //   to layer cache controller
    hls::stream<int>& axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& axis_read_layer_id,      // layer l 
    hls::stream<int>& axis_read_layer_pointer, // pair p in layer l
    hls::stream<int>& axis_write_layer_id, 
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
#pragma HLS INTERFACE axis port=axis_read_layer_id
#pragma HLS INTERFACE axis port=axis_read_layer_pointer
#pragma HLS INTERFACE axis port=axis_write_layer_id
#pragma HLS INTERFACE axis port=axis_page_ID_pair_read_nodes
#pragma HLS INTERFACE axis port=axis_join_finish

// in runtime (should from DRAM)
#pragma HLS INTERFACE m_axi port=in_pages_A offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=in_pages_B offset=slave bundle=gmem1

// output streams
#pragma HLS INTERFACE axis port=axis_page_pair_scheduler
#pragma HLS INTERFACE axis port=axis_intersect_count_directory_scheduler

// out 
#pragma HLS INTERFACE m_axi port=layer_cache  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem3

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
    
    hls::stream<int> s_join_finish_replicated[4]; 
#pragma HLS stream variable=s_join_finish_replicated depth=2

    hls::stream<int> s_intersect_count_directory;
#pragma HLS stream variable=s_intersect_count_directory depth=512

    hls::stream<result_t> s_result_pair_directory;
#pragma HLS stream variable=s_result_pair_directory depth=512

    hls::stream<int> s_intersect_count_leaf; 
#pragma HLS stream variable=s_intersect_count_leaf depth=512

    hls::stream<result_t> s_result_pair_leaf;
#pragma HLS stream variable=s_result_pair_leaf depth=512

    replicate_termination_signal<4>(
        axis_join_finish,
        s_join_finish_replicated); 

    read_nodes(
        // input
        in_pages_A,
        in_pages_B,
        axis_page_ID_pair_read_nodes,
        s_join_finish_replicated[0],
        // output
        s_meta_A,
        s_meta_B,
        s_page_A,
        s_page_B);

    layer_cache_memory_controller(
        // input
        //   argument
        root_id_A,
        root_id_B,
        //   memory 
        layer_cache,
        //   from join PE
        s_intersect_count_directory, 
        s_result_pair_directory,
        //   from scheduler
        axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
        axis_read_layer_id,      // layer l 
        axis_read_layer_pointer, // pair p in layer l
        axis_write_layer_id, 
        s_join_finish_replicated[1],
        // output
        //   to scheduler
        axis_page_pair_scheduler,      // for read request, return pair
        axis_intersect_count_directory_scheduler // for write request, return count
    );

    join_page(
        // input
        s_meta_A,
        s_meta_B,
        s_page_A,
        s_page_B,
        s_join_finish_replicated[2],
        // output
        //   for directory nodes: 
        s_intersect_count_directory, // per page pair
        s_result_pair_directory,
        //   for leaf nodes: 
        s_intersect_count_leaf, // per page pair
        s_result_pair_leaf);

    write_results(
        // input
        //   from join PE
        s_intersect_count_leaf, // per page pair
        s_result_pair_leaf,
        //   from the scheduler
        s_join_finish_replicated[3],  // final end signal 
        // out
        //    out format: the first number writes total intersection count, 
        //                while the rest are intersect ID pairs
        out_intersect);
}

}
