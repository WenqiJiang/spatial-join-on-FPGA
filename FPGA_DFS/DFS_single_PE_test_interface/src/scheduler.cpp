#pragma once

#include "types.hpp"
#include "utils.hpp"

extern "C" {

// The scheduler keeps track of where the FPGA is working on during the tree traversal.
void scheduler(
    // Input
    int max_level_A,
    int max_level_B,
    //   from layer cache controller
    hls::stream<pair_t>& axis_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& axis_intersect_count_directory_scheduler, // for write request, return count
    // Output
    //   to layer cache controller
    hls::stream<int>& axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& axis_read_layer_id,      // layer l 
    hls::stream<int>& axis_read_layer_pointer, // pair p in layer l
    hls::stream<int>& axis_write_layer_id, 
    //   to node reading PE
    hls::stream<pair_t>& axis_page_ID_pair_read_nodes,
    //   to the write results PE
    hls::stream<int>& axis_join_finish         // the final termination signal 
) {

// input streams
#pragma HLS INTERFACE axis port=axis_page_pair_scheduler
#pragma HLS INTERFACE axis port=axis_intersect_count_directory_scheduler

// output streams
#pragma HLS INTERFACE axis port=axis_read_write_control
#pragma HLS INTERFACE axis port=axis_read_layer_id
#pragma HLS INTERFACE axis port=axis_read_layer_pointer
#pragma HLS INTERFACE axis port=axis_write_layer_id
#pragma HLS INTERFACE axis port=axis_page_ID_pair_read_nodes
#pragma HLS INTERFACE axis port=axis_join_finish

    // Read every inpput once
    pair_t out_axis_page_pair_scheduler = block_read<pair_t>(axis_page_pair_scheduler);
    int out_axis_intersect_count_directory_scheduler = block_read<int>(axis_intersect_count_directory_scheduler);

    // assign values to output register
    int out_int = out_axis_page_pair_scheduler.id_A + out_axis_intersect_count_directory_scheduler;
    pair_t out_pair = out_axis_page_pair_scheduler;

    // Write every output once, based on inputs
    axis_read_write_control.write(out_int); 
    axis_read_layer_id.write(out_int);     
    axis_read_layer_pointer.write(out_int); 
    axis_write_layer_id.write(out_int);
    axis_page_ID_pair_read_nodes.write(out_pair);
    axis_join_finish.write(out_int);

}

}