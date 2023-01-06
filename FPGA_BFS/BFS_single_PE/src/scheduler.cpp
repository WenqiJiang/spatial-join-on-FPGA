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
    hls::stream<int>& axis_layer_cache_read_addr, 
    hls::stream<int>& axis_layer_cache_write_addr, 
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


    int max_level = max_level_A > max_level_B ? max_level_A: max_level_B;
    int num_pairs_per_level[MAX_TREE_LEVEL] = {0};  
    int start_addr_per_layer[MAX_TREE_LEVEL] = {0}; // for layer cache 

    // starting from fetching the pair of level 0, i.e., (root_A, root_B)
    num_pairs_per_level[0] = 1;

    // non-leaf join, write to the layer cache 
    for (int current_level = 1; current_level < max_level + 1; current_level++) {

        // compute address
        start_addr_per_layer[current_level] = 
            start_addr_per_layer[current_level - 1] + num_pairs_per_level[current_level - 1];
        int layer_cache_write_addr = start_addr_per_layer[current_level];

        // send join request
        for (int join_id = 0; join_id < num_pairs_per_level[current_level - 1]; join_id++) {
                
            // get the node pair to join from the last layer, and send it to the read node PE
            axis_read_write_control.write(0); // 0 -> read
            int layer_cache_read_addr = start_addr_per_layer[current_level - 1] + join_id;
            axis_layer_cache_read_addr.write(layer_cache_read_addr); 
            pair_t node_pairs = block_read<pair_t>(axis_page_pair_scheduler);

            // the join PE will automatically compute the results of the load nodes
            axis_page_ID_pair_read_nodes.write(node_pairs);   
                
            // write to layer cache for non-leaf joins
            if (current_level < max_level) {
                axis_read_write_control.write(1); // 1 -> write
                // send layer cache the write address
                axis_layer_cache_write_addr.write(layer_cache_write_addr);
                // get result count from the scheduler 
                int count = block_read<int>(axis_intersect_count_directory_scheduler);
                num_pairs_per_level[current_level] += count; 
                layer_cache_write_addr += count; 
            }
        }
    }
}

}