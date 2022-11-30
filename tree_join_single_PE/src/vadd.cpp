#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

extern "C" {

void vadd(  
    int max_level_A,
    int max_level_B,
    int root_id_A,
    int root_id_B,
    // in runtime (should from DRAM)
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    // out (intermediate)
    ap_uint<64>* layer_cache,
    // out (result) format: the first number writes total intersection count, 
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
#pragma HLS INTERFACE m_axi port=layer_cache  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=out_intersect  offset=slave bundle=gmem3

#pragma HLS dataflow

////////////////////     First Half: ADC     ////////////////////
    
    hls::stream<pair_t> s_page_ID_pair_read_nodes;
#pragma HLS stream variable=s_page_ID_pair_read_nodes depth=512

    hls::stream<node_meta_t> s_meta_A;
#pragma HLS stream variable=s_meta_A depth=512
    hls::stream<node_meta_t> s_meta_B;
#pragma HLS stream variable=s_meta_B depth=512

    hls::stream<obj_t> s_page_A;
#pragma HLS stream variable=s_page_A depth=512
    hls::stream<obj_t> s_page_B;
#pragma HLS stream variable=s_page_B depth=512

    hls::stream<int> s_join_finish; 
#pragma HLS stream variable=s_join_finish depth=512

    hls::stream<int> s_join_finish_replicated[3]; 
#pragma HLS stream variable=s_join_finish_replicated depth=512
#pragma HLS array_partition variable=s_join_finish_replicated

    read_nodes(
        // input
        in_pages_A,
        in_pages_B,
        s_page_ID_pair_read_nodes,
        s_join_finish_replicated[0],
        // output
        s_meta_A,
        s_meta_B,
        s_page_A,
        s_page_B
        );

    hls::stream<pair_t> s_page_pair_scheduler;
#pragma HLS stream variable=s_page_pair_scheduler depth=512

    hls::stream<int> s_intersect_count_directory_scheduler;
#pragma HLS stream variable=s_intersect_count_directory_scheduler depth=512

    hls::stream<int> s_read_write_control;
#pragma HLS stream variable=s_read_write_control depth=512

    hls::stream<int> s_read_layer_id;
#pragma HLS stream variable=s_read_layer_id depth=512

    hls::stream<int> s_read_layer_pointer;
#pragma HLS stream variable=s_read_layer_pointer depth=512

    hls::stream<int> s_write_layer_id;
#pragma HLS stream variable=s_write_layer_id depth=512


    int max_level = max_level_A > max_level_B ? max_level_A: max_level_B;

    scheduler(
        // Input
        max_level,  // max(level_A, level_B)
        //   from layer cache controller
        s_page_pair_scheduler,      // for read request, return pair
        s_intersect_count_directory_scheduler, // for write request, return count
        // Output
        //   to layer cache controller
        s_read_write_control, // 0 -> read from memory; 1 -> write to memory 
        s_read_layer_id,      // layer l 
        s_read_layer_pointer, // pair p in layer l
        s_write_layer_id, 
        //   to node reading PE
        s_page_ID_pair_read_nodes,
        //   to the write results PE
        s_join_finish         // the final termination signal 
    );


    replicate_termination_signal<3>(
        s_join_finish,
        s_join_finish_replicated); 

    hls::stream<int> s_intersect_count_directory;
#pragma HLS stream variable=s_intersect_count_directory depth=512

    hls::stream<pair_t> s_result_pair_directory;
#pragma HLS stream variable=s_result_pair_directory depth=512

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
        s_read_write_control, // 0 -> read from memory; 1 -> write to memory 
        s_read_layer_id,      // layer l 
        s_read_layer_pointer, // pair p in layer l
        s_write_layer_id, 
        // output
        //   to scheduler
        s_page_pair_scheduler,      // for read request, return pair
        s_intersect_count_directory_scheduler // for write request, return count
    );

    hls::stream<int> s_intersect_count_leaf; 
#pragma HLS stream variable=s_intersect_count_leaf depth=512

    hls::stream<pair_t> s_result_pair_leaf;
#pragma HLS stream variable=s_result_pair_leaf depth=512


    join_page(
        // input
        s_meta_A,
        s_meta_B,
        s_page_A,
        s_page_B,
        s_join_finish_replicated[1],
        // output
        //   for directory nodes: 
        s_intersect_count_directory, // per page pair
        s_result_pair_directory,
        //   for leaf nodes: 
        s_intersect_count_leaf, // per page pair
        s_result_pair_leaf
        );

    write_results(
        // input
        //   from join PE
        s_intersect_count_leaf, // per page pair
        s_result_pair_leaf,
        //   from the scheduler
        s_join_finish_replicated[2],  // final end signal 
        // out
        //    out format: the first number writes total intersection count, 
        //                while the rest are intersect ID pairs
        out_intersect);
}

}
