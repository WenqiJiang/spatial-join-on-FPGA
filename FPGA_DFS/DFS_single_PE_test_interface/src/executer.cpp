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

    // Write all output once
    pair_t out_axis_page_pair_scheduler;
    axis_page_pair_scheduler.write(out_axis_page_pair_scheduler);

    int out_axis_intersect_count_directory_scheduler;
    axis_intersect_count_directory_scheduler.write(out_axis_intersect_count_directory_scheduler);

    // Read all input once
    int in_axis_read_write_control = block_read<int>(axis_read_write_control); 
    int in_axis_read_layer_id = block_read<int>(axis_read_layer_id);
    int in_axis_read_layer_pointer = block_read<int>(axis_read_layer_pointer);
    int in_axis_write_layer_id = block_read<int>(axis_write_layer_id);
    //   to node reading PE
    pair_t in_axis_page_ID_pair_read_nodes = block_read<pair_t>(axis_page_ID_pair_read_nodes);
    //   to the write results PE
    int in_axis_join_finish = block_read<int>(axis_join_finish);

    // Write to DRAM (with read dependency)
    ap_uint<64> out_final = 
        in_axis_read_write_control + 
        in_axis_read_layer_id + 
        in_axis_read_layer_pointer + 
        in_axis_write_layer_id + 
        in_axis_page_ID_pair_read_nodes.id_A + 
        in_axis_join_finish;
    
    out_intersect[0] = out_final;
}

}
