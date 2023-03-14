#pragma once

#include "types.hpp"
#include "utils.hpp"

extern "C" {

// The scheduler keeps track of where the FPGA is working on during the tree traversal.
void scheduler(
    // Input
    int max_level_A,
    int max_level_B,
    //   from join PE
    hls::stream<int>& axis_idle_join_PE_ID,   // each PE write idle signal once a join finishes
    //   from layer cache controller
    hls::stream<pair_t>& axis_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& axis_intersect_count_directory_scheduler, // for write request, return count
    // Output
    //   to layer cache controller
    hls::stream<mem_burst_t>& axis_layer_cache_read_info, 
    hls::stream<int>& axis_layer_cache_write_addr, 
    hls::stream<int>& axis_num_layer_pairs, // number of pairs to join in this layer
    //   to node reading PE
    hls::stream<pair_t>& axis_page_ID_pair_read_nodes,
    hls::stream<int>& axis_join_PE_ID,
    //   to the write results PE
    hls::stream<int>& axis_PE_task_count, 
    hls::stream<int>& axis_join_finish         // the final termination signal 
) {

// input streams
#pragma HLS INTERFACE axis port=axis_idle_join_PE_ID
#pragma HLS INTERFACE axis port=axis_page_pair_scheduler
#pragma HLS INTERFACE axis port=axis_intersect_count_directory_scheduler

// output streams
#pragma HLS INTERFACE axis port=axis_layer_cache_read_info
#pragma HLS INTERFACE axis port=axis_layer_cache_write_addr
#pragma HLS INTERFACE axis port=axis_num_layer_pairs
#pragma HLS INTERFACE axis port=axis_page_ID_pair_read_nodes
#pragma HLS INTERFACE axis port=axis_join_PE_ID
#pragma HLS INTERFACE axis port=axis_PE_task_count
#pragma HLS INTERFACE axis port=axis_join_finish


    int max_level = max_level_A > max_level_B ? max_level_A: max_level_B;
    int num_pairs_per_level[MAX_TREE_LEVEL] = {0};  
    int start_addr_per_layer[MAX_TREE_LEVEL] = {0}; // for layer cache 
    int join_PE_in_use[N_JOIN_PE] = {0}; // 0 -> idle; i -> in use
    int join_PE_task_count[N_JOIN_PE] = {0}; // for tracking the number of tasks allocated to each PE

    const int pair_cache_size = 32;
    pair_t pair_cache[pair_cache_size]; // cache some pairs to read/join next 
    int pair_start_ptr = 0; // cache starts from the i-th element in the layer
    int valid_pair_num = 0;
    bool cache_valid = false; 

    // starting from fetching the pair of level 0, i.e., (root_A, root_B)
    num_pairs_per_level[0] = 1;

    // non-leaf join, write to the layer cache 
    for (int current_level = 1; current_level < max_level + 1; current_level++) {

        // send cache manager number of pairs
        axis_num_layer_pairs.write(num_pairs_per_level[current_level - 1]);

        // compute address
        start_addr_per_layer[current_level] = 
            start_addr_per_layer[current_level - 1] + num_pairs_per_level[current_level - 1];
        int layer_cache_write_addr = start_addr_per_layer[current_level];
        axis_layer_cache_write_addr.write(layer_cache_write_addr);

        // send join request
        for (int join_id = 0; join_id < num_pairs_per_level[current_level - 1]; join_id++) {
                
            // read from memory if cache miss
            //   there will be some dirty data read at the end of the layer, but they will never be used
            if (!cache_valid || join_id > pair_start_ptr + valid_pair_num - 1) {
                // get the node pair to join from the last layer, and send it to the read node PE
                mem_burst_t read_info;
                read_info.addr = start_addr_per_layer[current_level - 1] + join_id;
                read_info.num = num_pairs_per_level[current_level - 1] - join_id < pair_cache_size? 
                    num_pairs_per_level[current_level - 1] - join_id : pair_cache_size; 
                axis_layer_cache_read_info.write(read_info); 
                for (int read_pair_id = 0; read_pair_id < read_info.num; read_pair_id++) {
                    pair_cache[read_pair_id] = block_read<pair_t>(axis_page_pair_scheduler);
                }
                cache_valid = true;
                pair_start_ptr = join_id;
                valid_pair_num = read_info.num;
            } 
            // fetch a pair from cache
            pair_t node_pairs = pair_cache[join_id - pair_start_ptr];

            // loop until a PE is assigned the task
            while (true) {

                bool break_while = false; 

                // check if whether any PE is released
                while (!axis_idle_join_PE_ID.empty()) { // set as idle
                    int idle_join_PE_ID = block_read<int>(axis_idle_join_PE_ID);
                    join_PE_in_use[idle_join_PE_ID] = 0; 
                }

                for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {

                    // PE idle: assign the task
                    if (!join_PE_in_use[PE_id]) { 
                        // send the node pair and join PE id to the read PE
                        axis_page_ID_pair_read_nodes.write(node_pairs);   
                        axis_join_PE_ID.write(PE_id);
                        join_PE_in_use[PE_id] = 1; // set as busy 
                        join_PE_task_count[PE_id]++;
                        break_while = true; 
                        break;
                    }
                }

                if (break_while) {
                    break;
                }
            }
        }
   
        // invalidate cache 
        cache_valid = false;
        pair_start_ptr = 0;
        valid_pair_num = 0;

        // once all join signal of the layer is sent out, collect the results back from
        //   cache manager
        if (current_level < max_level) {
            // get result count from the scheduler 
            int count = block_read<int>(axis_intersect_count_directory_scheduler);
            num_pairs_per_level[current_level] = count; 
        }
    }

    // final finish signal
    axis_join_finish.write(1);

    // send the job count after the finish signal, otherwise it can block the process
    //   if the AXIS FIFO is not deep enough
    for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {
        axis_PE_task_count.write(join_PE_task_count[PE_id]);
    }
}

}